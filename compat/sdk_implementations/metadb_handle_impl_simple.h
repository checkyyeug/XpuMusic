/**
 * @file metadb_handle_impl_simple.h
 * @brief 简化的 metadb_handle 实现
 * @date 2025-12-11
 * 
 * 这是 metadb_handle 的最小化实现，只提供基本功能以通过编译。
 */

#pragma once

#include "common_includes.h"
#include "metadb_handle_interface.h"
#include "file_info_impl.h"
#include "metadb_handle_types.h"
#include <memory>
#include <string>
#include <mutex>

namespace foobar2000_sdk {

/**
 * @class metadb_handle_impl_simple
 * @brief metadb_handle 的简化实现
 * 
 * 只提供最基本的功能，确保项目能编译通过。
 */
class metadb_handle_impl_simple : public metadb_handle_interface {
private:
    // 文件位置
    playable_location location_;
    
    // 缓存的元数据
    std::unique_ptr<file_info_impl> info_;
    
    // 文件统计信息
    xpumusic_sdk::file_stats file_stats_;
    
    // 互斥锁，用于线程安全
    mutable std::mutex mutex_;
    
public:
    /**
     * @brief 构造函数
     */
    metadb_handle_impl_simple() : metadb_handle_interface() {
        info_ = std::make_unique<file_info_impl>();
        file_stats_.m_size = 0;
        file_stats_.m_timestamp = 0;
    }
    
    /**
     * @brief 析构函数
     */
    ~metadb_handle_impl_simple() override = default;
    
    /**
     * @brief 用 playable_location 初始化
     * @param loc 文件位置
     */
    void initialize(const playable_location& loc) {
        std::lock_guard<std::mutex> lock(mutex_);
        location_ = loc;
    }
    
    /**
     * @brief 获取文件位置（const 版本）
     * @return 位置的指针
     */
    const playable_location& get_location() const override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return location_; 
    }
    
    /**
     * @brief 获取元数据（const 版本）
     * @return file_info 的引用
     */
    const file_info_interface& get_info() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return *info_;
    }
    
    /**
     * @brief 获取元数据（非 const 版本）
     * @return file_info 的引用
     */
    file_info_interface& get_info() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return *info_;
    }
    
    /**
     * @brief 从实时文件获取元数据（简化版本）
     * @param p_info 输出 file_info
     * @param p_abort 中止回调
     * @param p_can_expire 是否允许过期缓存
     */
    void get_info_async(file_info_interface& p_info, xpumusic_sdk::abort_callback& p_abort, 
                       bool p_can_expire = false) const override {
        // 简化实现：直接复制当前信息
        std::lock_guard<std::mutex> lock(mutex_);
        p_info.copy_from(*info_);
    }
    
    /**
     * @brief 更新元数据
     * @param p_info 新的元数据
     * @param p_abort 中止回调
     */
    void update_info(const file_info_interface& p_info, xpumusic_sdk::abort_callback& p_abort) override {
        std::lock_guard<std::mutex> lock(mutex_);
        info_->copy_from(p_info);
    }
    
    /**
     * @brief 从文件刷新元数据（简化版本）
     * @param p_abort 中止回调
     */
    void refresh_info(xpumusic_sdk::abort_callback& p_abort) override {
        // 简化实现：什么都不做
        // 在实际实现中，这里会重新扫描文件
    }
    
    /**
     * @brief 获取文件统计信息
     * @return 文件统计
     */
    const xpumusic_sdk::file_stats& get_file_stats() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return file_stats_;
    }
    
    /**
     * @brief 获取路径
     * @return 文件路径
     */
    std::string get_path() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return location_.get_path();
    }
    
    /**
     * @brief 获取文件名
     * @return 文件名
     */
    std::string get_filename() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string path = location_.get_path();
        size_t pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }
    
    /**
     * @brief 获取目录
     * @return 目录路径
     */
    std::string get_directory() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string path = location_.get_path();
        size_t pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(0, pos) : "";
    }
    
    /**
     * @brief 获取位置哈希
     * @return 位置的哈希值
     */
    uint64_t get_location_hash() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::hash<std::string> hasher;
        return hasher(location_.get_path()) ^ (location_.get_subsong_index() << 32);
    }
    
    /**
     * @brief 检查是否与另一个 handle 相同
     * @param other 另一个 handle
     * @return true 如果相同
     */
    bool is_same(const metadb_handle_interface& other) const override {
        return get_location_hash() == other.get_location_hash();
    }
    
    /**
     * @brief 检查是否有效
     * @return true 如果有效
     */
    bool is_valid() const override {
        return !location_.get_path().empty();
    }
    
    /**
     * @brief 重新加载（简化版本）
     * @param p_abort 中止回调
     */
    void reload(xpumusic_sdk::abort_callback& p_abort) override {
        refresh_info(p_abort);
    }
    
    /**
     * @brief 引用计数增加（简化版本）
     * 在完整实现中，这会使用实际的引用计数
     */
    void ref_add_ref() override {
        // 简化版本：什么都不做
    }
    
    /**
     * @brief 引用计数释放（简化版本）
     * 在完整实现中，这会减少引用计数并在需要时删除对象
     */
    void ref_release() override {
        // 简化版本：什么都不做
    }
};

// 类型别名，保持兼容性
typedef metadb_handle_impl_simple metadb_handle_impl;

} // namespace foobar2000_sdk