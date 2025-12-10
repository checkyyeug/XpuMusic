/**
 * @file service_base.cpp
 * @brief foobar2000 service_base 的基础实现
 * @date 2025-12-09
 * 
 * 这是实现 foobar2000 兼容性层的关键第一步。
 * 所有 foobar2000 服务类都从这个基类继承，使用引用计数内存管理。
 */

#include "service_base.h"
#include "../xpumusic_sdk/foobar2000_sdk.h"
#include <mutex>
#include <vector>
#include <algorithm>

namespace foobar2000_sdk {

// 全局服务列表（简化版本）
static std::mutex g_service_mutex;
static std::vector<service_factory_base*> g_service_factories;

void service_list(t_service_list_func func, void* ctx) {
    std::lock_guard<std::mutex> lock(g_service_mutex);
    
    for (auto* factory : g_service_factories) {
        func(factory, ctx);
    }
}

// 服务注册辅助函数
void register_service_factory(service_factory_base* factory) {
    std::lock_guard<std::mutex> lock(g_service_mutex);
    g_service_factories.push_back(factory);
}

void unregister_service_factory(service_factory_base* factory) {
    std::lock_guard<std::mutex> lock(g_service_mutex);
    auto it = std::find(g_service_factories.begin(), g_service_factories.end(), factory);
    if (it != g_service_factories.end()) {
        g_service_factories.erase(it);
    }
}

} // namespace foobar2000_sdk
