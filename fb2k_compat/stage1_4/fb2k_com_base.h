#pragma once

#include <windows.h>
#include <unknwn.h>
#include <combaseapi.h>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <atomic>
#include <thread>
#include <chrono>

namespace fb2k {

// foobar2000 COM基础定义
#define FB2K_COM_INTERFACE(name, id) \
    struct name : public IUnknown { \
        static const GUID iid; \
        static const char* interface_name; \
    };

// 核心接口ID定义
// {B2C4C5A0-5C7D-4B8E-9F2A-1D3E5F7H9J2K}
static const GUID IID_IFB2KUnknown = 
    { 0xb2c4c5a0, 0x5c7d, 0x4b8e, { 0x9f, 0x2a, 0x1d, 0x3e, 0x5f, 0x7h, 0x9j, 0x2k } };

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
static const GUID IID_IFB2KService = 
    { 0xa1b2c3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 } };

// {F1E2D3C4-B5A6-9870-FEDC-BA0987654321}
static const GUID IID_IFB2KServiceProvider = 
    { 0xf1e2d3c4, 0xb5a6, 0x9870, { 0xfe, 0xdc, 0xba, 0x09, 0x87, 0x65, 0x43, 0x21 } };

// foobar2000基础未知接口
struct IFB2KUnknown : public IUnknown {
    // foobar2000扩展的未知接口
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppvObject) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetServiceGUID(GUID* pGuid) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsServiceSupported(REFGUID guidService) = 0;
};

// foobar2000服务接口基础
struct IFB2KService : public IFB2KUnknown {
    // 服务生命周期管理
    virtual HRESULT STDMETHODCALLTYPE Initialize() = 0;
    virtual HRESULT STDMETHODCALLTYPE Shutdown() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsInitialized() = 0;
    
    // 服务信息管理
    virtual HRESULT STDMETHODCALLTYPE GetServiceName(const char** name) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetServiceVersion(DWORD* version) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetServicePriority(DWORD* priority) = 0;
    
    // 服务状态
    virtual HRESULT STDMETHODCALLTYPE EnableService(BOOL enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsServiceEnabled() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetServiceStatus(DWORD* status) = 0;
};

// 服务提供接口
struct IFB2KServiceProvider : public IFB2KUnknown {
    // 服务注册和发现
    virtual HRESULT STDMETHODCALLTYPE RegisterService(REFGUID guidService, IUnknown* pService) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnregisterService(REFGUID guidService) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppvObject) = 0;
    
    // 服务枚举
    virtual HRESULT STDMETHODCALLTYPE EnumServices(GUID* services, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetServiceCount(DWORD* count) = 0;
    
    // 服务管理
    virtual HRESULT STDMETHODCALLTYPE StartAllServices() = 0;
    virtual HRESULT STDMETHODCALLTYPE StopAllServices() = 0;
    virtual HRESULT STDMETHODCALLTYPE InitializeAllServices() = 0;
    virtual HRESULT STDMETHODCALLTYPE ShutdownAllServices() = 0;
};

// foobar2000 COM对象基类
template<typename Interface>
class fb2k_com_object : public Interface {
public:
    fb2k_com_object() : ref_count_(1) {}
    virtual ~fb2k_com_object() = default;
    
    // IUnknown实现
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (!ppvObject) return E_POINTER;
        
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, Interface::iid)) {
            *ppvObject = static_cast<Interface*>(this);
            AddRef();
            return S_OK;
        }
        
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override {
        return ref_count_.fetch_add(1, std::memory_order_relaxed) + 1;
    }
    
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = ref_count_.fetch_sub(1, std::memory_order_relaxed) - 1;
        if (count == 0) {
            delete this;
        }
        return count;
    }
    
    // IFB2KUnknown基础实现
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppvObject) override {
        if (!ppvObject) return E_POINTER;
        *ppvObject = nullptr;
        
        // 默认实现：委托给全局服务提供者
        auto* provider = fb2k_service_provider::get_instance();
        if (provider) {
            return provider->QueryService(guidService, riid, ppvObject);
        }
        
        return E_NOINTERFACE;
    }
    
    HRESULT STDMETHODCALLTYPE GetServiceGUID(GUID* pGuid) override {
        if (!pGuid) return E_POINTER;
        *pGuid = Interface::iid;
        return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE IsServiceSupported(REFGUID guidService) override {
        auto* provider = fb2k_service_provider::get_instance();
        if (provider) {
            void* ptr = nullptr;
            HRESULT hr = provider->QueryService(guidService, IID_IUnknown, &ptr);
            if (SUCCEEDED(hr) && ptr) {
                static_cast<IUnknown*>(ptr)->Release();
                return S_OK;
            }
        }
        return S_FALSE;
    }
    
protected:
    std::atomic<ULONG> ref_count_;
};

