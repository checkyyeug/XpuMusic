/**
 * @file audio_chunk_impl.h
 * @brief audio_chunk 接口实现
 * @date 2025-12-09
 * 
 * audio_chunk 是 foobar2000 音频处理管道的核心数据结构。
 * 它将音频数据和格式信息一起封装，便于 DSP 处理。
 */

#pragma once

#include "common_includes.h"
#include "audio_chunk_interface.h"
#include "audio_sample.h"
#include "file_info_types.h"
#include <memory>
#include <vector>
#include <cstring>

namespace foobar2000_sdk {

/**
 * @class audio_chunk_impl
 * @brief audio_chunk 接口的完整实现
 * 
 * 关键特性：
 * - 可调整大小的音频数据缓冲区
 * - 多种音频格式支持（PCM int/float）
 * - 通道重映射和删除
 * - 采样率转换（使用 DSP）
 * - 增益应用和峰值扫描
 */
class audio_chunk_impl : public audio_chunk_interface {
private:
    /**
     * @struct buffer_t
     * @brief 内部音频数据缓冲区
     */
    struct buffer_t {
        std::vector<audio_sample> data;  // 使用 vector 管理内存
        uint32_t sample_count;           // 音频样本的帧数
        uint32_t channels;               // 音频通道数
        uint32_t sample_rate;            // 采样率
        
        buffer_t() : sample_count(0), channels(0), sample_rate(0) {}
        
        /**
         * @brief 设置缓冲区大小
         * @param new_size 新的大小（样本数）
         */
        void resize(size_t new_size) {
            data.resize(new_size);
            sample_count = static_cast<uint32_t>(new_size / (channels > 0 ? channels : 1));
        }
        
        /**
         * @brief 设置格式信息
         */
        void set_format(uint32_t rate, uint32_t ch) {
            sample_rate = rate;
            channels = ch;
        }
    };
    
    buffer_t buffer_;  // 音频数据缓冲区

public:
    /**
     * @brief 构造函数
     */
    audio_chunk_impl();
    
    /**
     * @brief 析构函数
     */
    ~audio_chunk_impl() override = default;
    
    /**
     * @brief 设置音频数据（深拷贝）
     * @param p_data 音频样本数据（交错格式）
     * @param p_sample_count 样本帧数（每个通道）
     * @param p_channels 通道数
     * @param p_sample_rate 采样率
     */
    void set_data(const audio_sample* p_data, size_t p_sample_count, 
                  uint32_t p_channels, uint32_t p_sample_rate) override;
    
    /**
     * @brief 设置音频数据（移动语义）
     * @param p_data 指向样本数据的指针（将被接管）
     * @param p_sample_count 样本帧数
     * @param p_channels 通道数
     * @param p_sample_rate 采样率
     * @param p_data_free 释放回调
     */
    void set_data(const audio_sample* p_data, size_t p_sample_count,
                  uint32_t p_channels, uint32_t p_sample_rate, 
                  void (* p_data_free)(audio_sample*)) override;
    
    /**
     * @brief 追加音频数据到当前缓冲区
     * @param p_data 要追加的样本
     * @param p_sample_count 样本帧数
     * @param p_channels 通道数（必须匹配当前格式）
     * @param p_sample_rate 采样率（必须匹配当前格式）
     */
    void data_append(const audio_sample* p_data, size_t p_sample_count) override;
    
    /**
     * @brief 保留缓冲区空间
     * @param p_sample_count 要保留的样本帧数
     */
    void data_pad(size_t p_sample_count) override;
    
    /**
     * @brief 获取音频样本数据（const 版本）
     * @return 指向样本数据的指针（交错格式：LRLRLR...）
     */
    audio_sample* get_data() override { return buffer_.data.data(); }
    
    /**
     * @brief 获取音频样本数据（非 const 版本）
     * @return 指向样本数据的指针
     */
    const audio_sample* get_data() const override { return buffer_.data.data(); }
    
    /**
     * @brief 获取样本帧数（每个通道）
     * @return 样本帧数
     */
    uint32_t get_sample_count() const override { return buffer_.sample_count; }
    
    /**
     * @brief 获取通道数
     * @return 通道数
     */
    uint32_t get_channels() const override { return buffer_.channels; }
    
    /**
     * @brief 获取采样率
     * @return 采样率（Hz）
     */
    uint32_t get_sample_rate() const override { return buffer_.sample_rate; }
    
