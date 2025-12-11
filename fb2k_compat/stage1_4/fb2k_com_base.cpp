#include "fb2k_com_base.h"
#include <windows.h>
#include <combaseapi.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <algorithm>

namespace fb2k {

// 全局服务提供者实例
std::unique_ptr<fb2k_service_provider_impl> fb2k_service_provider_impl::instance_;
std::mutex fb2k_service_provider_impl::instance_mutex_;

// fb2k_service_provider_impl实现
fb2k_service_provider_impl::fb2k_service_provider_impl() {
    std::cout << "[FB2K COM] 服务提供者创建" << std::endl;
}

fb2k_service_provider_impl::~fb2k_service_provider_impl() {
    std::cout << "[FB2K COM] 服务提供者销毁" << std::endl;
    
    // 清理所有服务
    std::lock_guard<std::mutex> lock(services_mutex_);
    for (auto& [guid, entry] : services_) {
        if (entry.service) {
            // 调用服务的Shutdown方法
            IFB2KService* service = nullptr;
            if (SUCCEEDED(entry.service->QueryInterface(IID_IFB2KService, (void**)&service))) {
                service->Shutdown();
                service->Release();
            }
            entry.service->Release();
        }
    }
    services_.clear();
}

HRESULT fb2k_service_provider_impl::RegisterService(REFGUID guidService, IUnknown* pService) {
    if (!pService) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    // 检查是否已存在
    auto it = services_.find(guidService);
    if (it != services_.end()) {
        std::cout << "[FB2K COM] 服务已存在，替换: " << guid_to_string(guidService) << std::endl;
        if (it->second.service) {
            it->second.service->Release();
        }
        it->second.service = pService;
        it->second.service->AddRef();
        return S_OK;
    }
    
    // 添加新服务
    service_entry entry;
    entry.guid = guidService;
    entry.service = pService;
    entry.initialized = false;
    entry.started = false;
    
    pService->AddRef();
    services_[guidService] = entry;
    
    std::cout << "[FB2K COM] 服务注册成功: " << guid_to_string(guidService) << std::endl;
    return S_OK;
}

HRESULT fb2k_service_provider_impl::UnregisterService(REFGUID guidService) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    auto it = services_.find(guidService);
    if (it == services_.end()) {
        return E_FAIL;
    }
    
    // 清理服务
    if (it->second.service) {
        // 调用服务的Shutdown方法
        IFB2KService* service = nullptr;
        if (SUCCEEDED(it->second.service->QueryInterface(IID_IFB2KService, (void**)&service))) {
            service->Shutdown();
            service->Release();
        }
        it->second.service->Release();
    }
    
    services_.erase(it);
    
    std::cout << "[FB2K COM] 服务注销: " << guid_to_string(guidService) << std::endl;
    return S_OK;
}

HRESULT fb2k_service_provider_impl::QueryService(REFGUID guidService, REFIID riid, void** ppvObject) {
    if (!ppvObject) return E_POINTER;
    *ppvObject = nullptr;
    
    service_entry* entry = nullptr;
    if (!find_service(guidService, &entry) || !entry || !entry->service) {
        return E_NOINTERFACE;
    }
    
    return entry->service->QueryInterface(riid, ppvObject);
}

