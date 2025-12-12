#pragma once

// 闃舵1.1锛氱湡瀹炵粍浠舵ˉ鎺ュ櫒
// 妗ユ帴鐪熷疄fb2k缁勪欢鍜屼富鏈虹幆澧?

#include "real_minihost.h"
#include <map>
#include <functional>

namespace fb2k {

// 鏈嶅姟妗ユ帴鍩虹被
template<typename Interface>
class ServiceBridge : public Interface {
protected:
    void* m_realInterface;
    bool m_ownInterface;
    
public:
    ServiceBridge(void* realInterface, bool ownInterface = false) 
        : m_realInterface(realInterface), m_ownInterface(ownInterface) {}
    
    virtual ~ServiceBridge() {
        if(m_ownInterface && m_realInterface) {
            // 閲婃斁鐪熷疄鎺ュ彛
            if(auto* unknown = static_cast<IUnknown*>(m_realInterface)) {
                unknown->Release();
            }
        }
    }
    
    void* GetRealInterface() const { return m_realInterface; }
    
    // 杈呭姪鍑芥暟锛氳幏鍙栫湡瀹炴帴鍙ｇ殑鏂规硶鎸囬拡
    template<typename FuncPtr>
    FuncPtr GetMethod(int index) {
        if(!m_realInterface) return nullptr;
        
        // 鍋囪鏄爣鍑嗙殑COM鎺ュ彛甯冨眬
        void** vtable = *(void***)m_realInterface;
        return reinterpret_cast<FuncPtr>(vtable[index]);
    }
};

// 鐪熷疄鐨勬枃浠朵俊鎭ˉ鎺ュ櫒
class RealFileInfoBridge : public ServiceBridge<FileInfo> {
public:
    RealFileInfoBridge(void* realFileInfo) : ServiceBridge(realFileInfo, false) {}
    
    // FileInfo鎺ュ彛瀹炵幇 - 妗ユ帴鍒扮湡瀹炲疄鐜?
    void reset() override {
        auto func = GetMethod<void(*)(void*)>(3); // 鍋囪reset鏄3涓柟娉?
        if(func) func(m_realInterface);
    }
    
    const char* meta_get(const char* name, size_t index) const override {
        auto func = GetMethod<const char*(*)(void*, const char*, size_t)>(4);
        return func ? func(m_realInterface, name, index) : nullptr;
    }
    
    size_t meta_get_count(const char* name) const override {
        auto func = GetMethod<size_t(*)(void*, const char*)>(5);
        return func ? func(m_realInterface, name) : 0;
    }
    
    void meta_set(const char* name, const char* value) override {
        auto func = GetMethod<void(*)(void*, const char*, const char*)>(6);
        if(func) func(m_realInterface, name, value);
    }
    
    double get_length() const override {
        auto func = GetMethod<double(*)(void*)>(7);
        return func ? func(m_realInterface) : 0.0;
    }
    
    void set_length(double length) override {
        auto func = GetMethod<void(*)(void*, double)>(8);
        if(func) func(m_realInterface, length);
    }
    
    const audio_info& get_audio_info() const override {
        static audio_info dummy;
        auto func = GetMethod<audio_info*(*)(void*)>(9);
        return func ? *func(m_realInterface) : dummy;
    }
    
    void set_audio_info(const audio_info& info) override {
        auto func = GetMethod<void(*)(void*, const audio_info*)>(10);
        if(func) func(m_realInterface, &info);
    }
    
    const file_stats& get_file_stats() const override {
        static file_stats dummy;
        auto func = GetMethod<file_stats*(*)(void*)>(11);
        return func ? *func(m_realInterface) : dummy;
    }
    
    void set_file_stats(const file_stats& stats) override {
        auto func = GetMethod<void(*)(void*, const file_stats*)>(12);
        if(func) func(m_realInterface, &stats);
    }
};

// 鐪熷疄鐨勪腑姝㈠洖璋冩ˉ鎺ュ櫒
class RealAbortCallbackBridge : public ServiceBridge<AbortCallback> {
public:
    RealAbortCallbackBridge(void* realAbortCallback) : ServiceBridge(realAbortCallback, false) {}
    
