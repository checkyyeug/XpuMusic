#pragma once

#include "../xpumusic_sdk/foobar2000_sdk_complete.h"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace foobar2000_sdk {

// Service Registry Bridge Implementation
// This bridges foobar2000's service system with our native implementation

class service_registry_bridge {
private:
    std::unordered_map<GUID, std::unique_ptr<service_factory_base>> services_;
    std::unordered_map<GUID, service_ptr_t<service_base>> singletons_;
    std::mutex mutex_;

    service_registry_bridge() = default;

public:
    static service_registry_bridge& get_instance() {
        static service_registry_bridge instance;
        return instance;
    }

    // Register a service factory
    bool register_service(const GUID& guid, std::unique_ptr<service_factory_base> factory) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (services_.find(guid) != services_.end()) {
            return false; // Already registered
        }

        services_[guid] = std::move(factory);
        return true;
    }

    // Unregister a service
    bool unregister_service(const GUID& guid) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = services_.find(guid);
        if (it == services_.end()) {
            return false;
        }

        services_.erase(it);
        singletons_.erase(guid);
        return true;
    }

    // Query for a service instance
    template<typename T>
    service_ptr_t<T> get_service(const GUID& guid) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check for singleton first
        auto singleton_it = singletons_.find(guid);
        if (singleton_it != singletons_.end()) {
            auto service = static_cast<T*>(singleton_it->second.get_ptr());
            if (service) {
                service->service_add_ref();
                return service_ptr_t<T>(service);
            }
        }

        // Create new instance from factory
        auto factory_it = services_.find(guid);
        if (factory_it == services_.end()) {
            return service_ptr_t<T>();
        }

        auto* base_service = factory_it->second->create_service();
        if (!base_service) {
            return service_ptr_t<T>();
        }

        auto* typed_service = static_cast<T*>(base_service);

        // Check if this should be a singleton
        // (In foobar2000, some services are singletons by nature)
        if (should_be_singleton(guid)) {
            singletons_[guid] = service_ptr_t<service_base>(typed_service);
        }

        return service_ptr_t<T>(typed_service);
    }

    // Query for service factory
    service_factory_base* get_factory(const GUID& guid) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = services_.find(guid);
        if (it != services_.end()) {
            return it->second.get();
        }

        return nullptr;
    }

    // Get all registered service GUIDs
    std::vector<GUID> get_registered_services() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<GUID> guids;
        guids.reserve(services_.size());

        for (const auto& pair : services_) {
            guids.push_back(pair.first);
        }

        return guids;
    }

    // Get service count
    size_t get_service_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return services_.size();
    }

    // Clear all services
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        services_.clear();
        singletons_.clear();
    }

private:
    // Determine if a service should be singleton
    bool should_be_singleton(const GUID& guid) {
        // Common service GUIDs that should be singletons
        // In a real implementation, this would be based on service type
        static const GUID singleton_services[] = {
            // Add known singleton service GUIDs here
        };

        for (const auto& singleton_guid : singleton_services) {
            if (guid == singleton_guid) {
                return true;
            }
        }

        return false;
    }
};

// Service factory wrapper for bridging
template<typename ServiceInterface, typename ServiceImplementation>
class service_factory_wrapper : public service_factory_base {
private:
    GUID guid_;
    std::atomic<int> ref_count_{1};

public:
    service_factory_wrapper(const GUID& guid) : guid_(guid) {}

    const GUID& get_guid() const override {
        return guid_;
    }

    service_base* create_service() override {
        return new ServiceImplementation();
    }

    // Reference counting implementation
    int service_add_ref() override {
        return ++ref_count_;
    }

    int service_release() override {
        if (--ref_count_ == 0) {
            delete this;
            return 0;
        }
        return ref_count_;
    }
};

// Helper macro to register services
#define REGISTER_FOOBAR_SERVICE(Interface, Implementation, guid) \
    namespace { \
        struct Implementation##_registrar { \
            Implementation##_registrar() { \
                auto factory = std::make_unique<service_factory_wrapper<Interface, Implementation>>(guid); \
                service_registry_bridge::get_instance().register_service(guid, std::move(factory)); \
            } \
        }; \
        static Implementation##_registrar Implementation##_registrar_instance; \
    }

} // namespace foobar2000_sdk