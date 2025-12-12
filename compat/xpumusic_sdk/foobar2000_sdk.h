#pragma once

// Foobar2000 SDK Interface Stubs
// This file contains minimal interface definitions for foobar2000 plugin compatibility
// These are clean-room implementations based on publicly documented interfaces

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <unordered_map>

namespace xpumusic_sdk {

// Forward declarations
class service_base;
class input_decoder;

// Audio information structure
struct audio_info {
    uint32_t m_sample_rate = 44100;
    uint32_t m_channels = 2;
    uint32_t m_bitrate = 0;
    double m_length = 0.0;
};

// File statistics structure
struct file_stats {
    uint64_t m_size = 0;
    uint64_t m_timestamp = 0;

    bool is_valid() const { return m_size > 0 && m_timestamp > 0; }
};

// Field value structure for multi-value metadata support
struct field_value {
    std::vector<std::string> values;  // Multi-value storage
    std::string joined_cache;         // Cached joined value
    bool cache_valid = false;

    const std::string& join(const std::string& separator = "; ") {
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
};

// GUID structure (Windows-style)
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
};

// Reference counting base class (mimics foobar2000's service_base)
class service_base {
public:
    virtual ~service_base() = default;

    // Reference counting interface
    virtual int service_add_ref() = 0;
    virtual int service_release() = 0;

protected:
    service_base() = default;
    service_base(const service_base&) = delete;
    service_base& operator=(const service_base&) = delete;
};

// Service factory base class
class service_factory_base : public service_base {
public:
    virtual ~service_factory_base() = default;

    // Get the GUID of the service this factory creates
    virtual const GUID& get_guid() const = 0;
};

// Smart pointer for service objects (mimics service_ptr_t)
template<typename T>
class service_ptr_t {
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
    
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* get_ptr() const { return ptr_; }
    
    bool is_valid() const { return ptr_ != nullptr; }
    bool is_empty() const { return ptr_ == nullptr; }
    
    void release() {
        if (ptr_) {
            ptr_->service_release();
            ptr_ = nullptr;
        }
    }
    
private:
    T* ptr_;
};

// Abort callback interface (for cancellable operations)
class abort_callback {
public:
    virtual ~abort_callback() = default;
    
    // Check if operation should be aborted
    virtual bool is_aborting() const = 0;
    
    // Sleep with abort check
    virtual void sleep(double seconds) = 0;
};

// No-op abort callback
class abort_callback_dummy : public abort_callback {
public:
    bool is_aborting() const override { return false; }
    void sleep(double) override {}
};

// Audio sample type
typedef float audio_sample;

// Audio chunk structure (mimics foobar2000's audio_chunk) - ABSTRACT INTERFACE
class audio_chunk {
public:
    virtual ~audio_chunk() = default;

    // Sample rate in Hz
    virtual uint32_t get_sample_rate() const = 0;
    virtual void set_sample_rate(uint32_t rate) = 0;

    // Number of channels
    virtual uint32_t get_channels() const = 0;
    virtual void set_channels(uint32_t ch) = 0;

    // Channel configuration bitmask
    virtual uint32_t get_channel_config() const = 0;
    virtual void set_channel_config(uint32_t config) = 0;

    // Number of samples per channel
    virtual size_t get_sample_count() const = 0;
    virtual void set_sample_count(size_t count) = 0;

    // Get audio data pointer
    virtual const audio_sample* get_data() const = 0;
    virtual audio_sample* get_data() = 0;

    // Set data size and allocate if needed
    virtual void set_data_size(size_t samples_per_channel) = 0;

    // Get duration in seconds
    virtual double get_duration() const = 0;

    // Reset chunk
    virtual void reset() = 0;

    // Additional virtual methods expected by foobar2000 API
    virtual void set_data(const audio_sample* p_data, size_t p_samples, uint32_t p_channels, uint32_t p_sample_rate) = 0;
    virtual size_t get_data_size() const = 0;
    virtual size_t get_data_bytes() const = 0;
    virtual audio_sample* get_channel_data(uint32_t p_channel) = 0;
    virtual const audio_sample* get_channel_data(uint32_t p_channel) const = 0;
    virtual void scale(const audio_sample& p_scale) = 0;
    virtual void copy(const audio_chunk& p_source) = 0;
    virtual bool is_valid() const = 0;
    virtual bool is_empty() const = 0;
};

// File information structure - ABSTRACT INTERFACE
class file_info {
public:
    virtual ~file_info() = default;

