#include "xpumusic_compat_manager.h"
#include "adapter_base.h"
#include "data_migration_manager.h"
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <iostream>

// Logging macros
#define COMPAT_LOG_ERROR(msg) std::cerr << msg << std::endl
#define COMPAT_LOG_WARN(msg)  std::cout << "[WARNING] " << msg << std::endl
#define COMPAT_LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define COMPAT_LOG_INFO(msg)  std::cout << "[INFO] " << msg << std::endl

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace mp {
namespace compat {

XpuMusicCompatManager::XpuMusicCompatManager()
    : status_(CompatStatus::NotInitialized)
    , enabled_features_(CompatFeature::None)
    , initialized_(false) {
}

XpuMusicCompatManager::~XpuMusicCompatManager() {
    shutdown();
}

Result XpuMusicCompatManager::initialize(const CompatConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return Result::Error;
    }

    status_ = CompatStatus::Initializing;
    config_ = config;

    log(2, "Initializing foobar2000 compatibility layer...");

    // Detect foobar2000 installation
    FoobarDetectionResult detection = detect_foobar2000();

    if (!detection.found && config_.foobar_install_path.empty()) {
        log(1, "foobar2000 installation not found. Compatibility features disabled.");
        status_ = CompatStatus::Disabled;
        enabled_features_ = CompatFeature::None;
        return Result::Success; // Not an error, just disabled
    }

    // Initialize enabled features based on configuration
    enabled_features_ = CompatFeature::None;

    if (config_.enable_plugin_compat) {
        enabled_features_ = enabled_features_ | CompatFeature::InputDecoder;
        log(2, "Plugin compatibility enabled");
    }

    if (config_.enable_data_migration) {
        enabled_features_ = enabled_features_ | CompatFeature::PlaylistMigration
                                                | CompatFeature::ConfigMigration
                                                | CompatFeature::LibraryMigration;
        log(2, "Data migration enabled");
    }

    // Initialize adapters
    if (is_feature_enabled(CompatFeature::InputDecoder)) {
        // Initialize input decoder adapters
        log(2, "Initializing input decoder adapters...");
    }

    if (is_feature_enabled(CompatFeature::PlaylistMigration)) {
        // Initialize playlist migration
        log(2, "Initializing playlist migration...");
    }

    initialized_ = true;
    status_ = CompatStatus::Ready;

    log(2, "foobar2000 compatibility layer initialized successfully");
    return Result::Success;
}

void XpuMusicCompatManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }
    
    log(2, "Shutting down foobar2000 compatibility layer...");
    
    // Shutdown migration manager
    if (migration_manager_) {
        migration_manager_->shutdown();
        migration_manager_.reset();
    }
    
    // Shutdown adapters
    for (auto& adapter : adapters_) {
        adapter->shutdown();
    }
    adapters_.clear();
    
    detected_plugins_.clear();
    status_ = CompatStatus::NotInitialized;
    initialized_ = false;
    
    log(2, "foobar2000 compatibility layer shutdown complete");
}

FoobarDetectionResult XpuMusicCompatManager::detect_foobar2000() {
#ifdef _WIN32
    return detect_windows();
#elif defined(__linux__)
    return detect_linux();
#elif defined(__APPLE__)
    return detect_macos();
#else
    return FoobarDetectionResult();
#endif
}

FoobarDetectionResult XpuMusicCompatManager::detect_windows() {
    FoobarDetectionResult result;
    
#ifdef _WIN32
    // Try registry lookup first
    HKEY hKey;
    const char* regPaths[] = {
        "SOFTWARE\\foobar2000",
        "SOFTWARE\\WOW6432Node\\foobar2000"
    };
    
    for (const char* regPath : regPaths) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char installPath[MAX_PATH];
            DWORD size = sizeof(installPath);
            
            if (RegQueryValueExA(hKey, "InstallDir", NULL, NULL, 
                                (LPBYTE)installPath, &size) == ERROR_SUCCESS) {
                result.found = true;
                result.install_path = installPath;
                
                // Try to get version
                size = sizeof(installPath);
                if (RegQueryValueExA(hKey, "Version", NULL, NULL,
                                    (LPBYTE)installPath, &size) == ERROR_SUCCESS) {
                    result.version = installPath;
                }
            }
            
            RegCloseKey(hKey);
            
            if (result.found) break;
        }
    }
    
    // Try common installation paths if registry lookup failed
    if (!result.found) {
        const char* commonPaths[] = {
            "C:\\Program Files\\foobar2000",
            "C:\\Program Files (x86)\\foobar2000"
        };
        
        for (const char* path : commonPaths) {
            if (validate_installation(path)) {
                result.found = true;
                result.install_path = path;
                break;
            }
        }
    }
    
    // Set component paths if found
    if (result.found) {
        result.components_path = result.install_path + "\\components";
        
        // User components path
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
            result.user_components_path = std::string(appDataPath) + 
                                        "\\foobar2000\\user-components";
        }
    }