    /**
     * @brief 获取音频数据总大小（以样本为单位）
     * @return 所有通道的总样本数
     */
    uint32_t get_data_size() const override { 
        return buffer_.sample_count * buffer_.channels; 
    }
    
    /**
     * @brief 获取音频数据大小（以字节为单位）
     * @return 字节大小
     */
    size_t get_data_bytes() const override { 
        return buffer_.data.size() * sizeof(audio_sample);
    }
    
    /**
     * @brief 获取特定通道的数据指针
     * @param p_channel 通道索引（0 开始）
     * @return 指向该通道数据的指针
     */
    audio_sample* get_channel_data(uint32_t p_channel) override;
    
    /**
     * @brief 获取特定通道的数据指针（const 版本）
     * @param p_channel 通道索引
     * @return 指向该通道数据的指针
     */
    const audio_sample* get_channel_data(uint32_t p_channel) const override;
    
    /**
     * @brief 设置音频格式信息（不修改数据）
     */
    void set_sample_rate(uint32_t p_sample_rate) override { 
        buffer_.sample_rate = p_sample_rate; 
    }
    
    /**
     * @brief 设置通道数
     * @param p_channels 通道数
     * @param p_preserve_data 是否为通道删除保留数据
     */
    void set_channels(uint32_t p_channels, bool p_preserve_data) override;
    
    /**
     * @brief 设置采样率（resample 模式）
     * @param p_sample_rate 新采样率
     * @param p_mode resample 模式
     */
    void set_sample_rate(uint32_t p_sample_rate, t_resample_mode p_mode) override;
    
    /**
     * @brief 转换样本格式（float -> int, int -> float）
     * @param p_target_format 目标格式
     */
    void convert(t_sample_point_format p_target_format) override;
    
    /**
     * @brief 应用增益到所有样本
     * @param p_gain 线性增益值（1.0 = 无变化）
     */
    void scale(const audio_sample& p_gain) override;
    
    /**
     * @brief 应用多个增益（每个通道不同）
     * @param p_gain 每个通道的增益数组
     */
    void scale(const audio_sample* p_gain) override;
    
    /**
     * @brief 计算音频数据的峰值电平
     * @param p_peak 输出峰值数组（大小 = 通道数）
     */
    void calculate_peak(audio_sample* p_peak) const override;
    
    /**
     * @brief 删除指定通道
     * @param p_channel 要删除的通道索引
     */
    void remove_channel(uint32_t p_channel) override;
    
    /**
     * @brief 复制特定通道到新缓冲区
     * @param p_channel 源通道索引
     * @param p_target 目标 audio_chunk
     */
    void copy_channel_to(uint32_t p_channel, audio_chunk_interface& p_target) const override;
    
    /**
     * @brief 从另一个 chunk 复制特定通道
     * @param p_channel 源通道索引
     * @param p_source 源 audio_chunk
     */
    void copy_channel_from(uint32_t p_channel, const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 从另一个 chunk 追加所有通道
     * @param p_source 源 audio_chunk
     */
    void duplicate(const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 从另一个 chunk 混音
     * @param p_source 要混入的源
     * @param p_count 要混音的样本数
     */
    void combine(const audio_chunk_interface& p_source, size_t p_count) override;
    
    /**
     * @brief 深拷贝另一个 chunk 的所有数据
     * @param p_source 源 audio_chunk
     */
    void copy(const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 从另一个 chunk 复制元数据（格式、状态）
     * @param p_source 源 audio_chunk
     */
    void copy_meta(const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 重置所有数据（清空缓冲区）
     */
    void reset() override;
    
    /**
     * @brief 反向遍历查找样本
     * @param p_start 起始位置
     * @return 有效的帧位置或 0
     */
    uint32_t find_peaks(uint32_t p_start) const override;
    
    /**
     * @brief 交换两个 chunk 的内容
     * @param p_other 要交换的 chunk
     */
    void swap(audio_chunk_interface& p_other) override;
    
    /**
     * @brief 测试两个 chunk 是否数据相等
     * @param p_other 要比较的 chunk
     * @return true 如果数据相等
     */
    bool audio_data_equals(const audio_chunk_interface& p_other) const override;
};

// 辅助函数：创建 audio_chunk
std::unique_ptr<audio_chunk_impl> audio_chunk_create();

} // namespace foobar2000_sdk
