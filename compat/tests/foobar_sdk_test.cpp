#include "test_framework.h"
#include "xpumusic_sdk/foobar2000_sdk.h"
#include "xpumusic_sdk/input_decoder.h"

using namespace mp::compat::test;
using namespace xpumusic_sdk;

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
        results.push_back(testFileInfo());
        
        return results;
    }
    
private:
    TestResult testServicePtr() {
        // Test service_ptr_t with a simple class
        class TestService : public service_base {
        public:
            int service_add_ref() override { ref_count_++; return ref_count_; }
            int service_release() override { ref_count_--; return ref_count_; }
        private:
            int ref_count_ = 0;
        };
        
        // Test construction
        service_ptr_t<TestService> ptr1;
        bool nullOnConstruct = ptr1.is_empty();
        
        // Test assignment
        service_ptr_t<TestService> ptr2(new TestService());
        bool notNullAfterAssign = !ptr2.is_empty();
        
        // Test copy constructor
        service_ptr_t<TestService> ptr3(ptr2);
        bool copyWorks = !ptr3.is_empty();
        
        // Test destruction (should not crash)
        ptr2.release();
        bool releaseWorks = ptr2.is_empty();
        
        bool success = nullOnConstruct && notNullAfterAssign && copyWorks && releaseWorks;
        return assertTrue("Service pointer operations", success);
    }
    
    TestResult testAudioChunk() {
        audio_chunk chunk;
        
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
    
    TestResult testFileInfo() {
        file_info info;
        
        // Test default values
        bool defaultValues = (info.length == 0.0) &&
                           (info.sample_rate == 0) &&
                           (info.channels == 0) &&
                           (info.bitrate == 0) &&
                           (info.codec == nullptr) &&
                           (info.tags == nullptr) &&
                           (info.tag_count == 0);
        
        return assertTrue("File info default values", defaultValues);
    }
};

// Register this test fixture
static struct RegisterFoobarSdkTest {
    RegisterFoobarSdkTest() {
        TestRunner::getInstance().registerFixture(std::make_unique<FoobarSdkTest>());
    }
} registerFoobarSdkTest;
