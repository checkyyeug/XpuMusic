#pragma once

#include "mp_plugin.h"
#include "service_registry.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace mp {
namespace core {

// Loaded plugin entry
struct LoadedPlugin {
    std::string path;
    void* library_handle = nullptr;
    IPlugin* plugin = nullptr;
    PluginInfo info = {nullptr, nullptr, nullptr, Version(0,0,0), Version(0,0,0), nullptr};
};

// Plugin host manages plugin lifecycle
class PluginHost {
public:
    PluginHost(ServiceRegistry* service_registry);
    ~PluginHost();
    
    // Scan directory for plugins
    Result scan_directory(const char* directory);
    
    // Load a specific plugin
    Result load_plugin(const char* path);
    
    // Unload a plugin
    Result unload_plugin(const char* uuid);
    
    // Initialize all loaded plugins
    Result initialize_plugins();
    
    // Shutdown all plugins
    void shutdown_plugins();
    
    // Query plugin by UUID
    IPlugin* get_plugin(const char* uuid);
    
    // Get all loaded plugins
    const std::vector<LoadedPlugin>& get_loaded_plugins() const {
        return loaded_plugins_;
    }
    
private:
    void* load_library(const char* path);
    void unload_library(void* handle);
    
    ServiceRegistry* service_registry_;
    std::vector<LoadedPlugin> loaded_plugins_;
    std::unordered_map<std::string, size_t> uuid_map_;
};

}} // namespace mp::core
