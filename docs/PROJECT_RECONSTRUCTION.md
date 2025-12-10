# XpuMusic v2.0 项目重建指南

本文档提供了从零开始重建整个XpuMusic v2.0项目的完整指南。

## 项目目录结构

```
XpuMusic/
├── README.md                    # 主项目文档
├── CMakeLists.txt              # 主CMake构建文件
├── build.sh                    # 构建脚本
├── src/                        # 源代码
│   ├── main.cpp                # 主程序入口
│   ├── music_player_config.cpp # 配置版音乐播放器
│   └── audio/                  # 音频处理模块
│       ├── audio_output.h/.cpp # 音频输出抽象
│       ├── audio_output_alsa.cpp
│       ├── audio_output_pulse.cpp
│       ├── audio_output_wasapi.cpp
│       ├── audio_output_coreaudio.cpp
│       ├── sample_rate_converter.h/.cpp     # 32位重采样
│       ├── sample_rate_converter_64.h/.cpp  # 64位重采样
│       ├── configured_resampler.h/.cpp       # 可配置重采样器
│       ├── adaptive_resampler.h/.cpp         # 自适应重采样
│       ├── linear_resampler.h/.cpp
│       ├── cubic_resampler.h/.cpp
│       ├── sinc_resampler.h/.cpp
│       ├── plugin_audio_decoder.h/.cpp       # 插件音频解码器
│       └── wav_writer.h/.cpp                # WAV文件写入
├── core/                       # 核心系统
│   ├── config_manager.h/.cpp   # 配置管理器
│   ├── plugin_manager.h/.cpp   # 插件管理器
│   ├── plugin_registry.h/.cpp  # 插件注册表
│   ├── core_engine.h/.cpp      # 核心引擎
│   ├── playback_engine.h/.cpp  # 播放引擎
│   ├── playlist_manager.h/.cpp # 播放列表管理
│   ├── visualization_engine.h/.cpp # 可视化引擎
│   ├── event_bus.h/.cpp        # 事件总线
│   └── service_registry.h/.cpp # 服务注册表
├── config/                     # 配置系统
│   ├── default_config.json     # 默认配置
│   └── themes/                 # 主题文件
├── sdk/                        # 插件SDK
│   ├── qoder_plugin_sdk.h      # 主要插件接口
│   ├── examples/               # 示例插件
│   └── headers/                # SDK头文件
├── compat/                     # 兼容层
│   ├── foobar2000/            # foobar2000兼容
│   └── adapters/              # 适配器
├── plugins/                    # 插件目录
│   ├── decoders/              # 解码器插件
│   ├── dsp/                   # DSP插件
│   └── outputs/               # 输出插件
├── platform/                  # 平台特定代码
│   ├── linux/                 # Linux平台
│   ├── windows/               # Windows平台
│   └── macos/                 # macOS平台
├── tests/                     # 测试代码
├── examples/                  # 示例程序
└── docs/                      # 文档
```

## 1. 核心接口定义

### SDK核心接口 (sdk/qoder_plugin_sdk.h)

