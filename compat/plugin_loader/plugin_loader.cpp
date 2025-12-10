/**
 * @file plugin_loader.cpp
 * @brief XpuMusicPluginLoader 实现
 * @date 2025-12-09
 */

#include "plugin_loader.h"
#include "../xpumusic_compat_manager.h"
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace mp {
namespace compat {

// ServiceFactoryWrapper 实现
ServiceFactoryWrapper::ServiceFactoryWrapper(service_factory_base* foobar_factory)
    : foobar_factory_(foobar_factory) {
    // foobar_factory 继承自 service_base，有引用计数
    if (foobar_factory) {
        foobar_factory->service_add_ref();
    }
}

xpumusic_sdk::service_base* ServiceFactoryWrapper::create_service() {
    if (!foobar_factory_) {
        return nullptr;
    }

    // The base factory doesn't have create_service method, so we return nullptr
    // This would need to be implemented based on actual foobar2000 plugin API
    return nullptr;
}

int ServiceFactoryWrapper::service_add_ref() {
    if (foobar_factory_) {
        return foobar_factory_->service_add_ref();
    }
    return 0;
}

int ServiceFactoryWrapper::service_release() {
    if (foobar_factory_) {
        return foobar_factory_->service_release();
    }
    return 0;
}

const xpumusic_sdk::GUID& ServiceFactoryWrapper::get_guid() const {
    static const xpumusic_sdk::GUID null_guid = {0};
    
    if (!foobar_factory_) {
        return null_guid;
    }
    
    return foobar_factory_->get_guid();
}

// XpuMusicPluginLoader 实现
XpuMusicPluginLoader::XpuMusicPluginLoader(XpuMusicCompatManager* compat_manager)
    : compat_manager_(compat_manager) {
}

XpuMusicPluginLoader::~XpuMusicPluginLoader() {
    unload_all();
}

#ifdef _WIN32
void* XpuMusicPluginLoader::load_library_internal(const char* path, std::string& error_msg) {
    // Windows: 使用 LoadLibrary
    HMODULE handle = LoadLibraryA(path);
    if (!handle) {
        DWORD error = GetLastError();
        std::ostringstream oss;
        oss << "LoadLibrary failed with error " << error;
        error_msg = oss.str();
        return nullptr;
    }
    
    error_msg.clear();
    return static_cast<void*>(handle);
}

void XpuMusicPluginLoader::unload_library_internal(void* handle) {
    if (handle) {
        FreeLibrary(static_cast<HMODULE>(handle));
    }
}
#else
void* XpuMusicPluginLoader::load_library_internal(const char* path, std::string& error_msg) {
    // Linux/macOS: 使用 dlopen
    void* handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        const char* dl_error = dlerror();
        error_msg = dl_error ? dl_error : "Unknown dlopen error";
        return nullptr;
    }
    
    error_msg.clear();
    return handle;
}

void XpuMusicPluginLoader::unload_library_internal(void* handle) {
    if (handle) {
        dlclose(handle);
    }
}
#endif

Result XpuMusicPluginLoader::parse_client_entry(void* handle, PluginModuleInfo& module_info) {
    if (!handle) {
        return Result::InvalidParameter;
    }
    
#ifdef _WIN32
    // Windows: 使用 GetProcAddress
    void* client_entry = GetProcAddress(static_cast<HMODULE>(handle), "_foobar2000_client_entry");
#else
    // Linux/macOS: 使用 dlsym
    void* client_entry = dlsym(handle, "_foobar2000_client_entry");
#endif
    
    if (!client_entry) {
        module_info.error = "Could not find _foobar2000_client_entry";
        return Result::Error;
    }
    
    // client_entry 是一个函数，返回服务入口列表
    // 在实际 foobar2000 中，这是复杂的注册机制
    // 这里简化处理：假设它是一个返回 service_factory_base** 的函数
    
    typedef service_factory_base* (*client_entry_func)();
    client_entry_func entry_func = reinterpret_cast<client_entry_func>(client_entry);
    
    try {
        // 调用入口函数获取第一个服务工厂
        service_factory_base* factory = entry_func();
        if (factory) {
            // 这表明至少有一个服务可用
            module_info.initialized = true;
        }
    } catch (...) {
        module_info.error = "Exception calling client entry";
        return Result::Error;
    }
    
    module_info.initialized = true;
    return Result::Success;
}

Result XpuMusicPluginLoader::enumerate_services(void* handle, std::vector<ServiceExportInfo>& services) {
    services.clear();
    
    if (!handle) {
        return Result::InvalidParameter;
    }
    
#ifdef _WIN32
    // 查找典型的 foobar2000 服务导出符号
    // 注意：这是简化的，实际 foobar2000 有更复杂的注册表
    
    // 在一些简单插件中，可以通过枚举导出函数来发现服务
    // HMODULE hmod = static_cast<HMODULE>(handle); // 保留以备将来使用
    
    // 获取 DOS 头（简化 - 实际实现需要遍历 PE 导出表）
    // 这里我们只是创建一个示例服务条目
    
    ServiceExportInfo info;
    info.name = "Unknown foobar2000 Service";
    memset(&info.guid, 0, sizeof(GUID));
    info.guid.Data1 = 0x12345678;  // 示例 GUID
    info.factory = nullptr;
    info.available = true;
    
    services.push_back(info);
    
#else
    // Linux: 使用 dlsym 查找已知符号
    // 类似地简化
    ServiceExportInfo info;
    info.name = "Unknown foobar2000 Service";
    memset(&info.guid, 0, sizeof(GUID));
    info.guid.Data1 = 0x87654321;  // 示例 GUID
    info.factory = nullptr;
    info.available = true;
    
    services.push_back(info);
#endif
    
    return Result::Success;
}

Result XpuMusicPluginLoader::register_services(const std::vector<ServiceExportInfo>& services) {
    if (!registry_bridge_) {
        return Result::NotImplemented;
    }
    
    for (const auto& service : services) {
        if (!service.available || !service.factory) {
            continue;
        }
        
        // 创建 wrapper
        auto wrapper = std::make_unique<ServiceFactoryWrapper>(service.factory);
        
        // 注册到服务注册表 - 需要转换 GUID 类型
        xpumusic_sdk::GUID fb_guid;
        #ifdef _WIN32
        memcpy(&fb_guid, &service.guid, sizeof(GUID));
        #else
        // 非Windows系统，手动复制GUID结构
        fb_guid.Data1 = service.guid.Data1;
        fb_guid.Data2 = service.guid.Data2;
        fb_guid.Data3 = service.guid.Data3;
        memcpy(fb_guid.Data4, service.guid.Data4, sizeof(service.guid.Data4));
        #endif
        Result result = registry_bridge_->register_service(fb_guid, wrapper.get());
        if (result != Result::Success) {
            std::ostringstream oss;
            oss << "Failed to register service: " << static_cast<int>(result);
            last_error_ = oss.str();
            return result;
        }
        
        // 保存 wrapper
        service_wrappers_.push_back(std::move(wrapper));
    }
    
    return Result::Success;
}

bool XpuMusicPluginLoader::validate_abi_compatibility(void* handle) {
    if (!handle) return false;
    
    // 简单的 ABI 验证：检查必要导出是否存在
#ifdef _WIN32
    // 查找版本或初始化函数
    // void* version_check = GetProcAddress(static_cast<HMODULE>(handle), "foobar2000_get_version"); // 保留以备将来使用
    void* client_entry = GetProcAddress(static_cast<HMODULE>(handle), "_foobar2000_client_entry");
    
    return client_entry != nullptr;
#else
    void* client_entry = dlsym(handle, "_foobar2000_client_entry");
    return client_entry != nullptr;
#endif
}

bool XpuMusicPluginLoader::validate_dependencies(const PluginModuleInfo& module_info) {
    (void)module_info;
    // 简化：假设所有依赖都满足
    // 真实实现会检查所需服务的可用性
    return true;
}

PluginModuleInfo XpuMusicPluginLoader::get_module_info(void* handle, const std::string& path) {
    PluginModuleInfo info;
    info.path = path;
    info.loaded = (handle != nullptr);
    info.library_handle = handle;
    
    if (handle) {
        // 从文件系统获取文件名
        std::filesystem::path p(path);
        info.name = p.filename().string();
    }
    
    return info;
}

Result XpuMusicPluginLoader::load_plugin(const char* dll_path) {
    if (!dll_path || strlen(dll_path) == 0) {
        return Result::InvalidParameter;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查是否已经加载
    if (is_plugin_loaded(dll_path)) {
        last_error_ = "Plugin already loaded";
        return Result::AlreadyInitialized;
    }
    
    // 加载 DLL
    std::string error_msg;
    void* handle = load_library_internal(dll_path, error_msg);
    if (!handle) {
        last_error_ = error_msg;
        return Result::FileError;
    }
    
    // 验证 ABI 兼容性
    if (!validate_abi_compatibility(handle)) {
        unload_library_internal(handle);
        last_error_ = "ABI validation failed - not a valid foobar2000 plugin";
        return Result::NotSupported;
    }
    
    // 解析客户端入口
    PluginModuleInfo module_info = get_module_info(handle, dll_path);
    Result result = parse_client_entry(handle, module_info);
    if (result != Result::Success) {
        unload_library_internal(handle);
        return result;
    }
    
    // 验证依赖
    if (!validate_dependencies(module_info)) {
        unload_library_internal(handle);
        last_error_ = "Dependency validation failed";
        return Result::Error;
    }
    
    // 枚举服务
    std::vector<ServiceExportInfo> services;
    result = enumerate_services(handle, services);
    if (result != Result::Success) {
        unload_library_internal(handle);
        return result;
    }
    
    // 注册服务
    result = register_services(services);
    if (result != Result::Success) {
        unload_library_internal(handle);
        return result;
    }
    
    // 保存模块信息
    module_info.loaded = true;
    module_info.service_count = static_cast<uint32_t>(services.size());
    modules_.push_back(module_info);
    
    // 注册到已注册服务缓存
    registered_services_.insert(registered_services_.end(), 
                               services.begin(), services.end());
    
    // 记录日志
    if (compat_manager_) {
        compat_manager_->log(2, "Loaded plugin: %s (%d services)", 
                            dll_path, module_info.service_count);
    }
    
    return Result::Success;
}

Result XpuMusicPluginLoader::unload_plugin(const char* dll_path) {
    if (!dll_path || strlen(dll_path) == 0) {
        return Result::InvalidParameter;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找模块
    auto it = std::find_if(modules_.begin(), modules_.end(),
        [dll_path](const PluginModuleInfo& info) {
            return info.path == dll_path;
        });
    
    if (it == modules_.end()) {
        last_error_ = "Plugin not found";
        return Result::FileNotFound;
    }
    
    // 卸载库
    if (it->library_handle) {
        unload_library_internal(it->library_handle);
    }
    
    // 从列表中移除
    modules_.erase(it);
    
    // TODO: 移除相关的服务 wrapper
    
    if (compat_manager_) {
        compat_manager_->log(2, "Unloaded plugin: %s", dll_path);
    }
    
    return Result::Success;
}

Result XpuMusicPluginLoader::load_plugins_from_directory(const char* directory) {
    if (!directory || strlen(directory) == 0) {
        return Result::InvalidParameter;
    }
    
    std::string dir_str = directory;
    
    // 确保路径分隔符正确
    char last_char = dir_str.empty() ? 0 : dir_str[dir_str.length() - 1];
    if (last_char != '\\' && last_char != '/') {
        dir_str += std::filesystem::path::preferred_separator;
    }
    
    // 检查目录是否存在
    if (!std::filesystem::exists(dir_str) || !std::filesystem::is_directory(dir_str)) {
        last_error_ = "Directory does not exist: " + dir_str;
        return Result::FileNotFound;
    }
    
    size_t loaded_count = 0;
    size_t failed_count = 0;
    
    // 遍历目录中的所有文件
    for (const auto& entry : std::filesystem::directory_iterator(dir_str)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        // 只处理 .dll 文件（Windows）或 .so 文件（Linux）
        if (extension != ".dll" && extension != ".so") {
            continue;
        }
        
        std::string file_path = entry.path().string();
        
        // 尝试加载插件
        Result result = load_plugin(file_path.c_str());
        if (result == Result::Success) {
            loaded_count++;
        } else {
            failed_count++;
        }
    }
    
    if (compat_manager_) {
        compat_manager_->log(2, "Scanned directory: %s - Loaded: %zu, Failed: %zu", 
                            directory, loaded_count, failed_count);
    }
    
    return (loaded_count > 0) ? Result::Success : Result::Error;
}

void XpuMusicPluginLoader::unload_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 卸载所有模块
    for (const auto& module : modules_) {
        if (module.library_handle) {
            unload_library_internal(module.library_handle);
        }
    }
    
    modules_.clear();
    service_wrappers_.clear();
    registered_services_.clear();
}

const PluginModuleInfo* XpuMusicPluginLoader::get_module(const char* path) const {
    if (!path) return nullptr;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(modules_.begin(), modules_.end(),
        [path](const PluginModuleInfo& info) {
            return info.path == path;
        });
    
    return (it != modules_.end()) ? &(*it) : nullptr;
}

bool XpuMusicPluginLoader::is_plugin_loaded(const char* path) const {
    return get_module(path) != nullptr;
}

void XpuMusicPluginLoader::set_registry_bridge(std::unique_ptr<ServiceRegistryBridge> bridge) {
    std::lock_guard<std::mutex> lock(mutex_);
    registry_bridge_ = std::move(bridge);
}

} // namespace compat
} // namespace mp
