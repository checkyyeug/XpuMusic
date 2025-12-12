#pragma once

#include "fb2k_com_base.h"
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace fb2k {

// foobar2000缁勪欢鎺ュ彛瀹氫箟

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567891}
static const GUID IID_IFB2KComponent = 
    { 0xa1b2c3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x91 } };

// {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
static const GUID IID_IFB2KComponentManager = 
    { 0xb2c3d4e5, 0xf6a7, 0x8901, { 0xbc, 0xde, 0xf1, 0x23, 0x45, 0x67, 0x89, 0x01 } };

// {C3D4E5F6-A7B8-9012-CDEF-234567890123}
static const GUID IID_IFB2KPluginLoader = 
    { 0xc3d4e5f6, 0xa7b8, 0x9012, { 0xcd, 0xef, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23 } };

// {D4E5F6A7-B8C9-0123-DEF0-345678901234}
static const GUID IID_IFB2KInitQuit = 
    { 0xd4e5f6a7, 0xb8c9, 0x0123, { 0xde, 0xf0, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34 } };

// 缁勪欢绫诲瀷鏋氫妇
enum class component_type {
    unknown = 0,
    input,          // 杈撳叆瑙ｇ爜鍣?
    output,         // 杈撳嚭璁惧
    dsp,            // DSP鏁堟灉鍣?
    visualisation,  // 鍙鍖?
    general,        // 閫氱敤鎻掍欢
    context_menu,   // 涓婁笅鏂囪彍鍗?
    toolbar,        // 宸ュ叿鏍?
    playlist_view,  // 鎾斁鍒楄〃瑙嗗浘
    album_art,      // 涓撹緫灏侀潰
    library,        // 濯掍綋搴?
    tagger,         // 鏍囩缂栬緫鍣?
    encoder         // 缂栫爜鍣?
};

// 缁勪欢淇℃伅缁撴瀯
struct component_info {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string file_path;
    std::string guid;
    component_type type;
    bool is_loaded;
    bool is_enabled;
    DWORD load_order;
    std::string dependencies;
    std::filesystem::file_time_type last_modified;
    size_t file_size;
};

// 缁勪欢鎺ュ彛
struct IFB2KComponent : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 缁勪欢鍩烘湰淇℃伅
    virtual HRESULT STDMETHODCALLTYPE GetComponentName(const char** name) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentVersion(const char** version) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentDescription(const char** description) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentAuthor(const char** author) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentGUID(const char** guid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentType(component_type* type) = 0;
    
    // 缁勪欢鐘舵€?
    virtual HRESULT STDMETHODCALLTYPE IsComponentLoaded() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsComponentEnabled() = 0;
    virtual HRESULT STDMETHODCALLTYPE SetComponentEnabled(bool enabled) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLoadOrder(DWORD* order) = 0;
    
    // 渚濊禆绠＄悊
    virtual HRESULT STDMETHODCALLTYPE GetDependencies(const char** dependencies) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDependencies(bool* satisfied) = 0;
    virtual HRESULT STDMETHODCALLTYPE LoadDependencies() = 0;
    
    // 閰嶇疆鏀寔
    virtual HRESULT STDMETHODCALLTYPE HasConfigDialog(bool* has_dialog) = 0;
    virtual HRESULT STDMETHODCALLTYPE ShowConfigDialog(HWND parent) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConfigData(BYTE** data, DWORD* size) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConfigData(const BYTE* data, DWORD size) = 0;
};

// 鍒濆鍖?閫€鍑烘帴鍙ｏ紙鎵€鏈夌粍浠跺繀椤诲疄鐜帮級
struct IFB2KInitQuit : public IFB2KUnknown {
    static const GUID iid;
    static const char* interface_name;
    
    // 鐢熷懡鍛ㄦ湡绠＄悊
    virtual HRESULT STDMETHODCALLTYPE OnInit() = 0;  // 绯荤粺鍒濆鍖栨椂璋冪敤
    virtual HRESULT STDMETHODCALLTYPE OnQuit() = 0;  // 绯荤粺閫€鍑烘椂璋冪敤
    
