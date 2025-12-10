/**
 * @file test_plugin_loader.cpp
 * @brief 插件加载器测试
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
// 测试用例
//==============================================================================

TEST(loader_create) {
    // 创建兼容性管理器
    XpuMusicCompatManager compat_manager;
    
    // 创建加载器
    XpuMusicPluginLoader loader(&compat_manager);
    
    assert_true(loader.get_module_count() == 0, "Initially no modules loaded");
}

TEST(wrapper_create) {
    // 创建 mock factory
    // 注意：在真实测试中需要实际的 foobar2000 插件
    service_factory_base* mock_factory = nullptr;
    
    ServiceFactoryWrapper wrapper(mock_factory);
    
    assert_true(wrapper.get_original_factory() == nullptr, "Should store factory");
}

TEST(registry_bridge_create) {
    // 创建 mock ServiceRegistry
    // 注意：在真实测试中需要实际的 ServiceRegistry
    mp::core::ServiceRegistry* mock_registry = nullptr;
    
    ServiceRegistryBridgeImpl bridge(mock_registry);
    
    assert_true(bridge.get_service_count() == 0, "Initially no services");
}

//==============================================================================
// 注意：这些是基础架构测试
// 实际插件测试需要真实的 foobar2000 DLL 文件
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
// 主函数
//==============================================================================

int main() {
    try {
        run_tests();
        
        std::cout << "\nNote: These are basic architecture tests." << std::endl;
        std::cout << "Full plugin loading tests require actual foobar2000 DLLs." << std::endl;
        
        if (failed > 0) {
            std::cerr << "\n❌ Plugin loader tests FAILED" << std::endl;
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
