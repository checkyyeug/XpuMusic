/**
 * @file service_base_impl.h
 * @brief Service base implementation for foobar2000 compatibility
 * @date 2025-12-10
 */

#pragma once

#include "../xpumusic_sdk/foobar2000.h"
#include <atomic>

namespace xpumusic_sdk {

class service_base_impl : public service_base {
private:
    std::atomic<int> ref_count_;

public:
    service_base_impl();
    virtual ~service_base_impl();

    // Reference counting
    int service_add_ref() override;
    int service_release() override;

    // Service information
    virtual const char* service_get_name() = 0;
    virtual const GUID* service_get_class_guid() = 0;

protected:
    void set_service_name(const char* name);
    void set_class_guid(const GUID& guid);

private:
    const char* service_name_;
    GUID class_guid_;
};

// RAII service pointer
template<typename T>
class service_ptr_impl {
private:
    T* ptr_;

public:
    service_ptr_impl() : ptr_(nullptr) {}
    service_ptr_impl(T* p) : ptr_(p) {
        if (ptr_) ptr_->service_add_ref();
    }
    service_ptr_impl(const service_ptr_impl& other) : ptr_(other.ptr_) {
        if (ptr_) ptr_->service_add_ref();
    }

    ~service_ptr_impl() {
        if (ptr_) ptr_->service_release();
    }

    service_ptr_impl& operator=(const service_ptr_impl& other) {
        if (this != &other) {
            if (ptr_) ptr_->service_release();
            ptr_ = other.ptr_;
            if (ptr_) ptr_->service_add_ref();
        }
        return *this;
    }

    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    bool is_valid() const { return ptr_ != nullptr; }

    void release() {
        if (ptr_) {
            ptr_->service_release();
            ptr_ = nullptr;
        }
    }
};

} // namespace xpumusic_sdk