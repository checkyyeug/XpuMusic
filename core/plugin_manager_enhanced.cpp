/**
 * @file plugin_manager_enhanced.cpp
 * @brief 澧炲己鎻掍欢绠＄悊鍣ㄧ殑瀹炵幇
 * @date 2025-12-10
 */

#include "plugin_manager_enhanced.h"
#include "../config/config_manager.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <dlfcn.h>

namespace xpumusic {

// PluginDependency implementation
bool PluginDependency::is_satisfied_by(const std::string& installed_version) const {
    // Simple version comparison - can be enhanced with semver
    return true; // TODO: Implement proper version checking
}

bool PluginDependency::is_compatible(const PluginInfo& info) const {
    if (name != info.name) return false;
    return is_satisfied_by(info.version);
}

// PluginManagerEnhanced implementation
PluginManagerEnhanced::PluginManagerEnhanced(
    uint32_t api_version,
    const std::shared_ptr<EventBus>& event_bus)
    : PluginManager(api_version)
    , event_bus_(event_bus) {
    setup_logger();
}

PluginManagerEnhanced::~PluginManagerEnhanced() {
    stop_file_watcher();
    save_all_plugin_configs();
}

void PluginManagerEnhanced::setup_logger() {
    try {
        plugin_logger_ = spdlog::get("plugin");
        if (!plugin_logger_) {
            plugin_logger_ = spdlog::stdout_color_mt("plugin");
        }
        plugin_logger_->set_level(spdlog::level::info);
        plugin_logger_->set_pattern("[%H:%M:%S] [%^%l%$] [plugin] %v");
    } catch (const std::exception& e) {
        std::cerr << "Failed to setup plugin logger: " << e.what() << std::endl;
    }
}

void PluginManagerEnhanced::initialize_enhanced(
    const HotReloadConfig& hot_config,
    const DependencyConfig& dep_config,
    const std::string& config_file) {

    hot_reload_config_ = hot_config;
    dep_config_ = dep_config;
    plugin_config_file_ = config_file;

    // Load existing configuration
    load_config_file();
    load_all_plugin_configs();

    // Start file watcher if enabled
    if (hot_reload_config_.enabled) {
        enable_hot_reload(true);
    }

    log_plugin_operation("initialized", "plugin_manager",
        "Hot reload: " + std::string(hot_reload_config_.enabled ? "enabled" : "disabled"));
}

// Hot reload implementation
void PluginManagerEnhanced::enable_hot_reload(bool enable) {
    hot_reload_enabled_ = enable;

    if (enable && !watch_thread_running_) {
        start_file_watcher();
    } else if (!enable && watch_thread_running_) {
        stop_file_watcher();
    }
}

bool PluginManagerEnhanced::is_hot_reload_enabled() const {
    return hot_reload_enabled_.load();
}

void PluginManagerEnhanced::set_hot_reload_config(const HotReloadConfig& config) {
    hot_reload_config_ = config;
}

const PluginManagerEnhanced::HotReloadConfig& PluginManagerEnhanced::get_hot_reload_config() const {
    return hot_reload_config_;
}

void PluginManagerEnhanced::start_file_watcher() {
    if (watch_thread_running_) return;

    watch_thread_running_ = true;
    watch_thread_ = std::thread(&PluginManagerEnhanced::file_watcher_thread, this);

    std::lock_guard<std::mutex> lock(stats_mutex_);
    enhanced_stats_.active_watchers++;
}

void PluginManagerEnhanced::stop_file_watcher() {
    if (!watch_thread_running_) return;

    watch_thread_running_ = false;
    watch_cv_.notify_all();

    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }

    std::lock_guard<std::mutex> lock(stats_mutex_);
    enhanced_stats_.active_watchers--;
}

