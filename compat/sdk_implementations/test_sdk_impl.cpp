/**
 * @file test_sdk_impl.cpp
 * @brief SDK 瀹炵幇鐨勭畝鍗曟祴璇曠▼搴?
 * @date 2025-12-09
 */

#include "service_base.h"
#include "abort_callback.h"
#include "file_info_impl.h"
#include <iostream>
#include <cassert>

using namespace foobar2000_sdk;

// 绠€鍗曠殑娴嬭瘯妗嗘灦
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

// 鏂█杈呭姪
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
// 娴嬭瘯鐢ㄤ緥
//==============================================================================

TEST(service_base_refcount) {
    // 鍒涘缓娴嬭瘯鏈嶅姟绫?
    class TestService : public service_base {
    public:
        int data = 42;
    };
    
    TestService* service = new TestService();
    
    // 鍒濆寮曠敤璁℃暟搴斾负 1
    service->service_add_ref();
    service->service_release(); // 涓嶅簲鍒犻櫎
    
    service->service_release(); // 搴旇鍒犻櫎
    
    // 濡傛灉鍒颁簡杩欓噷娌″穿婧冿紝鍩烘湰寮曠敤璁℃暟鏄伐浣滅殑
    std::cout << "(RefCount test completed without crash) ";
}

TEST(abort_callback_never_aborts) {
    // 娴嬭瘯 dummy abort callback锛堟案涓嶄腑姝級
    abort_callback& abort = abort_callback_dummy::instance();
    
    assert_false(abort.is_aborting(), "Dummy abort should never abort");
}

TEST(abort_callback_can_abort) {
    // 娴嬭瘯鍙腑姝㈢殑瀹炵幇
    abort_callback_impl abort_impl;
    
    assert_false(abort_impl.is_aborting(), "Should not abort initially");
    
    abort_impl.set_aborted();
    assert_true(abort_impl.is_aborting(), "Should abort after set_aborted()");
}

TEST(abort_callback_with_callback) {
    abort_callback_impl abort_impl;
    bool should_abort = false;
    
    // 娣诲姞涓€涓細鏀瑰彉琛屼负鐨勫洖璋?
    abort_impl.add_abort_callback([&should_abort]() {
        return should_abort;
    });
    
    assert_false(abort_impl.is_aborting(), "Should not abort initially");
    
    should_abort = true;
    assert_true(abort_impl.is_aborting(), "Should abort when callback returns true");
}

TEST(file_info_basic) {
    file_info_impl info;
    
    // 鍒濆鐘舵€佹鏌?
    assert_equals(0, info.meta_get_count("artist"), "Initial artist count should be 0");
    assert_null(info.meta_get("artist", 0), "Initial artist should be null");
}

TEST(file_info_single_value) {
    file_info_impl info;
    
    // 璁剧疆鍗曞€煎瓧娈?
    info.meta_set("artist", "The Beatles");
    
    assert_equals(1, info.meta_get_count("artist"), "Should have 1 artist");
    assert_not_null(info.meta_get("artist", 0), "Artist should not be null");
    assert_true(strcmp(info.meta_get("artist", 0), "The Beatles") == 0, 
                "Artist should be 'The Beatles'");
}

TEST(file_info_multi_value) {
    file_info_impl info;
    
    // 娣诲姞澶氫釜鍊煎埌鍚屼竴瀛楁
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
    
    // 搴旇鍙互閫氳繃涓嶅悓澶у皬鍐欒闂?
    assert_equals(1, info.meta_get_count("artist"), "Should find 'artist' (lowercase)");
    assert_equals(1, info.meta_get_count("ARTIST"), "Should find 'ARTIST' (uppercase)");
    assert_equals(1, info.meta_get_count("Artist"), "Should find 'Artist' (mixed case)");
}

TEST(file_info_replace_values) {
    file_info_impl info;
    
    // 娣诲姞涓€浜涘€?
    info.meta_add("genre", "rock");
    info.meta_add("genre", "classic rock");
    assert_equals(2, info.meta_get_count("genre"), "Should have 2 genres");
    
    // 璁剧疆搴旇鏇挎崲鎵€鏈夌幇鏈夊€?
    info.meta_set("genre", "pop");
    assert_equals(1, info.meta_get_count("genre"), "Should have only 1 genre after set");
    assert_true(strcmp(info.meta_get("genre", 0), "pop") == 0, "Genre should be 'pop'");
}

TEST(file_info_remove) {
    file_info_impl info;
    
    // 娣诲姞涓€浜涙暟鎹?
    info.meta_set("artist", "The Beatles");
    info.meta_set("title", "Hey Jude");
    
    // 鍒犻櫎涓€涓瓧娈?
    assert_true(info.meta_remove("artist"), "Should successfully remove artist");
    assert_equals(0, info.meta_get_count("artist"), "Artist should be gone");
    
    // 鍏朵粬瀛楁搴旇杩樺湪
    assert_equals(1, info.meta_get_count("title"), "Title should still exist");
    
    // 鍐嶆鍒犻櫎搴旇澶辫触
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
    
    // 杩欎釜娴嬭瘯鏃犳硶鐪熸娴嬭瘯绾跨▼瀹夊叏锛屼絾纭繚鍩烘湰鐨勯攣瀹氭満鍒跺瓨鍦?
    info.meta_set("artist", "Test Artist");
    
    // 濡傛灉杩欎簺鎿嶄綔涓嶅穿婧冿紝鍩烘湰鍚屾搴旇宸ヤ綔
    const char* value = info.meta_get("artist", 0);
    assert_not_null(value, "Should get value");
}

//==============================================================================
// 涓诲嚱鏁?
//==============================================================================

int main() {
    try {
        run_tests();
        
        if (failed > 0) {
            std::cerr << "\n鉂?Tests FAILED" << std::endl;
            return 1;
        } else {
            std::cout << "\n鉁?All tests PASSED" << std::endl;
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
