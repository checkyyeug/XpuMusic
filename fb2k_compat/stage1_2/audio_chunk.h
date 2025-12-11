#pragma once

// 阶段1.2：音频块接口
// 音频数据容器，用于在DSP链和输出设备之间传递音频数据

#include <cstdint>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include "../stage1_1/real_minihost.h"

namespace fb2k {

// 音频数据格式定义
enum class audio_format {
    float32,    // 32-bit浮点
    int16,      // 16-bit整数
    int24,      // 24-bit整数
    int32       // 32-bit整数
};

// 声道配置
enum class channel_config : uint32_t {
    mono        = 0x4,      // 1声道
    stereo      = 0x3,      // 2声道 (L, R)
    stereo_lfe  = 0x103,    // 2.1声道 (L, R, LFE)
    surround_5  = 0x37,     // 5.1声道 (L, R, C, LFE, Ls, Rs)
    surround_7  = 0xFF      // 7.1声道
};

// 音频块接口 - 符合foobar2000规范
class audio_chunk : public ServiceBase {
public:
    // 基础属性
    virtual float* get_data() = 0;                              // 获取数据指针
    virtual const float* get_data() const = 0;                  // 获取常量数据指针
    virtual size_t get_sample_count() const = 0;                // 获取采样数
    virtual uint32_t get_sample_rate() const = 0;               // 获取采样率
    virtual uint32_t get_channels() const = 0;                 // 获取声道数
    virtual uint32_t get_channel_config() const = 0;           // 获取声道配置
    virtual double get_duration() const = 0;                   // 获取时长（秒）
    
    // 数据操作
    virtual void set_data(const float* data, size_t samples, 
                         uint32_t channels, uint32_t sample_rate) = 0;
    virtual void set_data_size(size_t samples) = 0;            // 设置数据大小
    virtual void copy(const audio_chunk& source) = 0;          // 复制数据
    virtual void copy_from(const float* source, size_t samples, 
                          uint32_t channels, uint32_t sample_rate) = 0;
    virtual void reset() = 0;                                   // 重置数据
    
    // 声道数据访问
    virtual float* get_channel_data(uint32_t channel) = 0;     // 获取声道数据
    virtual const float* get_channel_data(uint32_t channel) const = 0;
    virtual size_t get_channel_data_size() const = 0;          // 获取声道数据大小
    
    // 数据操作
    virtual void scale(const float& scale) = 0;                // 缩放数据
    virtual void apply_gain(float gain) = 0;                   // 应用增益
    virtual void apply_ramp(float start_gain, float end_gain) = 0; // 应用渐变
    
    // 状态检查
    virtual bool is_valid() const = 0;                         // 数据是否有效
    virtual bool is_empty() const = 0;                         // 是否为空
    virtual size_t get_data_bytes() const = 0;                 // 获取数据字节数
};

// 音频块具体实现
class audio_chunk_impl : public audio_chunk {
private:
    std::vector<float> data_;           // 音频数据（交错格式）
    size_t sample_count_;               // 采样数（每声道）
    uint32_t sample_rate_;              // 采样率
    uint32_t channels_;                 // 声道数
    uint32_t channel_config_;           // 声道配置
    
public:
    audio_chunk_impl() : sample_count_(0), sample_rate_(44100), 
                        channels_(2), channel_config_(3) { // 立体声默认
        data_.resize(channels_ * 1024); // 默认分配空间
    }
    
    explicit audio_chunk_impl(size_t initial_size) : 
        sample_count_(0), sample_rate_(44100), channels_(2), channel_config_(3) {
        data_.resize(std::max(initial_size, size_t(1024)) * channels_);
    }
    
