/**
 * @file file_info_interface.h
 * @brief 独立的 file_info 接口定义（用于避免循环依赖）
 * @date 2025-12-09
 */

#pragma once

#include "../xpumusic_sdk/foobar2000_sdk.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>

// 前向声明 xpumusic_sdk 中的类型
namespace xpumusic_sdk {
struct audio_info;
struct file_stats;
struct field_value;
}

namespace foobar2000_sdk {

/**
 * @class file_info_interface
 * @brief file_info 抽象接口（纯虚类）
 */
class file_info_interface {
public:
    virtual ~file_info_interface() = default;
    
    // 默认构造函数
    file_info_interface() = default;
    
    /**
     * @brief 从元数据获取字段值
     * @param p_name 字段名（例如 "artist", "title"）
     * @param p_index 值索引（对于多值字段）
     * @return 字段值，如果不存在返回 nullptr
     */
    virtual const char* meta_get(const char* p_name, size_t p_index) const = 0;
    
    /**
     * @brief 获取字段的值数量
     * @param p_name 字段名
     * @return 值的数量，字段不存在返回 0
     */
    virtual size_t meta_get_count(const char* p_name) const = 0;
    
    /**
     * @brief 设置字段值（替换所有现有值）
     * @param p_name 字段名
     * @param p_value 新值
     * @return true 如果字段被修改
     */
    virtual bool meta_set(const char* p_name, const char* p_value) = 0;
    
    /**
     * @brief 移除字段的所有值
     * @param p_name 字段名
     * @return true 如果字段存在并被移除
     */
    virtual bool meta_remove(const char* p_name) = 0;
    
    /**
     * @brief 添加字段值（增加到现有值列表）
     * @param p_name 字段名
     * @param p_value 要添加的值
     * @return true 如果值成功添加
     */
    virtual bool meta_add(const char* p_name, const char* p_value) = 0;
    
    /**
     * @brief 删除字段的特定值
     * @param p_name 字段名
     * @param p_value 要删除的值
     * @return true 如果值存在并被删除
     */
    virtual bool meta_remove_value(const char* p_name, const char* p_value) = 0;
    
    /**
     * @brief 删除字段的特定索引值
     * @param p_name 字段名
     * @param p_index 值索引
     * @return true 如果值存在并被删除
     */
    virtual bool meta_remove_index(const char* p_name, size_t p_index) = 0;
    
    /**
     * @brief 获取所有字段名
     * @return 字段名向量
     */
    virtual std::vector<std::string> meta_enum_field_names() const = 0;
    
    /**
     * @brief 设置音频流信息
     * @param p_info 音频信息
     */
    virtual void set_audio_info(const xpumusic_sdk::audio_info& p_info) = 0;
    
    /**
     * @brief 获取音频流信息（const 版本）
     * @return 音频信息引用
     */
    virtual const xpumusic_sdk::audio_info& get_audio_info() const = 0;
    
    /**
     * @brief 获取音频流信息（非 const 版本）
     * @return 音频信息引用
     */
    virtual xpumusic_sdk::audio_info& get_audio_info() = 0;
    
    /**
     * @brief 设置文件统计信息（大小、时间戳）
     * @param p_stats 文件统计
     */
    virtual void set_file_stats(const xpumusic_sdk::file_stats& p_stats) = 0;
    
    /**
     * @brief 获取文件统计信息（const 版本）
     * @return 文件统计引用
     */
    virtual const xpumusic_sdk::file_stats& get_file_stats() const = 0;
    
    /**
     * @brief 获取文件统计信息（非 const 版本）
     * @return 文件统计引用
     */
    virtual xpumusic_sdk::file_stats& get_file_stats() = 0;
    
    /**
     * @brief 重置所有元数据（清空所有字段）
     */
    virtual void reset() = 0;
    
    /**
     * @brief 从另一个 file_info 对象复制所有数据
     * @param other 源对象
     */
    virtual void copy_from(const file_info_interface& other) = 0;
    
    /**
     * @brief 从另一个 file_info 对象合并非空字段
     * @param other 源对象
     * @details 保留现有值，添加缺失的字段
     */
    virtual void merge_from(const file_info_interface& other) = 0;
    
    /**
     * @brief 比较两个 file_info 对象的元数据是否相等
     * @param other 要比较的对象
     * @return true 如果元数据相等
     */
    virtual bool meta_equals(const file_info_interface& other) const = 0;
};

} // namespace foobar2000_sdk
