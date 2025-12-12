#include "service_registry.h"

namespace mp {
namespace core {

ServiceRegistry::ServiceRegistry() {
}

ServiceRegistry::~ServiceRegistry() {
}

Result ServiceRegistry::register_service(ServiceID id, void* service) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (services_.find(id) != services_.end()) {
        return Result::AlreadyInitialized;
    }
    
    services_[id] = service;
    return Result::Success;
}

Result ServiceRegistry::unregister_service(ServiceID id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(id);
    if (it == services_.end()) {
        return Result::InvalidParameter;
    }
    
    services_.erase(it);
    return Result::Success;
}

void* ServiceRegistry::query_service(ServiceID id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(id);
    if (it == services_.end()) {
        return nullptr;
    }
    
    return it->second;
}

}} // namespace mp::core