void PluginManagerEnhanced::file_watcher_thread() {
    plugin_logger_->info("File watcher thread started");

    while (watch_thread_running_) {
        try {
            // Check all plugin directories for changes
            for (const auto& dir : get_plugin_directories()) {
                if (!std::filesystem::exists(dir)) continue;

                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (!watch_thread_running_) break;

                    if (!entry.is_regular_file()) continue;

                    const auto& path = entry.path();
                    const auto ext = path.extension().string();

                    // Check if this is a plugin file we should watch
                    auto it = std::find(hot_reload_config_.watch_extensions.begin(),
                                       hot_reload_config_.watch_extensions.end(), ext);
                    if (it == hot_reload_config_.watch_extensions.end()) continue;

                    // Check if plugin is already loaded
                    std::string plugin_key = path.filename().string();
                    if (!is_plugin_loaded(plugin_key)) {
                        if (hot_reload_config_.auto_reload_on_change) {
                            log_plugin_operation("auto_loading", plugin_key);
                            load_native_plugin(path.string());
                        }
                        continue;
                    }

                    // Check if file was modified
                    auto current_time = entry.last_write_time();
                    auto metadata = get_plugin_metadata(plugin_key);
                    if (std::chrono::duration_cast<std::chrono::seconds>(
                        current_time.time_since_epoch()).count() > metadata.load_time) {

                        if (hot_reload_config_.auto_reload_on_change) {
                            log_plugin_operation("auto_reloading", plugin_key);
                            reload_plugin(plugin_key);
                        }
                    }
                }
            }

            // Wait for next check
            std::unique_lock<std::mutex> lock(watch_mutex_);
            watch_cv_.wait_for(lock, std::chrono::milliseconds(
                hot_reload_config_.watch_interval_ms),
                [this] { return !watch_thread_running_; });

        } catch (const std::exception& e) {
            plugin_logger_->error("Error in file watcher thread: {}", e.what());
        }
    }

    plugin_logger_->info("File watcher thread stopped");
}

bool PluginManagerEnhanced::reload_plugin(const std::string& plugin_key) {
    log_plugin_operation("reload_start", plugin_key);

    // Get the current plugin path from registry
    auto factory = get_factory(plugin_key);
    if (!factory) {
        report_error(plugin_key, "RELOAD_ERROR", "Plugin not found in registry");
        return false;
    }

    // Unload the plugin
    if (!unload_plugin(plugin_key)) {
        report_error(plugin_key, "RELOAD_ERROR", "Failed to unload plugin");
        return false;
    }

    // Load it again
    // Note: In a real implementation, we need to track the original file path
    // For now, this is a simplified version
    bool success = false;
    for (const auto& dir : get_plugin_directories()) {
        std::string full_path = dir + "/" + plugin_key;
        if (std::filesystem::exists(full_path)) {
            success = load_native_plugin(full_path);
            if (success) break;
        }
    }

    if (success) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        enhanced_stats_.hot_reload_count++;
        enhanced_stats_.last_reload = std::chrono::system_clock::now();

        // Update timestamp
        update_plugin_timestamp(plugin_key);

        publish_lifecycle_event(PluginLifecycleEvent(plugin_key, "reloaded"));
    } else {
        report_error(plugin_key, "RELOAD_ERROR", "Failed to reload plugin");
    }

    log_plugin_operation("reload_end", plugin_key, success ? "success" : "failed");
    return success;
}

bool PluginManagerEnhanced::reload_plugin_with_dependencies(const std::string& plugin_key) {
    // Get all plugins that depend on this one
    auto dependents = get_dependents(plugin_key);

    // Unload dependents first
    std::vector<std::string> reload_order;
    for (const auto& dep : dependents) {
        if (is_plugin_loaded(dep)) {
            unload_plugin(dep);
            reload_order.push_back(dep);
        }
    }

    // Unload the target plugin
    if (!unload_plugin(plugin_key)) {
        return false;
    }

    // Reload the target plugin
    if (!reload_plugin(plugin_key)) {
        return false;
    }

    // Reload dependents in reverse order
    std::reverse(reload_order.begin(), reload_order.end());
    for (const auto& dep : reload_order) {
        reload_plugin(dep);
    }

    return true;
}

// Dependency management
bool PluginManagerEnhanced::check_dependencies(const std::string& plugin_key) {
    auto it = plugin_metadata_.find(plugin_key);
    if (it == plugin_metadata_.end()) {
        return true; // No metadata, assume no dependencies
    }

    for (const auto& dep : it->second.dependencies) {
        // Check if dependency is satisfied
        bool satisfied = false;

        // First check if we have it loaded
        if (is_plugin_loaded(dep.name)) {
            satisfied = dep.is_satisfied_by(get_plugin_metadata(dep.name).version);
        }

        // If not loaded, check if it's available in the system
        if (!satisfied) {
            auto plugins = get_plugin_list();
            for (const auto& plugin : plugins) {
                if (plugin.name == dep.name) {
                    satisfied = dep.is_compatible(plugin);
                    break;
                }
            }
        }

        if (!satisfied && !dep.optional) {
            report_error(plugin_key, "DEPENDENCY_ERROR",
                "Missing required dependency: " + dep.name);
            return false;
        }
    }

    return true;
}

