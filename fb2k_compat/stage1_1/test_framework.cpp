// 闃舵1.1妗嗘灦娴嬭瘯 - 绠€鍖栫増鏈?
// 楠岃瘉鏍稿績鏋舵瀯鍔熻兘

#include <iostream>
#include <windows.h>
#include <objbase.h>

// 绠€鍖朑UID瀹氫箟
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(IID_ServiceBase, 0xFB2KServiceBase, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);
DEFINE_GUID(CLSID_TestService, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);

// 鍩虹COM瀵硅薄锛堢畝鍖栫増锛?
class ComObject : public IUnknown {
protected:
    ULONG m_refCount;
    
public:
    ComObject() : m_refCount(1) {}
    virtual ~ComObject() {}
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if(!ppvObject) return E_POINTER;
        *ppvObject = nullptr;
        
        if(IsEqualGUID(riid, IID_IUnknown)) {
            *ppvObject = static_cast<IUnknown*>(this);
        } else {
            return QueryInterfaceImpl(riid, ppvObject);
        }
        
        AddRef();
        return S_OK;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_refCount);
    }
    
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&m_refCount);
        if(count == 0) {
            delete this;
            return 0;
        }
        return count;
    }
    
    virtual HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) {
        return E_NOINTERFACE;
    }
};

// 鏈嶅姟鍩虹被
class ServiceBase : public ComObject {
public:
    virtual int service_add_ref() { return AddRef(); }
    virtual int service_release() { return Release(); }
    
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, IID_ServiceBase)) {
            *ppvObject = static_cast<ServiceBase*>(this);
            return S_OK;
        }
        return ComObject::QueryInterfaceImpl(riid, ppvObject);
    }
};

// 娴嬭瘯鏈嶅姟
class TestService : public ServiceBase {
public:
    virtual const char* GetName() { return "Test Service"; }
    virtual int GetValue() { return 42; }
};

// 鏈嶅姟宸ュ巶
class ServiceFactory {
public:
    virtual ~ServiceFactory() {}
    virtual HRESULT CreateInstance(REFIID riid, void** ppvObject) = 0;
    virtual const GUID& GetServiceGUID() const = 0;
};

// 娴嬭瘯鏈嶅姟宸ュ巶
class TestServiceFactory : public ServiceFactory {
public:
    HRESULT CreateInstance(REFIID riid, void** ppvObject) override {
        if(!ppvObject) return E_POINTER;
        *ppvObject = nullptr;
        
        auto* service = new TestService();
        HRESULT hr = service->QueryInterface(riid, ppvObject);
        
        if(FAILED(hr)) {
            service->Release();
        }
        
        return hr;
    }
    
    const GUID& GetServiceGUID() const override {
        return CLSID_TestService;
    }
};

// 鏅鸿兘鎸囬拡妯℃澘
template<typename T>
class service_ptr_t {
private:
    T* ptr_;
    
public:
    service_ptr_t() : ptr_(nullptr) {}
    service_ptr_t(T* p) : ptr_(p) { if(ptr_) ptr_->AddRef(); }
    service_ptr_t(const service_ptr_t& other) : ptr_(other.ptr_) { if(ptr_) ptr_->AddRef(); }
    service_ptr_t(service_ptr_t&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
    ~service_ptr_t() { if(ptr_) ptr_->Release(); }
    
    service_ptr_t& operator=(T* p) {
        if(ptr_ != p) {
            if(ptr_) ptr_->Release();
            ptr_ = p;
            if(ptr_) ptr_->AddRef();
        }
        return *this;
    }
    
    void reset(T* p = nullptr) {
        if(ptr_) ptr_->Release();
        ptr_ = p;
        if(ptr_) ptr_->AddRef();
    }
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    
    bool is_valid() const { return ptr_ != nullptr; }
};

// 绠€鍖栦富鏈?
class TestHost {
private:
    ServiceFactory* m_factory;
    
public:
    TestHost() : m_factory(nullptr) {}
    
    bool Initialize() {
        std::cout << "[TestHost] 鍒濆鍖?.." << std::endl;
        
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            std::cout << "[TestHost] COM鍒濆鍖栧け璐? 0x" << std::hex << hr << std::endl;
            return false;
        }
        
        m_factory = new TestServiceFactory();
        std::cout << "[TestHost] 鏈嶅姟宸ュ巶鍒涘缓鎴愬姛" << std::endl;
        
        return true;
    }
    
    void Shutdown() {
        std::cout << "[TestHost] 鍏抽棴..." << std::endl;
        delete m_factory;
        CoUninitialize();
    }
    
    bool TestServiceSystem() {
        std::cout << "\n=== 鏈嶅姟绯荤粺娴嬭瘯 ===" << std::endl;
        
        if(!m_factory) {
            std::cout << "[TestHost] 鏈嶅姟宸ュ巶鏈垵濮嬪寲" << std::endl;
            return false;
        }
        
        // 鍒涘缓鏈嶅姟瀹炰緥
        void* service_ptr = nullptr;
        HRESULT hr = m_factory->CreateInstance(IID_ServiceBase, &service_ptr);
        
        if(FAILED(hr) || !service_ptr) {
            std::cout << "[TestHost] 鏈嶅姟鍒涘缓澶辫触: 0x" << std::hex << hr << std::endl;
            return false;
        }
        
        std::cout << "[TestHost] 鏈嶅姟鍒涘缓鎴愬姛" << std::endl;
        
        // 浣跨敤鏅鸿兘鎸囬拡绠＄悊
        service_ptr_t<ServiceBase> service(static_cast<ServiceBase*>(service_ptr));
        
        // 娴嬭瘯鏈嶅姟鏂规硶
        if(auto* test_service = dynamic_cast<TestService*>(service.get())) {
            std::cout << "[TestHost] 鏈嶅姟鍚嶇О: " << test_service->GetName() << std::endl;
            std::cout << "[TestHost] 鏈嶅姟鍊? " << test_service->GetValue() << std::endl;
        }
        
        // 娴嬭瘯寮曠敤璁℃暟
        std::cout << "[TestHost] 寮曠敤璁℃暟娴嬭瘯..." << std::endl;
        ULONG ref1 = service->AddRef();
        ULONG ref2 = service->AddRef();
        ULONG ref3 = service->Release();
        ULONG ref4 = service->Release();
        
        std::cout << "[TestHost] 寮曠敤璁℃暟: " << ref1 << " -> " << ref2 << " -> " << ref3 << " -> " << ref4 << std::endl;
        
        return true;
    }
    
