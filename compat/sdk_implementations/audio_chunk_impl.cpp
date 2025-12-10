/**
 * @file audio_chunk_impl.cpp
 * @brief audio_chunk_impl 实现
 * @date 2025-12-09
 */

#include "audio_chunk_impl.h"
#include <algorithm>
#include <cmath>

namespace foobar2000_sdk {

audio_chunk_impl::audio_chunk_impl() {
    // 默认格式: 44.1kHz, 立体声
    buffer_.set_format(44100, 2);
}

void audio_chunk_impl::set_data(const audio_sample* p_data, size_t p_sample_count, 
                                uint32_t p_channels, uint32_t p_sample_rate) {
    if (!p_data || p_sample_count == 0 || p_channels == 0) {
        reset();
        return;
    }
    
    // 设置格式
    buffer_.set_format(p_sample_rate, p_channels);
    
    // 调整缓冲区大小
    size_t total_samples = p_sample_count * p_channels;
    buffer_.resize(total_samples);
    
    // 深拷贝数据
    if (p_data != buffer_.data.data()) {
        std::memcpy(buffer_.data.data(), p_data, total_samples * sizeof(audio_sample));
    }
    
    buffer_.sample_count = static_cast<uint32_t>(p_sample_count);
}

void audio_chunk_impl::set_data(const audio_sample* p_data, size_t p_sample_count,
                                uint32_t p_channels, uint32_t p_sample_rate,
                                void (* p_data_free)(audio_sample*)) {
    // 对于接管模式，我们直接拷贝数据（简化）
    set_data(p_data, p_sample_count, p_channels, p_sample_rate);
    
    // 调用释放回调（如果提供）
    if (p_data_free) {
        p_data_free(const_cast<audio_sample*>(p_data));
    }
}

void audio_chunk_impl::data_append(const audio_sample* p_data, size_t p_sample_count) {
    if (!p_data || p_sample_count == 0) {
        return;
    }
    
    // 验证格式匹配
    size_t old_size = buffer_.data.size();
    size_t append_samples = p_sample_count * buffer_.channels;
    
    // 调整大小以包含新数据
    buffer_.data.resize(old_size + append_samples);
    
    // 追加数据
    std::memcpy(buffer_.data.data() + old_size, p_data, 
                append_samples * sizeof(audio_sample));
    
    buffer_.sample_count += static_cast<uint32_t>(p_sample_count);
}

void audio_chunk_impl::data_pad(size_t p_sample_count) {
    if (p_sample_count == 0) {
        return;
    }
    
    size_t old_size = buffer_.data.size();
    size_t pad_samples = p_sample_count * buffer_.channels;
    
    // 用静音填充（0.0f）
    buffer_.data.resize(old_size + pad_samples, 0.0f);
    
    buffer_.sample_count += static_cast<uint32_t>(p_sample_count);
}

audio_sample* audio_chunk_impl::get_channel_data(uint32_t p_channel) {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return nullptr;
    }
    
    // 返回该通道的交错数据指针
    // 注意：这是 LRLRLR... 布局
    return &(buffer_.data[p_channel]);
}

const audio_sample* audio_chunk_impl::get_channel_data(uint32_t p_channel) const {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return nullptr;
    }
    
    return &(buffer_.data[p_channel]);
}

void audio_chunk_impl::set_channels(uint32_t p_channels, bool p_preserve_data) {
    if (p_channels == buffer_.channels) {
        return;  // 无变化
    }
    
    if (!p_preserve_data || buffer_.data.empty()) {
        // 简单设置通道数（不保留数据）
        buffer_.channels = p_channels;
        return;
    }
    
    // 保留数据模式
    if (p_channels < buffer_.channels) {
        // 删除通道：重新排列数据以移除特定通道
        uint32_t channels_to_remove = buffer_.channels - p_channels;
        
        // 简化：只保留前 p_channels 个通道
        std::vector<audio_sample> new_data;
        new_data.reserve(buffer_.sample_count * p_channels);
        
        for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
            for (uint32_t ch = 0; ch < p_channels; ++ch) {
                new_data.push_back(buffer_.data[i * buffer_.channels + ch]);
            }
        }
        
        buffer_.data = std::move(new_data);
        buffer_.channels = p_channels;
    } else if (p_channels > buffer_.channels) {
        // 添加通道：用静音填充新通道
        std::vector<audio_sample> new_data;
        new_data.reserve(buffer_.sample_count * p_channels);
        
        for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
            // 复制现有通道
            for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
                new_data.push_back(buffer_.data[i * buffer_.channels + ch]);
            }
            // 新通道填充静音
            for (uint32_t ch = buffer_.channels; ch < p_channels; ++ch) {
                new_data.push_back(0.0f);
            }
        }
        
        buffer_.data = std::move(new_data);
        buffer_.channels = p_channels;
    }
}

