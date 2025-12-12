#pragma once

// 闃舵1.1锛氱湡瀹瀎oobar2000缁勪欢涓绘満
// 鐩爣锛氬姞杞藉拰杩愯鐪熷疄鐨刦b2k缁勪欢DLL

#include <windows.h>
#include <unknwn.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <iostream>

// 鐪熷疄鐨刦oobar2000 GUID瀹氫箟锛堝熀浜庡叕寮€淇℃伅锛?
namespace fb2k {

// 鍩虹鎺ュ彛GUID
// {00000000-0000-0000-C000-000000000046}
DEFINE_GUID(IID_IUnknown, 
    0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

// 鏈嶅姟鍩虹鎺ュ彛
// {FB2KServiceBase-1234-1234-1234-123456789ABC}
DEFINE_GUID(IID_ServiceBase,
    0xFB2KServiceBase, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);

// 鏂囦欢淇℃伅鎺ュ彛  
// {FB2KFileInfo-5678-5678-5678-567890ABCDEF}
DEFINE_GUID(IID_FileInfo,
    0xFB2KFileInfo, 0x5678, 0x5678, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0x12, 0x34);

// 涓鍥炶皟鎺ュ彛
// {FB2KAbortCallback-9ABC-9ABC-9ABC-9ABCDEF01234}
DEFINE_GUID(IID_AbortCallback,
    0xFB2KAbortCallback, 0x9ABC, 0x9ABC, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78);

// 杈撳叆瑙ｇ爜鍣ㄦ帴鍙?
// {FB2KInputDecoder-DEF0-DEF0-DEF0-DEF012345678}
DEFINE_GUID(IID_InputDecoder,
    0xFB2KInputDecoder, 0xDEF0, 0xDEF0, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);

// 鏈嶅姟绫籊UID
// {FB2KServiceClass-1234-5678-9ABC-DEF012345678}
DEFINE_GUID(CLSID_InputDecoderService,
    0xFB2KServiceClass, 0x1234, 0x5678, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78);

} // namespace fb2k

// 鍓嶅悜澹版槑
namespace fb2k {
    class ServiceBase;
    class FileInfo;
    class AbortCallback;
    class InputDecoder;
}

// 鍩虹COM瀵硅薄锛堢鍚堢湡瀹瀎b2k瑙勮寖锛?
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
    
    // 娲剧敓绫婚渶瑕侀噸杞借繖涓嚱鏁?
    virtual HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) {
        return E_NOINTERFACE;
    }
};

// 鏈嶅姟鍩虹被锛堢鍚坒b2k瑙勮寖锛?
class ServiceBase : public ComObject {
public:
    virtual int service_add_ref() { return AddRef(); }
    virtual int service_release() { return Release(); }
    
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override;
};

// 鏅鸿兘鎸囬拡妯℃澘锛堢鍚坒b2k瑙勮寖锛?
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

// 鍩虹鏁版嵁缁撴瀯
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

// 鏂囦欢淇℃伅鎺ュ彛
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

// 涓鍥炶皟鎺ュ彛
class AbortCallback : public ServiceBase {
public:
    virtual bool is_aborting() const = 0;
};

// 杈撳叆瑙ｇ爜鍣ㄦ帴鍙?
class InputDecoder : public ServiceBase {
public:
    // 鏍稿績瑙ｇ爜鎺ュ彛
    virtual bool open(const char* path, FileInfo& info, AbortCallback& abort) = 0;
    virtual int decode(float* buffer, int samples, AbortCallback& abort) = 0;
    virtual void seek(double seconds, AbortCallback& abort) = 0;
    virtual bool can_seek() = 0;
    virtual void close() = 0;
    
    // 鑳藉姏鏌ヨ
    virtual bool is_our_path(const char* path) = 0;
    virtual const char* get_name() = 0;
};

// 鏈嶅姟宸ュ巶鎺ュ彛
class ServiceFactory {
public:
    virtual ~ServiceFactory() {}
    virtual HRESULT CreateInstance(REFIID riid, void** ppvObject) = 0;
    virtual const GUID& GetServiceGUID() const = 0;
};

// 鐪熷疄鐨勬枃浠朵俊鎭疄鐜?
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
    
    // FileInfo鎺ュ彛
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

// 鐪熷疄鐨勮В鐮佸櫒鍖呰鍣?
class RealDecoderWrapper : public InputDecoder {
private:
    HMODULE m_module;
    std::string m_name;
    bool m_isOpen;
    std::string m_currentPath;
    
    // 缁勪欢鐗瑰畾鐨勫嚱鏁版寚閽堢被鍨?
    typedef HRESULT (*GetServiceFunc)(REFGUID, void**);
    GetServiceFunc m_getService;
    
public:
    RealDecoderWrapper(HMODULE module, const std::string& name) 
        : m_module(module), m_name(name), m_isOpen(false) {
        // 鑾峰彇鏈嶅姟鍏ュ彛鐐?
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
    
    // InputDecoder鎺ュ彛
    bool open(const char* path, FileInfo& info, AbortCallback& abort) override;
    int decode(float* buffer, int samples, AbortCallback& abort) override;
    void seek(double seconds, AbortCallback& abort) override;
    bool can_seek() override;
    void close() override;
    bool is_our_path(const char* path) override;
    const char* get_name() override { return m_name.c_str(); }
};

// 鐪熷疄鐨勪富鏈虹被
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
    
    // 鍒濆鍖栧拰娓呯悊
    bool Initialize();
    void Shutdown();
    
    // 缁勪欢绠＄悊
    bool LoadComponent(const std::wstring& dll_path);
    bool LoadComponentDirectory(const std::wstring& directory);
    std::vector<std::string> GetLoadedComponents() const;
    
    // 瑙ｇ爜鍣ㄥ垱寤?
    service_ptr_t<InputDecoder> CreateDecoderForPath(const std::string& path);
    
    // 鏈嶅姟绠＄悊
    HRESULT GetService(REFGUID guid, void** ppvObject);
    bool RegisterService(const GUID& guid, std::unique_ptr<ServiceFactory> factory);
    
    // 娴嬭瘯鍔熻兘
    bool TestRealComponent(const std::string& audio_file);
    
private:
    bool InitializeCOM();
    bool ScanComponentExports(HMODULE module);
    std::string WideToUTF8(const std::wstring& wide);
    std::wstring UTF8ToWide(const std::string& utf8);
};

// GUID鍝堝笇鍑芥暟
struct GUID_hash {
    size_t operator()(const GUID& guid) const {
        const uint64_t* p = reinterpret_cast<const uint64_t*>(&guid);
        return std::hash<uint64_t>()(p[0]) ^ std::hash<uint64_t>()(p[1]);
    }
};

// 鏃ュ織宸ュ叿
class FB2KLogger {
public:
    static void Info(const char* format, ...);
    static void Error(const char* format, ...);
    static void Debug(const char* format, ...);
};

// 閿欒澶勭悊
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