/**
 * @file foobar_plugin_manager.h
 * @brief Foobar2000 plugin compatibility manager
 * @date 2025-12-13
 */

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include "plugin_error_handler.h"
#include "plugin_config.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Forward declarations
namespace xpumusic_sdk {
    class audio_decoder;
    struct audio_info;
}

/**
 * @brief Foobar2000 Plugin Manager
 * Handles loading and managing foobar2000 compatible plugins
 */
class FoobarPluginManager {
public:
    /**
     * @brief Plugin information structure
     */
    struct PluginInfo {
        std::string name;
        std::string version;
        std::string description;
        std::string file_path;
        std::vector<std::string> supported_extensions;
        std::vector<std::string> supported_mime_types;
        void* handle;  // Module handle
    };

    /**
     * @brief Decoder interface wrapper
     */
    class Decoder {
    public:
        virtual ~Decoder() = default;

        // Check if can decode file
        virtual bool can_decode(const std::string& file_path) = 0;

        // Open file for decoding
        virtual bool open(const std::string& file_path) = 0;

        // Get audio information
        virtual bool get_audio_info(xpumusic_sdk::audio_info& info) = 0;

        // Decode audio data
        virtual bool decode(float* buffer, size_t frames, size_t& decoded) = 0;

        // Seek to position
        virtual bool seek(double seconds) = 0;

        // Close decoder
        virtual void close() = 0;
    };

    using DecoderPtr = std::shared_ptr<Decoder>;

private:
    std::vector<PluginInfo> loaded_plugins_;
    std::map<std::string, DecoderPtr> active_decoders_;
    std::vector<PluginInfo> failed_plugins_;
    std::map<std::string, int> plugin_retry_count_;
    bool initialized_;
    std::unique_ptr<PluginErrorHandler> error_handler_;
    std::unique_ptr<PluginConfigManager> config_manager_;

public:
    FoobarPluginManager();
    ~FoobarPluginManager();

    // Initialize plugin system
    bool initialize();

    // Cleanup and shutdown
    void shutdown();

    // Initialize with config file
    bool initialize(const std::string& config_file = "plugins_config.json");

    // Load plugins from directory
    bool load_plugins_from_directory(const std::string& plugin_dir);

    // Load single plugin
    bool load_plugin(const std::string& plugin_path);

    // Get list of loaded plugins
    const std::vector<PluginInfo>& get_loaded_plugins() const;

    // Find decoder for file
    DecoderPtr find_decoder(const std::string& file_path);

    // Get supported file extensions
    std::vector<std::string> get_supported_extensions() const;

    // Get failed plugins
    const std::vector<PluginInfo>& get_failed_plugins() const;

    // Retry loading failed plugins
    bool retry_failed_plugins();

    // Unload specific plugin
    bool unload_plugin(const std::string& plugin_name);

    // Get error handler instance
    PluginErrorHandler* get_error_handler() const;

    // Generate error report
    std::string generate_error_report() const;

    // Configuration management
    PluginConfigManager* get_config_manager() const;

    // Get plugin configuration
    ConfigSection* get_plugin_config(const std::string& plugin_name);

    // Set plugin parameter
    bool set_plugin_parameter(const std::string& plugin_name, const std::string& param_key, const ConfigValue& value);

    // Get plugin parameter
    ConfigValue get_plugin_parameter(const std::string& plugin_name, const std::string& param_key, const ConfigValue& default_value = ConfigValue()) const;

    // Enable/disable plugin
    bool set_plugin_enabled(const std::string& plugin_name, bool enabled);

    // Check if plugin is enabled
    bool is_plugin_enabled(const std::string& plugin_name) const;

    // Save configuration
    bool save_configuration() const;

    // Load configuration
    bool load_configuration(const std::string& config_file = "");

private:
    // Register decoder from plugin
    bool register_decoder(const std::string& extension, DecoderPtr decoder);

    // Validate plugin before loading
    bool validate_plugin(const std::string& plugin_path, std::string& error_msg);

    // Unload plugin internal
    void unload_plugin_internal(const PluginInfo& plugin);

    // Check if plugin should be retried
    bool should_retry_plugin(const std::string& plugin_path) const;
};