#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <functional>

namespace mp {
namespace compat {
namespace test {

// Test result
struct TestResult {
    bool passed;
    std::string name;
    std::string message;
    
    TestResult(bool p, const std::string& n, const std::string& m = "")
        : passed(p), name(n), message(m) {}
};

// Test fixture base class
class TestFixture {
public:
    virtual ~TestFixture() = default;
    
    // Get test fixture name
    virtual std::string getName() const = 0;
    
    // Run all tests in this fixture
    virtual std::vector<TestResult> runTests() = 0;
    
protected:
    // Helper methods for assertions
    TestResult assertEqual(const std::string& testName, int expected, int actual) {
        if (expected == actual) {
            return TestResult(true, testName);
        } else {
            return TestResult(false, testName, 
                "Expected: " + std::to_string(expected) + 
                ", Actual: " + std::to_string(actual));
        }
    }
    
    TestResult assertTrue(const std::string& testName, bool condition) {
        if (condition) {
            return TestResult(true, testName);
        } else {
            return TestResult(false, testName, "Condition was false");
        }
    }
    
    TestResult assertFalse(const std::string& testName, bool condition) {
        if (!condition) {
            return TestResult(true, testName);
        } else {
            return TestResult(false, testName, "Condition was true");
        }
    }
};

// Test runner
class TestRunner {
public:
    static TestRunner& getInstance() {
        static TestRunner instance;
        return instance;
    }
    
    // Register a test fixture
    void registerFixture(std::unique_ptr<TestFixture> fixture) {
        fixtures_.push_back(std::move(fixture));
    }
    
    // Run all registered tests
    void runAllTests() {
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        
        std::cout << "Running compatibility layer tests..." << std::endl;
        std::cout << "=====================================" << std::endl;
        
        for (auto& fixture : fixtures_) {
            std::cout << "\nFixture: " << fixture->getName() << std::endl;
            std::cout << "-------------------------------------" << std::endl;
            
            auto results = fixture->runTests();
            totalTests += results.size();
            
            for (const auto& result : results) {
                if (result.passed) {
                    std::cout << "  PASS: " << result.name << std::endl;
                    passedTests++;
                } else {
                    std::cout << "  FAIL: " << result.name << " - " << result.message << std::endl;
                    failedTests++;
                }
            }
        }
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "Test Results:" << std::endl;
        std::cout << "  Total:  " << totalTests << std::endl;
        std::cout << "  Passed: " << passedTests << std::endl;
        std::cout << "  Failed: " << failedTests << std::endl;
        std::cout << "  Success Rate: " << (totalTests > 0 ? (passedTests * 100 / totalTests) : 0) << "%" << std::endl;
    }
    
private:
    TestRunner() = default;
    
    std::vector<std::unique_ptr<TestFixture>> fixtures_;
};

// Macro for easy test registration
#define REGISTER_TEST_FIXTURE(FixtureClass) \
    ::mp::compat::test::TestRunner::getInstance().registerFixture(std::make_unique<FixtureClass>());

} // namespace test
} // namespace compat
} // namespace mp
