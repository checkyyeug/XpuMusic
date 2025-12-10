/**
 * @file config_example.cpp
 * @brief 配置系统使用示例
 * @date 2025-12-10
 */

#include <iostream>
#include "config_manager.h"

using namespace xpumusic;

int main() {
    std::cout << "=== XpuMusic Configuration Example ===\n\n";

    // 初始化配置管理器
    ConfigManager& config = ConfigManagerSingleton::get_instance();

    // 使用默认配置文件初始化
    if (!config.initialize()) {
        std::cerr << "Failed to initialize configuration manager!\n";
        return 1;
    }

    std::cout << "Configuration loaded from: " << config.get_config_file_path() << "\n\n";

    // 读取配置
    const auto& app_config = config.get_config();

    std::cout << "Audio Configuration:\n";
    std::cout << "  Output Device: " << app_config.audio.output_device << "\n";
    std::cout << "  Sample Rate: " << app_config.audio.sample_rate << " Hz\n";
    std::cout << "  Channels: " << app_config.audio.channels << "\n";
    std::cout << "  Volume: " << app_config.audio.volume << "\n";
    std::cout << "  Mute: " << (app_config.audio.mute ? "Yes" : "No") << "\n\n";

    std::cout << "Player Configuration:\n";
    std::cout << "  Repeat: " << (app_config.player.repeat ? "Yes" : "No") << "\n";
    std::cout << "  Shuffle: " << (app_config.player.shuffle ? "Yes" : "No") << "\n";
    std::cout << "  Show Console: " << (app_config.player.show_console_output ? "Yes" : "No") << "\n\n";

    std::cout << "Plugin Directories:\n";
    for (const auto& dir : app_config.plugins.plugin_directories) {
        std::cout << "  - " << config.expand_path(dir) << "\n";
    }
    std::cout << "\n";

    // 修改配置
    std::cout << "Modifying configuration...\n";
    config.audio().volume = 0.5;
    config.audio().sample_rate = 48000;
    config.player().repeat = true;

    // 使用模板方法访问配置
    std::string backend = config.get_config_value<std::string>("player.preferred_backend", "auto");
    std::cout << "Preferred Backend: " << backend << "\n";

    // 设置新的配置值
    config.set_config_value("audio.mute", true);
    config.set_config_value("logging.level", "debug");

    // 保存配置
    std::string test_config = "test_config.json";
    if (config.export_config(test_config)) {
        std::cout << "\nConfiguration exported to: " << test_config << "\n";
    }

    // 配置验证
    AppConfig test_app_config = app_config;
    test_app_config.audio.sample_rate = 100;  // 无效值
    test_app_config.audio.volume = 2.0;       // 无效值

    std::cout << "\nValidating configuration...\n";
    if (config.validate_config(test_app_config)) {
        std::cout << "Configuration is valid!\n";
    } else {
        std::cout << "Configuration validation failed (as expected)!\n";
    }

    // 环境变量测试
    std::cout << "\nTesting environment variable support...\n";
    setenv("XPUMUSIC_AUDIO_VOLUME", "0.75", 1);
    setenv("XPUMUSIC_AUDIO_SAMPLE_RATE", "96000", 1);
    config.load_from_environment();

    double volume = config.get_config_value<double>("audio.volume", 0.0);
    int sample_rate = config.get_config_value<int>("audio.sample_rate", 0);
    std::cout << "Volume from environment: " << volume << "\n";
    std::cout << "Sample Rate from environment: " << sample_rate << "\n";

    // 配置变更通知
    std::cout << "\nSetting up change notifications...\n";
    config.add_change_callback([](const AppConfig& new_config) {
        std::cout << "Configuration changed!\n";
        std::cout << "  New volume: " << new_config.audio.volume << "\n";
        std::cout << "  New sample rate: " << new_config.audio.sample_rate << "\n";
    });

    // 再次修改配置以触发回调
    config.audio().volume = 0.9;
    config.notify_change();

    std::cout << "\n=== Example completed ===\n";

    return 0;
}