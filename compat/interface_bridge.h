/**
 * @file interface_bridge.h
 * @brief Interface isolation bridge for foobar2000 compatibility
 * @date 2025-12-13
 *
 * This file provides isolation mechanisms to prevent foobar2000-specific
 * implementation details from leaking into the main XpuMusic codebase.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

// Forward declarations - avoid exposing foobar2000 headers
namespace foobar2000 {
    class audio_chunk;
    class file_info;
    class abort_callback;
    class service_base;
}

namespace xpumusic_sdk {
    struct audio_info;
    struct file_stats;
    struct field_value;
}

namespace fb2k_compat {

/**
 * @brief Safe wrapper for foobar2000 objects
 *
 * Provides automatic cleanup and prevents direct access to foobar2000
 * internals from outside the compatibility layer.
 */
template<typename T>
class SafeWrapper {
public:
    using Deleter = std::function<void(T*)>;

    SafeWrapper(T* ptr, Deleter deleter)
        : ptr_(ptr, deleter) {}

    // Disable copy, allow move
    SafeWrapper(const SafeWrapper&) = delete;
    SafeWrapper& operator=(const SafeWrapper&) = delete;
    SafeWrapper(SafeWrapper&&) = default;
    SafeWrapper& operator=(SafeWrapper&&) = default;

    // Access only through safe methods
    T* get() const noexcept { return ptr_.get(); }
    bool is_valid() const noexcept { return ptr_ != nullptr; }

private:
    std::unique_ptr<T, Deleter> ptr_;
};

/**
 * @brief Service manager isolation layer
 *
 * Manages the lifecycle of foobar2000 services without exposing
 * service management details to the rest of the application.
 */
class ServiceManager {
public:
    ServiceManager();
    ~ServiceManager();

    // Prevent copying
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;

    /**
     * @brief Initialize service manager
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown service manager
     */
    void shutdown();

    /**
     * @brief Get service by type (template version)
     * @return Safe wrapper around the service
     */
    template<typename T>
    SafeWrapper<T> get_service();

    /**
     * @brief Check if a service is available
     * @return true if service exists
     */
    bool has_service(const std::string& service_name) const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Audio data bridge
 *
 * Safely converts between foobar2000 audio data and XpuMusic format
 * without exposing internal audio chunk representations.
 */
class AudioDataBridge {
public:
    /**
     * @brief Convert foobar2000 audio chunk to XpuMusic format
     * @param fb2k_chunk foobar2000 audio chunk
     * @param output_buffer Output buffer for converted data
     * @param frames Number of frames to convert
     * @return true if successful
     */
    static bool convert_from_foobar2000(
        void* fb2k_chunk,
        float* output_buffer,
        size_t frames
    );

    /**
     * @brief Convert XpuMusic format to foobar2000 audio chunk
     * @param input_buffer Input audio data
     * @param frames Number of frames
     * @return Safe wrapper around created audio chunk
     */
    static SafeWrapper<void> convert_to_foobar2000(
        const float* input_buffer,
        size_t frames,
        int channels,
        int sample_rate
    );
};

/**
 * @brief Metadata bridge
 *
 * Handles conversion between foobar2000 metadata and XpuMusic format
 * while preserving data integrity.
 */
class MetadataBridge {
public:
    /**
     * @brief Convert foobar2000 file info to XpuMusic format
     * @param fb2k_info foobar2000 file info
     * @param audio_info Output audio information
     * @param file_stats Output file statistics
     * @param metadata Output metadata key-value pairs
     * @return true if successful
     */
    static bool convert_from_foobar2000(
        void* fb2k_info,
        xpumusic_sdk::audio_info& audio_info,
        xpumusic_sdk::file_stats& file_stats,
        std::vector<std::pair<std::string, std::string>>& metadata
    );

    /**
     * @brief Get multi-value metadata field
     * @param fb2k_info foobar2000 file info
     * @param field_name Name of the metadata field
     * @return Field value with all entries
     */
    static xpumusic_sdk::field_value get_metadata_field(
        void* fb2k_info,
        const char* field_name
    );
};

/**
 * @brief Error handling isolation
 *
 * Converts foobar2000-specific errors and exceptions into
 * standard XpuMusic error handling mechanisms.
 */
class ErrorHandler {
public:
    /**
     * @brief Exception type for foobar2000 compatibility errors
     */
    class CompatibilityError : public std::runtime_error {
    public:
        explicit CompatibilityError(const std::string& msg)
            : std::runtime_error(msg) {}
    };

    /**
     * @brief Convert foobar2000 exception to standard exception
     * @param e foobar2000 exception
     * @return Converted standard exception
     */
    static std::exception_ptr convert_exception(std::exception_ptr e);

    /**
     * @brief Log foobar2000 error without exposing details
     * @param context Context where error occurred
     * @param error_code foobar2000 error code
     */
    static void log_error(const std::string& context, int error_code);
};

/**
 * @brief Plugin interface isolation
 *
 * Provides a safe interface to foobar2000 plugins without
 * exposing plugin internals.
 */
class PluginInterface {
public:
    /**
     * @brief Information about a loaded plugin
     */
    struct PluginInfo {
        std::string name;
        std::string version;
        std::string description;
        std::vector<std::string> supported_extensions;
    };

    /**
     * @brief Load a plugin from file
     * @param plugin_path Path to plugin DLL
     * @return Safe wrapper around loaded plugin
     */
    static SafeWrapper<void> load_plugin(const std::string& plugin_path);

    /**
     * @brief Get plugin information
     * @param plugin_handle Plugin handle
     * @return Plugin information
     */
    static PluginInfo get_plugin_info(void* plugin_handle);

    /**
     * @brief Check if plugin supports a file format
     * @param plugin_handle Plugin handle
     * @param file_extension File extension (without dot)
     * @return true if supported
     */
    static bool supports_format(void* plugin_handle, const std::string& file_extension);
};

/**
 * @brief Main bridge interface
 *
 * Primary interface for XpuMusic components to interact with
 * foobar2000 compatibility layer safely.
 */
class Bridge {
public:
    /**
     * @brief Initialize the bridge
     * @return true if successful
     */
    static bool initialize();

    /**
     * @brief Shutdown the bridge
     */
    static void shutdown();

    /**
     * @brief Get service manager
     * @return Reference to service manager
     */
    static ServiceManager& service_manager();

    /**
     * @brief Check if bridge is initialized
     * @return true if initialized
     */
    static bool is_initialized();

private:
    static bool initialized_;
    static ServiceManager service_manager_;
};

/**
 * @brief RAII helper for bridge initialization
 */
class BridgeInitializer {
public:
    BridgeInitializer() {
        Bridge::initialize();
    }

    ~BridgeInitializer() {
        Bridge::shutdown();
    }

    // Prevent copying
    BridgeInitializer(const BridgeInitializer&) = delete;
    BridgeInitializer& operator=(const BridgeInitializer&) = delete;
};

} // namespace fb2k_compat

// Convenience macros for using the bridge
#define FB2K_BRIDGE_INIT() \
    fb2k_compat::BridgeInitializer __bridge_initializer;

#define FB2K_BRIDGE_SAFE_GET(service_type) \
    fb2k_compat::Bridge::service_manager().get_service<service_type>()