// foobar2000服务实现基类
template<typename Interface>
class fb2k_service_impl : public fb2k_com_object<Interface> {
public:
    fb2k_service_impl() : initialized_(false), enabled_(true) {}
    
    // IFB2KService基础实现
    HRESULT STDMETHODCALLTYPE Initialize() override {
        if (initialized_) return S_OK;
        
        HRESULT hr = do_initialize();
        if (SUCCEEDED(hr)) {
            initialized_ = true;
        }
        return hr;
    }
    
    HRESULT STDMETHODCALLTYPE Shutdown() override {
        if (!initialized_) return S_OK;
        
        HRESULT hr = do_shutdown();
        initialized_ = false;
        return hr;
    }
    
    HRESULT STDMETHODCALLTYPE IsInitialized() override {
        return initialized_ ? S_OK : S_FALSE;
    }
    
    HRESULT STDMETHODCALLTYPE GetServiceName(const char** name) override {
        if (!name) return E_POINTER;
        *name = get_service_name_static();
        return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE GetServiceVersion(DWORD* version) override {
        if (!version) return E_POINTER;
        *version = get_service_version_static();
        return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE GetServicePriority(DWORD* priority) override {
        if (!priority) return E_POINTER;
        *priority = get_service_priority_static();
        return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE EnableService(BOOL enable) override {
        enabled_ = (enable != FALSE);
        return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE IsServiceEnabled() override {
        return enabled_ ? S_OK : S_FALSE;
    }
    
    HRESULT STDMETHODCALLTYPE GetServiceStatus(DWORD* status) override {
        if (!status) return E_POINTER;
        
        DWORD stat = 0;
        if (initialized_) stat |= 0x01;
        if (enabled_) stat |= 0x02;
        if (is_service_ready()) stat |= 0x04;
        
        *status = stat;
        return S_OK;
    }
    
protected:
    // 派生类需要重载的方法
    virtual HRESULT do_initialize() { return S_OK; }
    virtual HRESULT do_shutdown() { return S_OK; }
    virtual bool is_service_ready() { return initialized_ && enabled_; }
    
    static const char* get_service_name_static() { return "FB2KService"; }
    static DWORD get_service_version_static() { return 0x00010000; }  // 1.0.0.0
    static DWORD get_service_priority_static() { return 100; }
    
protected:
    std::atomic<bool> initialized_;
    std::atomic<bool> enabled_;
};

// foobar2000服务提供者实现
class fb2k_service_provider_impl : public fb2k_com_object<IFB2KServiceProvider> {
public:
    fb2k_service_provider_impl();
    ~fb2k_service_provider_impl();
    
    // IFB2KServiceProvider实现
    HRESULT STDMETHODCALLTYPE RegisterService(REFGUID guidService, IUnknown* pService) override;
    HRESULT STDMETHODCALLTYPE UnregisterService(REFGUID guidService) override;
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppvObject) override;
    HRESULT STDMETHODCALLTYPE EnumServices(GUID* services, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE GetServiceCount(DWORD* count) override;
    HRESULT STDMETHODCALLTYPE StartAllServices() override;
    HRESULT STDMETHODCALLTYPE StopAllServices() override;
    HRESULT STDMETHODCALLTYPE InitializeAllServices() override;
    HRESULT STDMETHODCALLTYPE ShutdownAllServices() override;
    
    // 全局实例访问
    static fb2k_service_provider_impl* get_instance();
    static void create_instance();
    static void destroy_instance();
    
private:
    struct service_entry {
        GUID guid;
        IUnknown* service;
        std::atomic<bool> initialized;
        std::atomic<bool> started;
    };
    
    std::map<GUID, service_entry, GUIDCompare> services_;
    std::mutex services_mutex_;
    
    static std::unique_ptr<fb2k_service_provider_impl> instance_;
    static std::mutex instance_mutex_;
    
    // GUID比较器
    struct GUIDCompare {
        bool operator()(const GUID& a, const GUID& b) const {
            return memcmp(&a, &b, sizeof(GUID)) < 0;
        }
    };
    
    bool find_service(REFGUID guid, service_entry** entry);
    HRESULT initialize_service(service_entry& entry);
    HRESULT shutdown_service(service_entry& entry);
    HRESULT start_service(service_entry& entry);
    HRESULT stop_service(service_entry& entry);
};

// GUID比较辅助函数
inline bool operator==(const GUID& a, const GUID& b) {
    return IsEqualGUID(a, b);
}

inline bool operator!=(const GUID& a, const GUID& b) {
    return !IsEqualGUID(a, b);
}

// 服务注册宏
#define FB2K_REGISTER_SERVICE(ServiceClass, ServiceGUID) \
    static struct ServiceClass##_registrar { \
        ServiceClass##_registrar() { \
            auto* provider = fb2k_service_provider::get_instance(); \
            if (provider) { \
                auto* service = new ServiceClass(); \
                provider->RegisterService(ServiceGUID, service); \
                service->Release(); \
            } \
        } \
    } ServiceClass##_registrar_instance;

// 服务查询辅助函数
template<typename Interface>
HRESULT fb2k_query_service(REFGUID guidService, Interface** ppInterface) {
    if (!ppInterface) return E_POINTER;
    
    auto* provider = fb2k_service_provider::get_instance();
    if (!provider) return E_FAIL;
    
    return provider->QueryService(guidService, Interface::iid, (void**)ppInterface);
}

// 全局服务提供者访问
inline fb2k_service_provider_impl* fb2k_service_provider::get_instance() {
    return fb2k_service_provider_impl::get_instance();
}

// foobar2000核心服务接口定义

// {5C3D37A1-8B4E-4F2D-9C6A-5E8F12345678}
static const GUID IID_IFB2KCore = 
    { 0x5c3d37a1, 0x8b4e, 0x4f2d, { 0x9c, 0x6a, 0x5e, 0x8f, 0x12, 0x34, 0x56, 0x78 } };

// {7A8B9C0D-1E2F-3A4B-5C6D-7E8F9A0B1C2D}
static const GUID IID_IFB2KPlaybackControl = 
    { 0x7a8b9c0d, 0x1e2f, 0x3a4b, { 0x5c, 0x6d, 0x7e, 0x8f, 0x9a, 0x0b, 0x1c, 0x2d } };

// {9B8C7D6E-5F4A-3B2C-1D0E-F9A8B7C6D5E4}
static const GUID IID_IFB2KMetadb = 
    { 0x9b8c7d6e, 0x5f4a, 0x3b2c, { 0x1d, 0x0e, 0xf9, 0xa8, 0xb7, 0xc6, 0xd5, 0xe4 } };

// {2D3E4F5A-6B7C-8D9E-0F1A-2B3C4D5E6F7A}
static const GUID IID_IFB2KConfigManager = 
    { 0x2d3e4f5a, 0x6b7c, 0x8d9e, { 0x0f, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x7a } };

// 核心服务接口
struct IFB2KCore : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 核心系统管理
    virtual HRESULT STDMETHODCALLTYPE GetVersion(DWORD* major, DWORD* minor, DWORD* build, DWORD* revision) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBuildDate(const char** date) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAppName(const char** name) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAppPath(const char** path) = 0;
    
    // 全局状态
    virtual HRESULT STDMETHODCALLTYPE IsShuttingDown() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsLowPriority() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMainThreadId(DWORD* thread_id) = 0;
    
    // 性能统计
    virtual HRESULT STDMETHODCALLTYPE GetPerformanceCounters(double* cpu_usage, double* memory_usage, double* audio_latency) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetPerformanceCounters() = 0;
};

// 播放控制接口
struct IFB2KPlaybackControl : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 播放状态
    virtual HRESULT STDMETHODCALLTYPE GetPlaybackState(DWORD* state) = 0; // 0=stopped, 1=playing, 2=paused
    virtual HRESULT STDMETHODCALLTYPE IsPlaying() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsPaused() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanPlay() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanPause() = 0;
    virtual HRESULT STDMETHODCALLTYPE CanStop() = 0;
    
