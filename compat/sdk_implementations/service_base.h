/**
 * @file service_base.h
 * @brief service_base 和引用计数的前向声明
 * @date 2025-12-09
 */

#pragma once

#include "../xpumusic_sdk/foobar2000_sdk.h"
#include <functional>
#include <cstdint>

namespace foobar2000_sdk {

// 前向声明
class service_base;
class service_factory_base;

// GUID 结构 is defined in foobar2000_sdk.h, using that definition
// We only add comparison operators here if needed

// 服务回调函数类型 definition
typedef std::function<void(service_factory_base* factory, void* ctx)> t_service_list_func;

// 全局服务注册辅助函数（前向声明）
// 将在后续提交中与服务注册中心集成
void service_list(t_service_list_func func, void* ctx);

} // namespace foobar2000_sdk