    // IUnknown 实现
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, __uuidof(audio_chunk))) {
            *ppvObject = static_cast<audio_chunk*>(this);
            return S_OK;
        }
        return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
    }
    
    // 基础属性
    float* get_data() override { 
        return data_.empty() ? nullptr : data_.data(); 
    }
    
    const float* get_data() const override { 
        return data_.empty() ? nullptr : data_.data(); 
    }
    
    size_t get_sample_count() const override { 
        return sample_count_; 
    }
    
    uint32_t get_sample_rate() const override { 
        return sample_rate_; 
    }
    
    uint32_t get_channels() const override { 
        return channels_; 
    }
    
    uint32_t get_channel_config() const override { 
        return channel_config_; 
    }
    
    double get_duration() const override {
        return sample_count_ > 0 ? double(sample_count_) / sample_rate_ : 0.0;
    }
    
    // 数据操作
    void set_data(const float* data, size_t samples, 
                 uint32_t channels, uint32_t sample_rate) override {
        if(!data || samples == 0 || channels == 0 || sample_rate == 0) {
            reset();
            return;
        }
        
        channels_ = channels;
        sample_rate_ = sample_rate;
        sample_count_ = samples;
        
        // 设置默认声道配置
        switch(channels) {
            case 1: channel_config_ = 0x4; break;      // mono
            case 2: channel_config_ = 0x3; break;      // stereo
            case 3: channel_config_ = 0x103; break;    // 2.1
            case 6: channel_config_ = 0x37; break;     // 5.1
            case 8: channel_config_ = 0xFF; break;     // 7.1
            default: channel_config_ = (1u << channels) - 1; break;
        }
        
        // 分配内存
        data_.resize(samples * channels);
        std::memcpy(data_.data(), data, samples * channels * sizeof(float));
    }
    
    void set_data_size(size_t samples) override {
        sample_count_ = samples;
        data_.resize(samples * channels_);
    }
    
    void copy(const audio_chunk& source) override {
        if(&source == this) return;
        
        sample_count_ = source.get_sample_count();
        sample_rate_ = source.get_sample_rate();
        channels_ = source.get_channels();
        channel_config_ = source.get_channel_config();
        
        size_t total_samples = sample_count_ * channels_;
        data_.resize(total_samples);
        
        const float* src_data = source.get_data();
        if(src_data) {
            std::memcpy(data_.data(), src_data, total_samples * sizeof(float));
        }
    }
    
    void copy_from(const float* source, size_t samples, 
                  uint32_t channels, uint32_t sample_rate) override {
        if(!source) return;
        
        sample_count_ = samples;
        sample_rate_ = sample_rate;
        channels_ = channels;
        
        // 设置声道配置
        switch(channels) {
            case 1: channel_config_ = 0x4; break;
            case 2: channel_config_ = 0x3; break;
            case 3: channel_config_ = 0x103; break;
            case 6: channel_config_ = 0x37; break;
            case 8: channel_config_ = 0xFF; break;
            default: channel_config_ = (1u << channels) - 1; break;
        }
        
        data_.resize(samples * channels);
        std::memcpy(data_.data(), source, samples * channels * sizeof(float));
    }
    
    void reset() override {
        data_.clear();
        sample_count_ = 0;
        sample_rate_ = 44100;
        channels_ = 2;
        channel_config_ = 0x3; // 立体声
        data_.resize(2048); // 重置为默认大小
    }
    
    // 声道数据访问
    float* get_channel_data(uint32_t channel) override {
        if(channel >= channels_ || data_.empty()) return nullptr;
        return data_.data() + channel;
    }
    
    const float* get_channel_data(uint32_t channel) const override {
        if(channel >= channels_ || data_.empty()) return nullptr;
        return data_.data() + channel;
    }
    
    size_t get_channel_data_size() const override {
        return sample_count_;
    }
    
    // 数据操作
    void scale(const float& scale) override {
        if(data_.empty() || scale == 1.0f) return;
        
        for(float& sample : data_) {
            sample *= scale;
        }
    }
    
    void apply_gain(float gain) override {
        scale(gain);
    }
    
    void apply_ramp(float start_gain, float end_gain) override {
        if(data_.empty() || (start_gain == 1.0f && end_gain == 1.0f)) return;
        
        const size_t total_samples = sample_count_ * channels_;
        const float gain_step = (end_gain - start_gain) / (total_samples - 1);
        
        for(size_t i = 0; i < total_samples; ++i) {
            float gain = start_gain + gain_step * i;
            data_[i] *= gain;
        }
    }
    
    // 状态检查
    bool is_valid() const override {
        return !data_.empty() && sample_count_ > 0 && 
               channels_ > 0 && sample_rate_ > 0;
    }
    
    bool is_empty() const override {
        return sample_count_ == 0;
    }
    
    size_t get_data_bytes() const override {
        return data_.size() * sizeof(float);
    }
};

