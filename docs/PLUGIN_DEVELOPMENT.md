# XpuMusic v2.0 Plugin Development

## 概述

XpuMusic v2.0 插件系统支持两种模式：
- **Native SDK**：现代化的C++17接口
- **foobar2000兼容层**：可直接加载现有foobar2000插件

## 快速开始

### 1. 创建插件项目

```bash
mkdir my_plugin
cd my_plugin
```

### 2. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_plugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(my_plugin MODULE my_plugin.cpp)

target_include_directories(my_plugin PRIVATE ${CMAKE_SOURCE_DIR}/sdk)
target_compile_definitions(my_plugin PRIVATE QODER_PLUGIN_BUILD=1)

set_target_properties(my_plugin PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
```

### 3. 音频解码器插件示例

```cpp
#include "xpumusic_plugin_sdk.h"
#include <fstream>

using namespace xpumusic;

class MyDecoder : public IAudioDecoder {
private:
    std::ifstream file_;
    AudioFormat format_;
    bool is_open_ = false;

public:
    bool initialize() override {
        set_state(PluginState::Initialized);
        return true;
    }

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "My Format Decoder";
        info.version = "1.0.0";
        info.author = "Your Name";
        info.description = "Custom audio format decoder";
        info.type = PluginType::AudioDecoder;
        info.api_version = QODER_PLUGIN_API_VERSION;
        info.supported_formats = {"myfmt"};
        return info;
    }

    bool can_decode(const std::string& file_path) override {
        return file_path.ends_with(".myfmt");
    }

    std::vector<std::string> get_supported_extensions() override {
        return {"myfmt"};
    }

    bool open(const std::string& file_path) override {
        file_.open(file_path, std::ios::binary);
        if (!file_.is_open()) {
            return false;
        }

        format_.sample_rate = 44100;
        format_.channels = 2;
        format_.bits_per_sample = 16;
        format_.is_float = false;

        is_open_ = true;
        set_state(PluginState::Active);
        return true;
    }

    int decode(AudioBuffer& buffer, int max_frames) override {
        // 解码音频数据
        return frames_decoded;
    }

    void close() override {
        file_.close();
        is_open_ = false;
    }

    AudioFormat get_format() const override {
        return format_;
    }
};

QODER_EXPORT_AUDIO_PLUGIN(MyDecoder)
```

### 4. DSP效果器插件示例

```cpp
#include "xpumusic_plugin_sdk.h"

using namespace xpumusic;

class MyDSP : public IDSPProcessor {
private:
    float volume_ = 1.0f;

public:
    bool initialize() override {
        set_state(PluginState::Initialized);
        return true;
    }

    bool configure(const AudioFormat& format) override {
        // 根据音频格式配置
        return true;
    }

    int process(float* input, float* output, int frames) override {
        for (int i = 0; i < frames * 2; ++i) {  // 假设立体声
            output[i] = input[i] * volume_;
        }
        return frames;
    }

    void set_parameter(const std::string& name, double value) override {
        if (name == "volume") {
            volume_ = static_cast<float>(value);
        }
    }

    double get_parameter(const std::string& name) const override {
        if (name == "volume") {
            return volume_;
        }
        return 0.0;
    }
};

QODER_EXPORT_DSP_PLUGIN(MyDSP)
```

## 插件配置

插件可以定义自己的配置项：

```cpp
class MyPlugin : public IAudioDecoder {
private:
    int quality_mode_ = 2;
    bool enable_enhancement_ = true;
    double threshold_ = 0.5;

public:
    // 获取插件配置模式
    nlohmann::json get_config_schema() const override {
        return {
            {"quality_mode", {
                {"type", "integer"},
                {"min", 1},
                {"max", 5},
                {"default", 2},
                {"description", "Processing quality (1-5)"}
            }},
            {"enable_enhancement", {
                {"type", "boolean"},
                {"default", true},
                {"description", "Enable enhancement processing"}
            }},
            {"threshold", {
                {"type", "number"},
                {"min", 0.0},
                {"max", 1.0},
                {"default", 0.5},
                {"description", "Detection threshold"}
            }}
        };
    }

    // 应用配置
    void apply_config(const nlohmann::json& config) override {
        if (config.contains("quality_mode")) {
            quality_mode_ = config["quality_mode"];
        }
        if (config.contains("enable_enhancement")) {
            enable_enhancement_ = config["enable_enhancement"];
        }
        if (config.contains("threshold")) {
            threshold_ = config["threshold"];
        }
    }
};
```

### 用户配置文件

用户可以在配置文件中设置插件选项：

```json
{
    "plugins": {
        "my_plugin": {
            "quality_mode": 3,
            "enable_enhancement": true,
            "threshold": 0.7
        }
    }
}
```

## 插件类型

1. **音频解码器** (`IAudioDecoder`)
   - 支持的音频格式
   - 文件读取和解码

2. **DSP处理器** (`IDSPProcessor`)
   - 音频效果处理
   - 参数控制

3. **音频输出** (`IAudioOutput`)
   - 平台特定的音频输出
   - 设备控制

## 部署

将编译好的插件文件（Linux: `.so`, Windows: `.dll`, macOS: `.dylib`）放置在：
- `./plugins/` (开发目录)
- `~/.xpumusic/plugins/` (用户目录)
- `/usr/lib/xpumusic/plugins/` (系统目录)

## 最佳实践

1. **性能优化**
   - 使用SIMD指令（如果可用）
   - 预分配缓冲区
   - 避免频繁的内存分配

2. **错误处理**
   - 使用RAII管理资源
   - 设置适当的错误状态
   - 提供有意义的错误信息

3. **跨平台兼容性**
   - 使用条件编译处理平台差异
   - 正确处理Unicode文件路径
   - 注意字节序问题