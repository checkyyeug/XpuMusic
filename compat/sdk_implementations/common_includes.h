/**
 * @file common_includes.h
 * @brief 鍏叡澶存枃浠跺寘鍚? * @date 2025-12-09
 */

#pragma once

// 鏍囧噯搴撳ご鏂囦欢
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

// XpuMusic SDK 澶存枃浠?#include "../xpumusic_sdk/foobar2000_sdk.h"

// 鏈湴绫诲瀷瀹氫箟
#include "audio_sample.h"
#include "file_info_types.h"

// Result 绫诲瀷瀹氫箟
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