HRESULT fb2k_service_provider_impl::EnumServices(GUID* services, DWORD* count) {
    if (!count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    DWORD service_count = static_cast<DWORD>(services_.size());
    if (!services) {
        *count = service_count;
        return S_OK;
    }
    
    DWORD to_copy = std::min(*count, service_count);
    DWORD index = 0;
    for (const auto& [guid, entry] : services_) {
        if (index >= to_copy) break;
        services[index++] = guid;
    }
    
    *count = to_copy;
    return S_OK;
}

HRESULT fb2k_service_provider_impl::GetServiceCount(DWORD* count) {
    if (!count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(services_mutex_);
    *count = static_cast<DWORD>(services_.size());
    return S_OK;
}

HRESULT fb2k_service_provider_impl::StartAllServices() {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    HRESULT overall_result = S_OK;
    for (auto& [guid, entry] : services_) {
        HRESULT hr = start_service(entry);
        if (FAILED(hr)) {
            std::cerr << "[FB2K COM] 启动服务失败: " << guid_to_string(guid) << std::endl;
            overall_result = hr;
        }
    }
    
    return overall_result;
}

HRESULT fb2k_service_provider_impl::StopAllServices() {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    HRESULT overall_result = S_OK;
    for (auto& [guid, entry] : services_) {
        HRESULT hr = stop_service(entry);
        if (FAILED(hr)) {
            std::cerr << "[FB2K COM] 停止服务失败: " << guid_to_string(guid) << std::endl;
            overall_result = hr;
        }
    }
    
    return overall_result;
}

HRESULT fb2k_service_provider_impl::InitializeAllServices() {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    HRESULT overall_result = S_OK;
    for (auto& [guid, entry] : services_) {
        HRESULT hr = initialize_service(entry);
        if (FAILED(hr)) {
            std::cerr << "[FB2K COM] 初始化服务失败: " << guid_to_string(guid) << std::endl;
            overall_result = hr;
        }
    }
    
    return overall_result;
}

HRESULT fb2k_service_provider_impl::ShutdownAllServices() {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    HRESULT overall_result = S_OK;
    for (auto& [guid, entry] : services_) {
        HRESULT hr = shutdown_service(entry);
        if (FAILED(hr)) {
            std::cerr << "[FB2K COM] 关闭服务失败: " << guid_to_string(guid) << std::endl;
            overall_result = hr;
        }
    }
    
    return overall_result;
}

// 全局实例管理
fb2k_service_provider_impl* fb2k_service_provider_impl::get_instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    return instance_.get();
}

void fb2k_service_provider_impl::create_instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = std::make_unique<fb2k_service_provider_impl>();
    }
}

void fb2k_service_provider_impl::destroy_instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_.reset();
}

// 私有辅助方法
bool fb2k_service_provider_impl::find_service(REFGUID guid, service_entry** entry) {
    auto it = services_.find(guid);
    if (it != services_.end()) {
        *entry = &it->second;
        return true;
    }
    return false;
}

HRESULT fb2k_service_provider_impl::initialize_service(service_entry& entry) {
    if (entry.initialized) return S_OK;
    
    if (!entry.service) return E_FAIL;
    
    IFB2KService* service = nullptr;
    HRESULT hr = entry.service->QueryInterface(IID_IFB2KService, (void**)&service);
    if (FAILED(hr)) return hr;
    
    hr = service->Initialize();
    service->Release();
    
    if (SUCCEEDED(hr)) {
        entry.initialized = true;
    }
    
    return hr;
}

HRESULT fb2k_service_provider_impl::shutdown_service(service_entry& entry) {
    if (!entry.initialized) return S_OK;
    
    if (!entry.service) return E_FAIL;
    
    IFB2KService* service = nullptr;
    HRESULT hr = entry.service->QueryInterface(IID_IFB2KService, (void**)&service);
    if (FAILED(hr)) return hr;
    
    hr = service->Shutdown();
    service->Release();
    
    entry.initialized = false;
    return hr;
}

HRESULT fb2k_service_provider_impl::start_service(service_entry& entry) {
    if (entry.started) return S_OK;
    entry.started = true;
    return S_OK;
}

HRESULT fb2k_service_provider_impl::stop_service(service_entry& entry) {
    if (!entry.started) return S_OK;
    entry.started = false;
    return S_OK;
}

// GUID转换辅助函数
std::string guid_to_string(const GUID& guid) {
    std::stringstream ss;
    ss << "{" << std::hex << std::uppercase << std::setfill('0');
    ss << std::setw(8) << guid.Data1 << "-";
    ss << std::setw(4) << guid.Data2 << "-";
    ss << std::setw(4) << guid.Data3 << "-";
    ss << std::setw(2) << (int)guid.Data4[0] << std::setw(2) << (int)guid.Data4[1] << "-";
    for (int i = 2; i < 8; i++) {
        ss << std::setw(2) << (int)guid.Data4[i];
    }
    ss << "}";
    return ss.str();
}