    // 楂樼骇鐢熷懡鍛ㄦ湡
    virtual HRESULT STDMETHODCALLTYPE OnSystemInit() = 0;   // 鍦ㄦ墍鏈夌粍浠跺姞杞藉悗璋冪敤
    virtual HRESULT STDMETHODCALLTYPE OnSystemQuit() = 0;   // 鍦ㄦ墍鏈夌粍浠跺嵏杞藉墠璋冪敤
    virtual HRESULT STDMETHODCALLTYPE OnConfigChanged() = 0; // 閰嶇疆鏀瑰彉鏃惰皟鐢?
};

// 缁勪欢绠＄悊鍣ㄦ帴鍙?
struct IFB2KComponentManager : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 缁勪欢鍙戠幇
    virtual HRESULT STDMETHODCALLTYPE ScanComponents(const char* directory) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumComponents(component_info** components, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentCount(DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindComponent(const char* guid, IFB2KComponent** component) = 0;
    
    // 缁勪欢鍔犺浇/鍗歌浇
    virtual HRESULT STDMETHODCALLTYPE LoadComponent(const char* file_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnloadComponent(const char* guid) = 0;
    virtual HRESULT STDMETHODCALLTYPE ReloadComponent(const char* guid) = 0;
    virtual HRESULT STDMETHODCALLTYPE LoadAllComponents() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnloadAllComponents() = 0;
    
    // 缁勪欢鐘舵€佺鐞?
    virtual HRESULT STDMETHODCALLTYPE EnableComponent(const char* guid, bool enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsComponentEnabled(const char* guid, bool* enabled) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetComponentLoadOrder(const char* guid, DWORD order) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentLoadOrder(const char* guid, DWORD* order) = 0;
    
    // 缁勪欢绫诲瀷绠＄悊
    virtual HRESULT STDMETHODCALLTYPE GetComponentsByType(component_type type, IFB2KComponent*** components, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentTypes(component_type** types, DWORD* count) = 0;
    
    // 閿欒澶勭悊
    virtual HRESULT STDMETHODCALLTYPE GetLastErrorMessage(char** message) = 0;
    virtual HRESULT STDMETHODCALLTYPE ClearErrorLog() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetErrorLog(char** log, DWORD* count) = 0;
};

// 鎻掍欢鍔犺浇鍣ㄦ帴鍙?
struct IFB2KPluginLoader : public IFB2KUnknown {
    static const GUID iid;
    static const char* interface_name;
    
    // DLL鍔犺浇
    virtual HRESULT STDMETHODCALLTYPE LoadPlugin(const char* dll_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnloadPlugin(const char* dll_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsPluginLoaded(const char* dll_path, bool* loaded) = 0;
    
    // 缁勪欢鎻愬彇
    virtual HRESULT STDMETHODCALLTYPE GetComponentsFromPlugin(const char* dll_path, IFB2KComponent*** components, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPluginInfo(const char* dll_path, component_info* info) = 0;
    
    // 渚濊禆绠＄悊
    virtual HRESULT STDMETHODCALLTYPE CheckPluginDependencies(const char* dll_path, bool* satisfied) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPluginDependencies(const char* dll_path, char*** dependencies, DWORD* count) = 0;
    
    // 瀹夊叏妫€鏌?
    virtual HRESULT STDMETHODCALLTYPE VerifyPluginSignature(const char* dll_path, bool* verified) = 0;
    virtual HRESULT STDMETHODCALLTYPE ScanPluginForMalware(const char* dll_path, bool* clean) = 0;
    
    // 鐗堟湰鍏煎鎬?
    virtual HRESULT STDMETHODCALLTYPE CheckPluginCompatibility(const char* dll_path, bool* compatible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPluginVersion(const char* dll_path, DWORD* version) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRequiredAPIVersion(const char* dll_path, DWORD* api_version) = 0;
};

// 缁勪欢宸ュ巶妯℃澘
template<typename ComponentClass>
class fb2k_component_factory {
public:
    static IUnknown* create_instance() {
        return new ComponentClass();
    }
    
    static void register_component(const char* guid, component_type type) {
        auto* manager = fb2k_get_component_manager();
        if (manager) {
            // 娉ㄥ唽缁勪欢宸ュ巶鍑芥暟
            component_factory_functions_[guid] = create_instance;
            component_types_[guid] = type;
        }
    }
    
    static IUnknown* create_component(const std::string& guid) {
        auto it = component_factory_functions_.find(guid);
        if (it != component_factory_functions_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    static component_type get_component_type(const std::string& guid) {
        auto it = component_types_.find(guid);
        if (it != component_types_.end()) {
            return it->second;
        }
        return component_type::unknown;
    }
    
private:
    static std::map<std::string, std::function<IUnknown*()>> component_factory_functions_;
    static std::map<std::string, component_type> component_types_;
};

// 闈欐€佹垚鍛樺畾涔?
template<typename ComponentClass>
std::map<std::string, std::function<IUnknown*()>> fb2k_component_factory<ComponentClass>::component_factory_functions_;

template<typename ComponentClass>
std::map<std::string, component_type> fb2k_component_factory<ComponentClass>::component_types_;

// 缁勪欢娉ㄥ唽瀹?
#define FB2K_REGISTER_COMPONENT(ComponentClass, ComponentGUID, ComponentType) \
    namespace { \
        struct ComponentClass##_registrar { \
            ComponentClass##_registrar() { \
                fb2k_component_factory<ComponentClass>::register_component(ComponentGUID, ComponentType); \
            } \
        } ComponentClass##_registrar_instance; \
    }

// 缁勪欢绠＄悊鍣ㄥ疄鐜?
class fb2k_component_manager_impl : public fb2k_service_impl<IFB2KComponentManager> {
public:
    fb2k_component_manager_impl();
    ~fb2k_component_manager_impl();
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
    // IFB2KComponentManager瀹炵幇
    HRESULT STDMETHODCALLTYPE ScanComponents(const char* directory) override;
    HRESULT STDMETHODCALLTYPE EnumComponents(component_info** components, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE GetComponentCount(DWORD* count) override;
    HRESULT STDMETHODCALLTYPE FindComponent(const char* guid, IFB2KComponent** component) override;
    HRESULT STDMETHODCALLTYPE LoadComponent(const char* file_path) override;
    HRESULT STDMETHODCALLTYPE UnloadComponent(const char* guid) override;
    HRESULT STDMETHODCALLTYPE ReloadComponent(const char* guid) override;
    HRESULT STDMETHODCALLTYPE LoadAllComponents() override;
    HRESULT STDMETHODCALLTYPE UnloadAllComponents() override;
    HRESULT STDMETHODCALLTYPE EnableComponent(const char* guid, bool enable) override;
    HRESULT STDMETHODCALLTYPE IsComponentEnabled(const char* guid, bool* enabled) override;
    HRESULT STDMETHODCALLTYPE SetComponentLoadOrder(const char* guid, DWORD order) override;
    HRESULT STDMETHODCALLTYPE GetComponentLoadOrder(const char* guid, DWORD* order) override;
    HRESULT STDMETHODCALLTYPE GetComponentsByType(component_type type, IFB2KComponent*** components, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE GetComponentTypes(component_type** types, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE GetLastErrorMessage(char** message) override;
    HRESULT STDMETHODCALLTYPE ClearErrorLog() override;
    HRESULT STDMETHODCALLTYPE GetErrorLog(char** log, DWORD* count) override;
    
private:
    struct component_entry {
        component_info info;
        IFB2KComponent* component;
        HMODULE dll_handle;
        std::vector<std::string> dependencies;
        bool dependency_satisfied;
    };
    
    std::map<std::string, component_entry> components_;
    std::vector<std::string> error_log_;
    std::mutex components_mutex_;
    std::mutex error_mutex_;
    
    std::string components_directory_;
    std::unique_ptr<IFB2KPluginLoader> plugin_loader_;
    
    bool scan_component_directory(const std::string& directory);
    bool load_component_file(const std::string& file_path);
    bool unload_component_internal(const std::string& guid);
    bool check_component_dependencies(const component_entry& entry);
    bool resolve_dependencies();
    void add_error(const std::string& error);
    void clear_error_log_internal();
    component_type detect_component_type(const std::string& file_path);
    bool extract_component_info(const std::string& file_path, component_info& info);
};

// 鎻掍欢鍔犺浇鍣ㄥ疄鐜?
class fb2k_plugin_loader_impl : public fb2k_com_object<IFB2KPluginLoader> {
public:
    fb2k_plugin_loader_impl();
    ~fb2k_plugin_loader_impl();
    
    // IFB2KPluginLoader瀹炵幇
    HRESULT STDMETHODCALLTYPE LoadPlugin(const char* dll_path) override;
    HRESULT STDMETHODCALLTYPE UnloadPlugin(const char* dll_path) override;
    HRESULT STDMETHODCALLTYPE IsPluginLoaded(const char* dll_path, bool* loaded) override;
    HRESULT STDMETHODCALLTYPE GetComponentsFromPlugin(const char* dll_path, IFB2KComponent*** components, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE GetPluginInfo(const char* dll_path, component_info* info) override;
    HRESULT STDMETHODCALLTYPE CheckPluginDependencies(const char* dll_path, bool* satisfied) override;
    HRESULT STDMETHODCALLTYPE GetPluginDependencies(const char* dll_path, char*** dependencies, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE VerifyPluginSignature(const char* dll_path, bool* verified) override;
    HRESULT STDMETHODCALLTYPE ScanPluginForMalware(const char* dll_path, bool* clean) override;
    HRESULT STDMETHODCALLTYPE CheckPluginCompatibility(const char* dll_path, bool* compatible) override;
    HRESULT STDMETHODCALLTYPE GetPluginVersion(const char* dll_path, DWORD* version) override;
    HRESULT STDMETHODCALLTYPE GetRequiredAPIVersion(const char* dll_path, DWORD* api_version) override;
    
private:
    struct plugin_info {
        std::string path;
        HMODULE handle;
        DWORD version;
        DWORD api_version;
        std::vector<std::string> dependencies;
        bool verified;
        bool compatible;
        std::vector<std::string> component_guids;
    };
    
    std::map<std::string, plugin_info> loaded_plugins_;
    std::mutex plugins_mutex_;
    
    // 鎻掍欢鍏ュ彛鐐瑰畾涔?
    typedef HRESULT (*FB2KGetComponentCountFunc)(DWORD* count);
    typedef HRESULT (*FB2KGetComponentInfoFunc)(DWORD index, char** guid, char** name, char** version, component_type* type);
    typedef HRESULT (*FB2KCreateComponentFunc)(const char* guid, IUnknown** component);
    typedef HRESULT (*FB2KGetPluginInfoFunc)(DWORD* version, DWORD* api_version, char** dependencies);
    typedef HRESULT (*FB2KInitPluginFunc)();
    typedef HRESULT (*FB2KQuitPluginFunc)();
    
    bool load_plugin_internal(const std::string& dll_path, plugin_info& info);
    bool unload_plugin_internal(const std::string& dll_path);
    bool verify_plugin_dependencies(const plugin_info& info);
    bool check_plugin_compatibility(const plugin_info& info);
    bool scan_plugin_dependencies(const std::string& dll_path, std::vector<std::string>& dependencies);
    bool extract_component_info(const std::string& dll_path, plugin_info& info);
    std::string get_plugin_error_string(DWORD error_code);
};

// 鍏ㄥ眬缁勪欢绠＄悊鍣ㄨ闂?
inline IFB2KComponentManager* fb2k_get_component_manager() {
    return fb2k_get_service<IFB2KComponentManager>();
}

// 鍏ㄥ眬鎻掍欢鍔犺浇鍣ㄨ闂?
inline IFB2KPluginLoader* fb2k_get_plugin_loader() {
    return fb2k_get_service<IFB2KPluginLoader>();
}

// 鎺ュ彛IID瀹氫箟
template<> const GUID fb2k::IFB2KComponent::iid = IID_IFB2KComponent;
template<> const char* fb2k::IFB2KComponent::interface_name = "IFB2KComponent";

template<> const GUID fb2k::IFB2KComponentManager::iid = IID_IFB2KComponentManager;
template<> const char* fb2k::IFB2KComponentManager::interface_name = "IFB2KComponentManager";

template<> const GUID fb2k::IFB2KPluginLoader::iid = IID_IFB2KPluginLoader;
template<> const char* fb2k::IFB2KPluginLoader::interface_name = "IFB2KPluginLoader";

template<> const GUID fb2k::IFB2KInitQuit::iid = IID_IFB2KInitQuit;
template<> const char* fb2k::IFB2KInitQuit::interface_name = "IFB2KInitQuit";

} // namespace fb2k