    bool is_aborting() const override {
        auto func = GetMethod<bool(*)(void*)>(3); // 鍋囪is_aborting鏄3涓柟娉?
        return func ? func(m_realInterface) : false;
    }
};

// 鐪熷疄鐨勮緭鍏ヨВ鐮佸櫒妗ユ帴鍣?
class RealInputDecoderBridge : public ServiceBridge<InputDecoder> {
public:
    RealInputDecoderBridge(void* realDecoder) : ServiceBridge(realDecoder, false) {}
    
    // InputDecoder鎺ュ彛瀹炵幇
    bool open(const char* path, FileInfo& info, AbortCallback& abort) override {
        auto func = GetMethod<bool(*)(void*, const char*, void*, void*)>(3);
        return func ? func(m_realInterface, path, &info, &abort) : false;
    }
    
    int decode(float* buffer, int samples, AbortCallback& abort) override {
        auto func = GetMethod<int(*)(void*, float*, int, void*)>(4);
        return func ? func(m_realInterface, buffer, samples, &abort) : 0;
    }
    
    void seek(double seconds, AbortCallback& abort) override {
        auto func = GetMethod<void(*)(void*, double, void*)>(5);
        if(func) func(m_realInterface, seconds, &abort);
    }
    
    bool can_seek() override {
        auto func = GetMethod<bool(*)(void*)>(6);
        return func ? func(m_realInterface) : false;
    }
    
    void close() override {
        auto func = GetMethod<void(*)(void*)>(7);
        if(func) func(m_realInterface);
    }
    
    bool is_our_path(const char* path) override {
        auto func = GetMethod<bool(*)(void*, const char*)>(8);
        return func ? func(m_realInterface, path) : false;
    }
    
    const char* get_name() override {
        auto func = GetMethod<const char*(*)(void*)>(9);
        return func ? func(m_realInterface) : "Unknown Real Decoder";
    }
};

// 缁勪欢閫傞厤鍣ㄥ熀绫?
class ComponentAdapter {
protected:
    HMODULE m_module;
    std::string m_name;
    bool m_isValid;
    
public:
    ComponentAdapter(HMODULE module, const std::string& name) 
        : m_module(module), m_name(name), m_isValid(false) {}
    
    virtual ~ComponentAdapter() {
        if(m_module) {
            FreeLibrary(m_module);
        }
    }
    
    bool IsValid() const { return m_isValid; }
    const std::string& GetName() const { return m_name; }
    
    virtual bool Initialize() = 0;
    virtual service_ptr_t<ServiceBase> CreateService(REFGUID guid) = 0;
};

// 杈撳叆瑙ｇ爜鍣ㄩ€傞厤鍣?
class InputDecoderAdapter : public ComponentAdapter {
private:
    typedef HRESULT (*GetServiceFunc)(REFGUID, void**);
    GetServiceFunc m_getService;
    
public:
    InputDecoderAdapter(HMODULE module, const std::string& name) 
        : ComponentAdapter(module, name) {
        
        // 鑾峰彇鏈嶅姟鍏ュ彛鐐?
        m_getService = (GetServiceFunc)GetProcAddress(module, "fb2k_get_service");
        if(!m_getService) {
            m_getService = (GetServiceFunc)GetProcAddress(module, "get_service");
        }
        if(!m_getService) {
            m_getService = (GetServiceFunc)GetProcAddress(module, "_fb2k_get_service@8");
        }
        
        m_isValid = (m_getService != nullptr);
    }
    
    bool Initialize() override {
        if(!m_isValid) {
            return false;
        }
        
        // 杩欓噷鍙互娣诲姞缁勪欢鐗瑰畾鐨勫垵濮嬪寲浠ｇ爜
        return true;
    }
    
