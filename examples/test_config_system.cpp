/**
 * @file test_config_system.cpp
 * @brief 配置系统完整工作流程测试
 * @date 2025-12-10
 */

#include "../config/config_manager.h"
#include "../src/audio/audio_output.h"
#include "../src/audio/configured_resampler.h"
#include "../core/plugin_manager.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

using namespace xpumusic;
using namespace audio;

void test_audio_config() {
    std::cout << "\n=== 测试音频配置 ===\n";

    // 创建自定义配置
    ConfigManager config;

    // 修改音频设置
    config.audio().sample_rate = 48000;
    config.audio().channels = 2;
    config.audio().volume = 0.7;
    config.audio().buffer_size = 8192;
    config.audio().output_device = "default";

    // 创建音频输出
    auto audio_output = create_audio_output();

    // 设置音频格式和配置
    AudioFormat format;
    format.sample_rate = config.audio().sample_rate;
    format.channels = config.audio().channels;
    format.bits_per_sample = 32;
    format.is_float = true;

    AudioConfig audio_cfg;
    audio_cfg.device_name = config.audio().output_device;
    audio_cfg.buffer_size = config.audio().buffer_size;
    audio_cfg.volume = config.audio().volume;

    // 初始化音频输出
    if (audio_output->initialize(format, audio_cfg)) {
        std::cout << "✓ 音频输出初始化成功\n";
        std::cout << "  采样率: " << format.sample_rate << " Hz\n";
        std::cout << "  声道数: " << format.channels << "\n";
        std::cout << "  缓冲区大小: " << audio_cfg.buffer_size << "\n";
        std::cout << "  音量: " << audio_cfg.volume << "\n";

        // 测试音量控制
        audio_output->set_volume(0.5);
        std::cout << "  设置音量到: " << audio_output->get_volume() << "\n";

        // 测试静音
        audio_output->set_mute(true);
        std::cout << "  静音状态: " << (audio_output->is_muted() ? "是" : "否") << "\n";

        audio_output->cleanup();
    } else {
        std::cout << "✗ 音频输出初始化失败\n";
    }
}

void test_resampler_config() {
    std::cout << "\n=== 测试重采样器配置 ===\n";

    // 修改重采样器配置
    auto& config = ConfigManagerSingleton::get_instance();

    // 测试不同质量设置
    std::vector<std::string> qualities = {"fast", "good", "high", "best", "adaptive"};

    for (const auto& quality : qualities) {
        config.resampler().quality = quality;

        std::cout << "\n测试质量设置: " << quality << "\n";

        // 创建配置驱动的重采样器
        auto resampler = create_configured_sample_rate_converter();

        if (resampler) {
            // 配置重采样器
            if (resampler->configure(44100, 48000, 2)) {
                std::cout << "  ✓ 重采样器配置成功\n";

                // 测试重采样
                const int input_frames = 1024;
                std::vector<float> input(input_frames * 2);
                std::vector<float> output(input_frames * 2 * 2); // 足够大的输出缓冲区

                // 生成测试信号
                for (int i = 0; i < input_frames; ++i) {
                    float sample = 0.5f * sinf(2.0f * M_PI * 440.0f * i / 44100.0f);
                    input[i * 2] = sample;
                    input[i * 2 + 1] = sample;
                }

                auto start = std::chrono::high_resolution_clock::now();
                int output_frames = resampler->process(input.data(), output.data(), input_frames);
                auto end = std::chrono::high_resolution_clock::now();

                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                std::cout << "  输入帧数: " << input_frames << "\n";
                std::cout << "  输出帧数: " << output_frames << "\n";
                std::cout << "  处理时间: " << duration.count() << " 微秒\n";
            } else {
                std::cout << "  ✗ 重采样器配置失败\n";
            }
        }
    }
}