```cpp
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "nlohmann/json.hpp"

namespace xpumusic {

enum class PluginType {
    AudioDecoder = 1,
    DSPProcessor = 2,
    AudioOutput = 3,
    Visualization = 4
};

enum class PluginState {
    Uninitialized,
    Initialized,
    Active,
    Paused,
    Error
};

struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    PluginType type;
    uint32_t api_version;
    std::vector<std::string> supported_formats;
};

struct AudioFormat {
    int sample_rate = 44100;
    int channels = 2;
    int bits_per_sample = 32;
    bool is_float = true;
};

class AudioBuffer {
public:
    AudioBuffer(int channels, int frames);
    ~AudioBuffer();

    float* data() { return data_; }
    const float* data() const { return data_; }
    int channels() const { return channels_; }
    int frames() const { return frames_; }
    int size() const { return channels_ * frames_; }

    void resize(int frames);
    void clear();

private:
    float* data_;
    int channels_;
    int frames_;
};

// 基础插件接口
class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual PluginState get_state() const = 0;
    virtual void set_state(PluginState state) = 0;

    virtual PluginInfo get_info() const = 0;
    virtual std::string get_last_error() const = 0;

protected:
    void set_last_error(const std::string& error);
};

// 音频解码器接口
class IAudioDecoder : public IPlugin {
public:
    virtual bool can_decode(const std::string& file_path) = 0;
    virtual std::vector<std::string> get_supported_extensions() = 0;

    virtual bool open(const std::string& file_path) = 0;
    virtual int decode(AudioBuffer& buffer, int max_frames) = 0;
    virtual void close() = 0;

    virtual AudioFormat get_format() const = 0;

    virtual nlohmann::json get_config_schema() const { return {}; }
    virtual void apply_config(const nlohmann::json& config) {}
};

// DSP处理器接口
class IDSPProcessor : public IPlugin {
public:
    virtual bool configure(const AudioFormat& format) = 0;
    virtual int process(float* input, float* output, int frames) = 0;

    virtual void set_parameter(const std::string& name, double value) {}
    virtual double get_parameter(const std::string& name) const { return 0.0; }
};

// 音频输出接口
class IAudioOutput : public IPlugin {
public:
    virtual bool initialize(const AudioFormat& format) = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual int write(const float* data, int frames) = 0;

    virtual bool set_volume(double volume) = 0;
    virtual bool set_mute(bool mute) = 0;

    virtual std::vector<std::string> get_available_devices() = 0;
    virtual bool set_device(const std::string& device) = 0;
};

// 可视化接口
class IVisualization : public IPlugin {
public:
    virtual bool initialize(const AudioFormat& format) = 0;
    virtual void process(const AudioBuffer& buffer) = 0;
    virtual void render() = 0;
};

// 插件工厂接口
template<typename T>
class ITypedPluginFactory {
public:
    virtual ~ITypedPluginFactory() = default;
    virtual std::unique_ptr<T> create_typed() = 0;
    virtual PluginInfo get_info() const = 0;
};

// 插件导出宏
#define QODER_PLUGIN_API_VERSION 1
#define QODER_EXPORT_PLUGIN(PluginClass, FactoryClass) \
    extern "C" { \
        QODER_EXPORT ITypedPluginFactory<PluginClass>* create_plugin_factory() { \
            return new FactoryClass(); \
        } \
        QODER_EXPORT void destroy_plugin_factory(ITypedPluginFactory<PluginClass>* factory) { \
            delete factory; \
        } \
    }

#define QODER_EXPORT_AUDIO_PLUGIN(PluginClass) \
    class PluginClass##Factory : public ITypedPluginFactory<IAudioDecoder> { \
    public: \
        std::unique_ptr<IAudioDecoder> create_typed() override { \
            return std::make_unique<PluginClass>(); \
        } \
        PluginInfo get_info() const override { \
            PluginClass plugin; \
            return plugin.get_info(); \
        } \
    }; \
    QODER_EXPORT_PLUGIN(PluginClass, PluginClass##Factory)

#define QODER_EXPORT_DSP_PLUGIN(PluginClass) \
    class PluginClass##Factory : public ITypedPluginFactory<IDSPProcessor> { \
    public: \
        std::unique_ptr<IDSPProcessor> create_typed() override { \
            return std::make_unique<PluginClass>(); \
        } \
        PluginInfo get_info() const override { \
            PluginClass plugin; \
            return plugin.get_info(); \
        } \
    }; \
    QODER_EXPORT_PLUGIN(PluginClass, PluginClass##Factory)

#define QODER_EXPORT_OUTPUT_PLUGIN(PluginClass) \
    class PluginClass##Factory : public ITypedPluginFactory<IAudioOutput> { \
    public: \
        std::unique_ptr<IAudioOutput> create_typed() override { \
            return std::make_unique<PluginClass>(); \
        } \
        PluginInfo get_info() const override { \
            PluginClass plugin; \
            return plugin.get_info(); \
        } \
    }; \
    QODER_EXPORT_PLUGIN(PluginClass, PluginClass##Factory)

// 平台特定的导出定义
#ifdef _WIN32
    #define QODER_EXPORT __declspec(dllexport)
#else
    #define QODER_EXPORT __attribute__((visibility("default")))
#endif

} // namespace qoder
```

