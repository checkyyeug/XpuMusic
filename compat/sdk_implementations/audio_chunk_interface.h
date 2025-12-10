/**
 * @file audio_chunk_interface.h
 * @brief audio_chunk 抽象接口定义
 * @date 2025-12-09
 */

#pragma once

#include "common_includes.h"
#include <memory>

namespace foobar2000_sdk {

// 前向声明
typedef float audio_sample;

// 采样点格式
enum t_sample_point_format {
    sample_point_format_float32 = 0,
    sample_point_format_int16 = 1,
    sample_point_format_int32 = 2,
    sample_point_format_int24 = 3,
};

// 重采样模式
enum t_resample_mode {
    resample_disabled = 0,
    resample_direct = 1,
    resample_preserve_length = 2,
};

/**
 * @class audio_chunk_interface
 * @brief audio_chunk 抽象接口
 */
class audio_chunk_interface {
public:
    virtual ~audio_chunk_interface() = default;
    
    /**
     * @brief 设置音频数据（深拷贝）
     * @param p_data 音频样本数据（交错格式）
     * @param p_sample_count 样本帧数（每个通道）
     * @param p_channels 通道数
     * @param p_sample_rate 采样率
     */
    virtual void set_data(const audio_sample* p_data, size_t p_sample_count, 
                          uint32_t p_channels, uint32_t p_sample_rate) = 0;
    
    /**
     * @brief 设置音频数据（移动语义）
     * @param p_data 指向样本数据的指针（将被接管）
     * @param p_sample_count 样本帧数
     * @param p_channels 通道数
     * @param p_sample_rate 采样率
     * @param p_data_free 释放回调
     */
    virtual void set_data(const audio_sample* p_data, size_t p_sample_count,
                          uint32_t p_channels, uint32_t p_sample_rate, 
                          void (* p_data_free)(audio_sample*)) = 0;
    
    /**
     * @brief 追加音频数据到当前缓冲区
     * @param p_data 要追加的样本
     * @param p_sample_count 样本帧数
     * @param p_channels 通道数（必须匹配当前格式）
     * @param p_sample_rate 采样率（必须匹配当前格式）
     */
    virtual void data_append(const audio_sample* p_data, size_t p_sample_count) = 0;
    
    /**
     * @brief 保留缓冲区空间
     * @param p_sample_count 要保留的样本帧数
     */
    virtual void data_pad(size_t p_sample_count) = 0;
    
    /**
     * @brief 获取音频样本数据（const 版本）
     * @return 指向样本数据的指针（交错格式：LRLRLR...）
     */
    virtual const audio_sample* get_data() const = 0;
    
    /**
     * @brief 获取音频样本数据（非 const 版本）
     * @return 指向样本数据的指针
     */
    virtual audio_sample* get_data() = 0;
    
    /**
     * @brief 获取样本帧数（每个通道）
     * @return 样本帧数
     */
    virtual uint32_t get_sample_count() const = 0;
    
    /**
     * @brief 获取通道数
     * @return 通道数
     */
    virtual uint32_t get_channels() const = 0;
    
    /**
     * @brief 获取采样率
     * @return 采样率（Hz）
     */
    virtual uint32_t get_sample_rate() const = 0;
    
    /**
     * @brief 获取音频数据总大小（以样本为单位）
     * @return 所有通道的总样本数
     */
    virtual uint32_t get_data_size() const = 0;
    
    /**
     * @brief 获取音频数据大小（以字节为单位）
     * @return 字节大小
     */
    virtual size_t get_data_bytes() const = 0;
    
    /**
     * @brief 获取特定通道的数据指针
     * @param p_channel 通道索引（0 开始）
     * @return 指向该通道数据的指针
     */
    virtual audio_sample* get_channel_data(uint32_t p_channel) = 0;
    
    /**
     * @brief 获取特定通道的数据指针（const 版本）
     * @param p_channel 通道索引
     * @return 指向该通道数据的指针
     */
    virtual const audio_sample* get_channel_data(uint32_t p_channel) const = 0;
    
    /**
     * @brief 设置音频格式信息（不修改数据）
     */
    virtual void set_sample_rate(uint32_t p_sample_rate) = 0;
    
    /**
     * @brief 设置通道数
     * @param p_channels 通道数
     * @param p_preserve_data 是否为通道删除保留数据
     */
    virtual void set_channels(uint32_t p_channels, bool p_preserve_data) = 0;
    
    /**
     * @brief 设置采样率（resample 模式）
     * @param p_sample_rate 新采样率
     * @param p_mode resample 模式
     */
    virtual void set_sample_rate(uint32_t p_sample_rate, t_resample_mode p_mode) = 0;
    
    /**
     * @brief 转换样本格式（float -> int, int -> float）
     * @param p_target_format 目标格式
     */
    virtual void convert(t_sample_point_format p_target_format) = 0;
    
    /**
     * @brief 应用增益到所有样本
     * @param p_gain 线性增益值（1.0 = 无变化）
     */
    virtual void scale(const audio_sample& p_gain) = 0;
    
    /**
     * @brief 应用多个增益（每个通道不同）
     * @param p_gain 每个通道的增益数组
     */
    virtual void scale(const audio_sample* p_gain) = 0;
    
    /**
     * @brief 计算音频数据的峰值电平
     * @param p_peak 输出峰值数组（大小 = 通道数）
     */
    virtual void calculate_peak(audio_sample* p_peak) const = 0;
    
    /**
     * @brief 删除指定通道
     * @param p_channel 要删除的通道索引
     */
    virtual void remove_channel(uint32_t p_channel) = 0;
    
    /**
     * @brief 复制特定通道到新缓冲区
     * @param p_channel 源通道索引
     * @param p_target 目标 audio_chunk
     */
    virtual void copy_channel_to(uint32_t p_channel, audio_chunk_interface& p_target) const = 0;
    
    /**
     * @brief 从另一个 chunk 复制特定通道
     * @param p_channel 源通道索引
     * @param p_source 源 audio_chunk
     */
    virtual void copy_channel_from(uint32_t p_channel, const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 从另一个 chunk 复制所有通道
     * @param p_source 源 audio_chunk
     */
    virtual void duplicate(const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 从另一个 chunk 混音
     * @param p_source 要混入的源
     * @param p_count 要混音的样本数
     */
    virtual void combine(const audio_chunk_interface& p_source, size_t p_count) = 0;
    
    /**
     * @brief 深拷贝另一个 chunk 的所有数据
     * @param p_source 源 audio_chunk
     */
    virtual void copy(const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 从另一个 chunk 复制元数据（格式、状态）
     * @param p_source 源 audio_chunk
     */
    virtual void copy_meta(const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 重置所有数据（清空缓冲区）
     */
    virtual void reset() = 0;
    
    /**
     * @brief 反向遍历查找样本
     * @param p_start 起始位置
     * @return 有效的帧位置或 0
     */
    virtual uint32_t find_peaks(uint32_t p_start) const = 0;
    
    /**
     * @brief 交换两个 chunk 的内容
     * @param p_other 要交换的 chunk
     */
    virtual void swap(audio_chunk_interface& p_other) = 0;
    
    /**
     * @brief 测试两个 chunk 是否数据相等
     * @param p_other 要比较的 chunk
     * @return true 如果数据相等
     */
    virtual bool audio_data_equals(const audio_chunk_interface& p_other) const = 0;
};

} // namespace foobar2000_sdk