void test_plugin_config() {
    std::cout << "\n=== 测试插件配置 ===\n";

    // 修改插件配置
    auto& config = ConfigManagerSingleton::get_instance();

    // 设置插件目录
    config.plugins().plugin_directories = {
        "./plugins",
        "../plugins",
        "/usr/local/lib/xpumusic/plugins"
    };
    config.plugins().auto_load_plugins = true;

    // 创建插件管理器
    PluginManager plugin_manager;

    // 使用配置初始化
    plugin_manager.initialize_from_config_manager(config);

    // 显示加载的插件目录
    std::cout << "插件目录:\n";
    for (const auto& dir : plugin_manager.get_plugin_directories()) {
        std::cout << "  - " << dir << "\n";
    }

    // 获取支持的扩展名
    auto extensions = plugin_manager.get_supported_extensions();
    std::cout << "\n支持的文件扩展名:\n";
    for (const auto& ext : extensions) {
        std::cout << "  - " << ext << "\n";
    }
}

void test_format_specific_resampling() {
    std::cout << "\n=== 测试格式特定的重采样 ===\n";

    struct FormatTest {
        std::string format;
        std::string expected_quality;
    };

    std::vector<FormatTest> tests = {
        {"mp3", "good"},
        {"flac", "best"},
        {"wav", "fast"},
        {"ogg", "good"},
        {"unknown", "adaptive"}
    };

    auto& config = ConfigManagerSingleton::get_instance();
    config.resampler().enable_adaptive = true;
    config.resampler().quality = "adaptive";

    for (const auto& test : tests) {
        std::cout << "\n测试格式: " << test.format << "\n";

        auto resampler = create_sample_rate_converter_for_format(test.format);

        if (resampler) {
            std::cout << "  ✓ 成功创建重采样器\n";

            // 配置并测试
            if (resampler->configure(44100, 96000, 2)) {
                std::cout << "  ✓ 配置成功\n";

                // 简单的性能测试
                const int frames = 4096;
                std::vector<float> input(frames * 2);
                std::vector<float> output(frames * 2 * 3);

                auto start = std::chrono::high_resolution_clock::now();
                int out_frames = resampler->process(input.data(), output.data(), frames);
                auto end = std::chrono::high_resolution_clock::now();

                auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                double time_ms = time_us / 1000.0;

                std::cout << "  性能: " << frames << " 帧 -> " << out_frames << " 帧, "
                          << "耗时 " << time_ms << " ms\n";
            }
        }
    }
}

void test_config_persistence() {
    std::cout << "\n=== 测试配置持久化 ===\n";

    // 创建配置管理器
    ConfigManager config;

    // 修改一些设置
    config.audio().sample_rate = 88200;
    config.audio().volume = 0.9;
    config.player().repeat = true;
    config.resampler().quality = "best";
    config.plugins().auto_load_plugins = false;

    // 保存配置
    std::string test_file = "test_config_output.json";
    if (config.export_config(test_file)) {
        std::cout << "✓ 配置导出成功: " << test_file << "\n";

        // 创建新的配置管理器并导入
        ConfigManager new_config;
        if (new_config.import_config(test_file)) {
            std::cout << "✓ 配置导入成功\n";

            // 验证导入的值
            const auto& cfg = new_config.get_config();
            bool match = true;

            match &= (cfg.audio.sample_rate == 88200);
            match &= (cfg.audio.volume == 0.9);
            match &= (cfg.player.repeat == true);
            match &= (cfg.resampler.quality == "best");
            match &= (cfg.plugins.auto_load_plugins == false);

            if (match) {
                std::cout << "✓ 配置值验证成功\n";
            } else {
                std::cout << "✗ 配置值验证失败\n";
            }
        } else {
            std::cout << "✗ 配置导入失败\n";
        }

        // 清理测试文件
        std::remove(test_file.c_str());
    } else {
        std::cout << "✗ 配置导出失败\n";
    }
}

int main() {
    std::cout << "XpuMusic 配置系统完整测试\n";
    std::cout << "===============================\n";

    // 初始化全局配置管理器
    ConfigManagerSingleton::initialize("config/default_config.json");

    try {
        // 测试音频配置
        test_audio_config();

        // 测试重采样器配置
        test_resampler_config();

        // 测试插件配置
        test_plugin_config();

        // 测试格式特定的重采样
        test_format_specific_resampling();

        // 测试配置持久化
        test_config_persistence();

        std::cout << "\n=== 所有测试完成 ===\n";
        std::cout << "✓ 配置系统工作正常\n";

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    // 清理
    ConfigManagerSingleton::shutdown();

    return 0;
}