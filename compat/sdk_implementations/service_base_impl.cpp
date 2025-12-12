/**
 * @file service_base_impl.cpp
 * @brief Service base implementation
 * @date 2025-12-10
 */

#include "service_base_impl.h"
#include <cstring>

namespace foobar2000 {

service_base_impl::service_base_impl()
    : ref_count_(0)
    , service_name_("unnamed") {
    memset(&class_guid_, 0, sizeof(class_guid_));
}

service_base_impl::~service_base_impl() {
}

void service_base_impl::service_add_ref() {
    ref_count_.fetch_add(1, std::memory_order_relaxed);
}

bool service_base_impl::service_release() {
    return ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 0;
}

bool service_base_impl::service_query(const GUID& guid, void** out) {
    if (!out) return false;

    // Check if it's our class GUID
    if (memcmp(&class_guid_, &guid, sizeof(GUID)) == 0) {
        *out = static_cast<void*>(this);
        return true;
    }

    *out = nullptr;
    return false;
}

const char* service_base_impl::service_get_name() {
    return service_name_;
}

void service_base_impl::set_service_name(const char* name) {
    service_name_ = name ? name : "unnamed";
}

void service_base_impl::set_class_guid(const GUID& guid) {
    class_guid_ = guid;
}

} // namespace foobar2000