    service_ptr_t<ServiceBase> CreateService(REFGUID guid) override {
        if(!m_isValid) {
            return service_ptr_t<ServiceBase>();
        }
        
        void* service_ptr = nullptr;
        HRESULT hr = m_getService(guid, &service_ptr);
        
        if(FAILED(hr) || !service_ptr) {
            return service_ptr_t<ServiceBase>();
        }
        
        // 鏍规嵁GUID鍒涘缓鐩稿簲鐨勬ˉ鎺ュ櫒
        if(IsEqualGUID(guid, IID_InputDecoder)) {
            return service_ptr_t<ServiceBase>(new RealInputDecoderBridge(service_ptr));
        } else if(IsEqualGUID(guid, IID_FileInfo)) {
            return service_ptr_t<ServiceBase>(new RealFileInfoBridge(service_ptr));
        } else if(IsEqualGUID(guid, IID_AbortCallback)) {
            return service_ptr_t<ServiceBase>(new RealAbortCallbackBridge(service_ptr));
        }
        
        // 濡傛灉涓嶇煡閬撳浣曞鐞嗭紝杩斿洖鍘熷鎺ュ彛
        return service_ptr_t<ServiceBase>(static_cast<ServiceBase*>(service_ptr));
    }
};

// 鏈嶅姟瀹氫綅鍣紙澧炲己鐗堬級
class EnhancedServiceLocator {
private:
    std::map<GUID, std::unique_ptr<ComponentAdapter>, GUID_hash> m_adapters;
    std::map<GUID, std::function<service_ptr_t<ServiceBase>()>, GUID_hash> m_factories;
    
public:
    bool RegisterComponent(const GUID& guid, std::unique_ptr<ComponentAdapter> adapter) {
        m_adapters[guid] = std::move(adapter);
        return true;
    }
    
    bool RegisterFactory(const GUID& guid, std::function<service_ptr_t<ServiceBase>()> factory) {
        m_factories[guid] = factory;
        return true;
    }
    
    service_ptr_t<ServiceBase> GetService(REFGUID guid) {
        // 棣栧厛灏濊瘯缁勪欢閫傞厤鍣?
        auto it = m_adapters.find(guid);
        if(it != m_adapters.end()) {
            return it->second->CreateService(guid);
        }
        
        // 鐒跺悗灏濊瘯宸ュ巶
        auto factory_it = m_factories.find(guid);
        if(factory_it != m_factories.end()) {
            return factory_it->second();
        }
        
        return service_ptr_t<ServiceBase>();
    }
    
    std::vector<GUID> GetRegisteredServices() const {
        std::vector<GUID> services;
        for(const auto& pair : m_adapters) {
            services.push_back(pair.first);
        }
        for(const auto& pair : m_factories) {
            services.push_back(pair.first);
        }
        return services;
    }
};

// 缁勪欢鍙戠幇鍣?
class ComponentDiscoverer {
public:
    struct ComponentInfo {
        std::wstring path;
        std::string name;
        std::string type;
        bool is_valid;
        std::string error_message;
    };
    
