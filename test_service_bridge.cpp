#include "compat/plugin_loader/plugin_loader.h"
#include "compat/foobar_sdk/foobar2000_sdk.h"
#include <iostream>

// Mock service registry for testing
namespace mp { 
namespace core {
    class ServiceRegistry {
    public:
        enum class Result { Success, InvalidParameter, AlreadyInitialized, FileNotFound };
        
        Result register_service(uint64_t service_id, void* service_ptr) { return Result::Success; }
        Result unregister_service(uint64_t service_id) { return Result::Success; }
    };
}
}

// Mock ServiceFactoryWrapper for testing
class MockServiceFactoryWrapper : public mp::compat::ServiceFactoryWrapper {
public:
    explicit MockServiceFactoryWrapper(foobar2000_sdk::service_factory_base* factory) 
        : ServiceFactoryWrapper(factory) {}
    
    foobar2000_sdk::service_ptr create_service() override { 
        return foobar2000_sdk::service_ptr(); 
    }
    
    const foobar2000_sdk::GUID& get_guid() const override { 
        static foobar2000_sdk::GUID dummy_guid = {};
        return dummy_guid; 
    }
};

int main() {
    std::cout << "Testing ServiceRegistryBridge compilation..." << std::endl;
    
    // Test creating a bridge (this will compile if interface is complete)
    mp::core::ServiceRegistry mock_registry;
    auto bridge = std::make_unique<mp::compat::ServiceRegistryBridgeImpl>(&mock_registry);
    
    std::cout << "ServiceRegistryBridge compiled and instantiated successfully!" << std::endl;
    
    // Test all interface methods are available
    foobar2000_sdk::GUID test_guid = {};
    test_guid.Data1 = 0x12345678;
    
    MockServiceFactoryWrapper* wrapper = nullptr; // nullptr for this test
    
    // These should compile without errors
    bridge->register_service(test_guid, wrapper);
    bridge->unregister_service(test_guid);
    bridge->query_service(test_guid);
    bridge->query_factory(test_guid);
    auto services = bridge->get_registered_services();
    size_t count = bridge->get_service_count();
    bridge->clear();
    
    std::cout << "All ServiceRegistryBridge methods compile correctly!" << std::endl;
    
    return 0;
}