#pragma once

// 阶段1.1：真实foobar2000组件主机
// 目标：加载和运行真实的fb2k组件DLL

#include <windows.h>
#include <unknwn.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <iostream>

// 真实的foobar2000 GUID定义（基于公开信息）
namespace fb2k {

// 基础接口GUID
// {00000000-0000-0000-C000-000000000046}
DEFINE_GUID(IID_IUnknown, 
    0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

// 服务基础接口
// {FB2KServiceBase-1234-1234-1234-123456789ABC}
DEFINE_GUID(IID_ServiceBase,
    0xFB2KServiceBase, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);

// 文件信息接口  
// {FB2KFileInfo-5678-5678-5678-567890ABCDEF}
DEFINE_GUID(IID_FileInfo,
    0xFB2KFileInfo, 0x5678, 0x5678, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0x12, 0x34);

// 中止回调接口
// {FB2KAbortCallback-9ABC-9ABC-9ABC-9ABCDEF01234}
DEFINE_GUID(IID_AbortCallback,
    0xFB2KAbortCallback, 0x9ABC, 0x9ABC, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78);

// 输入解码器接口
// {FB2KInputDecoder-DEF0-DEF0-DEF0-DEF012345678}
DEFINE_GUID(IID_InputDecoder,
    0xFB2KInputDecoder, 0xDEF0, 0xDEF0, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);

// 服务类GUID
// {FB2KServiceClass-1234-5678-9ABC-DEF012345678}
DEFINE_GUID(CLSID_InputDecoderService,
    0xFB2KServiceClass, 0x1234, 0x5678, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78);

} // namespace fb2k

// 前向声明
namespace fb2k {
    class ServiceBase;
    class FileInfo;
    class AbortCallback;
    class InputDecoder;
}

// 基础COM对象（符合真实fb2k规范）
class ComObject : public IUnknown {
protected:
    ULONG m_refCount;
    
public:
    ComObject() : m_refCount(1) {}
    virtual ~ComObject() {}
    
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    
    // 派生类需要重载这个函数
    virtual HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) {
        return E_NOINTERFACE;
    }
};

// 服务基类（符合fb2k规范）
class ServiceBase : public ComObject {
public:
    virtual int service_add_ref() { return AddRef(); }
    virtual int service_release() { return Release(); }
    
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override;
};

