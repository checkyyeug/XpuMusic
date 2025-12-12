/**
 * @file foobar_plugin_manager.cpp
 * @brief Foobar2000 plugin compatibility manager implementation
 * @date 2025-12-13
 */

#include "foobar_plugin_manager.h"
#include "../compat/sdk_implementations/foobar_sdk_wrapper.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <fstream>

// Macro to suppress unused parameter warnings
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (P)
#endif
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <dlfcn.h>
#include <dirent.h>
#endif

FoobarPluginManager::FoobarPluginManager()
    : initialized_(false),
      error_handler_(std::make_unique<PluginErrorHandler>()),
      config_manager_(std::make_unique<PluginConfigManager>()) {
}

FoobarPluginManager::~FoobarPluginManager() {
    shutdown();
}

// Simple decoder wrapper implementation
class FoobarDecoderWrapper : public FoobarPluginManager::Decoder {
private:
    FoobarPluginManager::PluginInfo plugin_info_;
    bool file_open_;
    std::string current_file_;

public:
    FoobarDecoderWrapper(const FoobarPluginManager::PluginInfo& info)
        : plugin_info_(info), file_open_(false) {
    }

    bool can_decode(const std::string& file_path) override {
        // Simple extension check
        size_t dot_pos = file_path.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string ext = file_path.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            for (const auto& supported_ext : plugin_info_.supported_extensions) {
                if (supported_ext == ext) {
                    return true;
                }
            }
        }
        return false;
    }

    bool open(const std::string& file_path) override {
        current_file_ = file_path;
        file_open_ = true;
        return true;
    }

    bool get_audio_info(xpumusic_sdk::audio_info& info) override {
        // Simplified - would call actual plugin
        info.m_sample_rate = 44100;
        info.m_channels = 2;
        info.m_bitrate = 128000;
        info.m_length = 180.0;  // 3 minutes default
        return true;
    }

    bool decode(float* buffer, size_t frames, size_t& decoded) override {
        // Simplified - would call actual plugin
        decoded = frames;
        // Fill with silence for now
        memset(buffer, 0, frames * 2 * sizeof(float));
        return true;
    }

    bool seek(double seconds) override {
        UNREFERENCED_PARAMETER(seconds);
        return true;
    }

    void close() override {
        file_open_ = false;
        current_file_.clear();
    }
};

bool FoobarPluginManager::initialize() {
    if (initialized_) {
        return true;
    }

    // Initialize error handler
    if (!error_handler_->initialize("plugin_errors.log", true, 500)) {
        std::cerr << "Warning: Could not initialize error logging" << std::endl;
    }

    error_handler_->log_info("Initializing Foobar2000 Plugin Manager...", "System");

    // Initialize SDK wrapper
    if (!xpumusic_sdk::initialize_foobar_sdk()) {
        error_handler_->log_critical("Failed to initialize Foobar2000 SDK", "System");
        return false;
    }

    initialized_ = true;
    error_handler_->log_info("Plugin Manager initialized successfully!", "System");
    return true;
}

void FoobarPluginManager::shutdown() {
    if (!initialized_) {
        return;
    }

    // Close all active decoders
    active_decoders_.clear();

    // Unload all plugins
    for (const auto& plugin : loaded_plugins_) {
        unload_plugin_internal(plugin);
    }
    loaded_plugins_.clear();

    // Shutdown SDK
    xpumusic_sdk::shutdown_foobar_sdk();

    initialized_ = false;
    std::cout << "Plugin Manager shutdown complete." << std::endl;
}