bool PluginManagerEnhanced::resolve_dependencies(const std::string& plugin_key) {
    if (!dep_config_.auto_resolve) {
        return true;
    }

    std::lock_guard<std::mutex> lock(stats_mutex_);
    enhanced_stats_.dependency_resolutions++;

    auto it = plugin_metadata_.find(plugin_key);
    if (it == plugin_metadata_.end()) {
        return true; // No dependencies
    }

    // Build dependency graph
    std::unordered_set<std::string> visited;
    std::vector<std::string> load_order;

    if (!dependency_dfs(plugin_key, visited, load_order)) {
        return false; // Circular dependency detected
    }

    // Load dependencies in order (excluding the target plugin)
    for (size_t i = 0; i < load_order.size() - 1; ++i) {
        const auto& dep_key = load_order[i];
        if (!is_plugin_loaded(dep_key)) {
            // Try to load dependency
            bool loaded = false;
            for (const auto& dir : get_plugin_directories()) {
                std::string full_path = dir + "/" + dep_key;
                if (std::filesystem::exists(full_path)) {
                    loaded = load_native_plugin(full_path);
                    break;
                }
            }

            if (!loaded) {
                report_error(plugin_key, "DEPENDENCY_ERROR",
                    "Failed to load dependency: " + dep_key);
                return false;
            }
        }
    }

    return true;
}

bool PluginManagerEnhanced::dependency_dfs(
    const std::string& plugin_key,
    std::unordered_set<std::string>& visited,
    std::vector<std::string>& order) {

    if (visited.find(plugin_key) != visited.end()) {
        // Circular dependency detected
        report_error(plugin_key, "DEPENDENCY_ERROR", "Circular dependency detected");
        return false;
    }

    visited.insert(plugin_key);

    auto it = plugin_metadata_.find(plugin_key);
    if (it != plugin_metadata_.end()) {
        for (const auto& dep : it->second.dependencies) {
            if (!dep.optional && is_plugin_available(dep.name)) {
                if (!dependency_dfs(dep.name, visited, order)) {
                    return false;
                }
            }
        }
    }

    order.push_back(plugin_key);
    return true;
}

std::vector<std::string> PluginManagerEnhanced::get_dependency_tree(const std::string& plugin_key) {
    std::vector<std::string> dependencies;

    auto it = plugin_metadata_.find(plugin_key);
    if (it != plugin_metadata_.end()) {
        for (const auto& dep : it->second.dependencies) {
            dependencies.push_back(dep.name);
            // Recursively get dependencies
            auto sub_deps = get_dependency_tree(dep.name);
            dependencies.insert(dependencies.end(), sub_deps.begin(), sub_deps.end());
        }
    }

    return dependencies;
}

std::vector<std::string> PluginManagerEnhanced::get_dependents(const std::string& plugin_key) {
    auto it = reverse_deps_.find(plugin_key);
    if (it != reverse_deps_.end()) {
        return it->second;
    }
    return {};
}

bool PluginManagerEnhanced::can_safely_unload(const std::string& plugin_key) {
    // Check if any active plugins depend on this one
    auto dependents = get_dependents(plugin_key);

    for (const auto& dependent : dependents) {
        if (is_plugin_loaded(dependent)) {
            return false;
        }
    }

    return true;
}

// Version compatibility
bool PluginManagerEnhanced::is_version_compatible(
    const std::string& plugin_key,
    const std::string& required_version) const {

    auto it = version_info_.find(plugin_key);
    if (it == version_info_.end()) {
        return true; // No version info, assume compatible
    }

    const auto& info = it->second;

    // Check if required version is within compatible range
    if (!info.min_compatible.empty() &&
        compare_versions(required_version, info.min_compatible) < 0) {
        return false;
    }

    if (!info.max_compatible.empty() &&
        compare_versions(required_version, info.max_compatible) > 0) {
        return false;
    }

    return true;
}