    // 播放控制
    virtual HRESULT STDMETHODCALLTYPE Play() = 0;
    virtual HRESULT STDMETHODCALLTYPE Pause() = 0;
    virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
    virtual HRESULT STDMETHODCALLTYPE PlayOrPause() = 0;
    virtual HRESULT STDMETHODCALLTYPE Previous() = 0;
    virtual HRESULT STDMETHODCALLTYPE Next() = 0;
    virtual HRESULT STDMETHODCALLTYPE Random() = 0;
    
    // 播放进度
    virtual HRESULT STDMETHODCALLTYPE GetPlaybackPosition(double* position_seconds) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPlaybackPosition(double position_seconds) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPlaybackLength(double* length_seconds) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPlaybackPercentage(double* percentage) = 0;
    
    // 音量控制
    virtual HRESULT STDMETHODCALLTYPE GetVolume(float* volume) = 0; // 0.0-1.0
    virtual HRESULT STDMETHODCALLTYPE SetVolume(float volume) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMute(bool* mute) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMute(bool mute) = 0;
    
    // 播放队列
    virtual HRESULT STDMETHODCALLTYPE GetQueueContents(const char*** items, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddToQueue(const char* item_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveFromQueue(DWORD index) = 0;
    virtual HRESULT STDMETHODCALLTYPE ClearQueue() = 0;
    
    // 播放统计
    virtual HRESULT STDMETHODCALLTYPE GetPlaybackStatistics(DWORD* total_plays, DWORD* total_time, const char** last_played) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetPlaybackStatistics() = 0;
};

// 元数据库接口
struct IFB2KMetadb : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 数据库操作
    virtual HRESULT STDMETHODCALLTYPE OpenDatabase(const char* path) = 0;
    virtual HRESULT STDMETHODCALLTYPE CloseDatabase() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsDatabaseOpen() = 0;
    
    // 元数据查询
    virtual HRESULT STDMETHODCALLTYPE GetMetaValue(const char* item_path, const char* meta_name, char** value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMetaValue(const char* item_path, const char* meta_name, const char* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAllMetaValues(const char* item_path, const char*** names, const char*** values, DWORD* count) = 0;
    
    // 文件信息
    virtual HRESULT STDMETHODCALLTYPE GetFileInfo(const char* item_path, DWORD* file_size, DWORD* bitrate, DWORD* duration, DWORD* sample_rate, DWORD* channels) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFileStats(const char* item_path, DWORD* play_count, DWORD* first_played, DWORD* last_played) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateFileStats(const char* item_path, DWORD play_count_increment) = 0;
    
    // 数据库维护
    virtual HRESULT STDMETHODCALLTYPE CompactDatabase() = 0;
    virtual HRESULT STDMETHODCALLTYPE VerifyDatabase() = 0;
    virtual HRESULT STDMETHODCALLTYPE BackupDatabase(const char* backup_path) = 0;
    
    // 批量操作
    virtual HRESULT STDMETHODCALLTYPE BeginTransaction() = 0;
    virtual HRESULT STDMETHODCALLTYPE CommitTransaction() = 0;
    virtual HRESULT STDMETHODCALLTYPE RollbackTransaction() = 0;
};

// 配置管理接口
struct IFB2KConfigManager : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 配置读写
    virtual HRESULT STDMETHODCALLTYPE GetConfigValue(const char* section, const char* key, DWORD* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConfigValue(const char* section, const char* key, DWORD value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConfigString(const char* section, const char* key, char** value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConfigString(const char* section, const char* key, const char* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConfigBinary(const char* section, const char* key, BYTE** data, DWORD* size) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConfigBinary(const char* section, const char* key, const BYTE* data, DWORD size) = 0;
    
    // 配置管理
    virtual HRESULT STDMETHODCALLTYPE DeleteConfigValue(const char* section, const char* key) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteConfigSection(const char* section) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumConfigSections(const char*** sections, DWORD* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumConfigKeys(const char* section, const char*** keys, DWORD* count) = 0;
    
    // 配置持久化
    virtual HRESULT STDMETHODCALLTYPE SaveConfig() = 0;
    virtual HRESULT STDMETHODCALLTYPE LoadConfig() = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetConfig() = 0;
    virtual HRESULT STDMETHODCALLTYPE ImportConfig(const char* file_path) = 0;
    virtual HRESULT STDMETHODCALLTYPE ExportConfig(const char* file_path) = 0;
    
    // 高级配置
    virtual HRESULT STDMETHODCALLTYPE GetConfigBool(const char* section, const char* key, bool* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConfigBool(const char* section, const char* key, bool value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConfigDouble(const char* section, const char* key, double* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConfigDouble(const char* section, const char* key, double value) = 0;
};

// 全局服务访问辅助函数
template<typename ServiceInterface>
ServiceInterface* fb2k_get_service() {
    ServiceInterface* service = nullptr;
    HRESULT hr = fb2k_query_service(ServiceInterface::iid, &service);
    return SUCCEEDED(hr) ? service : nullptr;
}

// 核心服务访问快捷方式
inline IFB2KCore* fb2k_core() {
    return fb2k_get_service<IFB2KCore>();
}

inline IFB2KPlaybackControl* fb2k_playback_control() {
    return fb2k_get_service<IFB2KPlaybackControl>();
}

inline IFB2KMetadb* fb2k_metadb() {
    return fb2k_get_service<IFB2KMetadb>();
}

inline IFB2KConfigManager* fb2k_config_manager() {
    return fb2k_get_service<IFB2KConfigManager>();
}

} // namespace fb2k

// 接口IID定义
template<> const GUID fb2k::IFB2KCore::iid = IID_IFB2KCore;
template<> const char* fb2k::IFB2KCore::interface_name = "IFB2KCore";

template<> const GUID fb2k::IFB2KPlaybackControl::iid = IID_IFB2KPlaybackControl;
template<> const char* fb2k::IFB2KPlaybackControl::interface_name = "IFB2KPlaybackControl";

template<> const GUID fb2k::IFB2KMetadb::iid = IID_IFB2KMetadb;
template<> const char* fb2k::IFB2KMetadb::interface_name = "IFB2KMetadb";

template<> const GUID fb2k::IFB2KConfigManager::iid = IID_IFB2KConfigManager;
template<> const char* fb2k::IFB2KConfigManager::interface_name = "IFB2KConfigManager";