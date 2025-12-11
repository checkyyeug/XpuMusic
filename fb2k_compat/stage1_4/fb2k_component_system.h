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

// foobar2000组件接口定义

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

// 组件类型枚举
enum class component_type {
    unknown = 0,
    input,          // 输入解码器
    output,         // 输出设备
    dsp,            // DSP效果器
    visualisation,  // 可视化
    general,        // 通用插件
    context_menu,   // 上下文菜单
    toolbar,        // 工具栏
    playlist_view,  // 播放列表视图
    album_art,      // 专辑封面
    library,        // 媒体库
    tagger,         // 标签编辑器
    encoder         // 编码器
};

// 组件信息结构
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

// 组件接口
struct IFB2KComponent : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 组件基本信息
    virtual HRESULT STDMETHODCALLTYPE GetComponentName(const char** name) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentVersion(const char** version) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentDescription(const char** description) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentAuthor(const char** author) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentGUID(const char** guid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentType(component_type* type) = 0;
    
    // 组件状态
    virtual HRESULT STDMETHODCALLTYPE IsComponentLoaded() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsComponentEnabled() = 0;
    virtual HRESULT STDMETHODCALLTYPE SetComponentEnabled(bool enabled) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLoadOrder(DWORD* order) = 0;
    
    // 依赖管理
    virtual HRESULT STDMETHODCALLTYPE GetDependencies(const char** dependencies) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDependencies(bool* satisfied) = 0;
    virtual HRESULT STDMETHODCALLTYPE LoadDependencies() = 0;
    
    // 配置支持
    virtual HRESULT STDMETHODCALLTYPE HasConfigDialog(bool* has_dialog) = 0;
    virtual HRESULT STDMETHODCALLTYPE ShowConfigDialog(HWND parent) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConfigData(BYTE** data, DWORD* size) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConfigData(const BYTE* data, DWORD size) = 0;
};

// 初始化/退出接口（所有组件必须实现）
struct IFB2KInitQuit : public IFB2KUnknown {
    static const GUID iid;
    static const char* interface_name;
    
    // 生命周期管理
    virtual HRESULT STDMETHODCALLTYPE OnInit() = 0;  // 系统初始化时调用
    virtual HRESULT STDMETHODCALLTYPE OnQuit() = 0;  // 系统退出时调用
    
    // 高级生命周期
    virtual HRESULT STDMETHODCALLTYPE OnSystemInit() = 0;   // 在所有组件加载后调用
    virtual HRESULT STDMETHODCALLTYPE OnSystemQuit() = 0;   // 在所有组件卸载前调用
    virtual HRESULT STDMETHODCALLTYPE OnConfigChanged() = 0; // 配置改变时调用
};

