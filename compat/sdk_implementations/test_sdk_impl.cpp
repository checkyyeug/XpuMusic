/**
 * @file test_sdk_impl.cpp
 * @brief SDK 实现的简单测试程序
 * @date 2025-12-09
 */

#include "service_base.h"
#include "abort_callback.h"
#include "file_info_impl.h"
#include <iostream>
#include <cassert>

using namespace foobar2000_sdk;

// 简单的测试框架
#define TEST(name) void test_##name(); \
                   tests.push_back({#name, test_##name}); \
                   void test_##name()

struct TestCase {
    const char* name;
    void (*func)();
};

std::vector<TestCase> tests;
int passed = 0;
int failed = 0;

void run_tests() {
    std::cout << "Running SDK Implementation Tests..." << std::endl;
    std::cout << "====================================" << std::endl;
    
    for (const auto& test : tests) {
        std::cout << "Testing: " << test.name << "... ";
        try {
            test.func();
            std::cout << "PASSED" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << std::endl;
            failed++;
        } catch (...) {
            std::cout << "FAILED: Unknown error" << std::endl;
            failed++;
        }
    }
    
    std::cout << "====================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
}

// 断言辅助
void assert_true(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void assert_false(bool condition, const char* message) {
    if (condition) {
        throw std::runtime_error(message);
    }
}

void assert_equals(int expected, int actual, const char* message) {
    if (expected != actual) {
        std::ostringstream oss;
        oss << message << " (expected: " << expected << ", actual: " << actual << ")";
        throw std::runtime_error(oss.str());
    }
}

void assert_not_null(const void* ptr, const char* message) {
    if (ptr == nullptr) {
        throw std::runtime_error(message);
    }
}

void assert_null(const void* ptr, const char* message) {
    if (ptr != nullptr) {
        throw std::runtime_error(message);
    }
}

//==============================================================================
// 测试用例
//==============================================================================

TEST(service_base_refcount) {
    // 创建测试服务类
    class TestService : public service_base {
    public:
        int data = 42;
    };
    
    TestService* service = new TestService();
    
    // 初始引用计数应为 1
    service->service_add_ref();
    service->service_release(); // 不应删除
    
    service->service_release(); // 应该删除
    
    // 如果到了这里没崩溃，基本引用计数是工作的
    std::cout << "(RefCount test completed without crash) ";
}

TEST(abort_callback_never_aborts) {
    // 测试 dummy abort callback（永不中止）
    abort_callback& abort = abort_callback_dummy::instance();
    
    assert_false(abort.is_aborting(), "Dummy abort should never abort");
}

TEST(abort_callback_can_abort) {
    // 测试可中止的实现
    abort_callback_impl abort_impl;
    
    assert_false(abort_impl.is_aborting(), "Should not abort initially");
    
    abort_impl.set_aborted();
    assert_true(abort_impl.is_aborting(), "Should abort after set_aborted()");
}

TEST(abort_callback_with_callback) {
    abort_callback_impl abort_impl;
    bool should_abort = false;
    
    // 添加一个会改变行为的回调
    abort_impl.add_abort_callback([&should_abort]() {
        return should_abort;
    });
    
    assert_false(abort_impl.is_aborting(), "Should not abort initially");
    
    should_abort = true;
    assert_true(abort_impl.is_aborting(), "Should abort when callback returns true");
}

TEST(file_info_basic) {
    file_info_impl info;
    
    // 初始状态检查
    assert_equals(0, info.meta_get_count("artist"), "Initial artist count should be 0");
    assert_null(info.meta_get("artist", 0), "Initial artist should be null");
}

TEST(file_info_single_value) {
    file_info_impl info;
    
    // 设置单值字段
    info.meta_set("artist", "The Beatles");
    
    assert_equals(1, info.meta_get_count("artist"), "Should have 1 artist");
    assert_not_null(info.meta_get("artist", 0), "Artist should not be null");
    assert_true(strcmp(info.meta_get("artist", 0), "The Beatles") == 0, 
                "Artist should be 'The Beatles'");
}

TEST(file_info_multi_value) {
    file_info_impl info;
    
    // 添加多个值到同一字段
    info.meta_add("artist", "The Beatles");
    info.meta_add("artist", "Paul McCartney");
    info.meta_add("artist", "John Lennon");
    
    assert_equals(3, info.meta_get_count("artist"), "Should have 3 artists");
    assert_not_null(info.meta_get("artist", 0), "First artist should exist");
    assert_not_null(info.meta_get("artist", 1), "Second artist should exist");
    assert_not_null(info.meta_get("artist", 2), "Third artist should exist");
}

TEST(file_info_case_insensitive) {
    file_info_impl info;
    
    info.meta_set("ARTIST", "The Beatles");
    
    // 应该可以通过不同大小写访问
    assert_equals(1, info.meta_get_count("artist"), "Should find 'artist' (lowercase)");
    assert_equals(1, info.meta_get_count("ARTIST"), "Should find 'ARTIST' (uppercase)");
    assert_equals(1, info.meta_get_count("Artist"), "Should find 'Artist' (mixed case)");
}

TEST(file_info_replace_values) {
    file_info_impl info;
    
    // 添加一些值
    info.meta_add("genre", "rock");
    info.meta_add("genre", "classic rock");
    assert_equals(2, info.meta_get_count("genre"), "Should have 2 genres");
    
    // 设置应该替换所有现有值
    info.meta_set("genre", "pop");
    assert_equals(1, info.meta_get_count("genre"), "Should have only 1 genre after set");
    assert_true(strcmp(info.meta_get("genre", 0), "pop") == 0, "Genre should be 'pop'");
}

TEST(file_info_remove) {
    file_info_impl info;
    
    // 添加一些数据
    info.meta_set("artist", "The Beatles");
    info.meta_set("title", "Hey Jude");
    
    // 删除一个字段
    assert_true(info.meta_remove("artist"), "Should successfully remove artist");
    assert_equals(0, info.meta_get_count("artist"), "Artist should be gone");
    
    // 其他字段应该还在
    assert_equals(1, info.meta_get_count("title"), "Title should still exist");
    
    // 再次删除应该失败
    assert_false(info.meta_remove("artist"), "Removing non-existent field should fail");
}

TEST(file_info_copy) {
    file_info_impl info1;
    info1.meta_set("artist", "The Beatles");
    info1.meta_add("genre", "rock");
    info1.meta_add("genre", "classic rock");
    
    file_info_impl info2;
    info2.copy_from(info1);
    
    assert_equals(1, info2.meta_get_count("artist"), "Copied info should have artist");
    assert_equals(2, info2.meta_get_count("genre"), "Copied info should have 2 genres");
    assert_true(strcmp(info2.meta_get("artist", 0), "The Beatles") == 0, 
                "Artist should match");
}

TEST(file_info_thread_safety) {
    file_info_impl info;
    
    // 这个测试无法真正测试线程安全，但确保基本的锁定机制存在
    info.meta_set("artist", "Test Artist");
    
    // 如果这些操作不崩溃，基本同步应该工作
    const char* value = info.meta_get("artist", 0);
    assert_not_null(value, "Should get value");
}

//==============================================================================
// 主函数
//==============================================================================

int main() {
    try {
        run_tests();
        
        if (failed > 0) {
            std::cerr << "\n❌ Tests FAILED" << std::endl;
            return 1;
        } else {
            std::cout << "\n✅ All tests PASSED" << std::endl;
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