// 核心服务实现
class fb2k_core_impl : public fb2k_service_impl<IFB2KCore> {
public:
    fb2k_core_impl();
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
    // IFB2KCore实现
    HRESULT STDMETHODCALLTYPE GetVersion(DWORD* major, DWORD* minor, DWORD* build, DWORD* revision) override;
    HRESULT STDMETHODCALLTYPE GetBuildDate(const char** date) override;
    HRESULT STDMETHODCALLTYPE GetAppName(const char** name) override;
    HRESULT STDMETHODCALLTYPE GetAppPath(const char** path) override;
    HRESULT STDMETHODCALLTYPE IsShuttingDown() override;
    HRESULT STDMETHODCALLTYPE IsLowPriority() override;
    HRESULT STDMETHODCALLTYPE GetMainThreadId(DWORD* thread_id) override;
    HRESULT STDMETHODCALLTYPE GetPerformanceCounters(double* cpu_usage, double* memory_usage, double* audio_latency) override;
    HRESULT STDMETHODCALLTYPE ResetPerformanceCounters() override;
    
private:
    DWORD version_major_;
    DWORD version_minor_;
    DWORD version_build_;
    DWORD version_revision_;
    
    std::string build_date_;
    std::string app_name_;
    std::string app_path_;
    
    std::atomic<bool> shutting_down_;
    std::atomic<bool> low_priority_;
    DWORD main_thread_id_;
    
    // 性能计数器
    double cpu_usage_;
    double memory_usage_;
    double audio_latency_;
    std::chrono::steady_clock::time_point start_time_;
    
    void initialize_paths();
    void update_performance_counters();
    double get_current_cpu_usage();
    double get_current_memory_usage();
};

