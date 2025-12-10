#pragma once

// Complete Foobar2000 SDK Implementation
// This is a clean-room implementation providing foobar2000 compatibility

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <thread>

namespace xpumusic_sdk {

// Forward declarations
class service_base;
class abort_callback;
class file_info;
class metadb_handle;

// GUID structure
struct GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];

    bool operator==(const GUID& other) const {
        return Data1 == other.Data1 && Data2 == other.Data2 &&
               Data3 == other.Data3 &&
               memcmp(Data4, other.Data4, 8) == 0;
    }

    bool operator!=(const GUID& other) const {
        return !(*this == other);
    }

    bool operator<(const GUID& other) const {
        if (Data1 != other.Data1) return Data1 < other.Data1;
        if (Data2 != other.Data2) return Data2 < other.Data2;
        if (Data3 != other.Data3) return Data3 < other.Data3;
        return memcmp(Data4, other.Data4, 8) < 0;
    }
};

// Audio information structure
struct audio_info {
    uint32_t m_sample_rate = 44100;
    uint32_t m_channels = 2;
    uint32_t m_bits_per_sample = 16;
    uint32_t m_bitrate = 0;
    double   m_length = 0.0;
    bool     m_is_float = false;

    audio_info() = default;

    void reset() {
        m_sample_rate = 44100;
        m_channels = 2;
        m_bits_per_sample = 16;
        m_bitrate = 0;
        m_length = 0.0;
        m_is_float = false;
    }
};

// File statistics structure
struct file_stats {
    uint64_t m_size = 0;
    uint64_t m_timestamp = 0;

    bool is_valid() const { return m_size > 0; }
};

// Field value structure for metadata
struct field_value {
    std::vector<std::string> values;
    mutable std::string joined_cache;
    mutable bool cache_valid = false;

    const std::string& join(const std::string& separator = "; ") const {
        if (!cache_valid) {
            joined_cache.clear();
            for (size_t i = 0; i < values.size(); ++i) {
                if (i > 0) joined_cache += separator;
                joined_cache += values[i];
            }
            cache_valid = true;
        }
        return joined_cache;
    }

    void add_value(const std::string& value) {
        values.push_back(value);
        cache_valid = false;
    }
};

// Reference counting base class
class service_base {
public:
    virtual ~service_base() = default;

    // Reference counting
    virtual int service_add_ref() = 0;
    virtual int service_release() = 0;

protected:
    service_base() = default;
    service_base(const service_base&) = delete;
    service_base& operator=(const service_base&) = delete;
};

// Service factory base
class service_factory_base : public service_base {
public:
    virtual ~service_factory_base() = default;

    virtual const GUID& get_guid() const = 0;
    virtual service_base* create_service() = 0;
};

// Service pointer template
template<typename T>
class service_ptr_t {
private:
    T* ptr_;

public:
    service_ptr_t() : ptr_(nullptr) {}

    explicit service_ptr_t(T* p) : ptr_(p) {
        if (ptr_) ptr_->service_add_ref();
    }

    service_ptr_t(const service_ptr_t& other) : ptr_(other.ptr_) {
        if (ptr_) ptr_->service_add_ref();
    }

    service_ptr_t(service_ptr_t&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    ~service_ptr_t() {
        if (ptr_) ptr_->service_release();
    }

    service_ptr_t& operator=(const service_ptr_t& other) {
        if (this != &other) {
            if (ptr_) ptr_->service_release();
            ptr_ = other.ptr_;
            if (ptr_) ptr_->service_add_ref();
        }
        return *this;
    }

    service_ptr_t& operator=(service_ptr_t&& other) noexcept {
        if (this != &other) {
            if (ptr_) ptr_->service_release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }

    T* get_ptr() const { return ptr_; }

    bool is_valid() const { return ptr_ != nullptr; }
    bool is_empty() const { return ptr_ == nullptr; }

    void release() {
        if (ptr_) {
            ptr_->service_release();
            ptr_ = nullptr;
        }
    }

    T* detach() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }
};

// Location class
class location_t {
private:
    std::string path_;

public:
    location_t() = default;
    explicit location_t(const char* path) : path_(path ? path : "") {}
    explicit location_t(const std::string& path) : path_(path) {}

    const char* get_path() const { return path_.c_str(); }
    const std::string& get_path_ref() const { return path_; }

    bool is_empty() const { return path_.empty(); }
    bool is_valid() const { return !path_.empty() && path_.length() < 4096; }

    void set_path(const char* path) { path_ = path ? path : ""; }
    void set_path(const std::string& path) { path_ = path; }

    bool operator==(const location_t& other) const { return path_ == other.path_; }
    bool operator!=(const location_t& other) const { return path_ != other.path_; }
    bool operator<(const location_t& other) const { return path_ < other.path_; }
};

// Playable location class
class playable_location : public location_t {
private:
    int subsong_index_ = 0;
    mutable std::string subsong_cache_;

public:
    playable_location() = default;
    playable_location(const std::string& path, int subsong)
        : location_t(path), subsong_index_(subsong) {}

