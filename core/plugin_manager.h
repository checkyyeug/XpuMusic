/**
 * @file plugin_manager.h
 * @brief XpuMusic 鎻掍欢绠＄悊鍣?- 缁熶竴绠＄悊鎵€鏈夋彃浠?
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
 * @brief 鎻掍欢绠＄悊鍣?
 *
 * 璐熻矗鎻掍欢鐨勫姞杞姐€佸嵏杞姐€佺敓鍛藉懆鏈熺鐞嗗拰杩愯鏃惰皟搴?
 */
class PluginManager {
private:
    // 鎻掍欢娉ㄥ唽琛?
    std::unique_ptr<PluginRegistry> registry_;

    // 宸插姞杞界殑鎻掍欢瀹炰緥锛堢敤浜庡崟渚嬫彃浠讹級
    std::unordered_map<std::string, std::unique_ptr<IPlugin>> instances_;

    // 鍔ㄦ€佸簱鍙ユ焺
    std::unordered_map<std::string, void*> library_handles_;

    // foobar2000鎻掍欢鍖呰鍣?
    std::vector<std::unique_ptr<compat::FoobarPluginWrapper>> foobar_plugins_;

    // 鎻掍欢鐩綍
    std::vector<std::string> plugin_directories_;

    // 涓绘満API鐗堟湰
    uint32_t host_api_version_;

    // 鎻掍欢鍔犺浇鍥炶皟
    std::function<void(const std::string&)> on_plugin_loaded_;
    std::function<void(const std::string&)> on_plugin_unloaded_;

public:
    explicit PluginManager(uint32_t api_version = xpumusic::XPUMUSIC_PLUGIN_API_VERSION);
    ~PluginManager();

    // 鍒濆鍖?
    void initialize(const PluginConfig& config);
    void initialize_from_config_manager(const ConfigManager& config_manager);

    // 鎻掍欢鐩綍绠＄悊
    void add_plugin_directory(const std::string& directory);
    void remove_plugin_directory(const std::string& directory);
    const std::vector<std::string>& get_plugin_directories() const;

    // 鍔犺浇鎻掍欢
    bool load_native_plugin(const std::string& path);
    bool load_foobar_plugin(const std::string& path);
    void load_plugins_from_directory(const std::string& directory);
    void load_all_plugins();

    // 鍗歌浇鎻掍欢
    bool unload_plugin(const std::string& key);
    void unload_all_plugins();

    // 鎻掍欢鏌ヨ
    std::unique_ptr<IAudioDecoder> get_decoder(const std::string& file_path);
    std::unique_ptr<IDSPProcessor> get_dsp_processor(const std::string& name);
    std::unique_ptr<IAudioOutput> get_audio_output(const std::string& device_id = "");
    std::unique_ptr<IVisualization> get_visualization(const std::string& name);

    // 鑾峰彇淇℃伅
    std::vector<PluginInfo> get_plugin_list() const;
    std::vector<PluginInfo> get_plugins_by_type(PluginType type) const;
    std::vector<std::string> get_supported_extensions() const;
    bool can_decode(const std::string& file_path) const;

    // 鎻掍欢绠＄悊
    bool is_plugin_loaded(const std::string& key) const;
    IPluginFactory* get_factory(const std::string& key) const;

    // 缁熻淇℃伅
    struct ManagerStats {
        PluginRegistry::Stats registry_stats;
        size_t loaded_instances;
        size_t foobar_plugins;
    };

    ManagerStats get_stats() const;

    // 浜嬩欢鍥炶皟
    void set_plugin_loaded_callback(std::function<void(const std::string&)> callback) {
        on_plugin_loaded_ = callback;
    }

    void set_plugin_unloaded_callback(std::function<void(const std::string&)> callback) {
        on_plugin_unloaded_ = callback;
    }

    // 鍐呴儴璁块棶锛堜緵楂樼骇鐢ㄦ埛浣跨敤锛?
    PluginRegistry* get_registry() const { return registry_.get(); }

private:
    // 骞冲彴鐩稿叧鐨勫姩鎬佸簱鍔犺浇
    void* load_library(const std::string& path);
    void unload_library(void* handle);
    void* get_library_symbol(void* handle, const std::string& symbol);

    // 鎻掍欢鍔犺浇杈呭姪鍑芥暟
    bool register_plugin_factory(const std::string& path,
                               std::unique_ptr<IPluginFactory> factory);
    std::string generate_plugin_key(const PluginInfo& info) const;

    // 鏂囦欢璺緞澶勭悊
    std::string get_file_extension(const std::string& path) const;
    bool is_foobar_plugin(const std::string& path) const;
};

} // namespace xpumusic