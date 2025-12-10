#include "plugin_host.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

#include <cstring>
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace mp {
namespace core {

PluginHost::PluginHost(ServiceRegistry* service_registry)
    : service_registry_(service_registry) {
}

PluginHost::~PluginHost() {
    shutdown_plugins();
}

Result PluginHost::scan_directory(const char* directory) {
    namespace fs = std::filesystem;
    
    try {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return Result::FileNotFound;
        }
        
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
            std::string ext = entry.path().extension().string();
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            // Check for plugin extension
#ifdef _WIN32
            if (ext != ".dll") {
                continue;
            }
#elif defined(__APPLE__)
            if (ext != ".dylib") {
                continue;
            }
#else
            if (ext != ".so") {
                continue;
            }
#endif
            
            // Try to load the plugin
            load_plugin(entry.path().string().c_str());
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning plugin directory: " << e.what() << std::endl;
        return Result::Error;
    }
    
    return Result::Success;
}

Result PluginHost::load_plugin(const char* path) {
    void* handle = load_library(path);
    if (!handle) {
        std::cerr << "Failed to load plugin library: " << path << std::endl;
#ifdef _WIN32
        DWORD error = GetLastError();
        std::cerr << "  Error code: " << error << std::endl;
#else
        std::cerr << "  Error: " << dlerror() << std::endl;
#endif
        return Result::Error;
    }
    
    // Get plugin entry point
    typedef IPlugin* (*CreatePluginFunc)();
#ifdef _WIN32
    CreatePluginFunc create_plugin = (CreatePluginFunc)GetProcAddress((HMODULE)handle, "create_plugin");
#else
    CreatePluginFunc create_plugin = (CreatePluginFunc)dlsym(handle, "create_plugin");
#endif
    
    if (!create_plugin) {
        std::cerr << "Plugin missing create_plugin export: " << path << std::endl;
        unload_library(handle);
        return Result::Error;
    }
    
    // Create plugin instance
    IPlugin* plugin = create_plugin();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance: " << path << std::endl;
        unload_library(handle);
        return Result::Error;
    }
    
    // Get plugin info
    const PluginInfo& info = plugin->get_plugin_info();
    
    // Check API version compatibility
    if (info.min_api_version.major > API_VERSION.major ||
        (info.min_api_version.major == API_VERSION.major && 
         info.min_api_version.minor > API_VERSION.minor)) {
        std::cerr << "Plugin API version incompatible: " << path << std::endl;
        std::cerr << "  Plugin requires: " << info.min_api_version.major 
                  << "." << info.min_api_version.minor << std::endl;
        std::cerr << "  Core provides: " << API_VERSION.major 
                  << "." << API_VERSION.minor << std::endl;
        
        typedef void (*DestroyPluginFunc)(IPlugin*);
#ifdef _WIN32
        DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)GetProcAddress((HMODULE)handle, "destroy_plugin");
#else
        DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)dlsym(handle, "destroy_plugin");
#endif
        if (destroy_plugin) {
            destroy_plugin(plugin);
        }
        unload_library(handle);
        return Result::NotSupported;
    }
    
    // Check for duplicate UUID
    if (uuid_map_.find(info.uuid) != uuid_map_.end()) {
        std::cerr << "Plugin with duplicate UUID: " << info.uuid << std::endl;
        
        typedef void (*DestroyPluginFunc)(IPlugin*);
#ifdef _WIN32
        DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)GetProcAddress((HMODULE)handle, "destroy_plugin");
#else
        DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)dlsym(handle, "destroy_plugin");
#endif
        if (destroy_plugin) {
            destroy_plugin(plugin);
        }
        unload_library(handle);
        return Result::AlreadyInitialized;
    }
    
    // Add to loaded plugins
    size_t index = loaded_plugins_.size();
    loaded_plugins_.emplace_back();
    LoadedPlugin& loaded = loaded_plugins_.back();
    
    loaded.path = path;
    loaded.library_handle = handle;
    loaded.plugin = plugin;
    loaded.info.name = info.name;
    loaded.info.author = info.author;
    loaded.info.description = info.description;
    loaded.info.version = info.version;
    loaded.info.min_api_version = info.min_api_version;
    loaded.info.uuid = info.uuid;
    
    uuid_map_[info.uuid] = index;
    
    std::cout << "Loaded plugin: " << info.name << " v" 
              << info.version.major << "." << info.version.minor << "." << info.version.patch
              << " (" << info.uuid << ")" << std::endl;
    
    return Result::Success;
}

Result PluginHost::unload_plugin(const char* uuid) {
    auto it = uuid_map_.find(uuid);
    if (it == uuid_map_.end()) {
        return Result::InvalidParameter;
    }
    
    size_t index = it->second;
    LoadedPlugin& loaded = loaded_plugins_[index];
    
    // Shutdown plugin
    loaded.plugin->shutdown();
    
    // Destroy plugin instance
    typedef void (*DestroyPluginFunc)(IPlugin*);
#ifdef _WIN32
    DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)GetProcAddress((HMODULE)loaded.library_handle, "destroy_plugin");
#else
    DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)dlsym(loaded.library_handle, "destroy_plugin");
#endif
    if (destroy_plugin) {
        destroy_plugin(loaded.plugin);
    }
    
    // Unload library
    unload_library(loaded.library_handle);
    
    // Remove from loaded plugins
    loaded_plugins_.erase(loaded_plugins_.begin() + index);
    uuid_map_.erase(it);
    
    // Update indices
    uuid_map_.clear();
    for (size_t i = 0; i < loaded_plugins_.size(); ++i) {
        uuid_map_[loaded_plugins_[i].info.uuid] = i;
    }
    
    return Result::Success;
}

Result PluginHost::initialize_plugins() {
    for (auto& loaded : loaded_plugins_) {
        Result result = loaded.plugin->initialize(service_registry_);
        if (result != Result::Success) {
            std::cerr << "Failed to initialize plugin: " << loaded.info.name << std::endl;
            return result;
        }
        
        std::cout << "Initialized plugin: " << loaded.info.name << std::endl;
    }
    
    return Result::Success;
}

void PluginHost::shutdown_plugins() {
    // Shutdown in reverse order
    for (auto it = loaded_plugins_.rbegin(); it != loaded_plugins_.rend(); ++it) {
        try {
            it->plugin->shutdown();
            
            typedef void (*DestroyPluginFunc)(IPlugin*);
#ifdef _WIN32
            DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)GetProcAddress((HMODULE)it->library_handle, "destroy_plugin");
#else
            DestroyPluginFunc destroy_plugin = (DestroyPluginFunc)dlsym(it->library_handle, "destroy_plugin");
#endif
            if (destroy_plugin) {
                destroy_plugin(it->plugin);
            }
            
            unload_library(it->library_handle);
        } catch (...) {
            // Ignore exceptions during shutdown
        }
    }
    
    loaded_plugins_.clear();
    uuid_map_.clear();
}

IPlugin* PluginHost::get_plugin(const char* uuid) {
    auto it = uuid_map_.find(uuid);
    if (it == uuid_map_.end()) {
        return nullptr;
    }
    
    return loaded_plugins_[it->second].plugin;
}

void* PluginHost::load_library(const char* path) {
#ifdef _WIN32
    return (void*)LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void PluginHost::unload_library(void* handle) {
    if (handle) {
#ifdef _WIN32
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
    }
}

}} // namespace mp::core
