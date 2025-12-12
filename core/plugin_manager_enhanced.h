/**
 * @file plugin_manager_enhanced.h
 * @brief 澧炲己鐨勬彃浠剁鐞嗗櫒 - 鏀寔鐑姞杞姐€佷緷璧栫鐞嗐€佺増鏈鏌ョ瓑鍔熻兘
 * @date 2025-12-10
 */

#pragma once

#include "plugin_manager.h"
#include "event_bus.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <chrono>

namespace xpumusic {

/**
 * @brief 鎻掍欢渚濊禆鎻忚堪
 */
struct PluginDependency {
    std::string name;          // 渚濊禆鐨勬彃浠跺悕
    std::string version;       // 鏈€浣庣増鏈姹?
    bool optional = false;     // 鏄惁鍙€変緷璧?

    bool is_satisfied_by(const std::string& installed_version) const;
    bool is_compatible(const PluginInfo& info) const;
};

/**
 * @brief 鎻掍欢鍏冩暟鎹?
 */
struct PluginMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string license;
    std::string homepage;
    std::vector<PluginDependency> dependencies;
    std::vector<std::string> provides;  // 鎻愪緵鐨勬湇鍔?鎺ュ彛
    std::string config_schema;          // JSON閰嶇疆妯″紡
    uint64_t load_time = 0;            // 鍔犺浇鏃堕棿鎴?
    bool auto_load = true;             // 鏄惁鑷姩鍔犺浇
    bool hot_reloadable = true;         // 鏄惁鏀寔鐑噸杞?
};

/**
 * @brief 鎻掍欢鍔犺浇鐘舵€?
 */
enum class PluginLoadState {
    NotLoaded,       // 鏈姞杞?
    Loading,         // 姝ｅ湪鍔犺浇
    Loaded,          // 宸插姞杞?
    Unloading,       // 姝ｅ湪鍗歌浇
    Error,           // 鍔犺浇閿欒
    Disabled         // 宸茬鐢?
};

/**
 * @brief 鎻掍欢閿欒淇℃伅
 */
struct PluginError {
    std::string plugin_key;
    std::string error_code;
    std::string error_message;
    std::string stack_trace;
    std::chrono::system_clock::time_point timestamp;

    PluginError(const std::string& key, const std::string& code,
                const std::string& message)
        : plugin_key(key), error_code(code), error_message(message)
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief 鎻掍欢鐢熷懡鍛ㄦ湡浜嬩欢
 */
struct PluginLifecycleEvent {
    std::string plugin_key;
    std::string action;  // "loading", "loaded", "unloading", "unloaded", "error"
    std::string details;
    std::chrono::system_clock::time_point timestamp;

    PluginLifecycleEvent(const std::string& key, const std::string& act,
                         const std::string& det = "")
        : plugin_key(key), action(act), details(det)
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief 澧炲己鐨勬彃浠剁鐞嗗櫒
 */
class PluginManagerEnhanced : public PluginManager {
public:
    // 鎻掍欢鐑姞杞介厤缃?
    struct HotReloadConfig {
        bool enabled = true;
        int watch_interval_ms = 1000;  // 鏂囦欢鐩戞帶闂撮殧
        bool auto_reload_on_change = true;
        std::vector<std::string> watch_extensions = {".so", ".dll", ".dylib"};
    };

    // 渚濊禆瑙ｆ瀽閰嶇疆
    struct DependencyConfig {
        bool auto_resolve = true;
        bool allow_downgrade = false;
        bool check_optional_deps = true;
        int max_resolve_attempts = 10;
    };

private:
    // 鐑姞杞界浉鍏?
    HotReloadConfig hot_reload_config_;
    std::atomic<bool> hot_reload_enabled_{true};
    std::thread watch_thread_;
    std::atomic<bool> watch_thread_running_{false};
    std::condition_variable watch_cv_;
    std::mutex watch_mutex_;

    // 渚濊禆绠＄悊
    DependencyConfig dep_config_;
    std::unordered_map<std::string, std::vector<std::string>> reverse_deps_;
    std::unordered_map<std::string, PluginMetadata> plugin_metadata_;

    // 鐗堟湰绠＄悊
    struct VersionInfo {
        std::string version;
        std::string min_compatible;
        std::string max_compatible;
        uint32_t api_version;
    };
    std::unordered_map<std::string, VersionInfo> version_info_;

    // 鍔犺浇鐘舵€佽窡韪?
    std::unordered_map<std::string, PluginLoadState> load_states_;
    std::mutex load_state_mutex_;

    // 閰嶇疆鎸佷箙鍖?
    std::string plugin_config_file_;
    std::unordered_map<std::string, nlohmann::json> plugin_configs_;
    std::mutex config_mutex_;

    // 閿欒澶勭悊
    std::vector<PluginError> error_history_;
    mutable std::mutex error_mutex_;
    size_t max_error_history_ = 1000;

    // 鏃ュ織璁板綍
    std::shared_ptr<spdlog::logger> plugin_logger_;

    // 浜嬩欢鍙戝竷
    std::shared_ptr<EventBus> event_bus_;

    // 缁熻淇℃伅
    struct EnhancedStats : public ManagerStats {
        size_t hot_reload_count = 0;
        size_t dependency_resolutions = 0;
        size_t failed_loads = 0;
        size_t active_watchers = 0;
        std::chrono::system_clock::time_point last_reload;
    };
    mutable std::mutex stats_mutex_;
    EnhancedStats enhanced_stats_;

public:
    explicit PluginManagerEnhanced(
        uint32_t api_version = xpumusic::XPUMUSIC_PLUGIN_API_VERSION,
        const std::shared_ptr<EventBus>& event_bus = nullptr);