## 2. 配置系统

### 配置管理器 (config/config_manager.h)

```cpp
#pragma once
#include <string>
#include <functional>
#include <mutex>
#include "nlohmann/json.hpp"

namespace xpumusic {

struct AudioConfig {
    std::string output_device = "default";
    int sample_rate = 44100;
    int channels = 2;
    int bits_per_sample = 32;
    bool use_float = true;
    int buffer_size = 4096;
    int buffer_count = 4;
    double volume = 0.8;
    bool mute = false;
    std::string equalizer_preset = "flat";
};

struct ResamplerConfig {
    std::string quality = "adaptive";
    int floating_precision = 32;
    bool enable_adaptive = true;
    double cpu_threshold = 0.8;
    bool use_anti_aliasing = true;
    double cutoff_ratio = 0.95;
    int filter_taps = 101;
    nlohmann::json format_quality;
};

struct PluginConfig {
    std::vector<std::string> plugin_directories;
    bool auto_load_plugins = true;
    int plugin_scan_interval = 0;
    int plugin_timeout = 5000;
};

struct PlayerConfig {
    bool repeat = false;
    bool shuffle = false;
    bool crossfade = false;
    double crossfade_duration = 2.0;
    std::string preferred_backend = "auto";
    bool show_console_output = true;
    bool show_progress_bar = true;
    nlohmann::json key_bindings;
    int max_history = 1000;
    bool save_history = true;
    std::string history_file;
};

struct LogConfig {
    std::string level = "info";
    bool console_output = true;
    bool file_output = false;
    std::string log_file;
    bool enable_rotation = true;
    int max_file_size = 10485760;
    int max_files = 5;
    bool include_timestamp = true;
    bool include_thread_id = false;
    bool include_function_name = true;
};

struct UIConfig {
    std::string theme = "default";
    std::string language = "en";
    std::string font_family = "default";
    double font_size = 12.0;
    bool save_window_size = true;
    int window_width = 1024;
    int window_height = 768;
    bool start_maximized = false;
};

struct AppConfig {
    std::string version = "2.0.0";
    AudioConfig audio;
    ResamplerConfig resampler;
    PluginConfig plugins;
    PlayerConfig player;
    LogConfig logging;
    UIConfig ui;
    nlohmann::json plugin_configs;
};

class ConfigManager {
public:
    static ConfigManager& get_instance();

    bool initialize(const std::string& config_file = "");
    bool load_config();
    bool save_config();
    bool reload_config();

    const AppConfig& get_config() const;
    AudioConfig& audio();
    ResamplerConfig& resampler();
    PluginConfig& plugins();
    PlayerConfig& player();
    LogConfig& logging();
    UIConfig& ui();

    void add_change_callback(std::function<void(const AppConfig&)> callback);

    template<typename T>
    void set_config_value(const std::string& path, const T& value);

    template<typename T>
    T get_config_value(const std::string& path, const T& default_value = T{}) const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool load_from_file(const std::string& path);
    bool save_to_file(const std::string& path);
    void apply_environment_overrides();
    bool validate_config();
    nlohmann::json config_to_json() const;
    void json_to_config(const nlohmann::json& json);

    AppConfig config_;
    std::string config_file_;
    std::vector<std::function<void(const AppConfig&)>> change_callbacks_;
    mutable std::mutex mutex_;
};

class ConfigManagerSingleton {
public:
    static ConfigManager& get_instance() {
        return ConfigManager::get_instance();
    }
    static void shutdown() {
        // 清理资源
    }
};

} // namespace qoder
```

### 默认配置 (config/default_config.json)

