# XpuMusic Plugin Development Guide

> Welcome to XpuMusic plugin development! This guide will help you create powerful extensions for XpuMusic.

## Table of Contents

1. [Plugin System Overview](#plugin-system-overview)
2. [Quick Start](#quick-start)
3. [Plugin Types](#plugin-types)
4. [API Reference](#api-reference)
5. [Development Examples](#development-examples)
6. [Debugging and Testing](#debugging-and-testing)
7. [Distribution](#distribution)
8. [Best Practices](#best-practices)
9. [FAQ](#faq)

## Plugin System Overview

XpuMusic supports a dual plugin architecture:
- **Native SDK** - Modern C++17 interface for optimal performance
- **Foobar2000 Compatibility Layer** - Directly load existing foobar2000 plugins

### Architecture

```
┌─────────────────┐
│  XpuMusic Core  │
└────────┬────────┘
         │
┌────────▼────────┐
│  Plugin Manager │
└────────┬────────┘
         │
    ┌────┼────┬─────┐
    │    │    │     │
┌───▼──┐┌─▼──┐┌─▼──┐┌─▼────┐
│Audio │DSP │Output│Meta  │
│Decoders│Effects│Device│Data  │
└───────┘└──────┘└─────┘└──────┘
```

## Quick Start

### 1. Environment Setup

```bash
# Ensure you have:
- Visual Studio 2019/2022 with C++ development tools
- CMake 3.16+
- Git

# Clone XpuMusic (for headers and examples)
git clone https://github.com/xpumusic/xpumusic.git
```

### 2. Create Plugin Project

```bash
mkdir my-awesome-plugin
cd my-awesome-plugin
```

### 3. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_awesome_plugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plugin as shared library
add_library(my_awesome_plugin MODULE
    src/plugin.cpp
    src/plugin.h
    src/decoder.cpp  # If it's a decoder
)

# Include XpuMusic headers
target_include_directories(my_awesome_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/xpumusic/src
    ${CMAKE_SOURCE_DIR}/xpumusic/compat
)

# Link required libraries
target_link_libraries(my_awesome_plugin PRIVATE
    xpumusic_sdk  # or specific components needed
)

# Output configuration
set_target_properties(my_awesome_plugin PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
```

## Plugin Types

### 1. Audio Decoder Plugins

Support new audio formats:

```cpp
// src/plugin.h
#pragma once
#include "xpumusic/decoder_interface.h"

class MyFormatDecoder : public xpumusic::IDecoder {
private:
    std::ifstream file_;
    xpumusic::AudioFormat format_;
    bool is_open_ = false;

public:
    MyFormatDecoder() = default;
    virtual ~MyFormatDecoder() { close(); }

    // Plugin info
    const char* get_name() const override {
        return "My Format Decoder v1.0";
    }

    const char* get_extensions() const override {
        return ".myfmt";
    }

    // File operations
    bool open(const char* filename) override {
        file_.open(filename, std::ios::binary);
        if (!file_.is_open()) return false;

        // Read format header
        if (!parse_header()) {
            file_.close();
            return false;
        }

        is_open_ = true;
        return true;
    }

    void close() override {
        if (file_.is_open()) {
            file_.close();
        }
        is_open_ = false;
    }

    // Audio info
    xpumusic::AudioInfo get_info() const override {
        xpumusic::AudioInfo info;
        info.sample_rate = format_.sample_rate;
        info.channels = format_.channels;
        info.bitrate = calculate_bitrate();
        info.duration = calculate_duration();
        return info;
    }

    // Decoding
    size_t decode(float* buffer, size_t frames) override {
        if (!is_open_) return 0;

        // Decode audio data
        size_t frames_decoded = 0;
        while (frames_decoded < frames && !file_.eof()) {
            // Your decoding logic here
            // Example: read frame, decode to float
            decode_frame(buffer + frames_decoded * format_.channels);
            frames_decoded++;
        }

        return frames_decoded;
    }

    // Seeking
    bool seek(double seconds) override {
        if (!is_open_) return false;

        size_t frame = static_cast<size_t>(seconds * format_.sample_rate);
        return seek_to_frame(frame);
    }
};
```

### 2. DSP Effect Plugins

Process audio in real-time:

```cpp
// src/effect.h
#pragma once
#include "xpumusic/effect_interface.h"
#include <vector>

class MyAwesomeEffect : public xpumusic::IEffect {
private:
    // Parameters
    float intensity_ = 0.5f;
    float threshold_ = 0.0f;
    bool bypass_ = false;

    // Internal state
    std::vector<float> delay_buffer_;
    size_t delay_pos_ = 0;
    xpumusic::AudioFormat format_;

public:
    bool initialize(const xpumusic::AudioFormat& format) override {
        format_ = format;

        // Initialize delay buffer for echo effect
        delay_buffer_.resize(format.sample_rate * 2);  // 2 seconds
        std::fill(delay_buffer_.begin(), delay_buffer_.end(), 0.0f);
        delay_pos_ = 0;

        return true;
    }

    void process(float* input, float* output, size_t frames) override {
        if (bypass_) {
            // Simple passthrough
            std::copy(input, input + frames * format_.channels, output);
            return;
        }

        for (size_t frame = 0; frame < frames; ++frame) {
            for (int ch = 0; ch < format_.channels; ++ch) {
                size_t idx = frame * format_.channels + ch;
                float sample = input[idx];

                // Apply delay effect
                float delayed = delay_buffer_[(delay_pos_ + ch) % delay_buffer_.size()];

                // Mix with original
                output[idx] = sample + delayed * intensity_;

                // Store for next iteration
                delay_buffer_[(delay_pos_ + ch) % delay_buffer_.size()] = sample;
            }
            delay_pos_ = (delay_pos_ + format_.channels) % delay_buffer_.size();
        }
    }

    // Parameter control
    void set_parameter(const char* name, double value) override {
        if (strcmp(name, "intensity") == 0) {
            intensity_ = static_cast<float>(value);
        } else if (strcmp(name, "threshold") == 0) {
            threshold_ = static_cast<float>(value);
        } else if (strcmp(name, "bypass") == 0) {
            bypass_ = value > 0.5;
        }
    }

    double get_parameter(const char* name) const override {
        if (strcmp(name, "intensity") == 0) return intensity_;
        if (strcmp(name, "threshold") == 0) return threshold_;
        if (strcmp(name, "bypass") == 0) return bypass_ ? 1.0 : 0.0;
        return 0.0;
    }

    // Parameter metadata
    ParameterInfo get_parameter_info(const char* name) const override {
        ParameterInfo info;
        if (strcmp(name, "intensity") == 0) {
            info.name = "intensity";
            info.display_name = "Effect Intensity";
            info.type = ParameterType::Float;
            info.min_value = 0.0;
            info.max_value = 1.0;
            info.default_value = 0.5;
            info.step = 0.01;
        }
        return info;
    }
};
```

### 3. Audio Output Plugins

Support new audio devices:

```cpp
// src/audio_output.h
#pragma once
#include "xpumusic/audio_output_interface.h"

class MyAudioOutput : public xpumusic::IAudioOutput {
private:
    // Device-specific handles
    void* device_handle_ = nullptr;
    xpumusic::AudioFormat format_;
    xpumusic::AudioConfig config_;
    bool is_initialized_ = false;

public:
    bool initialize(const xpumusic::AudioFormat& format,
                   const xpumusic::AudioConfig& config) override {
        format_ = format;
        config_ = config;

        // Initialize your audio device
        device_handle_ = open_audio_device(config.device_name);
        if (!device_handle_) return false;

        // Configure device
        if (!configure_device(format)) {
            close_audio_device(device_handle_);
            return false;
        }

        is_initialized_ = true;
        return true;
    }

    void start() override {
        if (is_initialized_) {
            start_playback(device_handle_);
        }
    }

    void stop() override {
        if (is_initialized_) {
            stop_playback(device_handle_);
        }
    }

    int write(const void* buffer, int frames) override {
        if (!is_initialized_) return 0;

        // Write audio to device
        return write_to_device(device_handle_, buffer, frames);
    }

    void set_volume(double volume) override {
        config_.volume = volume;
        if (is_initialized_) {
            set_device_volume(device_handle_, volume);
        }
    }

    double get_volume() const override {
        return config_.volume;
    }
};
```

## Development Examples

### Example 1: Volume Control Plugin

```cpp
// volume_plugin.cpp
#include "xpumusic/effect_interface.h"
#include <algorithm>
#include <cmath>

class VolumeControlPlugin : public xpumusic::IEffectPlugin {
public:
    const char* get_name() const override {
        return "Volume Control";
    }

    const char* get_description() const override {
        return "Simple volume control with mute support";
    }

    xpumusic::Version get_version() const override {
        return {1, 0, 0};
    }

    std::unique_ptr<xpumusic::IEffect> create_effect() override {
        return std::make_unique<VolumeControlEffect>();
    }
};

class VolumeControlEffect : public xpumusic::IEffect {
private:
    float volume_linear_ = 1.0f;
    bool is_muted_ = false;
    xpumusic::AudioFormat format_;

public:
    bool initialize(const xpumusic::AudioFormat& format) override {
        format_ = format;
        return true;
    }

    void process(float* input, float* output,
                size_t frames, int channels) override {
        const float actual_volume = is_muted_ ? 0.0f : volume_linear_;

        // SIMD-optimized volume application
        if (has_sse2()) {
            apply_volume_sse2(input, output, frames * channels, actual_volume);
        } else {
            for (size_t i = 0; i < frames * channels; ++i) {
                output[i] = input[i] * actual_volume;
                // Soft clipping to prevent distortion
                if (output[i] > 1.0f) output[i] = 1.0f;
                if (output[i] < -1.0f) output[i] = -1.0f;
            }
        }
    }

    void set_parameter(const char* name, double value) override {
        if (strcmp(name, "volume_db") == 0) {
            // Convert dB to linear: 20 * log10(linear)
            volume_linear_ = std::pow(10.0f, static_cast<float>(value) / 20.0f);
        } else if (strcmp(name, "volume_linear") == 0) {
            volume_linear_ = static_cast<float>(value);
        } else if (strcmp(name, "mute") == 0) {
            is_muted_ = value > 0.5;
        }
    }

    double get_parameter(const char* name) const override {
        if (strcmp(name, "volume_db") == 0) {
            return 20.0 * std::log10(volume_linear_);
        } else if (strcmp(name, "volume_linear") == 0) {
            return volume_linear_;
        } else if (strcmp(name, "mute") == 0) {
            return is_muted_ ? 1.0 : 0.0;
        }
        return 0.0;
    }

private:
    bool has_sse2() const {
#ifdef _MSC_VER
        int info[4];
        __cpuid(info, 1);
        return (info[3] & (1 << 26)) != 0;
#else
        return false;
#endif
    }

    void apply_volume_sse2(const float* input, float* output,
                           size_t samples, float volume) {
#ifdef __SSE2__
        __m128 volume_vec = _mm_set1_ps(volume);

        for (size_t i = 0; i < samples; i += 4) {
            __m128 data = _mm_loadu_ps(&input[i]);
            data = _mm_mul_ps(data, volume_vec);
            _mm_storeu_ps(&output[i], data);
        }
#endif
    }
};

// Plugin export
extern "C" __declspec(dllexport)
xpumusic::IEffectPlugin* create_effect_plugin() {
    return new VolumeControlPlugin();
}
```

### Example 2: Simple Format Decoder

```cpp
// simple_decoder.cpp
#include "xpumusic/decoder_interface.h"
#include <fstream>
#include <cstring>

class SimpleFormatDecoder : public xpumusic::IDecoder {
private:
    struct Header {
        char magic[4];      // "SFMT"
        uint32_t sample_rate;
        uint16_t channels;
        uint16_t bits_per_sample;
        uint64_t data_size;
    };

    std::ifstream file_;
    Header header_;
    bool is_open_ = false;
    size_t current_pos_ = 0;

public:
    const char* get_name() const override {
        return "Simple Format Decoder";
    }

    const char* get_extensions() const override {
        return ".sfmt";
    }

    bool open(const char* filename) override {
        file_.open(filename, std::ios::binary);
        if (!file_.is_open()) return false;

        // Read header
        file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        if (strncmp(header_.magic, "SFMT", 4) != 0) {
            file_.close();
            return false;
        }

        // Validate format
        if (header_.channels < 1 || header_.channels > 8) {
            file_.close();
            return false;
        }

        current_pos_ = 0;
        is_open_ = true;
        return true;
    }

    void close() override {
        if (file_.is_open()) {
            file_.close();
        }
        is_open_ = false;
    }

    xpumusic::AudioInfo get_info() const override {
        xpumusic::AudioInfo info;
        info.sample_rate = header_.sample_rate;
        info.channels = header_.channels;
        info.bits_per_sample = header_.bits_per_sample;
        info.bitrate = calculate_bitrate();
        info.duration = calculate_duration();
        return info;
    }

    size_t decode(float* buffer, size_t frames) override {
        if (!is_open_) return 0;

        const size_t samples_needed = frames * header_.channels;
        size_t samples_decoded = 0;

        if (header_.bits_per_sample == 16) {
            samples_decoded = decode_16bit(buffer, frames);
        } else if (header_.bits_per_sample == 24) {
            samples_decoded = decode_24bit(buffer, frames);
        } else if (header_.bits_per_sample == 32) {
            samples_decoded = decode_32bit(buffer, frames);
        }

        current_pos_ += samples_decoded;
        return samples_decoded / header_.channels;
    }

    bool seek(double seconds) override {
        if (!is_open_) return false;

        size_t target_frame = static_cast<size_t>(seconds * header_.sample_rate);
        size_t target_byte = sizeof(header_) + target_frame * header_.channels *
                           (header_.bits_per_sample / 8);

        file_.seekg(target_byte);
        if (!file_.good()) return false;

        current_pos_ = target_frame * header_.channels;
        return true;
    }

private:
    size_t decode_16bit(float* buffer, size_t frames) {
        const size_t samples = frames * header_.channels;
        std::vector<int16_t> temp(samples);

        file_.read(reinterpret_cast<char*>(temp.data()), samples * sizeof(int16_t));
        size_t read = file_.gcount() / sizeof(int16_t);

        for (size_t i = 0; i < read; ++i) {
            buffer[i] = static_cast<float>(temp[i]) / 32768.0f;
        }

        return read;
    }

    double calculate_bitrate() const {
        return header_.sample_rate * header_.channels * header_.bits_per_sample;
    }

    double calculate_duration() const {
        return static_cast<double>(header_.data_size) /
               (header_.sample_rate * header_.channels * header_.bits_per_sample / 8);
    }
};

// Plugin export
extern "C" __declspec(dllexport)
xpumusic::IDecoder* create_decoder() {
    return new SimpleFormatDecoder();
}
```

## Debugging and Testing

### 1. Unit Testing with GoogleTest

```cpp
// tests/test_volume_plugin.cpp
#include <gtest/gtest.h>
#include "volume_plugin.h"

class VolumePluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_unique<VolumeControlPlugin>();
        effect_ = plugin_->create_effect();

        xpumusic::AudioFormat format;
        format.sample_rate = 44100;
        format.channels = 2;
        effect_->initialize(format);
    }

    std::unique_ptr<VolumeControlPlugin> plugin_;
    std::unique_ptr<xpumusic::IEffect> effect_;
};

TEST_F(VolumePluginTest, VolumeDbToLinear) {
    effect_->set_parameter("volume_db", -6.0);  // -6dB = ~0.5
    EXPECT_NEAR(effect_->get_parameter("volume_linear"), 0.5, 0.01);
}

TEST_F(VolumePluginTest, VolumeAttenuation) {
    float input[] = {1.0f, -1.0f, 0.5f, -0.5f};
    float output[4];

    effect_->set_parameter("volume_db", -6.0);  // -6dB
    effect_->process(input, output, 2, 2);

    EXPECT_NEAR(output[0], 0.5f, 0.01f);
    EXPECT_NEAR(output[1], -0.5f, 0.01f);
    EXPECT_NEAR(output[2], 0.25f, 0.01f);
    EXPECT_NEAR(output[3], -0.25f, 0.01f);
}

TEST_F(VolumePluginTest, MuteFunctionality) {
    float input[] = {1.0f, -1.0f};
    float output[2];

    effect_->set_parameter("mute", 1.0);  // Muted
    effect_->process(input, output, 1, 2);

    EXPECT_EQ(output[0], 0.0f);
    EXPECT_EQ(output[1], 0.0f);
}
```

### 2. Integration Testing

```cpp
// tests/integration_test.cpp
#include "xpumusic/plugin_manager.h"

TEST(PluginIntegrationTest, LoadVolumePlugin) {
    auto manager = std::make_unique<xpumusic::PluginManager>();

    // Load plugin
    bool loaded = manager->load_plugin("volume_plugin.dll");
    EXPECT_TRUE(loaded);

    // Create effect
    auto effect = manager->create_effect("Volume Control");
    ASSERT_NE(effect, nullptr);

    // Test effect
    // ... test implementation ...
}
```

### 3. Performance Testing

```cpp
// tests/performance_test.cpp
#include <chrono>
#include <vector>

TEST(PerformanceTest, VolumePluginLatency) {
    const size_t BUFFER_SIZE = 4096;
    const int ITERATIONS = 10000;

    std::vector<float> input(BUFFER_SIZE);
    std::vector<float> output(BUFFER_SIZE);
    std::fill(input.begin(), input.end(), 0.5f);

    auto effect = create_volume_effect();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; ++i) {
        effect->process(input.data(), output.data(), BUFFER_SIZE / 2, 2);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avg_time_us = static_cast<double>(duration.count()) / ITERATIONS;
    EXPECT_LT(avg_time_us, 100.0);  // Should process buffer in < 100µs
}
```

## Distribution

### 1. Plugin Manifest

Create a `plugin.json` manifest:

```json
{
    "name": "My Awesome Plugin",
    "version": "1.0.0",
    "type": "effect",
    "author": "Your Name",
    "description": "An awesome audio effect for XpuMusic",
    "minimum_xpumusic_version": "1.0.0",
    "platforms": ["win32", "win64", "linux", "macos"],
    "files": {
        "win64": "my_plugin.dll",
        "linux": "libmy_plugin.so",
        "macos": "libmy_plugin.dylib"
    },
    "dependencies": [],
    "parameters": [
        {
            "name": "intensity",
            "type": "float",
            "min": 0.0,
            "max": 1.0,
            "default": 0.5
        }
    ]
}
```

### 2. Package Creation Script

```python
# package_plugin.py
import os
import json
import zipfile
import shutil

def create_plugin_package(plugin_dir):
    # Read manifest
    with open(os.path.join(plugin_dir, 'plugin.json')) as f:
        manifest = json.load(f)

    # Create package
    package_name = f"{manifest['name']}-{manifest['version']}.xpmpkg"

    with zipfile.ZipFile(package_name, 'w') as zf:
        # Add manifest
        zf.write(os.path.join(plugin_dir, 'plugin.json'), 'plugin.json')

        # Add files for current platform
        platform = get_current_platform()
        if platform in manifest['platforms']:
            file_path = os.path.join(plugin_dir, manifest['files'][platform])
            if os.path.exists(file_path):
                zf.write(file_path, os.path.basename(file_path))

        # Add documentation
        docs_dir = os.path.join(plugin_dir, 'docs')
        if os.path.exists(docs_dir):
            for root, dirs, files in os.walk(docs_dir):
                for file in files:
                    file_path = os.path.join(root, file)
                    arc_path = os.path.relpath(file_path, plugin_dir)
                    zf.write(file_path, arc_path)

    print(f"Package created: {package_name}")

if __name__ == "__main__":
    import sys
    create_plugin_package(sys.argv[1])
```

### 3. Installation

Users can install plugins via:

```bash
# Command line
xpumusic --install-plugin my_plugin.xpmpkg

# Or copy to plugins directory
cp my_plugin.dll ~/.xpumusic/plugins/
```

## Best Practices

### 1. Performance Optimization

```cpp
// Use SIMD when available
void process_with_simd(float* input, float* output, size_t samples) {
#ifdef __AVX__
    const __m256 scale = _mm256_set1_ps(0.5f);
    for (size_t i = 0; i < samples; i += 8) {
        __m256 data = _mm256_loadu_ps(&input[i]);
        data = _mm256_mul_ps(data, scale);
        _mm256_storeu_ps(&output[i], data);
    }
#else
    for (size_t i = 0; i < samples; ++i) {
        output[i] = input[i] * 0.5f;
    }
#endif
}

// Pre-allocate buffers
class MyEffect {
private:
    std::vector<float> buffer_;
    size_t buffer_size_;

public:
    bool initialize(const AudioFormat& format) {
        buffer_size_ = format.sample_rate;  // 1 second buffer
        buffer_.resize(buffer_size_);
        return true;
    }
};
```

### 2. Thread Safety

```cpp
class ThreadSafeEffect {
private:
    mutable std::mutex mutex_;
    float parameter_;

public:
    void set_parameter(float value) {
        std::lock_guard<std::mutex> lock(mutex_);
        parameter_ = value;
    }

    void process(float* buffer, size_t frames) {
        float param;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            param = parameter_;
        }

        // Process with captured parameter
        for (size_t i = 0; i < frames; ++i) {
            buffer[i] *= param;
        }
    }
};
```

### 3. Error Handling

```cpp
class RobustPlugin {
private:
    PluginState state_ = PluginState::Uninitialized;

public:
    bool initialize() {
        try {
            // Initialize resources
            if (!init_resources()) {
                set_error("Failed to initialize resources");
                return false;
            }

            state_ = PluginState::Initialized;
            return true;
        } catch (const std::exception& e) {
            set_error(e.what());
            state_ = PluginState::Error;
            return false;
        }
    }

private:
    void set_error(const std::string& message) {
        last_error_ = message;
        // Log error
        xpumusic::log_error("Plugin error: %s", message.c_str());
    }
};
```

## FAQ

### Q: How do I debug my plugin?

A: Several options:
1. Use Visual Studio debugger: Attach to music-player.exe process
2. Add logging: `xpumusic::log_info("Debug point")`
3. Use the plugin test harness: `xpumusic --test-plugin my_plugin.dll`

### Q: My plugin loads but doesn't show up?

A: Check:
1. Plugin exports correct functions
2. Plugin type matches what you're looking for
3. Plugin returned true from initialize()
4. Check logs for error messages

### Q: How to handle different sample rates?

A: Always check the format during initialization:
```cpp
bool initialize(const AudioFormat& format) override {
    if (format.sample_rate < 8000 || format.sample_rate > 192000) {
        return false;  // Unsupported sample rate
    }

    // Resample or adapt if necessary
    sample_rate_ = format.sample_rate;
    return true;
}
```

### Q: Memory management best practices?

A:
1. Use RAII for all resources
2. Prefer smart pointers (unique_ptr, shared_ptr)
3. Avoid raw new/delete
4. Clean up in destructor

### Q: How to distribute my plugin?

A: Options:
1. GitHub release with pre-built binaries
2. Xpuusic plugin repository (submit PR)
3. Package manager (if available)

## Resources

- [XpuMusic GitHub](https://github.com/xpumusic/xpumusic)
- [Plugin Examples](https://github.com/xpumusic/plugin-examples)
- [Developer Discord](https://discord.gg/xpumusic)
- [API Documentation](https://docs.xpumusic.org)

---

Happy coding! 🎵

Thank you for contributing to the XpuMusic ecosystem!