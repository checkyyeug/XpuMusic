/**
 * @file metadb_handle_impl.h
 * @brief metadb_handle 接口实现
 * @date 2025-12-09
 * 
 * metadb_handle 是 foobar2000 元数据系统的核心，表示单个音频文件的
 * 元数据和统计信息。它同时支持实时和持久化元数据。
 */

#pragma once

#include "common_includes.h"
#include "metadb_handle_interface.h"
#include "file_info_impl.h"
#include "file_info_types.h"
#include "../../sdk/headers/mp_types.h"
#include <memory>
#include <string>
#include <chrono>
#include <mutex>

namespace foobar2000_sdk {

// 前向声明
class metadb_impl;

/**
 * @class metadb_handle_impl
 * @brief metadb_handle 接口的实现
 * 
 * metadb_handle 表示单个音频文件的持久化元数据和实时状态。
 * 它支持：
 * - 多值元数据字段
 * - 播放统计（播放次数、最后播放等）
 * - 动态元数据更新
 * - 文件标识和验证
 */
class metadb_handle_impl : public metadb_handle_interface {
private:
    // 文件位置
    playable_location location_;
    
    // 缓存的元数据（可更新）
    std::unique_ptr<file_info_impl> info_;
    
    // 播放统计（使用接口中定义的结构）
    TrackStatistics stats_;
    
    // 文件标识（用于检测文件变化）
    file_stats file_stats_;
    
    // 指向 metadb 的指针（用于反向引用）
    metadb_impl* parent_db_ = nullptr;
    
    // 互斥锁，用于线程安全
    mutable std::mutex mutex_;
    
    // 是否已初始化标志
    bool initialized_ = false;
    
    // 加载元数据从文件
    Result load_metadata_from_file();
    
public:
    /**
     * @brief 构造函数
     */
    metadb_handle_impl();
    
    /**
     * @brief 析构函数
         */
    ~metadb_handle_impl() override = default;
    
    /**
     * @brief 用 playable_location 初始化
     * @param loc 文件位置
     * @param parent 父 metadb（可选）
     */
    Result initialize(const playable_location& loc, metadb_impl* parent = nullptr);
    
    /**
     * @brief 重置状态（清除所有数据）
     */
    void reset();
    
    //==========================================================================
    // metadb_handle 接口实现
    //==========================================================================
    
    /**
     * @brief 获取文件位置（const 版本）
     * @return 位置的指针
     */
    const playable_location& get_location() const override { return location_; }
    
    /**
     * @brief 获取元数据（const 版本）
     * @return file_info 的引用
     * 
     * @note 多线程调用时线程安全
     * @note 返回对象的内部数据可能被其他调用修改
     */
    const file_info_interface& get_info() const override;
    
    /**
     * @brief 获取元数据（非 const 版本）
     * @return file_info 的引用
     */
    file_info_interface& get_info() override;
    
    /**
     * @brief 从实时文件获取元数据（可能扫描）
     * @param p_info 输出 file_info
     * @param p_abort 中止回调
     * @param p_can_expire 是否允许过期缓存
     */
    void get_info_async(file_info_interface& p_info, abort_callback& p_abort, 
                       bool p_can_expire = false) const override;
    
    //==========================================================================
    // 统计信息访问
    //==========================================================================
    
    /**
     * @brief 获取播放统计（const 版本）
     * @return TrackStatistics 的引用
     */
    const TrackStatistics& get_statistics() const { return stats_; }
    
    /**
     * @brief 获取播放统计（非 const 版本）
     * @return TrackStatistics 的引用
     */
    TrackStatistics& get_statistics() { return stats_; }
    
    /**
     * @brief 更新元数据
     * @param p_info 新的元数据
     * @param p_abort 中止回调
     * 
     * @note 这会修改内部 file_info 并可能写入数据库
     */
    void update_info(const file_info_interface& p_info, abort_callback& p_abort) override;
    
    /**
     * @brief 从文件刷新元数据（如果文件已更改）
     * @param p_abort 中止回调
     */
    void refresh_info(abort_callback& p_abort) override;
    
    //==========================================================================
    // 实用函数
    //==========================================================================
    
    /**
     * @brief 获取文件名（从位置提取）
     * @return 文件名
     */
    std::string get_filename() const;
    
    /**
     * @brief 获取目录路径
     * @return 目录路径
     */
    std::string get_directory() const;
    
    /**
     * @brief 检查文件是否存在并可访问
     * @return true 如果文件可访问
     */
    bool is_file_valid() const;
    
    /**
     * @brief 设置父 metadb
     * @param parent 父 metadb_impl
     */
    void set_parent(metadb_impl* parent) { parent_db_ = parent; }
    
    /**
     * @brief 获取父 metadb
     * @return 父 metadb_impl 指针
     */
    metadb_impl* get_parent() const { return parent_db_; }
    
    /**
     * @brief 获取唯一标识字符串
     * @return 文件的唯一标识
     */
    std::string get_identifier() const;
    
    /**
     * @brief 检查两个 handle 是否指向同一文件
     * @param other 另一个 metadb_handle
     * @return true 如果指向同一文件
     */
    bool equals(const metadb_handle_impl& other) const;
};

// 辅助比较函数
inline bool operator==(const metadb_handle_impl& a, const metadb_handle_impl& b) {
    return a.equals(b);
}

inline bool operator!=(const metadb_handle_impl& a, const metadb_handle_impl& b) {
    return !a.equals(b);
}

// 辅助函数：创建 metadb_handle
std::unique_ptr<metadb_handle_impl> metadb_handle_create(const playable_location& loc);

} // namespace foobar2000_sdk