// 组件管理器接口
struct IFB2KComponentManager : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 组件发现
    virtual HRESULT STDMETHODCALLTYPE ScanComponents(const char* directory) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumComponents(component_info** components, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentCount(DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindComponent(const char* guid, IFB2KComponent** component) = 0;
    
    // 组件加载/卸载
    virtual HRESULT STDMETHODCALLTYPE LoadComponent(const char* file_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnloadComponent(const char* guid) = 0;
    virtual HRESULT STDMETHODCALLTYPE ReloadComponent(const char* guid) = 0;
    virtual HRESULT STDMETHODCALLTYPE LoadAllComponents() = 0;
    virtual HRESULT STDMETHODCALLTYPE UnloadAllComponents() = 0;
    
    // 组件状态管理
    virtual HRESULT STDMETHODCALLTYPE EnableComponent(const char* guid, bool enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsComponentEnabled(const char* guid, bool* enabled) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetComponentLoadOrder(const char* guid, DWORD order) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentLoadOrder(const char* guid, DWORD* order) = 0;
    
    // 组件类型管理
    virtual HRESULT STDMETHODCALLTYPE GetComponentsByType(component_type type, IFB2KComponent*** components, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentTypes(component_type** types, DWORD* count) = 0;
    
    // 错误处理
    virtual HRESULT STDMETHODCALLTYPE GetLastErrorMessage(char** message) = 0;
    virtual HRESULT STDMETHODCALLTYPE ClearErrorLog() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetErrorLog(char** log, DWORD* count) = 0;
};

// 插件加载器接口
struct IFB2KPluginLoader : public IFB2KUnknown {
    static const GUID iid;
    static const char* interface_name;
    
    // DLL加载
    virtual HRESULT STDMETHODCALLTYPE LoadPlugin(const char* dll_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnloadPlugin(const char* dll_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsPluginLoaded(const char* dll_path, bool* loaded) = 0;
    
    // 组件提取
    virtual HRESULT STDMETHODCALLTYPE GetComponentsFromPlugin(const char* dll_path, IFB2KComponent*** components, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPluginInfo(const char* dll_path, component_info* info) = 0;
    
    // 依赖管理
    virtual HRESULT STDMETHODCALLTYPE CheckPluginDependencies(const char* dll_path, bool* satisfied) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPluginDependencies(const char* dll_path, char*** dependencies, DWORD* count) = 0;
    
    // 安全检查
    virtual HRESULT STDMETHODCALLTYPE VerifyPluginSignature(const char* dll_path, bool* verified) = 0;
    virtual HRESULT STDMETHODCALLTYPE ScanPluginForMalware(const char* dll_path, bool* clean) = 0;
    
    // 版本兼容性
    virtual HRESULT STDMETHODCALLTYPE CheckPluginCompatibility(const char* dll_path, bool* compatible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPluginVersion(const char* dll_path, DWORD* version) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRequiredAPIVersion(const char* dll_path, DWORD* api_version) = 0;
};

// 组件工厂模板
template<typename ComponentClass>
class fb2k_component_factory {
public:
    static IUnknown* create_instance() {
        return new ComponentClass();
    }
    
    static void register_component(const char* guid, component_type type) {
        auto* manager = fb2k_get_component_manager();
        if (manager) {
            // 注册组件工厂函数
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

// 静态成员定义
template<typename ComponentClass>
std::map<std::string, std::function<IUnknown*()>> fb2k_component_factory<ComponentClass>::component_factory_functions_;

template<typename ComponentClass>
std::map<std::string, component_type> fb2k_component_factory<ComponentClass>::component_types_;

// 组件注册宏
#define FB2K_REGISTER_COMPONENT(ComponentClass, ComponentGUID, ComponentType) \
    namespace { \
        struct ComponentClass##_registrar { \
            ComponentClass##_registrar() { \
                fb2k_component_factory<ComponentClass>::register_component(ComponentGUID, ComponentType); \
            } \
        } ComponentClass##_registrar_instance; \
    }

// 组件管理器实现
class fb2k_component_manager_impl : public fb2k_service_impl<IFB2KComponentManager> {
public:
    fb2k_component_manager_impl();
    ~fb2k_component_manager_impl();
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
    // IFB2KComponentManager实现
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

// 插件加载器实现
class fb2k_plugin_loader_impl : public fb2k_com_object<IFB2KPluginLoader> {
public:
    fb2k_plugin_loader_impl();
    ~fb2k_plugin_loader_impl();
    
    // IFB2KPluginLoader实现
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
    
    // 插件入口点定义
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

// 全局组件管理器访问
inline IFB2KComponentManager* fb2k_get_component_manager() {
    return fb2k_get_service<IFB2KComponentManager>();
}

// 全局插件加载器访问
inline IFB2KPluginLoader* fb2k_get_plugin_loader() {
    return fb2k_get_service<IFB2KPluginLoader>();
}

// 接口IID定义
template<> const GUID fb2k::IFB2KComponent::iid = IID_IFB2KComponent;
template<> const char* fb2k::IFB2KComponent::interface_name = "IFB2KComponent";

template<> const GUID fb2k::IFB2KComponentManager::iid = IID_IFB2KComponentManager;
template<> const char* fb2k::IFB2KComponentManager::interface_name = "IFB2KComponentManager";

template<> const GUID fb2k::IFB2KPluginLoader::iid = IID_IFB2KPluginLoader;
template<> const char* fb2k::IFB2KPluginLoader::interface_name = "IFB2KPluginLoader";

template<> const GUID fb2k::IFB2KInitQuit::iid = IID_IFB2KInitQuit;
template<> const char* fb2k::IFB2KInitQuit::interface_name = "IFB2KInitQuit";

} // namespace fb2k