#endif
    
    return result;
}

FoobarDetectionResult XpuMusicCompatManager::detect_linux() {
    FoobarDetectionResult result;
    
#ifdef __linux__
    // On Linux, foobar2000 typically runs under Wine
    // Check common Wine prefix locations
    const char* home = getenv("HOME");
    if (home) {
        std::vector<std::string> winePaths = {
            std::string(home) + "/.wine/drive_c/Program Files/foobar2000",
            std::string(home) + "/.wine/drive_c/Program Files (x86)/foobar2000",
            std::string(home) + "/.local/share/wineprefixes/foobar2000/drive_c/Program Files/foobar2000"
        };
        
        for (const auto& path : winePaths) {
            if (validate_installation(path)) {
                result.found = true;
                result.install_path = path;
                result.components_path = path + "/components";
                break;
            }
        }
    }
#endif
    
    return result;
}

FoobarDetectionResult XpuMusicCompatManager::detect_macos() {
    FoobarDetectionResult result;
    // foobar2000 not officially supported on macOS
    // Could check for Wine installations similar to Linux
    return result;
}

bool XpuMusicCompatManager::validate_installation(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    
#ifdef _WIN32
    // Check for foobar2000.exe
    std::string exePath = path + "\\foobar2000.exe";
    DWORD attribs = GetFileAttributesA(exePath.c_str());
    if (attribs == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    
    // Check for components directory
    std::string compPath = path + "\\components";
    attribs = GetFileAttributesA(compPath.c_str());
    if (attribs == INVALID_FILE_ATTRIBUTES || !(attribs & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }
#else
    // Unix-like systems
    struct stat st;
    std::string exePath = path + "/foobar2000.exe";
    if (stat(exePath.c_str(), &st) != 0) {
        return false;
    }
    
    std::string compPath = path + "/components";
    if (stat(compPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        return false;
    }
#endif
    
    return true;
}

Result XpuMusicCompatManager::init_adapters() {
    // Adapters will be implemented in separate files
    // For now, just log that we're ready for adapters
    log(2, "Adapter initialization ready (adapters not yet implemented)");
    return Result::Success;
}

Result XpuMusicCompatManager::update_config(const CompatConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    config_ = config;
    log(2, "Configuration updated");
    
    return Result::Success;
}

Result XpuMusicCompatManager::register_adapter(std::unique_ptr<AdapterBase> adapter) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!adapter) {
        return Result::Error;
    }
    
    adapters_.push_back(std::move(adapter));
    log(2, "Adapter registered");
    
    return Result::Success;
}

Result XpuMusicCompatManager::scan_plugins(const std::string& directory) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    log(2, "Scanning for plugins in: %s", directory.c_str());
    
    // Plugin scanning will be implemented later
    // For now, just return success
    
    return Result::Success;
}

DataMigrationManager* XpuMusicCompatManager::get_migration_manager() {
    std::lock_guard<std::mutex> lock(mutex_);
    return migration_manager_.get();
}

void XpuMusicCompatManager::log(int level, const char* format, ...) {
    if (level > config_.adapter_logging_level) {
        return;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::string message(buffer);
    
    switch (level) {
        case 0: return; // Logging disabled
        case 1: COMPAT_LOG_ERROR("[XpuMusicCompat] " + message); break;
        case 2: COMPAT_LOG_WARN("[XpuMusicCompat] " + message); break;
        case 3: COMPAT_LOG_DEBUG("[XpuMusicCompat] " + message); break;
        default: COMPAT_LOG_INFO("[XpuMusicCompat] " + message); break;
    }
}

} // namespace compat
} // namespace mp
