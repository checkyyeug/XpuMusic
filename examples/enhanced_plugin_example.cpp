/**
 * @file enhanced_plugin_example.cpp
 * @brief 展示如何使用增强的插件系统
 * @date 2025-12-10
 */

#include "../core/plugin_manager_enhanced.h"
#include "../core/event_bus.h"
#include "../config/config_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace xpumusic;

// 示例插件实现
class ExampleDecoderPlugin : public IAudioDecoder {
private:
    PluginState state_ = PluginState::Uninitialized;
    std::string last_error_;
    std::shared_ptr<PluginManagerEnhanced> manager_;
    std::string plugin_name_;

public:
    ExampleDecoderPlugin() {
        manager_ = std::static_pointer_cast<PluginManagerEnhanced>(
            ConfigManagerSingleton::get_instance().get_plugin_manager()
        );
    }

    bool initialize() override {
        // 加载插件配置
        auto config = manager_->get_plugin_config(plugin_name_, {
            {"quality", "medium"},
            {"buffer_size", 4096},
            {"enable_caching", true}
        });

        // 验证配置
        std::string quality = config["quality"];
        if (quality != "low" && quality != "medium" && quality != "high") {
            set_last_error("Invalid quality setting: " + quality);
            state_ = PluginState::Error;
            return false;
        }

        state_ = PluginState::Active;
        return true;
    }

    void shutdown() override {
        // 保存当前配置
        nlohmann::json current_config;
        current_config["last_shutdown"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        manager_->set_plugin_config(plugin_name_, current_config);

        state_ = PluginState::Uninitialized;
    }

    PluginState get_state() const override {
        return state_;
    }

    void set_state(PluginState state) override {
        state_ = state;
    }

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "Example Decoder";
        info.version = "1.2.0";
        info.author = "Example Developer";
        info.description = "Example audio decoder plugin";
        info.type = PluginType::AudioDecoder;
        info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
        info.supported_formats = {"example", "test"};
        return info;
    }

    std::string get_last_error() const override {
        return last_error_;
    }

    bool can_decode(const std::string& file_path) override {
        return file_path.ends_with(".example") || file_path.ends_with(".test");
    }

    std::vector<std::string> get_supported_extensions() override {
        return {"example", "test"};
    }

    bool open(const std::string& file_path) override {
        if (state_ != PluginState::Active) {
            set_last_error("Plugin not active");
            return false;
        }

        // 模拟打开文件
        std::cout << "Opening file: " << file_path << std::endl;
        return true;
    }

    int decode(AudioBuffer& buffer, int max_frames) override {
        if (state_ != PluginState::Active) return 0;

        // 模拟解码
        return max_frames;
    }

    void close() override {
        std::cout << "Closing file" << std::endl;
    }

    AudioFormat get_format() const override {
        AudioFormat format;
        format.sample_rate = 44100;
        format.channels = 2;
        format.bits_per_sample = 32;
        format.is_float = true;
        return format;
    }

private:
    void set_last_error(const std::string& error) {
        last_error_ = error;
        manager_->report_error(plugin_name_, "DECODER_ERROR", error);
    }
};

// 插件工厂
class ExampleDecoderFactory : public ITypedPluginFactory<IAudioDecoder> {
public:
    std::unique_ptr<IAudioDecoder> create_typed() override {
        return std::make_unique<ExampleDecoderPlugin>();
    }

    PluginInfo get_info() const override {
        ExampleDecoderPlugin plugin;
        return plugin.get_info();
    }
};

// 导出插件
QODER_EXPORT_AUDIO_PLUGIN(ExampleDecoderPlugin)

