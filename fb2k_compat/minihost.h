#pragma once

// 阶段1：最小foobar2000主机接口
// 目标：加载并运行foo_input_std组件

#include <windows.h>
#include <unknwn.h>
#include <functional>
#include <string>
#include <vector>
#include <memory>

// 最小GUID定义
struct DECLSPEC_UUID("00000000-0000-0000-C000-000000000046") IUnknown;

// 前向声明
namespace fb2k {
    class service_base;
    class file_info;
    class abort_callback;
    class input_decoder;
    class playback_control;
}

// 基础服务接口
MIDL_INTERFACE("FB2KServiceBase")
class service_base : public IUnknown {
public:
    virtual int service_add_ref() = 0;
    virtual int service_release() = 0;
};

// 智能指针模板
template<typename T>
class service_ptr_t {
private:
    T* ptr_ = nullptr;
public:
    service_ptr_t() = default;
    ~service_ptr_t() { release(); }
    
    service_ptr_t(T* p) : ptr_(p) { if(ptr_) ptr_->AddRef(); }
    service_ptr_t(const service_ptr_t& other) : ptr_(other.ptr_) { if(ptr_) ptr_->AddRef(); }
    service_ptr_t(service_ptr_t&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
    
    service_ptr_t& operator=(T* p) { reset(p); return *this; }
    service_ptr_t& operator=(const service_ptr_t& other) { reset(other.ptr_); return *this; }
    service_ptr_t& operator=(service_ptr_t&& other) noexcept {
        if(this != &other) { release(); ptr_ = other.ptr_; other.ptr_ = nullptr; }
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
};

// 文件统计信息
struct file_stats {
    uint64_t size = 0;
    uint64_t timestamp = 0;
};

// 音频信息
struct audio_info {
    uint32_t sample_rate = 44100;
    uint32_t channels = 2;
    uint32_t bitrate = 0;
    double length = 0.0;
};

// 文件信息接口
MIDL_INTERFACE("FB2KFileInfo")
class file_info : public service_base {
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

// 中止回调
MIDL_INTERFACE("FB2KAbortCallback")
class abort_callback : public service_base {
public:
    virtual bool is_aborting() const = 0;
};

// 输入解码器接口 - 这是核心！
MIDL_INTERFACE("FB2KInputDecoder")
class input_decoder : public service_base {
public:
    // 核心解码接口
    virtual bool open(const char* path, fb2k::file_info& info, fb2k::abort_callback& abort) = 0;
    virtual int decode(float* buffer, int samples, fb2k::abort_callback& abort) = 0;
    virtual void seek(double seconds, fb2k::abort_callback& abort) = 0;
    virtual bool can_seek() = 0;
    virtual void close() = 0;
    
    // 能力查询
    virtual bool is_our_path(const char* path) = 0;
    virtual const char* get_name() = 0;
};

// 简单的文件信息实现
class file_info_impl : public file_info {
private:
    std::unordered_map<std::string, std::vector<std::string>> metadata_;
    audio_info audio_info_;
    file_stats file_stats_;
    double length_ = 0.0;
    
public:
    file_info_impl() = default;
    
    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if(!ppvObject) return E_POINTER;
        if(IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, __uuidof(file_info))) {
            *ppvObject = static_cast<file_info*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }  // 简单实现
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    
    // file_info接口
    void reset() override {
        metadata_.clear();
        audio_info_ = {};
        file_stats_ = {};
        length_ = 0.0;
    }
    
    const char* meta_get(const char* name, size_t index) const override {
        auto it = metadata_.find(name);
        if(it != metadata_.end() && index < it->second.size()) {
            return it->second[index].c_str();
        }
        return nullptr;
    }
    
    size_t meta_get_count(const char* name) const override {
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
    
    // service_base接口
    int service_add_ref() override { return 1; }
    int service_release() override { return 1; }
};

// 哑中止回调
class abort_callback_dummy : public abort_callback {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if(!ppvObject) return E_POINTER;
        if(IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, __uuidof(abort_callback))) {
            *ppvObject = static_cast<abort_callback*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    
    bool is_aborting() const override { return false; }
    int service_add_ref() override { return 1; }
    int service_release() override { return 1; }
};

} // namespace fb2k

// 主机类
class mini_host {
public:
    using decoder_ptr = fb2k::service_ptr_t<fb2k::input_decoder>;
    
    // 初始化COM
    bool initialize();
    
    // 加载组件DLL
    bool load_component(const std::wstring& dll_path);
    
    // 创建解码器
    decoder_ptr create_decoder_for_path(const std::string& path);
    
    // 获取已加载的组件信息
    std::vector<std::string> get_loaded_components() const;
    
    // 测试解码
    bool test_decode(const std::string& audio_file);
    
private:
    std::vector<HMODULE> loaded_modules_;
    std::vector<std::string> component_names_;
    
    // 获取输入解码器服务工厂
    using get_service_func = HRESULT(*)(const GUID&, void**);
    get_service_func get_service_ = nullptr;
};

// 辅助函数
std::string wide_to_utf8(const std::wstring& wide);
std::wstring utf8_to_wide(const std::string& utf8);
void log_info(const char* format, ...);
void log_error(const char* format, ...);