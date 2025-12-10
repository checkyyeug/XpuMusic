/**
 * @file common_includes.h
 * @brief 公共头文件包含
 * @date 2025-12-09
 */

#pragma once

// 标准库头文件
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>

// XpuMusic SDK 头文件
#include "../xpumusic_sdk/foobar2000_sdk.h"

// 本地类型定义
#include "audio_sample.h"
#include "file_info_types.h"

// Result 类型定义
namespace xpumusic_sdk {
enum class Result {
    Success = 0,
    Failure = 1,
    FileNotFound = 2,
    InvalidParameter = 3,
    NotImplemented = 4,
    AlreadyExists = 5,
    OutOfMemory = 6
};
} // namespace foobar2000_sdk