bool PluginManagerEnhanced::check_api_version(const PluginInfo& info) const {
    // Check major version match (required for compatibility)
    uint32_t required_major = xpumusic::XPUMUSIC_PLUGIN_API_VERSION >> 24;
    uint32_t plugin_major = info.api_version >> 24;

    return required_major == plugin_major;
}

std::string PluginManagerEnhanced::get_compatible_version_range(const std::string& plugin_key) const {
    auto it = version_info_.find(plugin_key);
    if (it == version_info_.end()) {
        return "any";
    }

    const auto& info = it->second;

    if (info.min_compatible.empty() && info.max_compatible.empty()) {
        return info.version;
    }

    std::string range = info.version;
    if (!info.min_compatible.empty() || !info.max_compatible.empty()) {
        range += " (compatible: ";
        if (!info.min_compatible.empty()) {
            range += ">=" + info.min_compatible;
        }
        if (!info.max_compatible.empty()) {
            if (!info.min_compatible.empty()) range += ", ";
            range += "<=" + info.max_compatible;
        }
        range += ")";
    }

    return range;
}

// Configuration management
bool PluginManagerEnhanced::load_plugin_config(const std::string& plugin_key) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    auto it = plugin_configs_.find(plugin_key);
    if (it != plugin_configs_.end()) {
        return true; // Already loaded
    }

    // Try to load from config file
    std::string config_path = get_config_file_path();
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json config;
        file >> config;

        if (config.contains("plugins") && config["plugins"].contains(plugin_key)) {
            plugin_configs_[plugin_key] = config["plugins"][plugin_key];
            return true;
        }
    } catch (const std::exception& e) {
        report_error(plugin_key, "CONFIG_LOAD_ERROR", e.what());
    }

    return false;
}

bool PluginManagerEnhanced::save_plugin_config(const std::string& plugin_key) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    auto it = plugin_configs_.find(plugin_key);
    if (it == plugin_configs_.end()) {
        return false; // No config to save
    }

    return save_config_file();
}

void PluginManagerEnhanced::set_plugin_config(
    const std::string& plugin_key,
    const nlohmann::json& config) {

    std::lock_guard<std::mutex> lock(config_mutex_);
    plugin_configs_[plugin_key] = config;

    // Publish config change event
    if (event_bus_) {
        nlohmann::json event;
        event["plugin"] = plugin_key;
        event["config"] = config;
        event_bus_->publish("plugin_config_changed", event);
    }
}

nlohmann::json PluginManagerEnhanced::get_plugin_config(
    const std::string& plugin_key,
    const nlohmann::json& default_config) const {

    std::lock_guard<std::mutex> lock(config_mutex_);

    auto it = plugin_configs_.find(plugin_key);
    if (it != plugin_configs_.end()) {
        return it->second;
    }

    return default_config;
}

void PluginManagerEnhanced::load_all_plugin_configs() {
    for (const auto& plugin : get_plugin_list()) {
        load_plugin_config(plugin.name);
    }
}

void PluginManagerEnhanced::save_all_plugin_configs() {
    save_config_file();
}

// Plugin state management
PluginLoadState PluginManagerEnhanced::get_plugin_state(const std::string& plugin_key) const {
    std::lock_guard<std::mutex> lock(load_state_mutex_);

    auto it = load_states_.find(plugin_key);
    if (it != load_states_.end()) {
        return it->second;
    }

    return PluginLoadState::NotLoaded;
}

std::vector<std::string> PluginManagerEnhanced::get_plugins_by_state(PluginLoadState state) const {
    std::vector<std::string> plugins;
    std::lock_guard<std::mutex> lock(load_state_mutex_);

    for (const auto& pair : load_states_) {
        if (pair.second == state) {
            plugins.push_back(pair.first);
        }
    }

    return plugins;
}

bool PluginManagerEnhanced::is_plugin_ready(const std::string& plugin_key) const {
    auto state = get_plugin_state(plugin_key);
    return state == PluginLoadState::Loaded;
}

// Error handling
std::vector<PluginError> PluginManagerEnhanced::get_error_history(
    const std::string& plugin_key) const {

    std::vector<PluginError> errors;
    std::lock_guard<std::mutex> lock(error_mutex_);

    for (const auto& error : error_history_) {
        if (plugin_key.empty() || error.plugin_key == plugin_key) {
            errors.push_back(error);
        }
    }

    return errors;
}

