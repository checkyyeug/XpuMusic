/**
 * @file test_plugin_loader.cpp
 * @brief 鎻掍欢鍔犺浇鍣ㄦ祴璇?
 * @date 2025-12-09
 */

#include "plugin_loader.h"
#include "../foobar_compat_manager.h"
#include <iostream>

using namespace mp::compat;
using namespace xpumusic_sdk;

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
    std::cout << "Running PluginLoader Tests..." << std::endl;
    std::cout << "=============================" << std::endl;
    
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
    
    std::cout << "=============================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
}

void assert_true(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

//==============================================================================
// 娴嬭瘯鐢ㄤ緥
//==============================================================================

TEST(loader_create) {
    // 鍒涘缓鍏煎鎬х鐞嗗櫒
    XpuMusicCompatManager compat_manager;
    
    // 鍒涘缓鍔犺浇鍣?
    XpuMusicPluginLoader loader(&compat_manager);
    
    assert_true(loader.get_module_count() == 0, "Initially no modules loaded");
}

TEST(wrapper_create) {
    // 鍒涘缓 mock factory
    // 娉ㄦ剰锛氬湪鐪熷疄娴嬭瘯涓渶瑕佸疄闄呯殑 foobar2000 鎻掍欢
    service_factory_base* mock_factory = nullptr;
    
    ServiceFactoryWrapper wrapper(mock_factory);
    
    assert_true(wrapper.get_original_factory() == nullptr, "Should store factory");
}

TEST(registry_bridge_create) {
    // 鍒涘缓 mock ServiceRegistry
    // 娉ㄦ剰锛氬湪鐪熷疄娴嬭瘯涓渶瑕佸疄闄呯殑 ServiceRegistry
    mp::core::ServiceRegistry* mock_registry = nullptr;
    
    ServiceRegistryBridgeImpl bridge(mock_registry);
    
    assert_true(bridge.get_service_count() == 0, "Initially no services");
}

//==============================================================================
// 娉ㄦ剰锛氳繖浜涙槸鍩虹鏋舵瀯娴嬭瘯
// 瀹為檯鎻掍欢娴嬭瘯闇€瑕佺湡瀹炵殑 foobar2000 DLL 鏂囦欢
//==============================================================================

#ifdef _WIN32
TEST(abort_with_null) {
    XpuMusicCompatManager compat_manager;
    XpuMusicPluginLoader loader(&compat_manager);
    
    Result result = loader.load_plugin(nullptr);
    assert_true(result == Result::InvalidParameter, "Should reject null path");
}

TEST(unload_with_null) {
    XpuMusicCompatManager compat_manager;
    XpuMusicPluginLoader loader(&compat_manager);
    
    Result result = loader.unload_plugin(nullptr);
    assert_true(result == Result::InvalidParameter, "Should reject null path");
}
#endif

//==============================================================================
// 涓诲嚱鏁?
//==============================================================================

int main() {
    try {
        run_tests();
        
        std::cout << "\nNote: These are basic architecture tests." << std::endl;
        std::cout << "Full plugin loading tests require actual foobar2000 DLLs." << std::endl;
        
        if (failed > 0) {
            std::cerr << "\n鉂?Plugin loader tests FAILED" << std::endl;
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