    int get_subsong_index() const { return subsong_index_; }

    void set_subsong_index(int index) {
        subsong_index_ = index;
        subsong_cache_.clear();
    }

    const char* get_subsong() const {
        if (subsong_cache_.empty()) {
            subsong_cache_ = std::to_string(subsong_index_);
        }
        return subsong_cache_.c_str();
    }

    bool operator==(const playable_location& other) const {
        return location_t::operator==(other) && subsong_index_ == other.subsong_index_;
    }

    bool operator!=(const playable_location& other) const {
        return !(*this == other);
    }
};

// File info interface
class file_info {
public:
    virtual ~file_info() = default;

    virtual void reset() = 0;
    virtual void copy(const file_info& source) = 0;

    // Meta info access
    virtual const char* meta_get(const char* name, int index) const = 0;
    virtual int meta_get_count(const char* name) const = 0;
    virtual bool meta_exists(const char* name) const = 0;
    virtual void meta_set(const char* name, const char* value) = 0;
    virtual void meta_add_value(const char* name, const char* value) = 0;
    virtual void meta_remove_field(const char* name) = 0;

    // Audio info access
    virtual const audio_info& get_info() const = 0;
    virtual void set_info(const audio_info& info) = 0;

    // File stats
    virtual const file_stats& get_file_stats() const = 0;
    virtual void set_file_stats(const file_stats& stats) = 0;
};

// Metadb handle interface
class metadb_handle {
private:
    std::atomic<int> ref_count_{1};
    playable_location location_;
    std::unique_ptr<file_info> info_;

public:
    metadb_handle() : info_(create_file_info()) {}

    virtual ~metadb_handle() = default;

    // Reference counting
    int service_add_ref() { return ++ref_count_; }
    int service_release() {
        if (--ref_count_ == 0) {
            delete this;
            return 0;
        }
        return ref_count_;
    }

    // Location access
    const playable_location& get_location() const { return location_; }
    void set_location(const playable_location& loc) { location_ = loc; }

    // File info access
    file_info& get_info_ref() { return *info_; }
    const file_info& get_info_ref() const { return *info_; }

    // Utility methods
    bool is_valid() const { return location_.is_valid(); }

protected:
    virtual file_info* create_file_info() = 0;
};

// Abort callback interface
class abort_callback {
public:
    virtual ~abort_callback() = default;

    virtual void abort() = 0;
    virtual bool is_aborting() const = 0;
    virtual void check() const {
        if (is_aborting()) {
            throw std::runtime_error("Operation aborted");
        }
    }
};

// Input decoder interface
class input_decoder {
public:
    virtual ~input_decoder() = default;

    virtual bool can_open(const char* path) = 0;
    virtual bool open(const char* path, file_info& info, abort_callback& abort) = 0;
    virtual int decode(float* buffer, int samples, abort_callback& abort) = 0;
    virtual void seek(double seconds, abort_callback& abort) = 0;
    virtual bool can_seek() = 0;
    virtual void close() = 0;
};

// Service registry
class service_registry {
private:
    std::unordered_map<GUID, std::unique_ptr<service_factory_base>> services_;
    std::mutex mutex_;

    service_registry() = default;

public:
    static service_registry& get_instance() {
        static service_registry instance;
        return instance;
    }

    void register_service(const GUID& guid, std::unique_ptr<service_factory_base> factory) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[guid] = std::move(factory);
    }

    template<typename T>
    service_ptr_t<T> get_service(const GUID& guid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(guid);
        if (it != services_.end()) {
            auto* service = static_cast<T*>(it->second->create_service());
            return service_ptr_t<T>(service);
        }
        return service_ptr_t<T>();
    }

    void unregister_service(const GUID& guid) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_.erase(guid);
    }
};

// Helper macros for service registration
#define DECLARE_SERVICE_GUID(name, g1, g2, g3, g4_1, g4_2, g4_3, g4_4, g4_5, g4_6, g4_7, g4_8) \
    const GUID name = { g1, g2, g3, { g4_1, g4_2, g4_3, g4_4, g4_5, g4_6, g4_7, g4_8 } }

#define REGISTER_SERVICE(service_interface, service_class, guid) \
    namespace { \
        class service_class##_factory : public service_factory_base { \
        public: \
            const GUID& get_guid() const override { return guid; } \
            service_base* create_service() override { return new service_class(); } \
        }; \
        \
        struct service_class##_registrar { \
            service_class##_registrar() { \
                service_registry::get_instance().register_service( \
                    guid, \
                    std::make_unique<service_class##_factory>()); \
            } \
        }; \
        \
        static service_class##_registrar registrar; \
    }

} // namespace xpumusic_sdk