/**
 * @file xpumusic_plugin_sdk.h
 * @brief XpuMusic 插件SDK - 核心接口定义
 * @date 2025-12-10
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstring>

// 平台相关定义
#ifdef _WIN32
    #ifdef XPUMUSIC_PLUGIN_BUILD
        #define XPUMUSIC_PLUGIN_API __declspec(dllexport)
    #else
        #define XPUMUSIC_PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define XPUMUSIC_PLUGIN_API __attribute__((visibility("default")))
#endif

// API版本
#define XPUMUSIC_PLUGIN_API_VERSION 1

namespace xpumusic {

// 前向声明
class IPlugin;
class IPluginFactory;

// 插件类型枚举
enum class PluginType {
    Unknown = 0,
    AudioDecoder = 1,  // 音频解码器
    DSPEffect = 2,     // DSP效果器
    AudioOutput = 3,   // 音频输出
    Visualization = 4, // 可视化
    Metadata = 5       // 元数据处理
};

// 插件信息结构
struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    PluginType type;
    uint32_t api_version;
    std::vector<std::string> supported_formats;

    PluginInfo() : type(PluginType::Unknown), api_version(0) {}
};

// 音频格式
struct AudioFormat {
    int sample_rate = 0;
    int channels = 0;
    int bits_per_sample = 0;
    bool is_float = false;
    int64_t channel_mask = 0;

    bool operator==(const AudioFormat& other) const {
        return sample_rate == other.sample_rate &&
               channels == other.channels &&
               bits_per_sample == other.bits_per_sample &&
               is_float == other.is_float;
    }

    bool operator!=(const AudioFormat& other) const {
        return !(*this == other);
    }
};

// 音频缓冲区
struct AudioBuffer {
    float* data = nullptr;
    int frames = 0;
    int channels = 0;

    AudioBuffer() = default;
    AudioBuffer(float* d, int f, int c) : data(d), frames(f), channels(c) {}

    size_t size_bytes() const {
        return frames * channels * sizeof(float);
    }

    void clear() {
        if (data && frames > 0 && channels > 0) {
            memset(data, 0, size_bytes());
        }
    }
};

// 元数据项
struct MetadataItem {
    std::string key;
    std::string value;

    MetadataItem() = default;
    MetadataItem(const std::string& k, const std::string& v) : key(k), value(v) {}
};

// 插件状态枚举
enum class PluginState {
    Uninitialized,
    Initialized,
    Active,
    Error
};

// 插件基类
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // 插件生命周期
    virtual bool initialize() = 0;
    virtual void finalize() = 0;

    // 获取插件信息
    virtual PluginInfo get_info() const = 0;

    // 获取/设置状态
    virtual PluginState get_state() const;
    virtual void set_state(PluginState state);

    // 错误处理
    virtual std::string get_last_error() const;

protected:
    PluginState state_ = PluginState::Uninitialized;
    std::string last_error_;

    // Helper method for derived classes
    void set_error(const std::string& error);
};

// 输入解码器接口
class IAudioDecoder : public IPlugin {
public:
    virtual ~IAudioDecoder() = default;

    // 格式支持
    virtual bool can_decode(const std::string& file_path) = 0;
    virtual std::vector<std::string> get_supported_extensions() = 0;

    // 解码操作
    virtual bool open(const std::string& file_path) = 0;
    virtual int decode(AudioBuffer& buffer, int max_frames) = 0;
    virtual bool seek(int64_t sample_pos) = 0;
    virtual void close() = 0;

    // 音频信息
    virtual AudioFormat get_format() const = 0;
    virtual int64_t get_length() const = 0;
    virtual double get_duration() const = 0;

    // 元数据
    virtual std::vector<MetadataItem> get_metadata() = 0;
    virtual std::string get_metadata_value(const std::string& key) = 0;

    // 位置信息
    virtual int64_t get_position() const = 0;
    virtual bool is_eof() const = 0;
};

// DSP效果器接口
class IDSPProcessor : public IPlugin {
public:
    virtual ~IDSPProcessor() = default;

    // 处理配置
    virtual bool configure(const AudioFormat& input_format,
                          const AudioFormat& output_format) = 0;

    // 处理音频
    virtual int process(const AudioBuffer& input,
                       AudioBuffer& output) = 0;

    // 参数控制
    virtual void set_parameter(const std::string& name, double value) = 0;
    virtual double get_parameter(const std::string& name) = 0;
    virtual std::vector<std::string> get_parameter_names() = 0;

    // 重置
    virtual void reset() = 0;

    // 延迟信息
    virtual int get_latency_samples() const = 0;
};

// 输出设备接口
class IAudioOutput : public IPlugin {
public:
    virtual ~IAudioOutput() = default;

    // 设备管理
    virtual std::vector<std::string> get_devices() = 0;
    virtual bool open_device(const std::string& device_id,
                            const AudioFormat& format) = 0;
    virtual void close_device() = 0;

    // 音频输出
    virtual int write(const AudioBuffer& buffer) = 0;
    virtual int get_latency() = 0;
    virtual void flush() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;

    // 设备信息
    virtual AudioFormat get_device_format(const std::string& device_id) = 0;
    virtual std::string get_current_device() const = 0;
};

// 可视化插件接口
class IVisualization : public IPlugin {
public:
    virtual ~IVisualization() = default;

    // 渲染设置
    virtual bool initialize_render(int width, int height) = 0;
    virtual void finalize_render() = 0;

    // 音频数据输入
    virtual void process_audio(const AudioBuffer& buffer) = 0;

    // 渲染
    virtual void render(void* target, int stride) = 0;

    // 配置
    virtual void set_background_color(uint32_t color) = 0;
    virtual void set_foreground_color(uint32_t color) = 0;
};

// 插件工厂接口
class IPluginFactory {
public:
    virtual ~IPluginFactory() = default;

    // 创建插件实例
    virtual std::unique_ptr<IPlugin> create() = 0;

    // 获取插件信息
    virtual PluginInfo get_info() const = 0;

    // 检查兼容性
    virtual bool is_compatible(uint32_t host_api_version) const {
        return get_info().api_version <= host_api_version;
    }
};

// 特化的工厂接口
template<typename Interface>
class ITypedPluginFactory : public IPluginFactory {
public:
    virtual ~ITypedPluginFactory() = default;

    // 创建类型化的插件实例
    virtual std::unique_ptr<Interface> create_typed() = 0;

    // 基类接口实现
    std::unique_ptr<IPlugin> create() override {
        return std::unique_ptr<IPlugin>(create_typed().release());
    }
};

// 插件注册回调
using PluginRegisterCallback = std::function<void(std::unique_ptr<IPluginFactory>)>;

// 插件导出宏
#define XPUMUSIC_DECLARE_PLUGIN(PluginClass, FactoryClass) \
    extern "C" { \
        XPUMUSIC_PLUGIN_API xpumusic::IPluginFactory* xpumusic_create_plugin_factory() { \
            return new FactoryClass(); \
        } \
        XPUMUSIC_PLUGIN_API const char* xpumusic_get_plugin_name() { \
            static PluginClass dummy; \
            return dummy.get_info().name.c_str(); \
        } \
    }

// 便利宏
#define XPUMUSIC_EXPORT_AUDIO_PLUGIN(PluginClass) \
    XPUMUSIC_DECLARE_PLUGIN(PluginClass, PluginClass##Factory)

#define XPUMUSIC_EXPORT_DSP_PLUGIN(PluginClass) \
    XPUMUSIC_DECLARE_PLUGIN(PluginClass, PluginClass##Factory)

} // namespace xpumusic