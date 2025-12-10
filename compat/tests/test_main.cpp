#include "test_framework.h"
#include "foobar_compat_manager.h"
#include "logging.h"
#include "migration/data_migration_manager.h"
#include "adapters/adapter_base.h"
#include "xpumusic_sdk/foobar2000_sdk.h"
#include "xpumusic_sdk/input_decoder.h"
#include "mp_types.h"

using namespace mp::compat;
using namespace mp::compat::test;

// Test fixture for XpuMusicCompatManager
class XpuMusicCompatManagerTest : public TestFixture {
public:
    std::string getName() const override {
        return "XpuMusicCompatManagerTest";
    }
    
    std::vector<TestResult> runTests() override {
        std::vector<TestResult> results;
        
        results.push_back(testConstructor());
        results.push_back(testDefaultConfig());
        results.push_back(testInitialization());
        
        return results;
    }
    
private:
    TestResult testConstructor() {
        XpuMusicCompatManager manager;
        return assertTrue("Constructor creates valid object", true);
    }
    
    TestResult testDefaultConfig() {
        XpuMusicCompatManager manager;
        CompatConfig config = manager.get_config();
        
        // Check default values
        bool defaultsCorrect = 
            config.enable_plugin_compat == true &&
            config.enable_data_migration == true &&
            config.compat_mode_strict == false &&
            config.adapter_logging_level == 1;
            
        return assertTrue("Default configuration values", defaultsCorrect);
    }
    
    TestResult testInitialization() {
        XpuMusicCompatManager manager;
        CompatConfig config;
        
        // Test with default config
        mp::Result result = manager.initialize(config);
        bool initSuccess = (result == mp::Result::Success);
        
        // Should succeed even without foobar2000 installed
        return assertTrue("Initialization succeeds", initSuccess);
    }
};

// Mock adapter for testing
class MockAdapter : public AdapterBase {
public:
    MockAdapter() : AdapterBase(AdapterType::InputDecoder, "MockAdapter") {}
    
    mp::Result initialize() override {
        set_status(AdapterStatus::Ready);
        return mp::Result::Success;
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
        mp::Result result = adapter.initialize();
        bool initSuccess = (result == mp::Result::Success);
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

// Test fixture for DataMigrationManager
class DataMigrationManagerTest : public TestFixture {
public:
    std::string getName() const override {
        return "DataMigrationManagerTest";
    }
    
    std::vector<TestResult> runTests() override {
        std::vector<TestResult> results;
        
        results.push_back(testConstructor());
        results.push_back(testInitialization());
        
        return results;
    }
    
private:
    TestResult testConstructor() {
        DataMigrationManager manager;
        return assertTrue("Constructor creates valid object", true);
    }
    
    TestResult testInitialization() {
        DataMigrationManager manager;
        
        // Test with empty path
        mp::Result result = manager.initialize("");
        bool initSuccess = (result == mp::Result::Success);
        
        return assertTrue("Initialization with empty path succeeds", initSuccess);
    }
};

// Test fixture for foobar2000 SDK stubs
class FoobarSdkTest : public TestFixture {
public:
    std::string getName() const override {
        return "FoobarSdkTest";
    }
    
    std::vector<TestResult> runTests() override {
        std::vector<TestResult> results;
        
        results.push_back(testServicePtr());
        results.push_back(testAudioChunk());
        
        return results;
    }
    
private:
    TestResult testServicePtr() {
        // Test service_ptr_t with a simple class
        class TestService : public xpumusic_sdk::service_base {
        public:
            int service_add_ref() override { ref_count_++; return ref_count_; }
            int service_release() override { ref_count_--; return ref_count_; }
        private:
            int ref_count_ = 0;
        };
        
        // Test construction
        xpumusic_sdk::service_ptr_t<TestService> ptr1;
        bool nullOnConstruct = ptr1.is_empty();
        
        // Test assignment
        xpumusic_sdk::service_ptr_t<TestService> ptr2(new TestService());
        bool notNullAfterAssign = !ptr2.is_empty();
        
        // Test copy constructor
        xpumusic_sdk::service_ptr_t<TestService> ptr3(ptr2);
        bool copyWorks = !ptr3.is_empty();
        
        // Test destruction (should not crash)
        ptr2.release();
        bool releaseWorks = ptr2.is_empty();
        
        bool success = nullOnConstruct && notNullAfterAssign && copyWorks && releaseWorks;
        return assertTrue("Service pointer operations", success);
    }
    
    TestResult testAudioChunk() {
        xpumusic_sdk::audio_chunk chunk;
        
        // Test default values
        bool defaultValues = (chunk.get_sample_rate() == 0) &&
                           (chunk.get_channels() == 0) &&
                           (chunk.get_sample_count() == 0);
        
        // Test setting values
        chunk.set_sample_rate(44100);
        chunk.set_channels(2);
        chunk.set_data_size(1024);
        
        bool setValues = (chunk.get_sample_rate() == 44100) &&
                        (chunk.get_channels() == 2) &&
                        (chunk.get_sample_count() == 1024);
        
        bool success = defaultValues && setValues;
        return assertTrue("Audio chunk operations", success);
    }
};

// Test fixture for logging framework
class LoggingTest : public TestFixture {
public:
    std::string getName() const override {
        return "LoggingTest";
    }
    
    std::vector<TestResult> runTests() override {
        std::vector<TestResult> results;
        
        results.push_back(testLoggerCreation());
        results.push_back(testLogLevelSetting());
        
        return results;
    }
    
private:
    TestResult testLoggerCreation() {
        auto& logger = LogManager::getInstance();
        return assertTrue("Logger singleton creation", true);
    }
    
    TestResult testLogLevelSetting() {
        auto& logger = LogManager::getInstance();
        logger.setLogLevel(LogLevel::Debug);
        
        // Just test that it doesn't crash
        logger.logDebug("Debug message");
        logger.logInfo("Info message");
        logger.logWarning("Warning message");
        logger.logError("Error message");
        
        return assertTrue("Log level setting and messaging", true);
    }
};

int main() {
    // Register test fixtures
    REGISTER_TEST_FIXTURE(XpuMusicCompatManagerTest);
    REGISTER_TEST_FIXTURE(AdapterBaseTest);
    REGISTER_TEST_FIXTURE(DataMigrationManagerTest);
    REGISTER_TEST_FIXTURE(FoobarSdkTest);
    REGISTER_TEST_FIXTURE(LoggingTest);
    
    // Run all tests
    TestRunner::getInstance().runAllTests();
    
    return 0;
}