// 音频块工具函数
class audio_chunk_utils {
public:
    // 创建指定格式的音频块
    static std::unique_ptr<audio_chunk> create_chunk(size_t samples, uint32_t channels, uint32_t sample_rate) {
        auto chunk = std::make_unique<audio_chunk_impl>(samples);
        chunk->set_data_size(samples);
        
        // 设置格式
        float* data = chunk->get_data();
        if(data) {
            chunk->set_data(data, samples, channels, sample_rate);
        }
        
        return chunk;
    }
    
    // 创建静音音频块
    static std::unique_ptr<audio_chunk> create_silence(size_t samples, uint32_t channels, uint32_t sample_rate) {
        auto chunk = std::make_unique<audio_chunk_impl>(samples);
        chunk->set_data_size(samples);
        
        float* data = chunk->get_data();
        if(data) {
            std::memset(data, 0, samples * channels * sizeof(float));
            chunk->set_data(data, samples, channels, sample_rate);
        }
        
        return chunk;
    }
    
    // 复制音频块
    static std::unique_ptr<audio_chunk> duplicate_chunk(const audio_chunk& source) {
        auto dest = std::make_unique<audio_chunk_impl>(source.get_sample_count());
        dest->copy(source);
        return dest;
    }
    
    // 合并音频块（串联）
    static std::unique_ptr<audio_chunk> concatenate_chunks(
        const audio_chunk& chunk1, const audio_chunk& chunk2) {
        
        if(chunk1.get_sample_rate() != chunk2.get_sample_rate() ||
           chunk1.get_channels() != chunk2.get_channels()) {
            return nullptr; // 格式不匹配
        }
        
        size_t total_samples = chunk1.get_sample_count() + chunk2.get_sample_count();
        auto result = std::make_unique<audio_chunk_impl>(total_samples);
        
        float* dest_data = result->get_data();
        if(!dest_data) return nullptr;
        
        // 复制第一个块
        const float* src1_data = chunk1.get_data();
        if(src1_data) {
            std::memcpy(dest_data, src1_data, 
                       chunk1.get_sample_count() * chunk1.get_channels() * sizeof(float));
        }
        
        // 复制第二个块
        const float* src2_data = chunk2.get_data();
        if(src2_data) {
            std::memcpy(dest_data + chunk1.get_sample_count() * chunk1.get_channels(),
                       src2_data,
                       chunk2.get_sample_count() * chunk2.get_channels() * sizeof(float));
        }
        
        result->set_data(dest_data, total_samples, 
                        chunk1.get_channels(), chunk1.get_sample_rate());
        
        return result;
    }
    
    // 应用增益到音频块
    static void apply_gain_to_chunk(audio_chunk& chunk, float gain) {
        chunk.apply_gain(gain);
    }
    
    // 获取音频块的RMS值
    static float calculate_rms(const audio_chunk& chunk) {
        if(chunk.is_empty()) return 0.0f;
        
        const float* data = chunk.get_data();
        if(!data) return 0.0f;
        
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        double sum_squares = 0.0;
        
        for(size_t i = 0; i < total_samples; ++i) {
            double sample = data[i];
            sum_squares += sample * sample;
        }
        
        return static_cast<float>(std::sqrt(sum_squares / total_samples));
    }
    
    // 获取音频块的峰值
    static float calculate_peak(const audio_chunk& chunk) {
        if(chunk.is_empty()) return 0.0f;
        
        const float* data = chunk.get_data();
        if(!data) return 0.0f;
        
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        float peak = 0.0f;
        
        for(size_t i = 0; i < total_samples; ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
        
        return peak;
    }
};

} // namespace fb2k