void audio_chunk_impl::set_sample_rate(uint32_t p_sample_rate, t_resample_mode p_mode) {
    if (p_sample_rate == buffer_.sample_rate || buffer_.data.empty()) {
        buffer_.sample_rate = p_sample_rate;
        return;
    }
    
    // 注意：真正的 resample 需要 DSP
    // 目前只更新采样率标记
    (void)p_mode;  // 暂时忽略模式
    buffer_.sample_rate = p_sample_rate;
}

void audio_chunk_impl::convert(t_sample_point_format p_target_format) {
    if (buffer_.data.empty()) {
        return;
    }
    
    // audio_sample 始终是 float（这是 foobar2000 的内部格式）
    // 这个函数用于与整数格式的外部转换
    // 对于内部实现，我们可以忽略转换（应该已经正确）
    (void)p_target_format;
}

void audio_chunk_impl::scale(const audio_sample& p_gain) {
    if (buffer_.data.empty() || p_gain == 1.0f) {
        return;
    }
    
    // 应用线性增益到所有样本
    for (auto& sample : buffer_.data) {
        sample *= p_gain;
    }
}

void audio_chunk_impl::scale(const audio_sample* p_gain) {
    if (buffer_.data.empty() || !p_gain) {
        return;
    }
    
    // 每个通道应用不同增益
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            buffer_.data[i * buffer_.channels + ch] *= p_gain[ch];
        }
    }
}

void audio_chunk_impl::calculate_peak(audio_sample* p_peak) const {
    if (buffer_.data.empty() || !p_peak) {
        return;
    }
    
    // 初始化峰值数组
    for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
        p_peak[ch] = 0.0f;
    }
    
    // 计算每个通道的峰值电平（绝对值最大值）
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            audio_sample sample = std::abs(buffer_.data[i * buffer_.channels + ch]);
            if (sample > p_peak[ch]) {
                p_peak[ch] = sample;
            }
        }
    }
}

void audio_chunk_impl::remove_channel(uint32_t p_channel) {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return;
    }
    
    // 删除特定通道
    std::vector<audio_sample> new_data;
    new_data.reserve(buffer_.sample_count * (buffer_.channels - 1));
    
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            if (ch != p_channel) {
                new_data.push_back(buffer_.data[i * buffer_.channels + ch]);
            }
        }
    }
    
    buffer_.data = std::move(new_data);
    buffer_.channels--;
}

void audio_chunk_impl::copy_channel_to(uint32_t p_channel, audio_chunk_interface& p_target) const {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return;
    }
    
    // 创建新的 chunk 用于目标
    audio_chunk_impl* target_impl = dynamic_cast<audio_chunk_impl*>(&p_target);
    if (!target_impl) {
        return;
    }
    
    // 设置目标格式
    target_impl->set_sample_rate(buffer_.sample_rate);
    target_impl->set_channels(1, false);  // 单通道
    
    // 分配空间并复制数据
    target_impl->buffer_.data.resize(buffer_.sample_count);
    target_impl->buffer_.sample_count = buffer_.sample_count;
    
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        target_impl->buffer_.data[i] = buffer_.data[i * buffer_.channels + p_channel];
    }
}