void PluginManagerEnhanced::clear_error_history(const std::string& plugin_key) {
    std::lock_guard<std::mutex> lock(error_mutex_);

    if (plugin_key.empty()) {
        error_history_.clear();
    } else {
        error_history_.erase(
            std::remove_if(error_history_.begin(), error_history_.end(),
                [&plugin_key](const PluginError& e) {
                    return e.plugin_key == plugin_key;
                }),
            error_history_.end());
    }
}

std::string PluginManagerEnhanced::get_last_error(const std::string& plugin_key) const {
    std::lock_guard<std::mutex> lock(error_mutex_);

    // Search backwards for the last error of this plugin
    for (auto it = error_history_.rbegin(); it != error_history_.rend(); ++it) {
        if (plugin_key.empty() || it->plugin_key == plugin_key) {
            return it->error_message;
        }
    }

    return "";
}

void PluginManagerEnhanced::report_error(
    const std::string& plugin_key,
    const std::string& error_code,
    const std::string& error_message) {

    PluginError error(plugin_key, error_code, error_message);

    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        error_history_.push_back(error);

        // Keep history size limited
        while (error_history_.size() > max_error_history_) {
            error_history_.erase(error_history_.begin());
        }
    }

    log_error(error);

    // Update load state
    {
        std::lock_guard<std::mutex> lock(load_state_mutex_);
        load_states_[plugin_key] = PluginLoadState::Error;
    }

    // Publish error event
    if (event_bus_) {
        nlohmann::json event;
        event["plugin"] = plugin_key;
        event["code"] = error_code;
        event["message"] = error_message;
        event_bus_->publish("plugin_error", event);
    }

    // Update stats
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        enhanced_stats_.failed_loads++;
    }
}

void PluginManagerEnhanced::log_error(const PluginError& error) {
    if (plugin_logger_) {
        plugin_logger_->error("[{}] {}: {}",
            error.error_code, error.plugin_key, error.error_message);
    }
}

// Metadata management
PluginMetadata PluginManagerEnhanced::get_plugin_metadata(const std::string& plugin_key) const {
    auto it = plugin_metadata_.find(plugin_key);
    if (it != plugin_metadata_.end()) {
        return it->second;
    }

    // Return empty metadata
    return PluginMetadata{};
}

void PluginManagerEnhanced::set_plugin_metadata(
    const std::string& plugin_key,
    const PluginMetadata& metadata) {

    plugin_metadata_[plugin_key] = metadata;

    // Update reverse dependencies
    for (const auto& dep : metadata.dependencies) {
        reverse_deps_[dep.name].push_back(plugin_key);
    }
}

