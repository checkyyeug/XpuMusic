/**
 * @file plugin_loader.cpp
 * @brief XpuMusicPluginLoader 瀹炵幇
 * @date 2025-12-09
 */

#include "plugin_loader.h"
#include "../xpumusic_compat_manager.h"
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace mp {
namespace compat {

// ServiceFactoryWrapper 瀹炵幇
ServiceFactoryWrapper::ServiceFactoryWrapper(service_factory_base* foobar_factory)
    : foobar_factory_(foobar_factory) {
    // foobar_factory 缁ф壙鑷?service_base锛屾湁寮曠敤璁℃暟
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

// XpuMusicPluginLoader 瀹炵幇
XpuMusicPluginLoader::XpuMusicPluginLoader(XpuMusicCompatManager* compat_manager)
    : compat_manager_(compat_manager) {
}

XpuMusicPluginLoader::~XpuMusicPluginLoader() {
    unload_all();
}

#ifdef _WIN32
void* XpuMusicPluginLoader::load_library_internal(const char* path, std::string& error_msg) {
    // Windows: 浣跨敤 LoadLibrary
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
    // Linux/macOS: 浣跨敤 dlopen
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
    // Windows: 浣跨敤 GetProcAddress
    void* client_entry = GetProcAddress(static_cast<HMODULE>(handle), "_foobar2000_client_entry");
#else
    // Linux/macOS: 浣跨敤 dlsym
    void* client_entry = dlsym(handle, "_foobar2000_client_entry");
#endif
    
    if (!client_entry) {
        module_info.error = "Could not find _foobar2000_client_entry";
        return Result::Error;
    }
    
    // client_entry 鏄竴涓嚱鏁帮紝杩斿洖鏈嶅姟鍏ュ彛鍒楄〃
    // 鍦ㄥ疄闄?foobar2000 涓紝杩欐槸澶嶆潅鐨勬敞鍐屾満鍒?
    // 杩欓噷绠€鍖栧鐞嗭細鍋囪瀹冩槸涓€涓繑鍥?service_factory_base** 鐨勫嚱鏁?
    
    typedef service_factory_base* (*client_entry_func)();
    client_entry_func entry_func = reinterpret_cast<client_entry_func>(client_entry);
    
    try {
        // 璋冪敤鍏ュ彛鍑芥暟鑾峰彇绗竴涓湇鍔″伐鍘?
        service_factory_base* factory = entry_func();
        if (factory) {
            // 杩欒〃鏄庤嚦灏戞湁涓€涓湇鍔″彲鐢?
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
    // 鏌ユ壘鍏稿瀷鐨?foobar2000 鏈嶅姟瀵煎嚭绗﹀彿
    // 娉ㄦ剰锛氳繖鏄畝鍖栫殑锛屽疄闄?foobar2000 鏈夋洿澶嶆潅鐨勬敞鍐岃〃
    
    // 鍦ㄤ竴浜涚畝鍗曟彃浠朵腑锛屽彲浠ラ€氳繃鏋氫妇瀵煎嚭鍑芥暟鏉ュ彂鐜版湇鍔?
    // HMODULE hmod = static_cast<HMODULE>(handle); // 淇濈暀浠ュ灏嗘潵浣跨敤
    
    // 鑾峰彇 DOS 澶达紙绠€鍖?- 瀹為檯瀹炵幇闇€瑕侀亶鍘?PE 瀵煎嚭琛級
    // 杩欓噷鎴戜滑鍙槸鍒涘缓涓€涓ず渚嬫湇鍔℃潯鐩?
    
    ServiceExportInfo info;
    info.name = "Unknown foobar2000 Service";
    memset(&info.guid, 0, sizeof(GUID));
    info.guid.Data1 = 0x12345678;  // 绀轰緥 GUID
    info.factory = nullptr;
    info.available = true;
    
    services.push_back(info);
    
#else
    // Linux: 浣跨敤 dlsym 鏌ユ壘宸茬煡绗﹀彿
    // 绫讳技鍦扮畝鍖?
    ServiceExportInfo info;
    info.name = "Unknown foobar2000 Service";
    memset(&info.guid, 0, sizeof(GUID));
    info.guid.Data1 = 0x87654321;  // 绀轰緥 GUID
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
        
        // 鍒涘缓 wrapper
        auto wrapper = std::make_unique<ServiceFactoryWrapper>(service.factory);
        
        // 娉ㄥ唽鍒版湇鍔℃敞鍐岃〃 - 闇€瑕佽浆鎹?GUID 绫诲瀷
        xpumusic_sdk::GUID fb_guid;
        #ifdef _WIN32
        memcpy(&fb_guid, &service.guid, sizeof(GUID));
        #else
        // 闈濿indows绯荤粺锛屾墜鍔ㄥ鍒禛UID缁撴瀯
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
        
        // 淇濆瓨 wrapper
        service_wrappers_.push_back(std::move(wrapper));
    }
    
    return Result::Success;
}

bool XpuMusicPluginLoader::validate_abi_compatibility(void* handle) {
    if (!handle) return false;
    
    // 绠€鍗曠殑 ABI 楠岃瘉锛氭鏌ュ繀瑕佸鍑烘槸鍚﹀瓨鍦?
#ifdef _WIN32
    // 鏌ユ壘鐗堟湰鎴栧垵濮嬪寲鍑芥暟
    // void* version_check = GetProcAddress(static_cast<HMODULE>(handle), "foobar2000_get_version"); // 淇濈暀浠ュ灏嗘潵浣跨敤
    void* client_entry = GetProcAddress(static_cast<HMODULE>(handle), "_foobar2000_client_entry");
    
    return client_entry != nullptr;
#else
    void* client_entry = dlsym(handle, "_foobar2000_client_entry");
    return client_entry != nullptr;
#endif
}

bool XpuMusicPluginLoader::validate_dependencies(const PluginModuleInfo& module_info) {
    (void)module_info;
    // 绠€鍖栵細鍋囪鎵€鏈変緷璧栭兘婊¤冻
    // 鐪熷疄瀹炵幇浼氭鏌ユ墍闇€鏈嶅姟鐨勫彲鐢ㄦ€?
    return true;
}

PluginModuleInfo XpuMusicPluginLoader::get_module_info(void* handle, const std::string& path) {
    PluginModuleInfo info;
    info.path = path;
    info.loaded = (handle != nullptr);
    info.library_handle = handle;
    
    if (handle) {
        // 浠庢枃浠剁郴缁熻幏鍙栨枃浠跺悕
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
    
    // 妫€鏌ユ槸鍚﹀凡缁忓姞杞?
    if (is_plugin_loaded(dll_path)) {
        last_error_ = "Plugin already loaded";
        return Result::AlreadyInitialized;
    }
    
    // 鍔犺浇 DLL
    std::string error_msg;
    void* handle = load_library_internal(dll_path, error_msg);
    if (!handle) {
        last_error_ = error_msg;
        return Result::FileError;
    }
    
    // 楠岃瘉 ABI 鍏煎鎬?
    if (!validate_abi_compatibility(handle)) {
        unload_library_internal(handle);
        last_error_ = "ABI validation failed - not a valid foobar2000 plugin";
        return Result::NotSupported;
    }
    
    // 瑙ｆ瀽瀹㈡埛绔叆鍙?
    PluginModuleInfo module_info = get_module_info(handle, dll_path);
    Result result = parse_client_entry(handle, module_info);
    if (result != Result::Success) {
        unload_library_internal(handle);
        return result;
    }
    
    // 楠岃瘉渚濊禆
    if (!validate_dependencies(module_info)) {
        unload_library_internal(handle);
        last_error_ = "Dependency validation failed";
        return Result::Error;
    }
    
    // 鏋氫妇鏈嶅姟
    std::vector<ServiceExportInfo> services;
    result = enumerate_services(handle, services);
    if (result != Result::Success) {
        unload_library_internal(handle);
        return result;
    }
    
    // 娉ㄥ唽鏈嶅姟
    result = register_services(services);
    if (result != Result::Success) {
        unload_library_internal(handle);
        return result;
    }
    
    // 淇濆瓨妯″潡淇℃伅
    module_info.loaded = true;
    module_info.service_count = static_cast<uint32_t>(services.size());
    modules_.push_back(module_info);
    
    // 娉ㄥ唽鍒板凡娉ㄥ唽鏈嶅姟缂撳瓨
    registered_services_.insert(registered_services_.end(), 
                               services.begin(), services.end());
    
    // 璁板綍鏃ュ織
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
    
    // 鏌ユ壘妯″潡
    auto it = std::find_if(modules_.begin(), modules_.end(),
        [dll_path](const PluginModuleInfo& info) {
            return info.path == dll_path;
        });
    
    if (it == modules_.end()) {
        last_error_ = "Plugin not found";
        return Result::FileNotFound;
    }
    
    // 鍗歌浇搴?
    if (it->library_handle) {
        unload_library_internal(it->library_handle);
    }
    
    // 浠庡垪琛ㄤ腑绉婚櫎
    modules_.erase(it);
    
    // TODO: 绉婚櫎鐩稿叧鐨勬湇鍔?wrapper
    
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
    
    // 纭繚璺緞鍒嗛殧绗︽纭?
    char last_char = dir_str.empty() ? 0 : dir_str[dir_str.length() - 1];
    if (last_char != '\\' && last_char != '/') {
        dir_str += std::filesystem::path::preferred_separator;
    }
    
    // 妫€鏌ョ洰褰曟槸鍚﹀瓨鍦?
    if (!std::filesystem::exists(dir_str) || !std::filesystem::is_directory(dir_str)) {
        last_error_ = "Directory does not exist: " + dir_str;
        return Result::FileNotFound;
    }
    
    size_t loaded_count = 0;
    size_t failed_count = 0;
    
    // 閬嶅巻鐩綍涓殑鎵€鏈夋枃浠?
    for (const auto& entry : std::filesystem::directory_iterator(dir_str)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        // 鍙鐞?.dll 鏂囦欢锛圵indows锛夋垨 .so 鏂囦欢锛圠inux锛?
        if (extension != ".dll" && extension != ".so") {
            continue;
        }
        
        std::string file_path = entry.path().string();
        
        // 灏濊瘯鍔犺浇鎻掍欢
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
    
    // 鍗歌浇鎵€鏈夋ā鍧?
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