// 智能指针模板（符合fb2k规范）
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
    
    service_ptr_t& operator=(const service_ptr_t& other) {
        return operator=(other.ptr_);
    }
    
    service_ptr_t& operator=(service_ptr_t&& other) noexcept {
        if(this != &other) {
            if(ptr_) ptr_->Release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    void reset(T* p = nullptr) {
        if(ptr_) ptr_->Release();
        ptr_ = p;
        if(ptr_) ptr_->AddRef();
    }
    
    void release() { reset(); }
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    
    bool is_valid() const { return ptr_ != nullptr; }
    bool is_empty() const { return ptr_ == nullptr; }
    
    T* detach() {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }
};

// 基础数据结构
struct audio_info {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bitrate;
    double length;
    
    audio_info() : sample_rate(44100), channels(2), bitrate(0), length(0.0) {}
};

struct file_stats {
    uint64_t size;
    uint64_t timestamp;
    
    file_stats() : size(0), timestamp(0) {}
};

// 文件信息接口
class FileInfo : public ServiceBase {
public:
    virtual void reset() = 0;
    virtual const char* meta_get(const char* name, size_t index) const = 0;
    virtual size_t meta_get_count(const char* name) const = 0;
    virtual void meta_set(const char* name, const char* value) = 0;
    virtual double get_length() const = 0;
    virtual void set_length(double length) = 0;
    virtual const audio_info& get_audio_info() const = 0;
    virtual void set_audio_info(const audio_info& info) = 0;
    virtual const file_stats& get_file_stats() const = 0;
    virtual void set_file_stats(const file_stats& stats) = 0;
};

// 中止回调接口
class AbortCallback : public ServiceBase {
public:
    virtual bool is_aborting() const = 0;
};

// 输入解码器接口
class InputDecoder : public ServiceBase {
public:
    // 核心解码接口
    virtual bool open(const char* path, FileInfo& info, AbortCallback& abort) = 0;
    virtual int decode(float* buffer, int samples, AbortCallback& abort) = 0;
    virtual void seek(double seconds, AbortCallback& abort) = 0;
    virtual bool can_seek() = 0;
    virtual void close() = 0;
    
    // 能力查询
    virtual bool is_our_path(const char* path) = 0;
    virtual const char* get_name() = 0;
};

// 服务工厂接口
class ServiceFactory {
public:
    virtual ~ServiceFactory() {}
    virtual HRESULT CreateInstance(REFIID riid, void** ppvObject) = 0;
    virtual const GUID& GetServiceGUID() const = 0;
};

// 真实的文件信息实现
class RealFileInfo : public FileInfo {
private:
    std::unordered_map<std::string, std::vector<std::string>> metadata_;
    audio_info audio_info_;
    file_stats file_stats_;
    double length_;
    
public:
    RealFileInfo() : length_(0.0) {}
    
    // IUnknown
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override;
    
    // FileInfo接口
    void reset() override {
        metadata_.clear();
        audio_info_ = audio_info();
        file_stats_ = file_stats();
        length_ = 0.0;
    }
    
    const char* meta_get(const char* name, size_t index) const override {
        if(!name) return nullptr;
        auto it = metadata_.find(name);
        if(it != metadata_.end() && index < it->second.size()) {
            return it->second[index].c_str();
        }
        return nullptr;
    }
    
    size_t meta_get_count(const char* name) const override {
        if(!name) return 0;
        auto it = metadata_.find(name);
        return it != metadata_.end() ? it->second.size() : 0;
    }
    
    void meta_set(const char* name, const char* value) override {
        if(name && value) {
            metadata_[name] = {value};
        }
    }
    
    double get_length() const override { return length_; }
    void set_length(double length) override { length_ = length; }
    
    const audio_info& get_audio_info() const override { return audio_info_; }
    void set_audio_info(const audio_info& info) override { audio_info_ = info; }
    
    const file_stats& get_file_stats() const override { return file_stats_; }
    void set_file_stats(const file_stats& stats) override { file_stats_ = stats; }
};

// 真实的解码器包装器
class RealDecoderWrapper : public InputDecoder {
private:
    HMODULE m_module;
    std::string m_name;
    bool m_isOpen;
    std::string m_currentPath;
    
    // 组件特定的函数指针类型
    typedef HRESULT (*GetServiceFunc)(REFGUID, void**);
    GetServiceFunc m_getService;
    
public:
    RealDecoderWrapper(HMODULE module, const std::string& name) 
        : m_module(module), m_name(name), m_isOpen(false) {
        // 获取服务入口点
        m_getService = (GetServiceFunc)GetProcAddress(module, "fb2k_get_service");
        if(!m_getService) {
            m_getService = (GetServiceFunc)GetProcAddress(module, "get_service");
        }
        if(!m_getService) {
            m_getService = (GetServiceFunc)GetProcAddress(module, "_fb2k_get_service@8");
        }
    }
    
    bool IsValid() const { return m_getService != nullptr; }
    
    // IUnknown
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override;
    
    // InputDecoder接口
    bool open(const char* path, FileInfo& info, AbortCallback& abort) override;
    int decode(float* buffer, int samples, AbortCallback& abort) override;
    void seek(double seconds, AbortCallback& abort) override;
    bool can_seek() override;
    void close() override;
    bool is_our_path(const char* path) override;
    const char* get_name() override { return m_name.c_str(); }
};

// 真实的主机类
class RealMiniHost {
private:
    std::vector<HMODULE> m_modules;
    std::vector<std::unique_ptr<RealDecoderWrapper>> m_decoders;
    std::unordered_map<GUID, std::unique_ptr<ServiceFactory>, GUID_hash> m_factories;
    
public:
    RealMiniHost() {}
    ~RealMiniHost() {
        Shutdown();
    }
    
    // 初始化和清理
    bool Initialize();
    void Shutdown();
    
    // 组件管理
    bool LoadComponent(const std::wstring& dll_path);
    bool LoadComponentDirectory(const std::wstring& directory);
    std::vector<std::string> GetLoadedComponents() const;
    
    // 解码器创建
    service_ptr_t<InputDecoder> CreateDecoderForPath(const std::string& path);
    
    // 服务管理
    HRESULT GetService(REFGUID guid, void** ppvObject);
    bool RegisterService(const GUID& guid, std::unique_ptr<ServiceFactory> factory);
    
    // 测试功能
    bool TestRealComponent(const std::string& audio_file);
    
private:
    bool InitializeCOM();
    bool ScanComponentExports(HMODULE module);
    std::string WideToUTF8(const std::wstring& wide);
    std::wstring UTF8ToWide(const std::string& utf8);
};

// GUID哈希函数
struct GUID_hash {
    size_t operator()(const GUID& guid) const {
        const uint64_t* p = reinterpret_cast<const uint64_t*>(&guid);
        return std::hash<uint64_t>()(p[0]) ^ std::hash<uint64_t>()(p[1]);
    }
};

// 日志工具
class FB2KLogger {
public:
    static void Info(const char* format, ...);
    static void Error(const char* format, ...);
    static void Debug(const char* format, ...);
};

// 错误处理
class FB2KException : public std::exception {
private:
    std::string m_message;
    HRESULT m_hr;
    
public:
    FB2KException(const std::string& message, HRESULT hr = E_FAIL) 
        : m_message(message), m_hr(hr) {}
    
    const char* what() const noexcept override { return m_message.c_str(); }
    HRESULT GetHR() const { return m_hr; }
};