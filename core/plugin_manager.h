/**
 * @file plugin_manager.h
 * @brief XpuMusic 插件管理器 - 统一管理所有插件
 * @date 2025-12-10
 */

#pragma once

#include "plugin_registry.h"
#include "../sdk/xpumusic_plugin_sdk.h"
#include "../compat/foobar2000/foobar_adapter.h"
#include "../config/config_manager.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

namespace xpumusic {

/**
 * @brief 插件管理器
 *
 * 负责插件的加载、卸载、生命周期管理和运行时调度
 */
class PluginManager {
private:
    // 插件注册表
    std::unique_ptr<PluginRegistry> registry_;

    // 已加载的插件实例（用于单例插件）
    std::unordered_map<std::string, std::unique_ptr<IPlugin>> instances_;

    // 动态库句柄
    std::unordered_map<std::string, void*> library_handles_;

    // foobar2000插件包装器
    std::vector<std::unique_ptr<compat::FoobarPluginWrapper>> foobar_plugins_;

    // 插件目录
    std::vector<std::string> plugin_directories_;

    // 主机API版本
    uint32_t host_api_version_;

    // 插件加载回调
    std::function<void(const std::string&)> on_plugin_loaded_;
    std::function<void(const std::string&)> on_plugin_unloaded_;

public:
    explicit PluginManager(uint32_t api_version = xpumusic::XPUMUSIC_PLUGIN_API_VERSION);
    ~PluginManager();

    // 初始化
    void initialize(const PluginConfig& config);
    void initialize_from_config_manager(const ConfigManager& config_manager);

    // 插件目录管理
    void add_plugin_directory(const std::string& directory);
    void remove_plugin_directory(const std::string& directory);
    const std::vector<std::string>& get_plugin_directories() const;

    // 加载插件
    bool load_native_plugin(const std::string& path);
    bool load_foobar_plugin(const std::string& path);
    void load_plugins_from_directory(const std::string& directory);
    void load_all_plugins();

    // 卸载插件
    bool unload_plugin(const std::string& key);
    void unload_all_plugins();

    // 插件查询
    std::unique_ptr<IAudioDecoder> get_decoder(const std::string& file_path);
    std::unique_ptr<IDSPProcessor> get_dsp_processor(const std::string& name);
    std::unique_ptr<IAudioOutput> get_audio_output(const std::string& device_id = "");
    std::unique_ptr<IVisualization> get_visualization(const std::string& name);

    // 获取信息
    std::vector<PluginInfo> get_plugin_list() const;
    std::vector<PluginInfo> get_plugins_by_type(PluginType type) const;
    std::vector<std::string> get_supported_extensions() const;
    bool can_decode(const std::string& file_path) const;

    // 插件管理
    bool is_plugin_loaded(const std::string& key) const;
    IPluginFactory* get_factory(const std::string& key) const;

    // 统计信息
    struct ManagerStats {
        PluginRegistry::Stats registry_stats;
        size_t loaded_instances;
        size_t foobar_plugins;
    };

    ManagerStats get_stats() const;

    // 事件回调
    void set_plugin_loaded_callback(std::function<void(const std::string&)> callback) {
        on_plugin_loaded_ = callback;
    }

    void set_plugin_unloaded_callback(std::function<void(const std::string&)> callback) {
        on_plugin_unloaded_ = callback;
    }

    // 内部访问（供高级用户使用）
    PluginRegistry* get_registry() const { return registry_.get(); }

private:
    // 平台相关的动态库加载
    void* load_library(const std::string& path);
    void unload_library(void* handle);
    void* get_library_symbol(void* handle, const std::string& symbol);

    // 插件加载辅助函数
    bool register_plugin_factory(const std::string& path,
                               std::unique_ptr<IPluginFactory> factory);
    std::string generate_plugin_key(const PluginInfo& info) const;

    // 文件路径处理
    std::string get_file_extension(const std::string& path) const;
    bool is_foobar_plugin(const std::string& path) const;
};

} // namespace xpumusic