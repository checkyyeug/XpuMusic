/**
 * @file file_info_impl.h
 * @brief file_info 接口的实现，支持多值字段
 * @date 2025-12-09
 * 
 * 这是元数据兼容性的核心。foobar2000 支持多值字段和复杂元数据操作，
 * 而当前项目仅支持简单的键值对。本实现桥接这一差距。
 */

#pragma once

#include "common_includes.h"
#include "file_info_interface.h"
#include "file_info_types.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

namespace foobar2000_sdk {

/**
 * @class file_info_impl
 * @brief file_info 接口的完整实现
 * 
 * 关键特性：
 * - 支持多值字段（tagger::["artist1", "artist2"]）
 * - 内置的音频信息和文件统计
 * - 线程安全的元数据操作
 * - 与 foobar2000 查询语法的兼容性
 */
class file_info_impl : public file_info_interface {
private:
    // 元数据字段映射：字段名 -> 值列表
    std::unordered_map<std::string, field_value> meta_fields_;
    
    // 音频信息（采样率、通道数等）
    audio_info audio_info_;
    
    // 文件统计信息（大小、时间戳）
    file_stats stats_;
    
    // 互斥锁，用于线程安全操作
    mutable std::mutex mutex_;
    
    /**
     * @brief 内部辅助：获取或创建字段
     * @param name 字段名
     * @return 字段值的引用
     */
    xpumusic_sdk::field_value& get_or_create_field(const char* name);
    
    /**
     * @brief 内部辅助：规范化字段名
     * @param name 原始字段名
     * @return 规范化后的字段名
     * 
     * fooar2000 字段名通常是大小写不敏感的
     * 并且有一些别名（例如 "date" 和 "year"）
     */
    std::string normalize_field_name(const std::string& name) const;

public:
    /**
     * @brief 构造函数
     */
    file_info_impl();
    
    /**
     * @brief 析构函数
     */
    ~file_info_impl() override = default;
    
    /**
     * @brief 复制构造函数
     */
    file_info_impl(const file_info_impl& other);
    
    /**
     * @brief 赋值操作符
     */
    file_info_impl& operator=(const file_info_impl& other);
    
    //==================================================================
    // file_info 接口实现
    //==================================================================
    
    /**
     * @brief 从元数据获取字段值
     * @param p_name 字段名（例如 "artist", "title"）
     * @param p_index 值索引（对于多值字段）
     * @return 字段值，如果不存在返回 nullptr
     */
    const char* meta_get(const char* p_name, size_t p_index) const override;
    
    /**
     * @brief 获取字段的值数量
     * @param p_name 字段名
     * @return 值的数量，字段不存在返回 0
     */
    size_t meta_get_count(const char* p_name) const override;
    
    /**
     * @brief 设置字段值（替换所有现有值）
     * @param p_name 字段名
     * @param p_value 新值
     * @return true 如果字段被修改
     */
    bool meta_set(const char* p_name, const char* p_value) override;
    
    /**
     * @brief 移除字段的所有值
     * @param p_name 字段名
     * @return true 如果字段存在并被移除
     */
    bool meta_remove(const char* p_name) override;
    
    /**
     * @brief 添加字段值（增加到现有值列表）
     * @param p_name 字段名
     * @param p_value 要添加的值
     * @return true 如果值成功添加
     */                               
    bool meta_add(const char* p_name, const char* p_value) override;
    
    /**
     * @brief 删除字段的特定值
     * @param p_name 字段名
     * @param p_value 要删除的值
     * @return true 如果值存在并被删除
     */
    bool meta_remove_value(const char* p_name, const char* p_value);
    
    /**
     * @brief 删除字段的特定索引值
     * @param p_name 字段名
     * @param p_index 值索引
     * @return true 如果值存在并被删除
     */
    bool meta_remove_index(const char* p_name, size_t p_index);
    
    /**
     * @brief 获取所有字段名
     * @return 字段名向量
     */
    std::vector<std::string> meta_enum_field_names() const;
    
    //==================================================================
    // 音频信息访问
    //==================================================================
    
    /**
     * @brief 设置音频流信息
     * @param p_info 音频信息
     */
    void set_audio_info(const audio_info& p_info) override {
        std::lock_guard<std::mutex> lock(mutex_);
        audio_info_ = p_info;
    }
    
    /**
     * @brief 获取音频流信息（const 版本）
     * @return 音频信息引用
     */
    const audio_info& get_audio_info() const override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return audio_info_; 
    }
    
    /**
     * @brief 获取音频流信息（非 const 版本）
     * @return 音频信息引用
     */
    audio_info& get_audio_info() override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return audio_info_; 
    }
    
    //==================================================================
    // 文件统计信息
    //==================================================================
    
    /**
     * @brief 设置文件统计信息（大小、时间戳）
     * @param p_stats 文件统计
     */
    void set_file_stats(const file_stats& p_stats) override { 
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = p_stats; 
    }
    
    /**
     * @brief 获取文件统计信息（const 版本）
     * @return 文件统计引用
     */
    const file_stats& get_file_stats() const override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_; 
    }
    
    /**
     * @brief 获取文件统计信息（非 const 版本）
     * @return 文件统计引用
     */
    file_stats& get_file_stats() override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_; 
    }
    
    /**
     * @brief 重置所有元数据（清空所有字段）
     */
    void reset();
    
    /**
     * @brief 从另一个 file_info 对象复制所有数据
     * @param other 源对象
     */
    void copy_from(const file_info_interface& other) override;
    
    /**
     * @brief 从另一个 file_info 对象合并非空字段
     * @param other 源对象
     * @details 保留现有值，添加缺失的字段
     */
    void merge_from(const file_info_interface& other) override;
    
    /**
     * @brief 比较两个 file_info 对象的元数据是否相等
     * @param other 要比较的对象
     * @return true 如果元数据相等
     */
    bool meta_equals(const file_info_interface& other) const override;
    
    /**
     * @brief 获取字节大小（用于调试和序列化）
     * @return 对象的总大小（近似值）
     */
    size_t get_approximate_size() const;
};

// 简化操作辅助函数
std::unique_ptr<file_info_impl> file_info_create();

// 从增强元数据创建 file_info（桥接函数）
std::unique_ptr<file_info_impl> file_info_from_metadata(
    const class EnhancedTrackInfo& track_info);

} // namespace foobar2000_sdk