    ~PluginManagerEnhanced() override;

    // 鍒濆鍖栧寮哄姛鑳?
    void initialize_enhanced(
        const HotReloadConfig& hot_config = {},
        const DependencyConfig& dep_config = {},
        const std::string& config_file = "~/.xpumusic/plugins.json");

    // 鐑姞杞藉姛鑳?
    void enable_hot_reload(bool enable = true);
    bool is_hot_reload_enabled() const;
    void set_hot_reload_config(const HotReloadConfig& config);
    const HotReloadConfig& get_hot_reload_config() const;

    // 鎵嬪姩鐑姞杞芥彃浠?
    bool reload_plugin(const std::string& plugin_key);
    bool reload_plugin_with_dependencies(const std::string& plugin_key);

    // 渚濊禆绠＄悊
    bool check_dependencies(const std::string& plugin_key);
    bool resolve_dependencies(const std::string& plugin_key);
    std::vector<std::string> get_dependency_tree(const std::string& plugin_key);
    std::vector<std::string> get_dependents(const std::string& plugin_key);
    bool can_safely_unload(const std::string& plugin_key);

    // 鐗堟湰鍏煎鎬?
    bool is_version_compatible(const std::string& plugin_key,
                              const std::string& required_version) const;
    bool check_api_version(const PluginInfo& info) const;
    std::string get_compatible_version_range(const std::string& plugin_key) const;

    // 閰嶇疆绠＄悊
    bool load_plugin_config(const std::string& plugin_key);
    bool save_plugin_config(const std::string& plugin_key);
    void set_plugin_config(const std::string& plugin_key, const nlohmann::json& config);
    nlohmann::json get_plugin_config(const std::string& plugin_key,
                                   const nlohmann::json& default_config = {}) const;
    void load_all_plugin_configs();
    void save_all_plugin_configs();

    // 鎻掍欢鐘舵€?
    PluginLoadState get_plugin_state(const std::string& plugin_key) const;
    std::vector<std::string> get_plugins_by_state(PluginLoadState state) const;
    bool is_plugin_ready(const std::string& plugin_key) const;

    // 閿欒澶勭悊
    std::vector<PluginError> get_error_history(const std::string& plugin_key = "") const;
    void clear_error_history(const std::string& plugin_key = "");
    std::string get_last_error(const std::string& plugin_key) const;
    void report_error(const std::string& plugin_key,
                      const std::string& error_code,
                      const std::string& error_message);

    // 鍏冩暟鎹?
    PluginMetadata get_plugin_metadata(const std::string& plugin_key) const;
    void set_plugin_metadata(const std::string& plugin_key,
                            const PluginMetadata& metadata);
    void update_plugin_timestamp(const std::string& plugin_key);

    // 鎵归噺鎿嶄綔
    struct BatchOperation {
        std::vector<std::string> plugins_to_load;
        std::vector<std::string> plugins_to_unload;
        bool resolve_dependencies = true;
        bool continue_on_error = false;
    };

    bool execute_batch_operation(const BatchOperation& op);

    // 缁熻淇℃伅
    EnhancedStats get_enhanced_stats() const;
    void reset_stats();

protected:
    // 閲嶅啓鍩虹被鏂规硶浠ユ坊鍔犲寮哄姛鑳?
    bool load_native_plugin(const std::string& path) override;
    bool unload_plugin(const std::string& key) override;

private:
    // 鐑姞杞藉疄鐜?
    void start_file_watcher();
    void stop_file_watcher();
    void file_watcher_thread();
    bool should_reload_plugin(const std::string& path, const std::string& plugin_key);

    // 渚濊禆瑙ｆ瀽瀹炵幇
    bool dependency_dfs(const std::string& plugin_key,
                       std::unordered_set<std::string>& visited,
                       std::vector<std::string>& order);
    bool check_version_compatibility(const std::string& version1,
                                   const std::string& version2) const;

    // 閰嶇疆鎸佷箙鍖栧疄鐜?
    std::string get_config_file_path() const;
    bool load_config_file();
    bool save_config_file();
    nlohmann::json serialize_plugin_metadata(const PluginMetadata& metadata) const;
    PluginMetadata deserialize_plugin_metadata(const nlohmann::json& json) const;

    // 閿欒澶勭悊瀹炵幇
    void log_error(const PluginError& error);
    void clear_old_errors();

    // 浜嬩欢鍙戝竷
    void publish_lifecycle_event(const PluginLifecycleEvent& event);

    // 鏃ュ織璁板綍
    void setup_logger();
    void log_plugin_operation(const std::string& operation,
                             const std::string& plugin_key,
                             const std::string& details = "");

    // 鐗堟湰姣旇緝
    int compare_versions(const std::string& v1, const std::string& v2) const;
    std::tuple<int, int, int> parse_version(const std::string& version) const;
};

/**
 * @brief 鎻掍欢绠＄悊鍣ㄥ伐鍘?
 */
class PluginManagerFactory {
public:
    static std::unique_ptr<PluginManagerEnhanced> create(
        uint32_t api_version = xpumusic::XPUMUSIC_PLUGIN_API_VERSION,
        const std::shared_ptr<EventBus>& event_bus = nullptr) {
        return std::make_unique<PluginManagerEnhanced>(api_version, event_bus);
    }
};

} // namespace xpumusic