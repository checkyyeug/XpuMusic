#pragma once

#include "mp_types.h"

namespace mp {

// Forward declarations
class IServiceRegistry;
class IEventBus;

// Plugin information
struct PluginInfo {
    const char* name;           // Plugin name
    const char* author;         // Plugin author
    const char* description;    // Plugin description
    Version version;            // Plugin version
    Version min_api_version;    // Minimum required API version
    const char* uuid;           // Unique plugin identifier (UUID string)
};

// Plugin dependency
struct PluginDependency {
    const char* uuid;           // UUID of required plugin
    Version min_version;        // Minimum required version
};

// Base plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Get plugin information
    virtual const PluginInfo& get_plugin_info() const = 0;
    
    // Get plugin capabilities
    virtual PluginCapability get_capabilities() const = 0;
    
    // Get plugin dependencies
    virtual void get_dependencies(const PluginDependency** deps, size_t* count) const {
        *deps = nullptr;
        *count = 0;
    }
    
    // Initialize plugin with service registry
    virtual Result initialize(IServiceRegistry* services) = 0;
    
    // Shutdown plugin (must complete within 5 seconds)
    virtual void shutdown() = 0;
    
    // Get service implementation by ID
    virtual void* get_service(ServiceID id) {
        (void)id;
        return nullptr;
    }
};

// Service registry interface
class IServiceRegistry {
public:
    virtual ~IServiceRegistry() = default;
    
    // Register a service
    virtual Result register_service(ServiceID id, void* service) = 0;
    
    // Unregister a service
    virtual Result unregister_service(ServiceID id) = 0;
    
    // Query a service
    virtual void* query_service(ServiceID id) = 0;
};

// Plugin export macro
#ifdef _WIN32
    #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Plugin entry point
// Each plugin must export this function
PLUGIN_EXPORT IPlugin* create_plugin();

// Plugin exit point
// Each plugin must export this function
PLUGIN_EXPORT void destroy_plugin(IPlugin* plugin);

} // namespace mp

// Helper macro for plugin implementation
#define MP_DEFINE_PLUGIN(PluginClass) \
    PLUGIN_EXPORT mp::IPlugin* create_plugin() { \
        return new PluginClass(); \
    } \
    PLUGIN_EXPORT void destroy_plugin(mp::IPlugin* plugin) { \
        delete plugin; \
    }