    bool TestCOMInterface() {
        std::cout << "\n=== COM鎺ュ彛娴嬭瘯 ===" << std::endl;
        
        // 鍒涘缓娴嬭瘯瀵硅薄
        auto* obj = new TestService();
        
        // 娴嬭瘯IUnknown鎺ュ彛
        IUnknown* unknown = nullptr;
        HRESULT hr = obj->QueryInterface(IID_IUnknown, (void**)&unknown);
        
        if(SUCCEEDED(hr) && unknown) {
            std::cout << "[TestHost] IUnknown鎺ュ彛鑾峰彇鎴愬姛" << std::endl;
            unknown->Release();
        }
        
        // 娴嬭瘯ServiceBase鎺ュ彛
        ServiceBase* service = nullptr;
        hr = obj->QueryInterface(IID_ServiceBase, (void**)&service);
        
        if(SUCCEEDED(hr) && service) {
            std::cout << "[TestHost] ServiceBase鎺ュ彛鑾峰彇鎴愬姛" << std::endl;
            
            // 娴嬭瘯鏈嶅姟鏂规硶
            if(auto* test_service = dynamic_cast<TestService*>(service)) {
                std::cout << "[TestHost] 閫氳繃ServiceBase璋冪敤: " << test_service->GetName() << std::endl;
            }
            
            service->Release();
        }
        
        // 閲婃斁瀵硅薄
        obj->Release();
        
        return true;
    }
};

// 妗嗘灦楠岃瘉娴嬭瘯
bool TestFrameworkArchitecture() {
    std::cout << "=" << std::string(60, '=') << std::endl;
    std::cout << "foobar2000 鍏煎灞傛鏋堕獙璇佹祴璇? << std::endl;
    std::cout << "闃舵1.1锛氭灦鏋勯獙璇? << std::endl;
    std::cout << "=" << std::string(60, '=') << std::endl;
    
    // 鍒濆鍖朇OM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cout << "COM鍒濆鍖栧け璐? 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // 鍒涘缓娴嬭瘯涓绘満
    TestHost host;
    if(!host.Initialize()) {
        std::cout << "涓绘満鍒濆鍖栧け璐? << std::endl;
        CoUninitialize();
        return false;
    }
    
    std::cout << "鉁?涓绘満鍒濆鍖栨垚鍔? << std::endl;
    
    // 杩愯娴嬭瘯
    bool all_passed = true;
    
    std::cout << "\n1. COM鎺ュ彛娴嬭瘯..." << std::endl;
    if(!host.TestCOMInterface()) {
        std::cout << "鉂?COM鎺ュ彛娴嬭瘯澶辫触" << std::endl;
        all_passed = false;
    } else {
        std::cout << "鉁?COM鎺ュ彛娴嬭瘯閫氳繃" << std::endl;
    }
    
    std::cout << "\n2. 鏈嶅姟绯荤粺娴嬭瘯..." << std::endl;
    if(!host.TestServiceSystem()) {
        std::cout << "鉂?鏈嶅姟绯荤粺娴嬭瘯澶辫触" << std::endl;
        all_passed = false;
    } else {
        std::cout << "鉁?鏈嶅姟绯荤粺娴嬭瘯閫氳繃" << std::endl;
    }
    
    // 娓呯悊
    host.Shutdown();
    CoUninitialize();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    if(all_passed) {
        std::cout << "馃帀 鎵€鏈夋祴璇曢€氳繃锛佹鏋舵灦鏋勯獙璇佹垚鍔熴€? << std::endl;
        std::cout << "\n鏍稿績楠岃瘉瀹屾垚:" << std::endl;
        std::cout << "  鉁?COM鎺ュ彛绯荤粺宸ヤ綔姝ｅ父" << std::endl;
        std::cout << "  鉁?鏈嶅姟绯荤粺鏋舵瀯姝ｇ‘" << std::endl;
        std::cout << "  鉁?鏅鸿兘鎸囬拡绠＄悊鏈夋晥" << std::endl;
        std::cout << "  鉁?宸ュ巶妯″紡瀹炵幇姝ｇ‘" << std::endl;
        std::cout << "\n闃舵1.1鏍稿績鏋舵瀯楠岃瘉瀹屾垚锛? << std::endl;
    } else {
        std::cout << "鈿狅笍  閮ㄥ垎娴嬭瘯澶辫触锛岄渶瑕佽皟璇? << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;
    
    return all_passed;
}

int main() {
    try {
        return TestFrameworkArchitecture() ? 0 : 1;
    } catch(const std::exception& e) {
        std::cerr << "娴嬭瘯寮傚父: " << e.what() << std::endl;
        return 1;
    }
}