    // Basic info management
    virtual void reset() = 0;
    virtual bool is_valid() const = 0;

    // Metadata interface
    virtual const char* meta_get(const char* name, size_t index) const = 0;
    virtual size_t meta_get_count(const char* name) const = 0;
    virtual bool meta_set(const char* name, const char* value) = 0;
    virtual bool meta_add(const char* name, const char* value) = 0;
    virtual bool meta_remove(const char* name) = 0;
    virtual void meta_remove_index(const char* name, size_t index) = 0;
    virtual std::vector<std::string> meta_enumerate() const = 0;

    // Length
    virtual double get_length() const = 0;
    virtual void set_length(double length) = 0;

    // Audio properties
    virtual uint32_t get_sample_rate() const = 0;
    virtual void set_sample_rate(uint32_t rate) = 0;
    virtual uint32_t get_channels() const = 0;
    virtual void set_channels(uint32_t channels) = 0;
    virtual uint32_t get_bitrate() const = 0;
    virtual void set_bitrate(uint32_t bitrate) = 0;

    // Codec information
    virtual const char* get_codec() const = 0;
    virtual void set_codec(const char* codec) = 0;

    // Copy operations
    virtual void copy(const file_info& other) = 0;
    virtual void merge(const file_info& other) = 0;

    // File statistics
    virtual const file_stats& get_stats() const = 0;
    virtual file_stats& get_stats() = 0;
    virtual void set_stats(const file_stats& stats) = 0;

    // Audio info
    virtual const audio_info& get_audio_info() const = 0;
    virtual audio_info& get_audio_info() = 0;
    virtual void set_audio_info(const audio_info& info) = 0;
};

// Playable location structure
class playable_location {
public:
    virtual ~playable_location() = default;

    virtual const char* get_path() const = 0;
    virtual void set_path(const char* path) = 0;
    virtual uint32_t get_subsong_index() const = 0;
    virtual void set_subsong_index(uint32_t index) = 0;
    virtual bool is_empty() const = 0;
};

// Metadb interface
class metadb : public service_base {
public:
    virtual ~metadb() = default;
    // Methods to be defined
};

// Playback control interface
class playback_control : public service_base {
public:
    virtual ~playback_control() = default;
    // Methods to be defined
};

// Input manager interface
class input_manager : public service_base {
public:
    virtual ~input_manager() = default;
    virtual bool instantiate() = 0;
    virtual bool open(const char* filename, service_ptr_t<input_decoder>& decoder, abort_callback& abort) = 0;
};

// Input decoder interface
class input_decoder : public service_base {
public:
    virtual ~input_decoder() = default;
    virtual bool get_info(uint32_t subsong, file_info& info, abort_callback& abort) = 0;
};


// Helper template for service creation
template<typename T>
service_ptr_t<T> standard_api_create_t() {
    // For now return empty smart pointer since we don't have concrete implementations
    return service_ptr_t<T>();
}

// Concrete implementations (minimal for testing)
class concrete_input_manager : public input_manager {
public:
    bool instantiate() override { return true; }
    bool open(const char* filename, service_ptr_t<input_decoder>& decoder, abort_callback& abort) override {
        // Return false for now
        return false;
    }
};

class concrete_input_decoder : public input_decoder {
public:
    bool get_info(uint32_t subsong, file_info& info, abort_callback& abort) override {
        // Return false for now
        return false;
    }
};

// Metadb handle interface - ABSTRACT INTERFACE
class metadb_handle {
public:
    virtual ~metadb_handle() = default;

    // Get location of the media file
    virtual const playable_location& get_location() const = 0;

    // Get metadata information
    virtual const file_info& get_info() const = 0;
    virtual file_info& get_info() = 0;
    virtual void set_info(const file_info& info) = 0;

    // Get file stats
    virtual const file_stats& get_file_stats() const = 0;

    // Get unique handle hash
    virtual uint64_t get_location_hash() const = 0;

    // Comparison operators
    virtual bool is_same(const metadb_handle& other) const = 0;
    virtual bool is_valid() const = 0;

    // Update from file
    virtual void reload(abort_callback& p_abort) = 0;

    // Get directory
    virtual std::string get_path() const = 0;
    virtual std::string get_filename() const = 0;
    virtual std::string get_directory() const = 0;

    // Reference counting for the handle
    virtual void ref_add_ref() = 0;
    virtual void ref_release() = 0;
};

} // namespace xpumusic_sdk