    static std::vector<ComponentInfo> DiscoverComponents(const std::wstring& directory) {
        std::vector<ComponentInfo> components;
        
        if(!fs::exists(directory)) {
            return components;
        }
        
        try {
            for(const auto& entry : fs::directory_iterator(directory)) {
                if(entry.is_regular_file() && entry.path().extension() == L".dll") {
                    ComponentInfo info;
                    info.path = entry.path().wstring();
                    info.name = WideToUTF8(entry.path().filename().wstring());
                    info.type = IdentifyComponentType(info.name);
                    
                    // 楠岃瘉缁勪欢
                    auto validation_result = ValidateComponent(entry.path().wstring());
                    info.is_valid = validation_result.first;
                    info.error_message = validation_result.second;
                    
                    components.push_back(info);
                }
            }
        } catch(const std::exception& e) {
            // 璁板綍閿欒浣嗕笉涓柇
        }
        
        return components;
    }
    
private:
    static std::string WideToUTF8(const std::wstring& wide) {
        if(wide.empty()) return "";
        
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if(size <= 0) return "";
        
        std::string result(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
        return result;
    }
    
    static std::string IdentifyComponentType(const std::string& filename) {
        std::string lower = filename;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if(lower.find("input") != std::string::npos || 
           lower.find("decoder") != std::string::npos) {
            return "input_decoder";
        } else if(lower.find("dsp") != std::string::npos) {
            return "dsp";
        } else if(lower.find("output") != std::string::npos) {
            return "output";
        }
        
        return "unknown";
    }
    
    static std::pair<bool, std::string> ValidateComponent(const std::wstring& path) {
        HMODULE module = LoadLibraryW(path.c_str());
        if(!module) {
            DWORD error = GetLastError();
            return {false, "鏃犳硶鍔犺浇DLL: " + std::to_string(error)};
        }
        
        // 妫€鏌ュ叧閿鍑哄嚱鏁?
        bool has_service_export = false;
        
        const char* service_exports[] = {
            "fb2k_get_service",
            "get_service", 
            "_fb2k_get_service@8",
            "DllGetClassObject"
        };
        
        for(const char* export_name : service_exports) {
            if(GetProcAddress(module, export_name)) {
                has_service_export = true;
                break;
            }
        }
        
        FreeLibrary(module);
        
        if(!has_service_export) {
            return {false, "鏈壘鍒版湇鍔″鍑哄嚱鏁?};
        }
        
        return {true, ""};
    }
};

// 澧炲己鐗堜富鏈?
class EnhancedMiniHost : public RealMiniHost {
private:
    EnhancedServiceLocator m_serviceLocator;
    std::map<std::string, std::unique_ptr<ComponentAdapter>> m_componentAdapters;
    
public:
    bool LoadComponentEnhanced(const std::wstring& dll_path) {
        FB2KLogger::Info("澧炲己鍔犺浇缁勪欢: %s", WideToUTF8(dll_path).c_str());
        
        // 鍩虹鍔犺浇
        if(!LoadComponent(dll_path)) {
            return false;
        }
        
        // 鑾峰彇鍒氬姞杞界殑妯″潡
        HMODULE module = m_modules.back();
        std::string name = WideToUTF8(fs::path(dll_path).filename().wstring());
        
        // 鍒涘缓閫傚綋鐨勯€傞厤鍣?
        std::unique_ptr<ComponentAdapter> adapter;
        
        if(name.find("input") != std::string::npos || name.find("decoder") != std::string::npos) {
            adapter = std::make_unique<InputDecoderAdapter>(module, name);
        }
        // 鍙互娣诲姞鏇村閫傞厤鍣ㄧ被鍨?..
        
        if(adapter && adapter->Initialize()) {
            // 娉ㄥ唽閫傞厤鍣ㄥ鐞嗙殑鏈嶅姟
            m_serviceLocator.RegisterComponent(CLSID_InputDecoderService, std::move(adapter));
            
            FB2KLogger::Info("缁勪欢澧炲己閫傞厤瀹屾垚: %s", name.c_str());
            return true;
        }
        
        FB2KLogger::Warning("缁勪欢澧炲己閫傞厤澶辫触: %s", name.c_str());
        return false;
    }
    
    service_ptr_t<ServiceBase> GetEnhancedService(REFGUID guid) {
        return m_serviceLocator.GetService(guid);
    }
    
    // 鑷姩鍙戠幇鍜屽姞杞界粍浠?
    bool AutoDiscoverAndLoadComponents(const std::wstring& directory) {
        FB2KLogger::Info("鑷姩鍙戠幇缁勪欢: %s", WideToUTF8(directory).c_str());
        
        auto components = ComponentDiscoverer::DiscoverComponents(directory);
        
        FB2KLogger::Info("鍙戠幇 %zu 涓綔鍦ㄧ粍浠?, components.size());
        
        int loaded = 0;
        int failed = 0;
        
        for(const auto& comp : components) {
            if(comp.is_valid) {
                if(LoadComponentEnhanced(comp.path)) {
                    loaded++;
                } else {
                    failed++;
                }
            } else {
                FB2KLogger::Warning("璺宠繃鏃犳晥缁勪欢: %s - %s", comp.name.c_str(), comp.error_message.c_str());
                failed++;
            }
        }
        
        FB2KLogger::Info("鑷姩鍔犺浇瀹屾垚: %d 鎴愬姛, %d 澶辫触", loaded, failed);
        return loaded > 0;
    }
};

} // namespace fb2k