// 播放控制实现
class fb2k_playback_control_impl : public fb2k_service_impl<IFB2KPlaybackControl> {
public:
    fb2k_playback_control_impl();
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
    // IFB2KPlaybackControl实现
    HRESULT STDMETHODCALLTYPE GetPlaybackState(DWORD* state) override;
    HRESULT STDMETHODCALLTYPE IsPlaying() override;
    HRESULT STDMETHODCALLTYPE IsPaused() override;
    HRESULT STDMETHODCALLTYPE CanPlay() override;
    HRESULT STDMETHODCALLTYPE CanPause() override;
    HRESULT STDMETHODCALLTYPE CanStop() override;
    HRESULT STDMETHODCALLTYPE Play() override;
    HRESULT STDMETHODCALLTYPE Pause() override;
    HRESULT STDMETHODCALLTYPE Stop() override;
    HRESULT STDMETHODCALLTYPE PlayOrPause() override;
    HRESULT STDMETHODCALLTYPE Previous() override;
    HRESULT STDMETHODCALLTYPE Next() override;
    HRESULT STDMETHODCALLTYPE Random() override;
    HRESULT STDMETHODCALLTYPE GetPlaybackPosition(double* position_seconds) override;
    HRESULT STDMETHODCALLTYPE SetPlaybackPosition(double position_seconds) override;
    HRESULT STDMETHODCALLTYPE GetPlaybackLength(double* length_seconds) override;
    HRESULT STDMETHODCALLTYPE GetPlaybackPercentage(double* percentage) override;
    HRESULT STDMETHODCALLTYPE GetVolume(float* volume) override;
    HRESULT STDMETHODCALLTYPE SetVolume(float volume) override;
    HRESULT STDMETHODCALLTYPE GetMute(bool* mute) override;
    HRESULT STDMETHODCALLTYPE SetMute(bool mute) override;
    HRESULT STDMETHODCALLTYPE GetQueueContents(const char*** items, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE AddToQueue(const char* item_path) override;
    HRESULT STDMETHODCALLTYPE RemoveFromQueue(DWORD index) override;
    HRESULT STDMETHODCALLTYPE ClearQueue() override;
    HRESULT STDMETHODCALLTYPE GetPlaybackStatistics(DWORD* total_plays, DWORD* total_time, const char** last_played) override;
    HRESULT STDMETHODCALLTYPE ResetPlaybackStatistics() override;
    
private:
    enum playback_state {
        state_stopped = 0,
        state_playing = 1,
        state_paused = 2
    };
    
    std::atomic<playback_state> current_state_;
    std::atomic<float> volume_;
    std::atomic<bool> mute_;
    std::atomic<double> playback_position_;
    std::atomic<double> playback_length_;
    
    std::vector<std::string> play_queue_;
    std::mutex queue_mutex_;
    
    // 播放统计
    DWORD total_play_count_;
    DWORD total_play_time_;
    std::string last_played_item_;
    std::mutex stats_mutex_;
    
    void update_playback_statistics();
    bool validate_playback_position(double position);
};

// 元数据库实现
class fb2k_metadb_impl : public fb2k_service_impl<IFB2KMetadb> {
public:
    fb2k_metadb_impl();
    ~fb2k_metadb_impl();
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
    // IFB2KMetadb实现
    HRESULT STDMETHODCALLTYPE OpenDatabase(const char* path) override;
    HRESULT STDMETHODCALLTYPE CloseDatabase() override;
    HRESULT STDMETHODCALLTYPE IsDatabaseOpen() override;
    HRESULT STDMETHODCALLTYPE GetMetaValue(const char* item_path, const char* meta_name, char** value) override;
    HRESULT STDMETHODCALLTYPE SetMetaValue(const char* item_path, const char* meta_name, const char* value) override;
    HRESULT STDMETHODCALLTYPE GetAllMetaValues(const char* item_path, const char*** names, const char*** values, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE GetFileInfo(const char* item_path, DWORD* file_size, DWORD* bitrate, DWORD* duration, DWORD* sample_rate, DWORD* channels) override;
    HRESULT STDMETHODCALLTYPE GetFileStats(const char* item_path, DWORD* play_count, DWORD* first_played, DWORD* last_played) override;
    HRESULT STDMETHODCALLTYPE UpdateFileStats(const char* item_path, DWORD play_count_increment) override;
    HRESULT STDMETHODCALLTYPE CompactDatabase() override;
    HRESULT STDMETHODCALLTYPE VerifyDatabase() override;
    HRESULT STDMETHODCALLTYPE BackupDatabase(const char* backup_path) override;
    HRESULT STDMETHODCALLTYPE BeginTransaction() override;
    HRESULT STDMETHODCALLTYPE CommitTransaction() override;
    HRESULT STDMETHODCALLTYPE RollbackTransaction() override;
    
private:
    struct file_info {
        std::string path;
        DWORD file_size;
        DWORD bitrate;
        DWORD duration;
        DWORD sample_rate;
        DWORD channels;
        DWORD play_count;
        DWORD first_played;
        DWORD last_played;
        std::map<std::string, std::string> meta_values;
    };
    
    std::string database_path_;
    std::map<std::string, file_info> file_database_;
    std::mutex database_mutex_;
    std::atomic<bool> database_open_;
    std::atomic<int> transaction_level_;
    
    std::map<std::string, file_info> transaction_backup_;
    
    bool load_database();
    bool save_database();
    file_info* find_file_info(const std::string& path);
    const file_info* find_file_info(const std::string& path) const;
    void parse_file_path(const std::string& path, file_info& info);
    std::string get_meta_file_path() const;
};

// 配置管理实现
class fb2k_config_manager_impl : public fb2k_service_impl<IFB2KConfigManager> {
public:
    fb2k_config_manager_impl();
    ~fb2k_config_manager_impl();
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
    // IFB2KConfigManager实现
    HRESULT STDMETHODCALLTYPE GetConfigValue(const char* section, const char* key, DWORD* value) override;
    HRESULT STDMETHODCALLTYPE SetConfigValue(const char* section, const char* key, DWORD value) override;
    HRESULT STDMETHODCALLTYPE GetConfigString(const char* section, const char* key, char** value) override;
    HRESULT STDMETHODCALLTYPE SetConfigString(const char* section, const char* key, const char* value) override;
    HRESULT STDMETHODCALLTYPE GetConfigBinary(const char* section, const char* key, BYTE** data, DWORD* size) override;
    HRESULT STDMETHODCALLTYPE SetConfigBinary(const char* section, const char* key, const BYTE* data, DWORD size) override;
    HRESULT STDMETHODCALLTYPE DeleteConfigValue(const char* section, const char* key) override;
    HRESULT STDMETHODCALLTYPE DeleteConfigSection(const char* section) override;
    HRESULT STDMETHODCALLTYPE EnumConfigSections(const char*** sections, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE EnumConfigKeys(const char* section, const char*** keys, DWORD* count) override;
    HRESULT STDMETHODCALLTYPE SaveConfig() override;
    HRESULT STDMETHODCALLTYPE LoadConfig() override;
    HRESULT STDMETHODCALLTYPE ResetConfig() override;
    HRESULT STDMETHODCALLTYPE ImportConfig(const char* file_path) override;
    HRESULT STDMETHODCALLTYPE ExportConfig(const char* file_path) override;
    HRESULT STDMETHODCALLTYPE GetConfigBool(const char* section, const char* key, bool* value) override;
    HRESULT STDMETHODCALLTYPE SetConfigBool(const char* section, const char* key, bool value) override;
    HRESULT STDMETHODCALLTYPE GetConfigDouble(const char* section, const char* key, double* value) override;
    HRESULT STDMETHODCALLTYPE SetConfigDouble(const char* section, const char* key, double value) override;
    
private:
    struct config_value {
        enum type { INTEGER, STRING, BINARY, BOOLEAN, DOUBLE } value_type;
        
        union {
            DWORD int_value;
            bool bool_value;
            double double_value;
        };
        
        std::string string_value;
        std::vector<BYTE> binary_value;
        
        config_value() : value_type(INTEGER) { int_value = 0; }
    };
    
    std::map<std::string, std::map<std::string, config_value>> config_data_;
    std::mutex config_mutex_;
    std::string config_file_path_;
    std::atomic<bool> config_modified_;
    
    bool load_config_from_file(const std::string& file_path);
    bool save_config_to_file(const std::string& file_path);
    std::string get_config_file_path() const;
    void set_config_modified(bool modified);
    
    template<typename T>
    HRESULT get_config_value_t(const char* section, const char* key, T* value, config_value::type type);
    
    template<typename T>
    HRESULT set_config_value_t(const char* section, const char* key, T value, config_value::type type);
};

// 核心服务注册宏
#define FB2K_REGISTER_CORE_SERVICE(ServiceClass, ServiceGUID) \
    namespace { \
        struct ServiceClass##_auto_register { \
            ServiceClass##_auto_register() { \
                fb2k_service_provider_impl::create_instance(); \
                auto* provider = fb2k_service_provider_impl::get_instance(); \
                if (provider) { \
                    auto* service = new ServiceClass(); \
                    provider->RegisterService(ServiceGUID, service); \
                    service->Release(); \
                } \
            } \
        } ServiceClass##_auto_register_instance; \
    }

// 初始化核心服务
void initialize_fb2k_core_services() {
    std::cout << "[FB2K COM] 初始化核心服务..." << std::endl;
    
    fb2k_service_provider_impl::create_instance();
    auto* provider = fb2k_service_provider_impl::get_instance();
    if (!provider) {
        std::cerr << "[FB2K COM] 服务提供者创建失败" << std::endl;
        return;
    }
    
    // 注册核心服务
    auto* core_service = new fb2k_core_impl();
    provider->RegisterService(IID_IFB2KCore, core_service);
    core_service->Release();
    
    auto* playback_service = new fb2k_playback_control_impl();
    provider->RegisterService(IID_IFB2KPlaybackControl, playback_service);
    playback_service->Release();
    
    auto* metadb_service = new fb2k_metadb_impl();
    provider->RegisterService(IID_IFB2KMetadb, metadb_service);
    metadb_service->Release();
    
    auto* config_service = new fb2k_config_manager_impl();
    provider->RegisterService(IID_IFB2KConfigManager, config_service);
    config_service->Release();
    
    // 初始化所有服务
    provider->InitializeAllServices();
    
    std::cout << "[FB2K COM] 核心服务初始化完成" << std::endl;
}

// 清理核心服务
void shutdown_fb2k_core_services() {
    std::cout << "[FB2K COM] 关闭核心服务..." << std::endl;
    
    auto* provider = fb2k_service_provider_impl::get_instance();
    if (provider) {
        provider->ShutdownAllServices();
        fb2k_service_provider_impl::destroy_instance();
    }
    
    std::cout << "[FB2K COM] 核心服务已关闭" << std::endl;
}

// fb2k_core_impl实现
fb2k_core_impl::fb2k_core_impl() 
    : version_major_(1), version_minor_(6), version_build_(0), version_revision_(0)
    , shutting_down_(false), low_priority_(false), main_thread_id_(GetCurrentThreadId())
    , cpu_usage_(0.0), memory_usage_(0.0), audio_latency_(0.0) {
    start_time_ = std::chrono::steady_clock::now();
}

HRESULT fb2k_core_impl::do_initialize() {
    std::cout << "[FB2K Core] 初始化核心服务" << std::endl;
    initialize_paths();
    return S_OK;
}

HRESULT fb2k_core_impl::do_shutdown() {
    std::cout << "[FB2K Core] 关闭核心服务" << std::endl;
    shutting_down_ = true;
    return S_OK;
}

HRESULT fb2k_core_impl::GetVersion(DWORD* major, DWORD* minor, DWORD* build, DWORD* revision) {
    if (major) *major = version_major_;
    if (minor) *minor = version_minor_;
    if (build) *build = version_build_;
    if (revision) *revision = version_revision_;
    return S_OK;
}

HRESULT fb2k_core_impl::GetBuildDate(const char** date) {
    if (!date) return E_POINTER;
    if (build_date_.empty()) {
        auto now = std::time(nullptr);
        auto* timeinfo = std::localtime(&now);
        build_date_ = std::string(std::asctime(timeinfo));
        // 移除末尾的换行符
        if (!build_date_.empty() && build_date_.back() == '\n') {
            build_date_.pop_back();
        }
    }
    *date = build_date_.c_str();
    return S_OK;
}

HRESULT fb2k_core_impl::GetAppName(const char** name) {
    if (!name) return E_POINTER;
    if (app_name_.empty()) {
        app_name_ = "foobar2000 Compatible Player";
    }
    *name = app_name_.c_str();
    return S_OK;
}

HRESULT fb2k_core_impl::GetAppPath(const char** path) {
    if (!name) return E_POINTER;
    if (app_path_.empty()) {
        initialize_paths();
    }
    *path = app_path_.c_str();
    return S_OK;
}

HRESULT fb2k_core_impl::IsShuttingDown() {
    return shutting_down_ ? S_OK : S_FALSE;
}

HRESULT fb2k_core_impl::IsLowPriority() {
    return low_priority_ ? S_OK : S_FALSE;
}

HRESULT fb2k_core_impl::GetMainThreadId(DWORD* thread_id) {
    if (!thread_id) return E_POINTER;
    *thread_id = main_thread_id_;
    return S_OK;
}

HRESULT fb2k_core_impl::GetPerformanceCounters(double* cpu_usage, double* memory_usage, double* audio_latency) {
    update_performance_counters();
    
    if (cpu_usage) *cpu_usage = cpu_usage_;
    if (memory_usage) *memory_usage = memory_usage_;
    if (audio_latency) *audio_latency = audio_latency_;
    
    return S_OK;
}

HRESULT fb2k_core_impl::ResetPerformanceCounters() {
    cpu_usage_ = 0.0;
    memory_usage_ = 0.0;
    audio_latency_ = 0.0;
    start_time_ = std::chrono::steady_clock::now();
    return S_OK;
}

void fb2k_core_impl::initialize_paths() {
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buffer, MAX_PATH) > 0) {
        app_path_ = buffer;
        // 提取目录路径
        size_t last_slash = app_path_.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            app_path_ = app_path_.substr(0, last_slash);
        }
    }
}