void PluginManagerEnhanced::update_plugin_timestamp(const std::string& plugin_key) {
    auto it = plugin_metadata_.find(plugin_key);
    if (it != plugin_metadata_.end()) {
        it->second.load_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}

// Event publishing
void PluginManagerEnhanced::publish_lifecycle_event(const PluginLifecycleEvent& event) {
    if (event_bus_) {
        nlohmann::json json_event;
        json_event["plugin"] = event.plugin_key;
        json_event["action"] = event.action;
        json_event["details"] = event.details;
        json_event["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            event.timestamp.time_since_epoch()).count();

        event_bus_->publish("plugin_lifecycle", json_event);
    }
}

// Logging
void PluginManagerEnhanced::log_plugin_operation(
    const std::string& operation,
    const std::string& plugin_key,
    const std::string& details) {

    if (plugin_logger_) {
        plugin_logger_->info("{}: {}{}{}",
            operation,
            plugin_key,
            details.empty() ? "" : " - ",
            details);
    }
}

// Utility functions
int PluginManagerEnhanced::compare_versions(
    const std::string& v1,
    const std::string& v2) const {

    auto t1 = parse_version(v1);
    auto t2 = parse_version(v2);

    if (t1 > t2) return 1;
    if (t1 < t2) return -1;
    return 0;
}

std::tuple<int, int, int> PluginManagerEnhanced::parse_version(
    const std::string& version) const {

    std::tuple<int, int, int> result{0, 0, 0};
    std::istringstream iss(version);
    std::string part;

    // Parse major version
    if (std::getline(iss, part, '.')) {
        std::get<0>(result) = std::stoi(part);
    }

    // Parse minor version
    if (std::getline(iss, part, '.')) {
        std::get<1>(result) = std::stoi(part);
    }

    // Parse patch version
    if (std::getline(iss, part, '.')) {
        std::get<2>(result) = std::stoi(part);
    }

    return result;
}

// Configuration file handling
std::string PluginManagerEnhanced::get_config_file_path() const {
    // Expand ~ to home directory
    if (plugin_config_file_[0] == '~') {
        char* home = getenv("HOME");
        if (home) {
            return std::string(home) + plugin_config_file_.substr(1);
        }
    }
    return plugin_config_file_;
}

bool PluginManagerEnhanced::load_config_file() {
    std::string config_path = get_config_file_path();
    std::ifstream file(config_path);

    if (!file.is_open()) {
        return false; // File doesn't exist, that's OK
    }

    try {
        nlohmann::json config;
        file >> config;

        // Load plugin configurations
        if (config.contains("plugins")) {
            for (auto& [key, value] : config["plugins"].items()) {
                plugin_configs_[key] = value;
            }
        }

        // Load plugin metadata
        if (config.contains("metadata")) {
            for (auto& [key, value] : config["metadata"].items()) {
                PluginMetadata metadata = deserialize_plugin_metadata(value);
                set_plugin_metadata(key, metadata);
            }
        }

        // Load version info
        if (config.contains("versions")) {
            for (auto& [key, value] : config["versions"].items()) {
                VersionInfo info;
                info.version = value.value("version", "");
                info.min_compatible = value.value("min_compatible", "");
                info.max_compatible = value.value("max_compatible", "");
                info.api_version = value.value("api_version", 0);
                version_info_[key] = info;
            }
        }

        return true;
    } catch (const std::exception& e) {
        report_error("config_loader", "CONFIG_ERROR", e.what());
        return false;
    }
}

bool PluginManagerEnhanced::save_config_file() {
    std::string config_path = get_config_file_path();

    // Create directory if it doesn't exist
    std::filesystem::path path(config_path);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream file(config_path);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json config;

        // Save plugin configurations
        config["plugins"] = plugin_configs_;

        // Save plugin metadata
        for (const auto& [key, metadata] : plugin_metadata_) {
            config["metadata"][key] = serialize_plugin_metadata(metadata);
        }

        // Save version info
        for (const auto& [key, info] : version_info_) {
            nlohmann::json version_info;
            version_info["version"] = info.version;
            version_info["min_compatible"] = info.min_compatible;
            version_info["max_compatible"] = info.max_compatible;
            version_info["api_version"] = info.api_version;
            config["versions"][key] = version_info;
        }

        file << config.dump(4);
        return true;
    } catch (const std::exception& e) {
        report_error("config_saver", "CONFIG_ERROR", e.what());
        return false;
    }
}

nlohmann::json PluginManagerEnhanced::serialize_plugin_metadata(
    const PluginMetadata& metadata) const {

    nlohmann::json json;
    json["name"] = metadata.name;
    json["version"] = metadata.version;
    json["author"] = metadata.author;
    json["description"] = metadata.description;
    json["license"] = metadata.license;
    json["homepage"] = metadata.homepage;
    json["auto_load"] = metadata.auto_load;
    json["hot_reloadable"] = metadata.hot_reloadable;
    json["load_time"] = metadata.load_time;

    // Serialize dependencies
    nlohmann::json deps = nlohmann::json::array();
    for (const auto& dep : metadata.dependencies) {
        nlohmann::json dep_json;
        dep_json["name"] = dep.name;
        dep_json["version"] = dep.version;
        dep_json["optional"] = dep.optional;
        deps.push_back(dep_json);
    }
    json["dependencies"] = deps;

    // Serialize provides
    json["provides"] = metadata.provides;

    if (!metadata.config_schema.empty()) {
        json["config_schema"] = nlohmann::json::parse(metadata.config_schema);
    }

    return json;
}

PluginMetadata PluginManagerEnhanced::deserialize_plugin_metadata(
    const nlohmann::json& json) const {

    PluginMetadata metadata;
    metadata.name = json.value("name", "");
    metadata.version = json.value("version", "");
    metadata.author = json.value("author", "");
    metadata.description = json.value("description", "");
    metadata.license = json.value("license", "");
    metadata.homepage = json.value("homepage", "");
    metadata.auto_load = json.value("auto_load", true);
    metadata.hot_reloadable = json.value("hot_reloadable", true);
    metadata.load_time = json.value("load_time", 0);

    // Deserialize dependencies
    if (json.contains("dependencies")) {
        for (const auto& dep_json : json["dependencies"]) {
            PluginDependency dep;
            dep.name = dep_json.value("name", "");
            dep.version = dep_json.value("version", "");
            dep.optional = dep_json.value("optional", false);
            metadata.dependencies.push_back(dep);
        }
    }

    // Deserialize provides
    if (json.contains("provides")) {
        for (const auto& provide : json["provides"]) {
            metadata.provides.push_back(provide.get<std::string>());
        }
    }

    // Deserialize config schema
    if (json.contains("config_schema")) {
        metadata.config_schema = json["config_schema"].dump();
    }

    return metadata;
}

// Enhanced stats
PluginManagerEnhanced::EnhancedStats PluginManagerEnhanced::get_enhanced_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    EnhancedStats stats = enhanced_stats_;
    stats.registry_stats = get_stats();  // Include base stats

    return stats;
}

