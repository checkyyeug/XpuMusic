/**
 * @file service_base.h
 * @brief service_base and reference counting forward declarations
 * @date 2025-12-09
 */

#pragma once

#include "../xpumusic_sdk/foobar2000_sdk.h"
#include <functional>
#include <cstdint>

namespace xpumusic_sdk {

// Forward declarations
class service_base;
class service_factory_base;

// Service callback function type definition
typedef std::function<void(service_factory_base* factory, void* ctx)> t_service_list_func;

} // namespace xpumusic_sdk