```json
{
    "version": "2.0.0",
    "audio": {
        "output_device": "default",
        "sample_rate": 44100,
        "channels": 2,
        "bits_per_sample": 32,
        "use_float": true,
        "buffer_size": 4096,
        "buffer_count": 4,
        "volume": 0.8,
        "mute": false,
        "equalizer_preset": "flat"
    },
    "resampler": {
        "quality": "adaptive",
        "floating_precision": 32,
        "enable_adaptive": true,
        "cpu_threshold": 0.8,
        "use_anti_aliasing": true,
        "cutoff_ratio": 0.95,
        "filter_taps": 101,
        "format_quality": {
            "mp3": "good",
            "flac": "best",
            "wav": "fast",
            "ogg": "good",
            "aac": "good",
            "m4a": "good",
            "wma": "fast",
            "ape": "high"
        }
    },
    "plugins": {
        "plugin_directories": [
            "./plugins",
            "~/.xpumusic/plugins",
            "/usr/lib/xpumusic/plugins"
        ],
        "auto_load_plugins": true,
        "plugin_scan_interval": 0,
        "plugin_timeout": 5000
    },
    "player": {
        "repeat": false,
        "shuffle": false,
        "crossfade": false,
        "crossfade_duration": 2.0,
        "preferred_backend": "auto",
        "show_console_output": true,
        "show_progress_bar": true,
        "key_bindings": {
            "play": "space",
            "pause": "p",
            "stop": "s",
            "next": "n",
            "previous": "b",
            "quit": "q"
        },
        "max_history": 1000,
        "save_history": true,
        "history_file": "~/.xpumusic/history.json"
    },
    "logging": {
        "level": "info",
        "console_output": true,
        "file_output": false,
        "log_file": "~/.xpumusic/xpumusic.log",
        "enable_rotation": true,
        "max_file_size": 10485760,
        "max_files": 5,
        "include_timestamp": true,
        "include_thread_id": false,
        "include_function_name": true
    },
    "ui": {
        "theme": "default",
        "language": "en",
        "font_family": "default",
        "font_size": 12.0,
        "save_window_size": true,
        "window_width": 1024,
        "window_height": 768,
        "start_maximized": false
    }
}
```

## 3. 音频处理系统

### 采样率转换器接口 (src/audio/sample_rate_converter.h)

```cpp
#pragma once
#include "sdk/xpumusic_plugin_sdk.h"

namespace xpumusic::audio {

class ISampleRateConverter {
public:
    virtual ~ISampleRateConverter() = default;

    virtual bool configure(int input_rate, int output_rate, int channels) = 0;
    virtual int process(const float* input, float* output, int input_frames) = 0;
    virtual int get_output_frames(int input_frames) const = 0;
    virtual void reset() = 0;

    virtual void set_quality(int quality) {}
    virtual int get_quality() const { return 0; }
};

class ISampleRateConverter64 {
public:
    virtual ~ISampleRateConverter64() = default;

    virtual bool configure(int input_rate, int output_rate, int channels) = 0;
    virtual int process(const double* input, double* output, int input_frames) = 0;
    virtual int get_output_frames(int input_frames) const = 0;
    virtual void reset() = 0;
};

// 32位实现
class LinearSampleRateConverter : public ISampleRateConverter {
public:
    LinearSampleRateConverter();
    ~LinearSampleRateConverter() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const float* input, float* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double position_;
    std::vector<float> history_;
};

class CubicSampleRateConverter : public ISampleRateConverter {
public:
    CubicSampleRateConverter();
    ~CubicSampleRateConverter() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const float* input, float* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double position_;
    std::vector<float> history_;
};

class SincSampleRateConverter : public ISampleRateConverter {
public:
    SincSampleRateConverter(int taps = 8);
    ~SincSampleRateConverter() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const float* input, float* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

    void set_quality(int taps);

private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double position_;
    int taps_;
    std::vector<float> history_;
    std::vector<float> sinc_table_;

    float sinc(double x) const;
    float window(double x) const;
};

// 64位实现
class LinearSampleRateConverter64 : public ISampleRateConverter64 {
public:
    LinearSampleRateConverter64();
    ~LinearSampleRateConverter64() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const double* input, double* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double position_;
    std::vector<double> history_;
};

class CubicSampleRateConverter64 : public ISampleRateConverter64 {
public:
    CubicSampleRateConverter64();
    ~CubicSampleRateConverter64() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const double* input, double* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double position_;
    std::vector<double> history_;
};

class SincSampleRateConverter64 : public ISampleRateConverter64 {
public:
    SincSampleRateConverter64(int taps = 8);
    ~SincSampleRateConverter64() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const double* input, double* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

    void set_quality(int taps);

private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double position_;
    int taps_;
    std::vector<double> history_;
    std::vector<double> sinc_table_;

    double sinc(double x) const;
    double window(double x) const;
};

// 自适应重采样器
class AdaptiveSampleRateConverter : public ISampleRateConverter {
public:
    AdaptiveSampleRateConverter();
    ~AdaptiveSampleRateConverter() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const float* input, float* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

    void set_cpu_threshold(double threshold);
    void update_cpu_usage(double usage);

private:
    enum class Quality {
        Fast,
        Good,
        High,
        Best
    };

    int input_rate_;
    int output_rate_;
    int channels_;
    double cpu_threshold_;
    double current_cpu_usage_;
    Quality current_quality_;

    std::unique_ptr<LinearSampleRateConverter> linear_;
    std::unique_ptr<CubicSampleRateConverter> cubic_;
    std::unique_ptr<SincSampleRateConverter> sinc_low_;
    std::unique_ptr<SincSampleRateConverter> sinc_high_;

    ISampleRateConverter* get_current_converter();
    void select_quality();
};

} // namespace xpumusic::audio
```