void audio_chunk_impl::copy_channel_from(uint32_t p_channel, const audio_chunk_interface& p_source) {
    if (buffer_.data.empty()) {
        return;
    }
    
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl || source_impl->buffer_.data.empty()) {
        return;
    }
    
    // 确保格式匹配
    if (buffer_.sample_rate != source_impl->buffer_.sample_rate ||
        buffer_.sample_count != source_impl->buffer_.sample_count) {
        return;
    }
    
    // 复制通道数据
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        if (p_channel < buffer_.channels) {
            buffer_.data[i * buffer_.channels + p_channel] = 
                source_impl->buffer_.data[i * source_impl->buffer_.channels];
        }
    }
}

void audio_chunk_impl::duplicate(const audio_chunk_interface& p_source) {
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl) {
        return;
    }
    
    *this = *source_impl;  // 深拷贝
}

void audio_chunk_impl::combine(const audio_chunk_interface& p_source, size_t p_count) {
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl || source_impl->buffer_.data.empty()) {
        return;
    }
    
    // 确保格式兼容
    if (buffer_.sample_rate != source_impl->buffer_.sample_rate ||
        buffer_.channels != source_impl->buffer_.channels) {
        return;
    }
    
    // 限制混音样本数
    size_t samples_to_combine = std::min(p_count, 
                                        static_cast<size_t>(source_impl->buffer_.sample_count));
    samples_to_combine = std::min(samples_to_combine, 
                                 static_cast<size_t>(buffer_.sample_count));
    
    // 简单混音（相加）
    for (uint32_t i = 0; i < samples_to_combine; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            size_t idx = i * buffer_.channels + ch;
            buffer_.data[idx] += source_impl->buffer_.data[idx];
        }
    }
}

void audio_chunk_impl::copy(const audio_chunk_interface& p_source) {
    duplicate(p_source);
}

void audio_chunk_impl::copy_meta(const audio_chunk_interface& p_source) {
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl) {
        return;
    }
    
    // 只复制元数据，不复制音频数据
    buffer_.sample_rate = source_impl->buffer_.sample_rate;
    buffer_.channels = source_impl->buffer_.channels;
    buffer_.sample_count = source_impl->buffer_.sample_count;
}

void audio_chunk_impl::reset() {
    buffer_.data.clear();
    buffer_.sample_count = 0;
    buffer_.channels = 2;  // 默认立体声
    buffer_.sample_rate = 44100;  // 默认 44.1kHz
}

uint32_t audio_chunk_impl::find_peaks(uint32_t p_start) const {
    if (buffer_.data.empty() || buffer_.channels == 0) {
        return 0;
    }
    
    // 简化实现：如果样本接近 0，返回有效位置
    // 真正的实现需要 scan backwards 找到非零样本
    (void)p_start;
    
    return buffer_.sample_count > 0 ? buffer_.sample_count - 1 : 0;
}

void audio_chunk_impl::swap(audio_chunk_interface& p_other) {
    audio_chunk_impl* other_impl = dynamic_cast<audio_chunk_impl*>(&p_other);
    if (!other_impl) {
        return;
    }
    
    std::swap(buffer_, other_impl->buffer_);
}

bool audio_chunk_impl::audio_data_equals(const audio_chunk_interface& p_other) const {
    const audio_chunk_impl* other_impl = dynamic_cast<const audio_chunk_impl*>(&p_other);
    if (!other_impl) {
        return false;
    }
    
    // 检查格式匹配
    if (buffer_.sample_rate != other_impl->buffer_.sample_rate ||
        buffer_.channels != other_impl->buffer_.channels ||
        buffer_.sample_count != other_impl->buffer_.sample_count) {
        return false;
    }
    
    // 比较所有样本是否相等（允许微小浮点误差）
    const float epsilon = 1e-6f;
    for (size_t i = 0; i < buffer_.data.size(); ++i) {
        if (std::abs(buffer_.data[i] - other_impl->buffer_.data[i]) > epsilon) {
            return false;
        }
    }
    
    return true;
}

// 辅助函数
std::unique_ptr<audio_chunk_impl> audio_chunk_create() {
    return std::make_unique<audio_chunk_impl>();
}

} // namespace foobar2000_sdk