bool FoobarPluginManager::load_plugins_from_directory(const std::string& plugin_dir) {
    std::cout << "Loading plugins from: " << plugin_dir << std::endl;

#ifdef _WIN32
    std::string search_path = plugin_dir + "\\*.dll";
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "No plugins found in directory: " << plugin_dir << std::endl;
        return true;  // Not an error
    }

    bool loaded_any = false;
    do {
        std::string plugin_path = plugin_dir + "\\" + find_data.cFileName;
        if (load_plugin(plugin_path)) {
            loaded_any = true;
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
    return loaded_any;
#else
    DIR* dir = opendir(plugin_dir.c_str());
    if (!dir) {
        std::cout << "Cannot open plugin directory: " << plugin_dir << std::endl;
        return true;
    }

    struct dirent* entry;
    bool loaded_any = false;
    while ((entry = readdir(dir)) != nullptr) {
        if (strstr(entry->d_name, ".so")) {
            std::string plugin_path = plugin_dir + "/" + entry->d_name;
            if (load_plugin(plugin_path)) {
                loaded_any = true;
            }
        }
    }

    closedir(dir);
    return loaded_any;
#endif
}

bool FoobarPluginManager::load_plugin(const std::string& plugin_path) {
    error_handler_->log_info("Attempting to load plugin: " + plugin_path);

    // Validate plugin file
    std::string validation_error;
    if (!validate_plugin(plugin_path, validation_error)) {
        error_handler_->log_error(ErrorSeverity::Error, PluginErrorCode::InvalidFileFormat,
                                 validation_error, "", plugin_path);
        return false;
    }

#ifdef _WIN32
    HMODULE hModule = LoadLibraryA(plugin_path.c_str());
    if (!hModule) {
        DWORD error_code = GetLastError();
        std::string error_msg = "Failed to load plugin library";

        if (error_code == ERROR_ACCESS_DENIED) {
            error_msg = "Access denied - insufficient permissions";
        } else if (error_code == ERROR_BAD_EXE_FORMAT) {
            error_msg = "Invalid file format or corrupted file";
        } else if (error_code == ERROR_NOT_ENOUGH_MEMORY) {
            error_msg = "Insufficient memory to load plugin";
        }

        error_handler_->log_error(ErrorSeverity::Error, PluginErrorCode::LibraryLoadFailed,
                                 error_msg, "", plugin_path, "Error code: " + std::to_string(error_code));
        return false;
    }

    // Try to find plugin entry point
    typedef void* (*GetPluginInfoFunc)();
    GetPluginInfoFunc get_plugin_info = (GetPluginInfoFunc)GetProcAddress(hModule, "get_plugin_info");

    if (!get_plugin_info) {
        // Try alternative entry points
        const char* entry_points[] = {"_get_plugin_info@0", "GetPluginInfo", "plugin_init", "foobar2000_get_plugin"};
        for (const char* ep : entry_points) {
            get_plugin_info = (GetPluginInfoFunc)GetProcAddress(hModule, ep);
            if (get_plugin_info) break;
        }
    }

    if (!get_plugin_info) {
        error_handler_->log_error(ErrorSeverity::Error, PluginErrorCode::EntryPointNotFound,
                                 "Plugin entry point not found - plugin may be incompatible",
                                 "", plugin_path);
        FreeLibrary(hModule);
        return false;
    }

    // Get plugin info
    void* plugin_data = nullptr;
    try {
        plugin_data = get_plugin_info();
    } catch (...) {
        error_handler_->log_error(ErrorSeverity::Critical, PluginErrorCode::PluginCrashed,
                                 "Plugin entry point crashed during initialization",
                                 "", plugin_path);
        FreeLibrary(hModule);
        return false;
    }

    if (!plugin_data) {
        error_handler_->log_error(ErrorSeverity::Error, PluginErrorCode::InitializationFailed,
                                 "Failed to get plugin info from entry point",
                                 "", plugin_path);
        FreeLibrary(hModule);
        return false;
    }

    // Create plugin info entry
    PluginInfo info;
    info.name = PathFindFileNameA(plugin_path.c_str());
    info.version = "1.0";
    info.description = "Foobar2000 Plugin";
    info.file_path = plugin_path;
    info.handle = hModule;

    // Extract supported extensions from filename or metadata
    // This is simplified - real implementation would parse plugin capabilities
    info.supported_extensions.push_back("mp3");
    info.supported_extensions.push_back("flac");
    info.supported_extensions.push_back("ogg");
    info.supported_extensions.push_back("wav");

    loaded_plugins_.push_back(info);
    error_handler_->log_info("Successfully loaded plugin: " + info.name, info.name);
    return true;

#else
    void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
    if (!handle) {
        const char* error = dlerror();
        error_handler_->log_error(ErrorSeverity::Error, PluginErrorCode::LibraryLoadFailed,
                                 std::string("Failed to load plugin: ") + (error ? error : "unknown error"),
                                 "", plugin_path);
        return false;
    }

    // Similar implementation for Linux/Unix
    // ... (omitted for brevity)

    dlclose(handle);
    return true;
#endif
}

const std::vector<FoobarPluginManager::PluginInfo>& FoobarPluginManager::get_loaded_plugins() const {
    return loaded_plugins_;
}

FoobarPluginManager::DecoderPtr FoobarPluginManager::find_decoder(const std::string& file_path) {
    // Extract extension
    std::string extension;
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        extension = file_path.substr(dot_pos + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    }

    // Check if we have a decoder for this extension
    auto it = active_decoders_.find(extension);
    if (it != active_decoders_.end()) {
        return it->second;
    }

      // Check if any loaded plugin can handle this extension
    for (const auto& plugin : loaded_plugins_) {
        for (const auto& ext : plugin.supported_extensions) {
            if (ext == extension) {
                // Create a simple decoder wrapper
                auto decoder = std::make_shared<FoobarDecoderWrapper>(plugin);
                active_decoders_[extension] = decoder;
                return decoder;
            }
        }
    }

    return nullptr;
}

std::vector<std::string> FoobarPluginManager::get_supported_extensions() const {
    std::vector<std::string> extensions;
    for (const auto& plugin : loaded_plugins_) {
        extensions.insert(extensions.end(),
                         plugin.supported_extensions.begin(),
                         plugin.supported_extensions.end());
    }

    // Remove duplicates
    std::sort(extensions.begin(), extensions.end());
    extensions.erase(std::unique(extensions.begin(), extensions.end()), extensions.end());

    return extensions;
}

bool FoobarPluginManager::register_decoder(const std::string& extension, DecoderPtr decoder) {
    active_decoders_[extension] = decoder;
    return true;
}

const std::vector<FoobarPluginManager::PluginInfo>& FoobarPluginManager::get_failed_plugins() const {
    return failed_plugins_;
}

bool FoobarPluginManager::retry_failed_plugins() {
    error_handler_->log_info("Retrying failed plugins...", "System");

    bool any_success = false;
    auto it = failed_plugins_.begin();

    while (it != failed_plugins_.end()) {
        const std::string& plugin_path = it->file_path;

        if (should_retry_plugin(plugin_path)) {
            error_handler_->log_info("Retrying plugin: " + plugin_path);

            if (load_plugin(plugin_path)) {
                // Success - remove from failed list
                error_handler_->log_info("Successfully retried plugin: " + it->name, it->name);
                it = failed_plugins_.erase(it);
                any_success = true;
            } else {
                // Failed again - increment retry count
                plugin_retry_count_[plugin_path]++;
                ++it;
            }
        } else {
            ++it;
        }
    }

    return any_success;
}

bool FoobarPluginManager::unload_plugin(const std::string& plugin_name) {
    // Find and unload the plugin
    for (auto it = loaded_plugins_.begin(); it != loaded_plugins_.end(); ++it) {
        if (it->name == plugin_name) {
            error_handler_->log_info("Unloading plugin: " + plugin_name, plugin_name);
            unload_plugin_internal(*it);
            loaded_plugins_.erase(it);
            return true;
        }
    }

    error_handler_->log_warning("Plugin not found for unloading: " + plugin_name);
    return false;
}

PluginErrorHandler* FoobarPluginManager::get_error_handler() const {
    return error_handler_.get();
}

std::string FoobarPluginManager::generate_error_report() const {
    std::string report = "=== Foobar Plugin Manager Error Report ===\n\n";

    // Plugin statistics
    report += "Plugin Statistics:\n";
    report += "  Loaded Plugins: " + std::to_string(loaded_plugins_.size()) + "\n";
    report += "  Failed Plugins: " + std::to_string(failed_plugins_.size()) + "\n";
    report += "  Active Decoders: " + std::to_string(active_decoders_.size()) + "\n\n";

    // Error handler report
    if (error_handler_) {
        report += error_handler_->generate_error_report();
    }

    // Failed plugins list
    if (!failed_plugins_.empty()) {
        report += "\nFailed Plugins:\n";
        for (const auto& plugin : failed_plugins_) {
            report += "  - " + plugin.name + " (" + plugin.file_path + ")\n";
        }
    }

    return report;
}

bool FoobarPluginManager::validate_plugin(const std::string& plugin_path, std::string& error_msg) {
    // Check if file exists
    std::ifstream file(plugin_path);
    if (!file.good()) {
        error_msg = "Plugin file does not exist or cannot be accessed";
        return false;
    }
    file.close();

    // Check file size
    std::filesystem::path path(plugin_path);
    try {
        uintmax_t file_size = std::filesystem::file_size(path);
        if (file_size == 0) {
            error_msg = "Plugin file is empty";
            return false;
        }
        if (file_size > 100 * 1024 * 1024) {  // 100MB limit
            error_msg = "Plugin file is too large (>100MB)";
            return false;
        }
    } catch (const std::exception& e) {
        error_msg = std::string("Failed to check file size: ") + e.what();
        return false;
    }

    // Check file extension
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
#ifdef _WIN32
    if (ext != ".dll") {
        error_msg = "Invalid plugin extension (expected .dll on Windows)";
        return false;
    }
#else
    if (ext != ".so") {
        error_msg = "Invalid plugin extension (expected .so on Linux/Unix)";
        return false;
    }
#endif

    // Check file magic bytes
    file.open(plugin_path, std::ios::binary);
    if (file.is_open()) {
        char magic[2] = {0};
        file.read(magic, 2);
        file.close();

#ifdef _WIN32
        // Check for PE header
        if (magic[0] != 'M' || magic[1] != 'Z') {
            error_msg = "Invalid PE header - not a valid Windows DLL";
            return false;
        }
#endif
    }

    return true;
}

void FoobarPluginManager::unload_plugin_internal(const PluginInfo& plugin) {
    if (plugin.handle) {
        try {
#ifdef _WIN32
            if (!FreeLibrary((HMODULE)plugin.handle)) {
                DWORD error = GetLastError();
                error_handler_->log_error(ErrorSeverity::Warning, PluginErrorCode::InitializationFailed,
                                         "Failed to unload plugin cleanly",
                                         plugin.name, plugin.file_path,
                                         "Error code: " + std::to_string(error));
            }
#else
            if (dlclose(plugin.handle) != 0) {
                const char* error = dlerror();
                error_handler_->log_error(ErrorSeverity::Warning, PluginErrorCode::InitializationFailed,
                                         std::string("Failed to unload plugin: ") + (error ? error : "unknown"),
                                         plugin.name, plugin.file_path);
            }
#endif
        } catch (...) {
            error_handler_->log_error(ErrorSeverity::Error, PluginErrorCode::PluginCrashed,
                                     "Exception occurred during plugin unload",
                                     plugin.name, plugin.file_path);
        }
    }

    // Remove any decoders associated with this plugin
    auto it = active_decoders_.begin();
    while (it != active_decoders_.end()) {
        if (it->second && it->second.use_count() == 1) {
            it = active_decoders_.erase(it);
        } else {
            ++it;
        }
    }
}

bool FoobarPluginManager::should_retry_plugin(const std::string& plugin_path) const {
    auto it = plugin_retry_count_.find(plugin_path);
    if (it == plugin_retry_count_.end()) {
        return true;  // First time trying
    }

    // Allow up to 3 retries
    return it->second < 3;
}

bool FoobarPluginManager::initialize(const std::string& config_file) {
    if (initialized_) {
        return true;
    }

    // Initialize configuration manager first
    if (!config_manager_->initialize(config_file, true)) {
        error_handler_->log_warning("Could not initialize plugin configuration, using defaults");
    }

    // Initialize error handler
    if (!error_handler_->initialize("plugin_errors.log", true, 500)) {
        std::cerr << "Warning: Could not initialize error logging" << std::endl;
    }

    error_handler_->log_info("Initializing Foobar2000 Plugin Manager...", "System");

    // Initialize SDK wrapper
    if (!xpumusic_sdk::initialize_foobar_sdk()) {
        error_handler_->log_critical("Failed to initialize Foobar2000 SDK", "System");
        return false;
    }

    initialized_ = true;
    error_handler_->log_info("Plugin Manager initialized successfully!", "System");
    return true;
}

PluginConfigManager* FoobarPluginManager::get_config_manager() const {
    return config_manager_.get();
}

ConfigSection* FoobarPluginManager::get_plugin_config(const std::string& plugin_name) {
    return config_manager_->get_section(plugin_name);
}

bool FoobarPluginManager::set_plugin_parameter(const std::string& plugin_name, const std::string& param_key, const ConfigValue& value) {
    ConfigSection* section = get_plugin_config(plugin_name);
    if (!section) {
        section = config_manager_->create_section(plugin_name);
        if (!section) {
            return false;
        }
    }

    // If parameter doesn't exist, add it
    if (!section->get_param(param_key)) {
        ConfigParam param(param_key, param_key, "Auto-generated parameter", value);
        section->add_param(param);
    }

    if (section->set_value(param_key, value)) {
        error_handler_->log_info("Updated plugin parameter: " + param_key + " = " +
                                std::visit([](auto&& arg) -> std::string {
                                    using T = std::decay_t<decltype(arg)>;
                                    if constexpr (std::is_same_v<T, bool>) {
                                        return arg ? "true" : "false";
                                    } else if constexpr (std::is_same_v<T, int>) {
                                        return std::to_string(arg);
                                    } else if constexpr (std::is_same_v<T, double>) {
                                        return std::to_string(arg);
                                    } else if constexpr (std::is_same_v<T, std::string>) {
                                        return arg;
                                    } else {
                                        return "";
                                    }
                                }, value), plugin_name);
        config_manager_->notify_change(plugin_name, param_key, value);
        return true;
    }

    return false;
}

ConfigValue FoobarPluginManager::get_plugin_parameter(const std::string& plugin_name, const std::string& param_key, const ConfigValue& default_value) const {
    const ConfigSection* section = config_manager_->get_section(plugin_name);
    if (!section) {
        return default_value;
    }

    ConfigValue value = section->get_value(param_key);
    // Check if value is empty (default return for missing key)
    if (std::holds_alternative<std::string>(value) && std::get<std::string>(value).empty()) {
        return default_value;
    }
    return value;
}

bool FoobarPluginManager::set_plugin_enabled(const std::string& plugin_name, bool enabled) {
    ConfigSection* section = get_plugin_config(plugin_name);
    if (!section) {
        section = config_manager_->create_section(plugin_name);
        if (!section) {
            return false;
        }
    }

    section->set_enabled(enabled);
    error_handler_->log_info("Plugin " + plugin_name + " " + (enabled ? "enabled" : "disabled"));
    config_manager_->notify_change(plugin_name, "enabled", enabled);
    return true;
}

bool FoobarPluginManager::is_plugin_enabled(const std::string& plugin_name) const {
    const ConfigSection* section = config_manager_->get_section(plugin_name);
    return section ? section->is_enabled() : true;  // Default to enabled
}

bool FoobarPluginManager::save_configuration() const {
    return config_manager_->save_config();
}

bool FoobarPluginManager::load_configuration(const std::string& config_file) {
    if (!config_file.empty()) {
        return config_manager_->initialize(config_file);
    }
    return config_manager_->load_config();
}