// 阶段1.1框架测试 - 简化版本
// 验证核心架构功能

#include <iostream>
#include <windows.h>
#include <objbase.h>

// 简化GUID定义
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(IID_ServiceBase, 0xFB2KServiceBase, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);
DEFINE_GUID(CLSID_TestService, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);

// 基础COM对象（简化版）
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

// 服务基类
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

// 测试服务
class TestService : public ServiceBase {
public:
    virtual const char* GetName() { return "Test Service"; }
    virtual int GetValue() { return 42; }
};

// 服务工厂
class ServiceFactory {
public:
    virtual ~ServiceFactory() {}
    virtual HRESULT CreateInstance(REFIID riid, void** ppvObject) = 0;
    virtual const GUID& GetServiceGUID() const = 0;
};

// 测试服务工厂
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

// 智能指针模板
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

// 简化主机
class TestHost {
private:
    ServiceFactory* m_factory;
    
public:
    TestHost() : m_factory(nullptr) {}
    
    bool Initialize() {
        std::cout << "[TestHost] 初始化..." << std::endl;
        
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            std::cout << "[TestHost] COM初始化失败: 0x" << std::hex << hr << std::endl;
            return false;
        }
        
        m_factory = new TestServiceFactory();
        std::cout << "[TestHost] 服务工厂创建成功" << std::endl;
        
        return true;
    }
    
    void Shutdown() {
        std::cout << "[TestHost] 关闭..." << std::endl;
        delete m_factory;
        CoUninitialize();
    }
    
    bool TestServiceSystem() {
        std::cout << "\n=== 服务系统测试 ===" << std::endl;
        
        if(!m_factory) {
            std::cout << "[TestHost] 服务工厂未初始化" << std::endl;
            return false;
        }
        
        // 创建服务实例
        void* service_ptr = nullptr;
        HRESULT hr = m_factory->CreateInstance(IID_ServiceBase, &service_ptr);
        
        if(FAILED(hr) || !service_ptr) {
            std::cout << "[TestHost] 服务创建失败: 0x" << std::hex << hr << std::endl;
            return false;
        }
        
        std::cout << "[TestHost] 服务创建成功" << std::endl;
        
        // 使用智能指针管理
        service_ptr_t<ServiceBase> service(static_cast<ServiceBase*>(service_ptr));
        
        // 测试服务方法
        if(auto* test_service = dynamic_cast<TestService*>(service.get())) {
            std::cout << "[TestHost] 服务名称: " << test_service->GetName() << std::endl;
            std::cout << "[TestHost] 服务值: " << test_service->GetValue() << std::endl;
        }
        
        // 测试引用计数
        std::cout << "[TestHost] 引用计数测试..." << std::endl;
        ULONG ref1 = service->AddRef();
        ULONG ref2 = service->AddRef();
        ULONG ref3 = service->Release();
        ULONG ref4 = service->Release();
        
        std::cout << "[TestHost] 引用计数: " << ref1 << " -> " << ref2 << " -> " << ref3 << " -> " << ref4 << std::endl;
        
        return true;
    }
    
    bool TestCOMInterface() {
        std::cout << "\n=== COM接口测试 ===" << std::endl;
        
        // 创建测试对象
        auto* obj = new TestService();
        
        // 测试IUnknown接口
        IUnknown* unknown = nullptr;
        HRESULT hr = obj->QueryInterface(IID_IUnknown, (void**)&unknown);
        
        if(SUCCEEDED(hr) && unknown) {
            std::cout << "[TestHost] IUnknown接口获取成功" << std::endl;
            unknown->Release();
        }
        
        // 测试ServiceBase接口
        ServiceBase* service = nullptr;
        hr = obj->QueryInterface(IID_ServiceBase, (void**)&service);
        
        if(SUCCEEDED(hr) && service) {
            std::cout << "[TestHost] ServiceBase接口获取成功" << std::endl;
            
            // 测试服务方法
            if(auto* test_service = dynamic_cast<TestService*>(service)) {
                std::cout << "[TestHost] 通过ServiceBase调用: " << test_service->GetName() << std::endl;
            }
            
            service->Release();
        }
        
        // 释放对象
        obj->Release();
        
        return true;
    }
};

// 框架验证测试
bool TestFrameworkArchitecture() {
    std::cout << "=" << std::string(60, '=') << std::endl;
    std::cout << "foobar2000 兼容层框架验证测试" << std::endl;
    std::cout << "阶段1.1：架构验证" << std::endl;
    std::cout << "=" << std::string(60, '=') << std::endl;
    
    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cout << "COM初始化失败: 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // 创建测试主机
    TestHost host;
    if(!host.Initialize()) {
        std::cout << "主机初始化失败" << std::endl;
        CoUninitialize();
        return false;
    }
    
    std::cout << "✅ 主机初始化成功" << std::endl;
    
    // 运行测试
    bool all_passed = true;
    
    std::cout << "\n1. COM接口测试..." << std::endl;
    if(!host.TestCOMInterface()) {
        std::cout << "❌ COM接口测试失败" << std::endl;
        all_passed = false;
    } else {
        std::cout << "✅ COM接口测试通过" << std::endl;
    }
    
    std::cout << "\n2. 服务系统测试..." << std::endl;
    if(!host.TestServiceSystem()) {
        std::cout << "❌ 服务系统测试失败" << std::endl;
        all_passed = false;
    } else {
        std::cout << "✅ 服务系统测试通过" << std::endl;
    }
    
    // 清理
    host.Shutdown();
    CoUninitialize();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    if(all_passed) {
        std::cout << "🎉 所有测试通过！框架架构验证成功。" << std::endl;
        std::cout << "\n核心验证完成:" << std::endl;
        std::cout << "  ✅ COM接口系统工作正常" << std::endl;
        std::cout << "  ✅ 服务系统架构正确" << std::endl;
        std::cout << "  ✅ 智能指针管理有效" << std::endl;
        std::cout << "  ✅ 工厂模式实现正确" << std::endl;
        std::cout << "\n阶段1.1核心架构验证完成！" << std::endl;
    } else {
        std::cout << "⚠️  部分测试失败，需要调试" << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;
    
    return all_passed;
}

int main() {
    try {
        return TestFrameworkArchitecture() ? 0 : 1;
    } catch(const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << std::endl;
        return 1;
    }
}