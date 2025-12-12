/**
 * @file abort_callback.h
 * @brief 绠€鍖栫増鏈殑 abort_callback 澶存枃浠? * @date 2025-12-09
 */

#pragma once

#include "../xpumusic_sdk/foobar2000_sdk.h"

// 浣跨敤 xpumusic_sdk 鍛藉悕绌洪棿涓殑绫诲瀷
using xpumusic_sdk::abort_callback;
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>

namespace foobar2000_sdk {

/**
 * @class abort_callback_impl
 * @brief abort_callback 鐨勫叿浣撳疄鐜?- 绠€鍖栫増鏈? */
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
        
        // 涓嶈淇敼鐘舵€侊紝鍙鏌?        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(callback_mutex_));
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