void fb2k_core_impl::update_performance_counters() {
    cpu_usage_ = get_current_cpu_usage();
    memory_usage_ = get_current_memory_usage();
    // audio_latency_ 应该由音频子系统更新
}

double fb2k_core_impl::get_current_cpu_usage() {
    // 简化的CPU使用率计算
    static FILETIME prev_idle, prev_kernel, prev_user;
    FILETIME idle, kernel, user;
    
    if (GetSystemTimes(&idle, &kernel, &user)) {
        if (prev_idle.dwLowDateTime != 0) {
            ULONGLONG idle_diff = (idle.dwLowDateTime + ((ULONGLONG)idle.dwHighDateTime << 32)) - 
                                 (prev_idle.dwLowDateTime + ((ULONGLONG)prev_idle.dwHighDateTime << 32));
            ULONGLONG kernel_diff = (kernel.dwLowDateTime + ((ULONGLONG)kernel.dwHighDateTime << 32)) - 
                                   (prev_kernel.dwLowDateTime + ((ULONGLONG)prev_kernel.dwHighDateTime << 32));
            ULONGLONG user_diff = (user.dwLowDateTime + ((ULONGLONG)user.dwHighDateTime << 32)) - 
                                 (prev_user.dwLowDateTime + ((ULONGLONG)prev_user.dwHighDateTime << 32));
            
            if (kernel_diff + user_diff > 0) {
                return (double)(kernel_diff + user_diff - idle_diff) / (kernel_diff + user_diff) * 100.0;
            }
        }
        
        prev_idle = idle;
        prev_kernel = kernel;
        prev_user = user;
    }
    
    return 0.0;
}

