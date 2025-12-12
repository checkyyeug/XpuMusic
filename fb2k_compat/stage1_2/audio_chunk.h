#pragma once

// 闃舵1.2锛氶煶棰戝潡鎺ュ彛
// 闊抽鏁版嵁瀹瑰櫒锛岀敤浜庡湪DSP閾惧拰杈撳嚭璁惧涔嬮棿浼犻€掗煶棰戞暟鎹?

#include <cstdint>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include "../stage1_1/real_minihost.h"

namespace fb2k {

// 闊抽鏁版嵁鏍煎紡瀹氫箟
enum class audio_format {
    float32,    // 32-bit娴偣
    int16,      // 16-bit鏁存暟
    int24,      // 24-bit鏁存暟
    int32       // 32-bit鏁存暟
};

// 澹伴亾閰嶇疆
enum class channel_config : uint32_t {
    mono        = 0x4,      // 1澹伴亾
    stereo      = 0x3,      // 2澹伴亾 (L, R)
    stereo_lfe  = 0x103,    // 2.1澹伴亾 (L, R, LFE)
    surround_5  = 0x37,     // 5.1澹伴亾 (L, R, C, LFE, Ls, Rs)
    surround_7  = 0xFF      // 7.1澹伴亾
};

// 闊抽鍧楁帴鍙?- 绗﹀悎foobar2000瑙勮寖
class audio_chunk : public ServiceBase {
public:
    // 鍩虹灞炴€?
    virtual float* get_data() = 0;                              // 鑾峰彇鏁版嵁鎸囬拡
    virtual const float* get_data() const = 0;                  // 鑾峰彇甯搁噺鏁版嵁鎸囬拡
    virtual size_t get_sample_count() const = 0;                // 鑾峰彇閲囨牱鏁?
    virtual uint32_t get_sample_rate() const = 0;               // 鑾峰彇閲囨牱鐜?
    virtual uint32_t get_channels() const = 0;                 // 鑾峰彇澹伴亾鏁?
    virtual uint32_t get_channel_config() const = 0;           // 鑾峰彇澹伴亾閰嶇疆
    virtual double get_duration() const = 0;                   // 鑾峰彇鏃堕暱锛堢锛?
    
    // 鏁版嵁鎿嶄綔
    virtual void set_data(const float* data, size_t samples, 
                         uint32_t channels, uint32_t sample_rate) = 0;
    virtual void set_data_size(size_t samples) = 0;            // 璁剧疆鏁版嵁澶у皬
    virtual void copy(const audio_chunk& source) = 0;          // 澶嶅埗鏁版嵁
    virtual void copy_from(const float* source, size_t samples, 
                          uint32_t channels, uint32_t sample_rate) = 0;
    virtual void reset() = 0;                                   // 閲嶇疆鏁版嵁
    
    // 澹伴亾鏁版嵁璁块棶
    virtual float* get_channel_data(uint32_t channel) = 0;     // 鑾峰彇澹伴亾鏁版嵁
    virtual const float* get_channel_data(uint32_t channel) const = 0;
    virtual size_t get_channel_data_size() const = 0;          // 鑾峰彇澹伴亾鏁版嵁澶у皬
    
    // 鏁版嵁鎿嶄綔
    virtual void scale(const float& scale) = 0;                // 缂╂斁鏁版嵁
    virtual void apply_gain(float gain) = 0;                   // 搴旂敤澧炵泭
    virtual void apply_ramp(float start_gain, float end_gain) = 0; // 搴旂敤娓愬彉
    
    // 鐘舵€佹鏌?
    virtual bool is_valid() const = 0;                         // 鏁版嵁鏄惁鏈夋晥
    virtual bool is_empty() const = 0;                         // 鏄惁涓虹┖
    virtual size_t get_data_bytes() const = 0;                 // 鑾峰彇鏁版嵁瀛楄妭鏁?
};

// 闊抽鍧楀叿浣撳疄鐜?
class audio_chunk_impl : public audio_chunk {
private:
    std::vector<float> data_;           // 闊抽鏁版嵁锛堜氦閿欐牸寮忥級
    size_t sample_count_;               // 閲囨牱鏁帮紙姣忓０閬擄級
    uint32_t sample_rate_;              // 閲囨牱鐜?
    uint32_t channels_;                 // 澹伴亾鏁?
    uint32_t channel_config_;           // 澹伴亾閰嶇疆
    
public:
    audio_chunk_impl() : sample_count_(0), sample_rate_(44100), 
                        channels_(2), channel_config_(3) { // 绔嬩綋澹伴粯璁?
        data_.resize(channels_ * 1024); // 榛樿鍒嗛厤绌洪棿
    }
    
