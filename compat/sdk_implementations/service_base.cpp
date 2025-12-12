/**
 * @file service_base.cpp
 * @brief foobar2000 service_base basic implementation
 * @date 2025-12-09
 * 
 * This is the first critical step in implementing foobar2000 compatibility layer.
 * All foobar2000 service classes inherit from this base class, using reference counted memory management.
 */

#include "service_base.h"
#include "../xpumusic_sdk/foobar2000_sdk.h"
#include <mutex>
#include <vector>
#include <algorithm>

namespace xpumusic_sdk {

// Global service list (simplified version)
static std::mutex g_service_mutex;
static std::vector<service_factory_base*> g_service_factories;

// Forward declared functions
void service_list(t_service_list_func func, void* ctx);
void register_service_factory(service_factory_base* factory);
void unregister_service_factory(service_factory_base* factory);

void service_list(t_service_list_func func, void* ctx) {
    std::lock_guard<std::mutex> lock(g_service_mutex);
    
    for (auto* factory : g_service_factories) {
        func(factory, ctx);
    }
}

// Service registration helper function
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

} // namespace xpumusic_sdk