### 可配置重采样器 (src/audio/configured_resampler.h)

```cpp
#pragma once
#include "sample_rate_converter.h"
#include "sample_rate_converter_64.h"
#include "config/config_manager.h"

namespace xpumusic::audio {

class ConfiguredSampleRateConverter : public ISampleRateConverter {
public:
    ConfiguredSampleRateConverter();
    ~ConfiguredSampleRateConverter() override;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const float* input, float* output, int input_frames) override;
    int get_output_frames(int input_frames) const override;
    void reset() override;

    void set_precision(int bits); // 32 or 64

private:
    std::unique_ptr<ISampleRateConverter> converter_;
    std::unique_ptr<ISampleRateConverter64> converter64_;
    bool use_64bit_;
    int input_rate_;
    int output_rate_;
    int channels_;

    std::unique_ptr<float[]> temp_input_;
    std::unique_ptr<float[]> temp_output_;
    std::unique_ptr<double[]> temp_input64_;
    std::unique_ptr<double[]> temp_output64_;

    void create_converter(const std::string& quality);
};

} // namespace xpumusic::audio
```

## 4. 音频输出抽象

### 音频输出接口 (src/audio/audio_output.h)

```cpp
#pragma once
#include "sdk/xpumusic_plugin_sdk.h"
#include <vector>
#include <atomic>

namespace xpumusic::audio {

class AudioOutputBase : public IAudioOutput {
public:
    AudioOutputBase();
    ~AudioOutputBase() override = default;

    bool initialize(const AudioFormat& format) override;
    bool start() override;
    bool stop() override;

    bool set_volume(double volume) override;
    bool set_mute(bool mute) override;

    double get_volume() const { return volume_; }
    bool get_mute() const { return mute_; }

protected:
    AudioFormat format_;
    std::atomic<double> volume_{0.8};
    std::atomic<bool> mute_{false};
    bool initialized_{false};
    bool running_{false};

    virtual bool do_initialize() = 0;
    virtual bool do_start() = 0;
    virtual bool do_stop() = 0;
    virtual int do_write(const float* data, int frames) = 0;
};

class AudioOutputRegistry {
public:
    static AudioOutputRegistry& get_instance();

    void register_output(const std::string& name,
                         std::function<std::unique_ptr<IAudioOutput>()> factory);

    std::unique_ptr<IAudioOutput> create_output(const std::string& name);
    std::vector<std::string> get_available_outputs();

    std::unique_ptr<IAudioOutput> create_default_output();

private:
    AudioOutputRegistry() = default;
    std::map<std::string, std::function<std::unique_ptr<IAudioOutput>()>> outputs_;
};

#define REGISTER_AUDIO_OUTPUT(name, class_name) \
    namespace { \
        struct name##_registrar { \
            name##_registrar() { \
                AudioOutputRegistry::get_instance().register_output( \
                    #name, []() { return std::make_unique<class_name>(); }); \
            } \
        }; \
        static name##_registrar registrar; \
    }

} // namespace xpumusic::audio
```