double fb2k_core_impl::get_current_memory_usage() {
    // 获取当前进程内存使用
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / (1024.0 * 1024.0); // MB
    }
    return 0.0;
}

// fb2k_playback_control_impl实现
fb2k_playback_control_impl::fb2k_playback_control_impl()
    : current_state_(state_stopped), volume_(1.0f), mute_(false)
    , playback_position_(0.0), playback_length_(0.0)
    , total_play_count_(0), total_play_time_(0) {
}

HRESULT fb2k_playback_control_impl::do_initialize() {
    std::cout << "[FB2K Playback] 初始化播放控制服务" << std::endl;
    return S_OK;
}

HRESULT fb2k_playback_control_impl::do_shutdown() {
    std::cout << "[FB2K Playback] 关闭播放控制服务" << std::endl;
    Stop();
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetPlaybackState(DWORD* state) {
    if (!state) return E_POINTER;
    *state = static_cast<DWORD>(current_state_.load());
    return S_OK;
}

HRESULT fb2k_playback_control_impl::IsPlaying() {
    return (current_state_ == state_playing) ? S_OK : S_FALSE;
}

HRESULT fb2k_playback_control_impl::IsPaused() {
    return (current_state_ == state_paused) ? S_OK : S_FALSE;
}

HRESULT fb2k_playback_control_impl::CanPlay() {
    return (current_state_ != state_playing) ? S_OK : S_FALSE;
}

HRESULT fb2k_playback_control_impl::CanPause() {
    return (current_state_ == state_playing) ? S_OK : S_FALSE;
}

HRESULT fb2k_playback_control_impl::CanStop() {
    return (current_state_ != state_stopped) ? S_OK : S_FALSE;
}

HRESULT fb2k_playback_control_impl::Play() {
    if (current_state_ == state_playing) return S_OK;
    
    current_state_ = state_playing;
    std::cout << "[FB2K Playback] 开始播放" << std::endl;
    
    // 这里应该启动实际的音频播放
    // 目前只是状态更新
    
    return S_OK;
}

HRESULT fb2k_playback_control_impl::Pause() {
    if (current_state_ != state_playing) return E_FAIL;
    
    current_state_ = state_paused;
    std::cout << "[FB2K Playback] 暂停播放" << std::endl;
    
    return S_OK;
}

HRESULT fb2k_playback_control_impl::Stop() {
    if (current_state_ == state_stopped) return S_OK;
    
    current_state_ = state_stopped;
    playback_position_ = 0.0;
    std::cout << "[FB2K Playback] 停止播放" << std::endl;
    
    return S_OK;
}

HRESULT fb2k_playback_control_impl::PlayOrPause() {
    if (current_state_ == state_playing) {
        return Pause();
    } else {
        return Play();
    }
}

HRESULT fb2k_playback_control_impl::Previous() {
    std::cout << "[FB2K Playback] 上一曲" << std::endl;
    // 实现播放队列前移
    return S_OK;
}

HRESULT fb2k_playback_control_impl::Next() {
    std::cout << "[FB2K Playback] 下一曲" << std::endl;
    // 实现播放队列后移
    return S_OK;
}

HRESULT fb2k_playback_control_impl::Random() {
    std::cout << "[FB2K Playback] 随机播放" << std::endl;
    // 实现随机播放
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetPlaybackPosition(double* position_seconds) {
    if (!position_seconds) return E_POINTER;
    *position_seconds = playback_position_.load();
    return S_OK;
}

HRESULT fb2k_playback_control_impl::SetPlaybackPosition(double position_seconds) {
    if (!validate_playback_position(position_seconds)) return E_INVALIDARG;
    
    playback_position_ = position_seconds;
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetPlaybackLength(double* length_seconds) {
    if (!length_seconds) return E_POINTER;
    *length_seconds = playback_length_.load();
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetPlaybackPercentage(double* percentage) {
    if (!percentage) return E_POINTER;
    
    double length = playback_length_.load();
    if (length <= 0.0) {
        *percentage = 0.0;
    } else {
        *percentage = (playback_position_.load() / length) * 100.0;
    }
    
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetVolume(float* volume) {
    if (!volume) return E_POINTER;
    *volume = volume_.load();
    return S_OK;
}

HRESULT fb2k_playback_control_impl::SetVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    std::cout << "[FB2K Playback] 音量设置为: " << volume_.load() << std::endl;
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetMute(bool* mute) {
    if (!mute) return E_POINTER;
    *mute = mute_.load();
    return S_OK;
}

HRESULT fb2k_playback_control_impl::SetMute(bool mute) {
    mute_ = mute;
    std::cout << "[FB2K Playback] 静音设置为: " << (mute ? "开启" : "关闭") << std::endl;
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetQueueContents(const char*** items, DWORD* count) {
    if (!items || !count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    // 这里应该返回实际的队列内容
    *items = nullptr;
    *count = 0;
    
    return S_OK;
}

HRESULT fb2k_playback_control_impl::AddToQueue(const char* item_path) {
    if (!item_path) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    play_queue_.push_back(item_path);
    
    std::cout << "[FB2K Playback] 添加到队列: " << item_path << std::endl;
    return S_OK;
}

HRESULT fb2k_playback_control_impl::RemoveFromQueue(DWORD index) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (index >= play_queue_.size()) return E_INVALIDARG;
    
    play_queue_.erase(play_queue_.begin() + index);
    return S_OK;
}

HRESULT fb2k_playback_control_impl::ClearQueue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    play_queue_.clear();
    return S_OK;
}

HRESULT fb2k_playback_control_impl::GetPlaybackStatistics(DWORD* total_plays, DWORD* total_time, const char** last_played) {
    if (!total_plays || !total_time || !last_played) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    *total_plays = total_play_count_;
    *total_time = total_play_time_;
    *last_played = last_played_item_.c_str();
    
    return S_OK;
}

HRESULT fb2k_playback_control_impl::ResetPlaybackStatistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    total_play_count_ = 0;
    total_play_time_ = 0;
    last_played_item_.clear();
    return S_OK;
}

bool fb2k_playback_control_impl::validate_playback_position(double position) {
    return position >= 0.0 && position <= playback_length_.load();
}

// 核心服务注册
static struct fb2k_core_service_registrar {
    fb2k_core_service_registrar() {
        initialize_fb2k_core_services();
    }
    ~fb2k_core_service_registrar() {
        shutdown_fb2k_core_services();
    }
} fb2k_core_service_registrar_instance;

} // namespace fb2k