    explicit audio_chunk_impl(size_t initial_size) : 
        sample_count_(0), sample_rate_(44100), channels_(2), channel_config_(3) {
        data_.resize(std::max(initial_size, size_t(1024)) * channels_);
    }
    
    // IUnknown 瀹炵幇
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, __uuidof(audio_chunk))) {
            *ppvObject = static_cast<audio_chunk*>(this);
            return S_OK;
        }
        return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
    }
    
    // 鍩虹灞炴€?
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
    
    // 鏁版嵁鎿嶄綔
    void set_data(const float* data, size_t samples, 
                 uint32_t channels, uint32_t sample_rate) override {
        if(!data || samples == 0 || channels == 0 || sample_rate == 0) {
            reset();
            return;
        }
        
        channels_ = channels;
        sample_rate_ = sample_rate;
        sample_count_ = samples;
        
        // 璁剧疆榛樿澹伴亾閰嶇疆
        switch(channels) {
            case 1: channel_config_ = 0x4; break;      // mono
            case 2: channel_config_ = 0x3; break;      // stereo
            case 3: channel_config_ = 0x103; break;    // 2.1
            case 6: channel_config_ = 0x37; break;     // 5.1
            case 8: channel_config_ = 0xFF; break;     // 7.1
            default: channel_config_ = (1u << channels) - 1; break;
        }
        
        // 鍒嗛厤鍐呭瓨
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
        
        // 璁剧疆澹伴亾閰嶇疆
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
        channel_config_ = 0x3; // 绔嬩綋澹?
        data_.resize(2048); // 閲嶇疆涓洪粯璁ゅぇ灏?
    }
    
    // 澹伴亾鏁版嵁璁块棶
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
    
    // 鏁版嵁鎿嶄綔
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
    
    // 鐘舵€佹鏌?
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

// 闊抽鍧楀伐鍏峰嚱鏁?
class audio_chunk_utils {
public:
    // 鍒涘缓鎸囧畾鏍煎紡鐨勯煶棰戝潡
    static std::unique_ptr<audio_chunk> create_chunk(size_t samples, uint32_t channels, uint32_t sample_rate) {
        auto chunk = std::make_unique<audio_chunk_impl>(samples);
        chunk->set_data_size(samples);
        
        // 璁剧疆鏍煎紡
        float* data = chunk->get_data();
        if(data) {
            chunk->set_data(data, samples, channels, sample_rate);
        }
        
        return chunk;
    }
    
    // 鍒涘缓闈欓煶闊抽鍧?
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
    
    // 澶嶅埗闊抽鍧?
    static std::unique_ptr<audio_chunk> duplicate_chunk(const audio_chunk& source) {
        auto dest = std::make_unique<audio_chunk_impl>(source.get_sample_count());
        dest->copy(source);
        return dest;
    }
    
    // 鍚堝苟闊抽鍧楋紙涓茶仈锛?
    static std::unique_ptr<audio_chunk> concatenate_chunks(
        const audio_chunk& chunk1, const audio_chunk& chunk2) {
        
        if(chunk1.get_sample_rate() != chunk2.get_sample_rate() ||
           chunk1.get_channels() != chunk2.get_channels()) {
            return nullptr; // 鏍煎紡涓嶅尮閰?
        }
        
        size_t total_samples = chunk1.get_sample_count() + chunk2.get_sample_count();
        auto result = std::make_unique<audio_chunk_impl>(total_samples);
        
        float* dest_data = result->get_data();
        if(!dest_data) return nullptr;
        
        // 澶嶅埗绗竴涓潡
        const float* src1_data = chunk1.get_data();
        if(src1_data) {
            std::memcpy(dest_data, src1_data, 
                       chunk1.get_sample_count() * chunk1.get_channels() * sizeof(float));
        }
        
        // 澶嶅埗绗簩涓潡
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
    
    // 搴旂敤澧炵泭鍒伴煶棰戝潡
    static void apply_gain_to_chunk(audio_chunk& chunk, float gain) {
        chunk.apply_gain(gain);
    }
    
    // 鑾峰彇闊抽鍧楃殑RMS鍊?
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
    
    // 鑾峰彇闊抽鍧楃殑宄板€?
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