## 5. 构建系统

### 主CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(XpuMusic VERSION 2.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖
find_package(PkgConfig QUIET)

# nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include_directories(external/nlohmann/include)
endif()

# 线程库
find_package(Threads REQUIRED)

# 音频后端检测
include(cmake/AudioBackend.cmake)

# 子目录
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(src)
add_subdirectory(sdk)
add_subdirectory(plugins)

# 主要可执行文件
add_executable(music-player-config
    src/music_player_config.cpp
)

target_link_libraries(music-player-config PRIVATE
    qoder_config
    qoder_core
    qoder_audio
    Threads::Threads
)

if(nlohmann_json_FOUND)
    target_link_libraries(music-player-config PRIVATE nlohmann_json::nlohmann_json)
endif()

# 安装规则
install(TARGETS music-player-config
    RUNTIME DESTINATION bin
)

install(FILES config/default_config.json
    DESTINATION share/xpumusic/config
)

install(DIRECTORY plugins/
    DESTINATION lib/xpumusic/plugins
    FILES_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "*.dylib"
)
```

### cmake/AudioBackend.cmake

```cmake
# 音频后端自动检测

# 检测ALSA
if(PKG_CONFIG_FOUND)
    pkg_check_modules(ALSA alsa)
    if(ALSA_FOUND)
        set(QODER_HAVE_ALSA TRUE)
        add_definitions(-DQODER_HAVE_ALSA)
        message(STATUS "Found ALSA: ${ALSA_LIBRARIES}")
    endif()
endif()

# 检测PulseAudio
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PULSE libpulse)
    if(PULSE_FOUND)
        set(QODER_HAVE_PULSE TRUE)
        add_definitions(-DQODER_HAVE_PULSE)
        message(STATUS "Found PulseAudio: ${PULSE_LIBRARIES}")
    endif()
endif()

# Windows WASAPI
if(WIN32)
    set(QODER_HAVE_WASAPI TRUE)
    add_definitions(-DQODER_HAVE_WASAPI)
    message(STATUS "Using WASAPI on Windows")
endif()

# macOS CoreAudio
if(APPLE)
    set(QODER_HAVE_COREAUDIO TRUE)
    add_definitions(-DQODER_HAVE_COREAUDIO)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    message(STATUS "Using CoreAudio on macOS")
endif()

# 设置链接库
set(QODER_AUDIO_LIBS)
if(QODER_HAVE_ALSA)
    list(APPEND QODER_AUDIO_LIBS ${ALSA_LIBRARIES})
endif()
if(QODER_HAVE_PULSE)
    list(APPEND QODER_AUDIO_LIBS ${PULSE_LIBRARIES})
endif()
if(QODER_HAVE_COREAUDIO)
    list(APPEND QODER_AUDIO_LIBS
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
    )
endif()
```

### 构建脚本 (build.sh)

```bash
#!/bin/bash
set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building XpuMusic v2.0${NC}"

# 检查依赖
echo "Checking dependencies..."

# 检查CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake not found. Please install CMake 3.20 or newer.${NC}"
    exit 1
fi

# 检查编译器
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo -e "${RED}Error: No C++ compiler found. Please install g++ or clang++.${NC}"
    exit 1
fi

# 创建构建目录
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置构建
echo "Configuring build..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# 编译
echo "Building..."
make -j$(nproc)

# 运行测试
echo "Running tests..."
if [ -f "test/test-all" ]; then
    ./test/test-all
fi

echo -e "${GREEN}Build completed successfully!${NC}"
echo ""
echo "To install system-wide, run:"
echo "  sudo make install"
echo ""
echo "To run the music player:"
echo "  ./bin/music-player-config"
```

## 6. 测试系统

### 测试主程序 (tests/test_all.cpp)

```cpp
#include <gtest/gtest.h>
#include "config/config_manager.h"
#include "core/plugin_manager.h"
#include "src/audio/sample_rate_converter.h"