int main() {
    std::cout << "=== Enhanced Plugin System Example ===" << std::endl;

    // 创建事件总线
    auto event_bus = std::make_shared<EventBus>();

    // 创建增强的插件管理器
    auto manager = PluginManagerFactory::create(
        XPUMUSIC_PLUGIN_API_VERSION,
        event_bus
    );

    // 初始化配置
    PluginManagerEnhanced::HotReloadConfig hot_config;
    hot_config.enabled = true;
    hot_config.watch_interval_ms = 500;
    hot_config.auto_reload_on_change = true;

    PluginManagerEnhanced::DependencyConfig dep_config;
    dep_config.auto_resolve = true;
    dep_config.check_optional_deps = true;

    manager->initialize_enhanced(hot_config, dep_config);

    // 设置插件目录
    manager->add_plugin_directory("./plugins");
    manager->add_plugin_directory("~/.xpumusic/plugins");

    // 监听插件生命周期事件
    event_bus->subscribe("plugin_lifecycle", [](const nlohmann::json& event) {
        std::string action = event["action"];
        std::string plugin = event["plugin"];
        std::string timestamp = std::to_string(event["timestamp"]);

        std::cout << "[" << timestamp << "] Plugin '" << plugin
                  << "' " << action << std::endl;
    });

    // 监听配置变更事件
    event_bus->subscribe("plugin_config_changed", [](const nlohmann::json& event) {
        std::string plugin = event["plugin"];
        nlohmann::json config = event["config"];

        std::cout << "Configuration changed for '" << plugin
                  << "': " << config.dump(2) << std::endl;
    });

    // 监听错误事件
    event_bus->subscribe("plugin_error", [](const nlohmann::json& event) {
        std::string plugin = event["plugin"];
        std::string code = event["code"];
        std::string message = event["message"];

        std::cout << "ERROR in '" << plugin << "' [" << code
                  << "]: " << message << std::endl;
    });

    // 手动加载插件（如果存在）
    std::string plugin_path = "libexample_decoder.so";
    if (std::filesystem::exists(plugin_path)) {
        std::cout << "\nLoading plugin from: " << plugin_path << std::endl;

        bool success = manager->load_native_plugin(plugin_path);
        if (success) {
            std::cout << "Plugin loaded successfully!" << std::endl;

            // 测试插件配置
            auto config = manager->get_plugin_config("Example Decoder");
            std::cout << "Current config: " << config.dump(2) << std::endl;

            // 修改配置
            nlohmann::json new_config = config;
            new_config["quality"] = "high";
            new_config["user_setting"] = "test value";
            manager->set_plugin_config("Example Decoder", new_config);

            // 获取解码器实例
            auto decoder = manager->get_decoder("test.example");
            if (decoder) {
                std::cout << "Got decoder instance!" << std::endl;
                std::cout << "Decoder info: " << decoder->get_info().name << std::endl;
            }
        } else {
            std::cout << "Failed to load plugin" << std::endl;

            // 查看错误
            auto errors = manager->get_error_history();
            for (const auto& error : errors) {
                std::cout << "Error: " << error.error_message << std::endl;
            }
        }
    }

    // 演示依赖管理
    std::cout << "\n=== Dependency Management Demo ===" << std::endl;

    // 模拟有依赖的插件元数据
    PluginMetadata metadata;
    metadata.name = "PluginWithDeps";
    metadata.version = "1.0.0";
    metadata.dependencies = {
        {"BaseLibrary", ">=1.0.0", false},
        {"OptionalLib", ">=0.5.0", true}
    };
    manager->set_plugin_metadata("PluginWithDeps", metadata);

    // 检查依赖
    bool deps_ok = manager->check_dependencies("PluginWithDeps");
    std::cout << "Dependencies satisfied: " << (deps_ok ? "Yes" : "No") << std::endl;

    // 获取依赖树
    auto deps = manager->get_dependency_tree("PluginWithDeps");
    std::cout << "Dependencies: ";
    for (const auto& dep : deps) {
        std::cout << dep << " ";
    }
    std::cout << std::endl;

    // 演示版本兼容性
    std::cout << "\n=== Version Compatibility Demo ===" << std::endl;

    bool compatible = manager->is_version_compatible("PluginWithDeps", "1.2.0");
    std::cout << "Version 1.2.0 compatible: " << (compatible ? "Yes" : "No") << std::endl;

    std::string version_range = manager->get_compatible_version_range("PluginWithDeps");
    std::cout << "Compatible version range: " << version_range << std::endl;

    // 演示热加载（如果启用）
    if (manager->is_hot_reload_enabled()) {
        std::cout << "\n=== Hot Reload Demo ===" << std::endl;
        std::cout << "Hot reload is enabled" << std::endl;
        std::cout << "Watch interval: " << hot_config.watch_interval_ms << "ms" << std::endl;

        // 模拟文件修改
        std::cout << "Try modifying a plugin file to see hot reload in action..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // 显示统计信息
    std::cout << "\n=== Plugin Statistics ===" << std::endl;
    auto stats = manager->get_enhanced_stats();
    std::cout << "Total plugins: " << stats.registry_stats.total_plugins << std::endl;
    std::cout << "Loaded plugins: " << stats.registry_stats.loaded_plugins << std::endl;
    std::cout << "Hot reload count: " << stats.hot_reload_count << std::endl;
    std::cout << "Dependency resolutions: " << stats.dependency_resolutions << std::endl;
    std::cout << "Failed loads: " << stats.failed_loads << std::endl;
    std::cout << "Active watchers: " << stats.active_watchers << std::endl;

    // 显示所有插件状态
    std::cout << "\n=== Plugin States ===" << std::endl;
    for (auto state : {PluginLoadState::Loaded, PluginLoadState::Error}) {
        auto plugins = manager->get_plugins_by_state(state);
        if (!plugins.empty()) {
            std::cout << "State: " << static_cast<int>(state) << std::endl;
            for (const auto& plugin : plugins) {
                std::cout << "  - " << plugin << std::endl;
            }
        }
    }

    // 运行一段时间以观察事件
    std::cout << "\nRunning for 5 seconds to observe events..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 保存所有配置
    manager->save_all_plugin_configs();
    std::cout << "\nConfiguration saved." << std::endl;

    std::cout << "\n=== Example Completed ===" << std::endl;
    return 0;
}