void PluginManagerEnhanced::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    enhanced_stats_ = EnhancedStats{};
}

// Overridden methods with enhancements
bool PluginManagerEnhanced::load_native_plugin(const std::string& path) {
    std::string plugin_key = std::filesystem::path(path).filename().string();

    // Set loading state
    {
        std::lock_guard<std::mutex> lock(load_state_mutex_);
        load_states_[plugin_key] = PluginLoadState::Loading;
    }

    publish_lifecycle_event(PluginLifecycleEvent(plugin_key, "loading"));
    log_plugin_operation("loading", plugin_key);

    // Check dependencies first
    if (!check_dependencies(plugin_key)) {
        {
            std::lock_guard<std::mutex> lock(load_state_mutex_);
            load_states_[plugin_key] = PluginLoadState::Error;
        }
        return false;
    }

    // Load plugin using base implementation
    bool success = PluginManager::load_native_plugin(path);

    if (success) {
        // Resolve dependencies
        if (!resolve_dependencies(plugin_key)) {
            unload_plugin(plugin_key);
            return false;
        }

        // Update state
        {
            std::lock_guard<std::mutex> lock(load_state_mutex_);
            load_states_[plugin_key] = PluginLoadState::Loaded;
        }

        // Load configuration
        load_plugin_config(plugin_key);

        publish_lifecycle_event(PluginLifecycleEvent(plugin_key, "loaded"));
    } else {
        {
            std::lock_guard<std::mutex> lock(load_state_mutex_);
            load_states_[plugin_key] = PluginLoadState::Error;
        }

        publish_lifecycle_event(PluginLifecycleEvent(plugin_key, "error", "Failed to load"));
    }

    return success;
}

bool PluginManagerEnhanced::unload_plugin(const std::string& key) {
    if (!is_plugin_loaded(key)) {
        return false;
    }

    // Check if safe to unload
    if (!can_safely_unload(key)) {
        report_error(key, "UNLOAD_ERROR", "Plugin has active dependents");
        return false;
    }

    // Set unloading state
    {
        std::lock_guard<std::mutex> lock(load_state_mutex_);
        load_states_[key] = PluginLoadState::Unloading;
    }

    publish_lifecycle_event(PluginLifecycleEvent(key, "unloading"));
    log_plugin_operation("unloading", key);

    // Save configuration
    save_plugin_config(key);

    // Unload using base implementation
    bool success = PluginManager::unload_plugin(key);

    // Update state
    {
        std::lock_guard<std::mutex> lock(load_state_mutex_);
        load_states_[key] = success ? PluginLoadState::NotLoaded : PluginLoadState::Error;
    }

    if (success) {
        publish_lifecycle_event(PluginLifecycleEvent(key, "unloaded"));
    } else {
        publish_lifecycle_event(PluginLifecycleEvent(key, "error", "Failed to unload"));
    }

    return success;
}

} // namespace xpumusic