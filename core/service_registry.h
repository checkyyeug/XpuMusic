#pragma once

#include "mp_plugin.h"
#include <unordered_map>
#include <mutex>

namespace mp {
namespace core {

// Service registry implementation
class ServiceRegistry : public IServiceRegistry {
public:
    ServiceRegistry();
    ~ServiceRegistry() override;
    
    // IServiceRegistry implementation
    Result register_service(ServiceID id, void* service) override;
    Result unregister_service(ServiceID id) override;
    void* query_service(ServiceID id) override;
    
private:
    std::unordered_map<ServiceID, void*> services_;
    std::mutex mutex_;
};

}} // namespace mp::core
