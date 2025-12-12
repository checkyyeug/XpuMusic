#pragma once

#include "mp_types.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace mp {
namespace compat {

// Forward declarations
class AdapterBase;
class InputDecoderAdapter;
class DataMigrationManager;

// Compatibility configuration
struct CompatConfig {
    std::string foobar_install_path;
    bool enable_plugin_compat = true;
    bool enable_data_migration = true;
    bool compat_mode_strict = false;
    int adapter_logging_level = 1;  // 0=off, 1=errors, 2=warnings, 3=debug
    
    CompatConfig() = default;
};

// Compatibility status
enum class CompatStatus {
    NotInitialized,
    Initializing,
    Ready,
    Disabled,
    Error
};

// Foobar2000 detection result
struct FoobarDetectionResult {
    bool found = false;
    std::string install_path;
    std::string version;
    std::string components_path;
    std::string user_components_path;
    
    FoobarDetectionResult() = default;
};

// Compatibility feature flags
enum class CompatFeature : uint32_t {
    None = 0,
    InputDecoder = 1 << 0,      // Input decoder plugin support
    DSPChain = 1 << 1,          // DSP plugin support
    UIExtension = 1 << 2,       // UI extension support
    Metadb = 1 << 3,            // Metadata database
    PlaylistMigration = 1 << 4, // FPL playlist conversion
    ConfigMigration = 1 << 5,   // Configuration migration
    LibraryMigration = 1 << 6   // Library database migration
};

inline CompatFeature operator|(CompatFeature a, CompatFeature b) {
    return static_cast<CompatFeature>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline CompatFeature operator&(CompatFeature a, CompatFeature b) {
    return static_cast<CompatFeature>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// XpuMusic Compatibility Manager
// Central coordinator for all foobar2000 compatibility features
class XpuMusicCompatManager {
public:
    XpuMusicCompatManager();
    ~XpuMusicCompatManager();
    
    // Initialize compatibility layer
    Result initialize(const CompatConfig& config);
    
    // Shutdown compatibility layer
    void shutdown();
    
    // Detect foobar2000 installation
    FoobarDetectionResult detect_foobar2000();
    
    // Get current status
    CompatStatus get_status() const { return status_; }
    
    // Get enabled features
    CompatFeature get_enabled_features() const { return enabled_features_; }
    
    // Check if specific feature is enabled
    bool is_feature_enabled(CompatFeature feature) const {
        return static_cast<uint32_t>(enabled_features_ & feature) != 0;
    }
    
    // Get configuration
    const CompatConfig& get_config() const { return config_; }
    
    // Update configuration
    Result update_config(const CompatConfig& config);
    
    // Register an adapter
    Result register_adapter(std::unique_ptr<AdapterBase> adapter);
    
    // Get adapter by type
    template<typename T>
    T* get_adapter() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& adapter : adapters_) {
            T* typed = dynamic_cast<T*>(adapter.get());
            if (typed) return typed;
        }
        return nullptr;
    }
    
    // Scan for foobar2000 plugins
    Result scan_plugins(const std::string& directory);
    
    // Get detected plugin count
    size_t get_plugin_count() const { return detected_plugins_.size(); }
    
    // Get detected plugin info
    struct PluginInfo {
        std::string path;
        std::string name;
        std::string type;
        bool loaded;
    };
    const std::vector<PluginInfo>& get_detected_plugins() const {
        return detected_plugins_;
    }
    
    // Get data migration manager
    DataMigrationManager* get_migration_manager();
    
    // Log compatibility message
    void log(int level, const char* format, ...);
    
private:
    // Platform-specific detection
    FoobarDetectionResult detect_windows();
    FoobarDetectionResult detect_linux();
    FoobarDetectionResult detect_macos();
    
    // Initialize adapters
    Result init_adapters();
    
    // Validate installation
    bool validate_installation(const std::string& path);
    
    CompatConfig config_;
    CompatStatus status_;
    CompatFeature enabled_features_;
    
    std::vector<std::unique_ptr<AdapterBase>> adapters_;
    std::vector<PluginInfo> detected_plugins_;
    std::unique_ptr<DataMigrationManager> migration_manager_;
    
    mutable std::mutex mutex_;
    bool initialized_;
};

} // namespace compat
} // namespace mp
