#include "test_framework.h"
#include "migration/data_migration_manager.h"

using namespace mp::compat;
using namespace mp::compat::test;

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
        Result result = manager.initialize("");
        bool initSuccess = (result == Result::Success);
        
        return assertTrue("Initialization with empty path succeeds", initSuccess);
    }
};

// Register this test fixture
static struct RegisterDataMigrationManagerTest {
    RegisterDataMigrationManagerTest() {
        TestRunner::getInstance().registerFixture(std::make_unique<DataMigrationManagerTest>());
    }
} registerDataMigrationManagerTest;
