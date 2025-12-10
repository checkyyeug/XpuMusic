/**
 * @file plugin_manager_enhanced.h
 * @brief 增强的插件管理器 - 支持热加载、依赖管理、版本检查等功能
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
 * @brief 插件依赖描述
 */
struct PluginDependency {
    std::string name;          // 依赖的插件名
    std::string version;       // 最低版本要求
    bool optional = false;     // 是否可选依赖

    bool is_satisfied_by(const std::string& installed_version) const;
    bool is_compatible(const PluginInfo& info) const;
};

/**
 * @brief 插件元数据
 */
struct PluginMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string license;
    std::string homepage;
    std::vector<PluginDependency> dependencies;
    std::vector<std::string> provides;  // 提供的服务/接口
    std::string config_schema;          // JSON配置模式
    uint64_t load_time = 0;            // 加载时间戳
    bool auto_load = true;             // 是否自动加载
    bool hot_reloadable = true;         // 是否支持热重载
};

/**
 * @brief 插件加载状态
 */
enum class PluginLoadState {
    NotLoaded,       // 未加载
    Loading,         // 正在加载
    Loaded,          // 已加载
    Unloading,       // 正在卸载
    Error,           // 加载错误
    Disabled         // 已禁用
};

/**
 * @brief 插件错误信息
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
 * @brief 插件生命周期事件
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
 * @brief 增强的插件管理器
 */
class PluginManagerEnhanced : public PluginManager {
public:
    // 插件热加载配置
    struct HotReloadConfig {
        bool enabled = true;
        int watch_interval_ms = 1000;  // 文件监控间隔
        bool auto_reload_on_change = true;
        std::vector<std::string> watch_extensions = {".so", ".dll", ".dylib"};
    };

    // 依赖解析配置
    struct DependencyConfig {
        bool auto_resolve = true;
        bool allow_downgrade = false;
        bool check_optional_deps = true;
        int max_resolve_attempts = 10;
    };

private:
    // 热加载相关
    HotReloadConfig hot_reload_config_;
    std::atomic<bool> hot_reload_enabled_{true};
    std::thread watch_thread_;
    std::atomic<bool> watch_thread_running_{false};
    std::condition_variable watch_cv_;
    std::mutex watch_mutex_;

    // 依赖管理
    DependencyConfig dep_config_;
    std::unordered_map<std::string, std::vector<std::string>> reverse_deps_;
    std::unordered_map<std::string, PluginMetadata> plugin_metadata_;

    // 版本管理
    struct VersionInfo {
        std::string version;
        std::string min_compatible;
        std::string max_compatible;
        uint32_t api_version;
    };
    std::unordered_map<std::string, VersionInfo> version_info_;

    // 加载状态跟踪
    std::unordered_map<std::string, PluginLoadState> load_states_;
    std::mutex load_state_mutex_;

    // 配置持久化
    std::string plugin_config_file_;
    std::unordered_map<std::string, nlohmann::json> plugin_configs_;
    std::mutex config_mutex_;

    // 错误处理
    std::vector<PluginError> error_history_;
    mutable std::mutex error_mutex_;
    size_t max_error_history_ = 1000;

    // 日志记录
    std::shared_ptr<spdlog::logger> plugin_logger_;

    // 事件发布
    std::shared_ptr<EventBus> event_bus_;

    // 统计信息
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

    // 初始化增强功能
    void initialize_enhanced(
        const HotReloadConfig& hot_config = {},
        const DependencyConfig& dep_config = {},
        const std::string& config_file = "~/.xpumusic/plugins.json");

    // 热加载功能
    void enable_hot_reload(bool enable = true);
    bool is_hot_reload_enabled() const;
    void set_hot_reload_config(const HotReloadConfig& config);
    const HotReloadConfig& get_hot_reload_config() const;

    // 手动热加载插件
    bool reload_plugin(const std::string& plugin_key);
    bool reload_plugin_with_dependencies(const std::string& plugin_key);

    // 依赖管理
    bool check_dependencies(const std::string& plugin_key);
    bool resolve_dependencies(const std::string& plugin_key);
    std::vector<std::string> get_dependency_tree(const std::string& plugin_key);
    std::vector<std::string> get_dependents(const std::string& plugin_key);
    bool can_safely_unload(const std::string& plugin_key);

    // 版本兼容性
    bool is_version_compatible(const std::string& plugin_key,
                              const std::string& required_version) const;
    bool check_api_version(const PluginInfo& info) const;
    std::string get_compatible_version_range(const std::string& plugin_key) const;

    // 配置管理
    bool load_plugin_config(const std::string& plugin_key);
    bool save_plugin_config(const std::string& plugin_key);
    void set_plugin_config(const std::string& plugin_key, const nlohmann::json& config);
    nlohmann::json get_plugin_config(const std::string& plugin_key,
                                   const nlohmann::json& default_config = {}) const;
    void load_all_plugin_configs();
    void save_all_plugin_configs();

    // 插件状态
    PluginLoadState get_plugin_state(const std::string& plugin_key) const;
    std::vector<std::string> get_plugins_by_state(PluginLoadState state) const;
    bool is_plugin_ready(const std::string& plugin_key) const;

    // 错误处理
    std::vector<PluginError> get_error_history(const std::string& plugin_key = "") const;
    void clear_error_history(const std::string& plugin_key = "");
    std::string get_last_error(const std::string& plugin_key) const;
    void report_error(const std::string& plugin_key,
                      const std::string& error_code,
                      const std::string& error_message);

    // 元数据
    PluginMetadata get_plugin_metadata(const std::string& plugin_key) const;
    void set_plugin_metadata(const std::string& plugin_key,
                            const PluginMetadata& metadata);
    void update_plugin_timestamp(const std::string& plugin_key);

    // 批量操作
    struct BatchOperation {
        std::vector<std::string> plugins_to_load;
        std::vector<std::string> plugins_to_unload;
        bool resolve_dependencies = true;
        bool continue_on_error = false;
    };

    bool execute_batch_operation(const BatchOperation& op);

    // 统计信息
    EnhancedStats get_enhanced_stats() const;
    void reset_stats();

protected:
    // 重写基类方法以添加增强功能
    bool load_native_plugin(const std::string& path) override;
    bool unload_plugin(const std::string& key) override;

private:
    // 热加载实现
    void start_file_watcher();
    void stop_file_watcher();
    void file_watcher_thread();
    bool should_reload_plugin(const std::string& path, const std::string& plugin_key);

    // 依赖解析实现
    bool dependency_dfs(const std::string& plugin_key,
                       std::unordered_set<std::string>& visited,
                       std::vector<std::string>& order);
    bool check_version_compatibility(const std::string& version1,
                                   const std::string& version2) const;

    // 配置持久化实现
    std::string get_config_file_path() const;
    bool load_config_file();
    bool save_config_file();
    nlohmann::json serialize_plugin_metadata(const PluginMetadata& metadata) const;
    PluginMetadata deserialize_plugin_metadata(const nlohmann::json& json) const;

    // 错误处理实现
    void log_error(const PluginError& error);
    void clear_old_errors();

    // 事件发布
    void publish_lifecycle_event(const PluginLifecycleEvent& event);

    // 日志记录
    void setup_logger();
    void log_plugin_operation(const std::string& operation,
                             const std::string& plugin_key,
                             const std::string& details = "");

    // 版本比较
    int compare_versions(const std::string& v1, const std::string& v2) const;
    std::tuple<int, int, int> parse_version(const std::string& version) const;
};

/**
 * @brief 插件管理器工厂
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