#include "test_framework.h"
#include "adapters/adapter_base.h"

using namespace mp::compat;
using namespace mp::compat::test;

// Mock adapter for testing
class MockAdapter : public AdapterBase {
public:
    MockAdapter() : AdapterBase(AdapterType::InputDecoder, "MockAdapter") {}
    
    Result initialize() override {
        set_status(AdapterStatus::Ready);
        return Result::Success;
    }
    
    void shutdown() override {
        set_status(AdapterStatus::NotInitialized);
    }
};

// Test fixture for AdapterBase
class AdapterBaseTest : public TestFixture {
public:
    std::string getName() const override {
        return "AdapterBaseTest";
    }
    
    std::vector<TestResult> runTests() override {
        std::vector<TestResult> results;
        
        results.push_back(testConstructor());
        results.push_back(testGetTypeAndName());
        results.push_back(testStatusManagement());
        results.push_back(testAdapterStats());
        
        return results;
    }
    
private:
    TestResult testConstructor() {
        MockAdapter adapter;
        return assertTrue("Constructor creates valid object", true);
    }
    
    TestResult testGetTypeAndName() {
        MockAdapter adapter;
        bool correctType = (adapter.get_type() == AdapterType::InputDecoder);
        bool correctName = (adapter.get_name() == "MockAdapter");
        
        bool success = correctType && correctName;
        return assertTrue("Type and name accessors", success);
    }
    
    TestResult testStatusManagement() {
        MockAdapter adapter;
        
        // Initially not initialized
        bool notInit = (adapter.get_status() == AdapterStatus::NotInitialized);
        bool notReady = !adapter.is_ready();
        
        // Initialize
        Result result = adapter.initialize();
        bool initSuccess = (result == Result::Success);
        bool isReady = adapter.is_ready();
        
        bool success = notInit && notReady && initSuccess && isReady;
        return assertTrue("Status management", success);
    }
    
    TestResult testAdapterStats() {
        AdapterStats stats;
        
        // Record some calls
        stats.record_call(true, 10.5, 1024);
        stats.record_call(true, 15.2, 2048);
        stats.record_call(false, 5.1, 0);
        
        bool countsCorrect = (stats.calls_total == 3) && 
                           (stats.calls_success == 2) && 
                           (stats.calls_failed == 1) &&
                           (stats.bytes_processed == 3072);
                           
        bool timingReasonable = (stats.total_time_ms > 30.0) && 
                              (stats.avg_time_ms > 10.0);
        
        bool success = countsCorrect && timingReasonable;
        return assertTrue("Adapter statistics tracking", success);
    }
};

// Register this test fixture
static struct RegisterAdapterBaseTest {
    RegisterAdapterBaseTest() {
        TestRunner::getInstance().registerFixture(std::make_unique<AdapterBaseTest>());
    }
} registerAdapterBaseTest;
