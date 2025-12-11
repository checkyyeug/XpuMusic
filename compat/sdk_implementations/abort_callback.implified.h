/**
 * @file abort_callback.h
 * @brief 简化版本的 abort_callback 头文件
 * @date 2025-12-09
 */

#pragma once

#include "../xpumusic_sdk/foobar2000_sdk.h"

// 使用 xpumusic_sdk 命名空间中的类型
using xpumusic_sdk::abort_callback;
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>

namespace foobar2000_sdk {

/**
 * @class abort_callback_impl
 * @brief abort_callback 的具体实现 - 简化版本
 */
class abort_callback_impl : public abort_callback {
private:
    std::atomic<bool> aborted_{false};
    mutable std::mutex callback_mutex_;
    std::vector<std::function<bool()>> abort_callbacks_;

public:
    abort_callback_impl() : aborted_(false) {}
    
    virtual ~abort_callback_impl() = default;
    
    bool is_aborting() const override {
        if (aborted_.load(std::memory_order_acquire)) {
            return true;
        }
        
        // 不要修改状态，只检查
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(callback_mutex_));
        for (const auto& callback : abort_callbacks_) {
            try {
                if (callback()) {
                    return true;
                }
            } catch (...) {
                return true;
            }
        }
        
        return false;
    }
    
    void set_aborted() {
        aborted_.store(true, std::memory_order_release);
    }
    
    void add_abort_callback(std::function<bool()> callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        abort_callbacks_.push_back(callback);
    }
    
    void reset() {
        aborted_.store(false, std::memory_order_release);
        std::lock_guard<std::mutex> lock(callback_mutex_);
        abort_callbacks_.clear();
    }
};

} // namespace foobar2000_sdk