// 配置系统测试
TEST(ConfigTest, LoadDefaultConfig) {
    ConfigManager& config = ConfigManager::get_instance();
    EXPECT_TRUE(config.initialize());

    const auto& cfg = config.get_config();
    EXPECT_EQ(cfg.audio.sample_rate, 44100);
    EXPECT_EQ(cfg.resampler.floating_precision, 32);
}

// 插件系统测试
TEST(PluginTest, LoadPlugin) {
    PluginManager& manager = PluginManager::get_instance();
    EXPECT_TRUE(manager.initialize());

    auto plugins = manager.get_available_plugins();
    EXPECT_GT(plugins.size(), 0);
}

// 重采样器测试
TEST(ResamplerTest, BasicConversion) {
    xpumusic::audio::LinearSampleRateConverter resampler;
    EXPECT_TRUE(resampler.configure(44100, 48000, 2));

    std::vector<float> input(4410 * 2); // 100ms of stereo audio
    std::vector<float> output(4800 * 2);

    int output_frames = resampler.process(input.data(), output.data(), 4410);
    EXPECT_EQ(output_frames, 4800);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

## 7. 部署和安装

### 安装脚本 (scripts/install.sh)

```bash
#!/bin/bash
set -e

INSTALL_PREFIX=${1:-/usr/local}

echo "Installing XpuMusic to $INSTALL_PREFIX"

# 创建目录
sudo mkdir -p "$INSTALL_PREFIX/bin"
sudo mkdir -p "$INSTALL_PREFIX/lib/xpumusic/plugins"
sudo mkdir -p "$INSTALL_PREFIX/share/xpumusic/config"
sudo mkdir -p "$INSTALL_PREFIX/include/xpumusic"
sudo mkdir -p "$INSTALL_PREFIX/share/doc/xpumusic"

# 安装二进制文件
sudo cp build/bin/music-player-config "$INSTALL_PREFIX/bin/"

# 安装配置文件
sudo cp config/default_config.json "$INSTALL_PREFIX/share/xpumusic/config/"

# 安装头文件
sudo cp -r sdk/*.h "$INSTALL_PREFIX/include/xpumusic/"

# 安装文档
sudo cp -r docs/*.md "$INSTALL_PREFIX/share/doc/xpumusic/"

# 安装插件
sudo cp build/plugins/* "$INSTALL_PREFIX/lib/xpumusic/plugins/" 2>/dev/null || true

# 设置权限
sudo chmod +x "$INSTALL_PREFIX/bin/music-player-config"

echo "Installation complete!"
echo ""
echo "Run 'music-player-config' to start the player."
echo "Configuration file: ~/.xpumusic/config.json"
```

## 重建步骤总结

1. **创建项目目录结构**
   ```bash
   mkdir -p XpuMusic/{src/audio,core,config,sdk,compat,plugins,platform,tests,docs}
   ```

2. **复制核心文件**
   - `sdk/qoder_plugin_sdk.h`
   - `config/config_manager.h/.cpp`
   - `config/default_config.json`
   - `src/audio/` 下的所有音频处理文件
   - `core/` 下的核心系统文件
   - `CMakeLists.txt` 和 `build.sh`

3. **安装依赖**
   ```bash
   # Ubuntu/Debian
   sudo apt install cmake build-essential nlohmann-json3-dev
   sudo apt install libasound2-dev libpulse-dev  # Linux音频

   # macOS
   brew install cmake nlohmann-json

   # Windows
   # 使用vcpkg安装nlohmann/json
   ```

4. **构建项目**
   ```bash
   ./build.sh
   ```

5. **测试**
   ```bash
   cd build
   ./bin/music-player-config --list-devices
   ./bin/music-player-config test.wav
   ```

6. **安装（可选）**
   ```bash
   sudo make install
   ```

## 重要提示

- 确保所有头文件包含路径正确
- 根据平台安装相应的音频开发库
- 64位浮点处理需要更多CPU资源
- 插件路径配置要正确设置
- 配置文件优先级：命令行 > 用户配置 > 系统配置 > 默认配置