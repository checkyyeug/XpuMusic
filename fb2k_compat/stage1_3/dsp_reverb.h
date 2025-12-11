#pragma once

// 阶段1.3：混响效果器
// 专业级混响效果器，支持多种混响算法

#include "dsp_manager.h"
#include <vector>
#include <array>
#include <random>
#include <algorithm>

namespace fb2k {

// 混响类型
enum class reverb_type {
    room,        // 房间混响
    hall,        // 大厅混响
    plate,       // 板式混响
    spring,      // 弹簧混响
    cathedral,   // 大教堂混响
    stadium,     // 体育场混响
    custom       // 自定义混响
};

// 混响参数
struct reverb_parameters {
    reverb_type type;
    float room_size;        // 房间大小 (0.0 - 1.0)
    float damping;          // 阻尼 (0.0 - 1.0)
    float wet_level;        // 湿信号电平 (0.0 - 1.0)
    float dry_level;        // 干信号电平 (0.0 - 1.0)
    float width;            // 立体声宽度 (0.0 - 1.0)
    float predelay;         // 预延迟 (ms)
    float decay_time;       // 衰减时间 (s)
    float diffusion;        // 扩散 (0.0 - 1.0)
    float modulation_rate;  // 调制速率 (Hz)
    float modulation_depth; // 调制深度 (0.0 - 1.0)
    bool enable_modulation; // 启用调制
    bool enable_filtering;  // 启用滤波
    
    reverb_parameters() :
        type(reverb_type::room),
        room_size(0.5f),
        damping(0.5f),
        wet_level(0.3f),
        dry_level(1.0f),
        width(1.0f),
        predelay(0.0f),
        decay_time(1.0f),
        diffusion(0.7f),
        modulation_rate(0.2f),
        modulation_depth(0.1f),
        enable_modulation(true),
        enable_filtering(false) {}
};

// 全通滤波器（用于混响）
class allpass_filter {
private:
    std::vector<float> buffer_;
    size_t buffer_size_;
    size_t read_pos_;
    size_t write_pos_;
    float feedback_;
    
public:
    allpass_filter(size_t delay_samples, float feedback = 0.5f)
        : buffer_size_(delay_samples), feedback_(feedback),
          read_pos_(0), write_pos_(0) {
        buffer_.resize(buffer_size_, 0.0f);
    }
    
    float process(float input) {
        float delayed = buffer_[read_pos_];
        float output = -input + delayed;
        float temp = input + feedback_ * delayed;
        
        buffer_[write_pos_] = temp;
        
        read_pos_ = (read_pos_ + 1) % buffer_size_;
        write_pos_ = (write_pos_ + 1) % buffer_size_;
        
        return output;
    }
    
    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        read_pos_ = 0;
        write_pos_ = 0;
    }
};

// 梳状滤波器（用于混响）
class comb_filter {
private:
    std::vector<float> buffer_;
    size_t buffer_size_;
    size_t read_pos_;
    size_t write_pos_;
    float feedback_;
    float damping_;
    float damping_filter_state_;
    
public:
    comb_filter(size_t delay_samples, float feedback = 0.84f, float damping = 0.2f)
        : buffer_size_(delay_samples), feedback_(feedback), damping_(damping),
          read_pos_(0), write_pos_(0), damping_filter_state_(0.0f) {
        buffer_.resize(buffer_size_, 0.0f);
    }
    
    float process(float input) {
        float delayed = buffer_[read_pos_];
        
        // 应用阻尼滤波器
        damping_filter_state_ = delayed + damping_ * (damping_filter_state_ - delayed);
        
        float output = damping_filter_state_;
        float temp = input + feedback_ * damping_filter_state_;
        
        buffer_[write_pos_] = temp;
        
        read_pos_ = (read_pos_ + 1) % buffer_size_;
        write_pos_ = (write_pos_ + 1) % buffer_size_;
        
        return output;
    }
    
    void reset() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        read_pos_ = 0;
        write_pos_ = 0;
        damping_filter_state_ = 0.0f;
    }
    
    void set_feedback(float feedback) { feedback_ = feedback; }
    void set_damping(float damping) { damping_ = damping; }
};

// 早期反射生成器
class early_reflections {
private:
    std::vector<size_t> delay_times_;
    std::vector<float> delay_gains_;
    std::vector<std::vector<float>> delay_buffers_;
    std::vector<size_t> delay_positions_;
    
public:
    early_reflections(const std::vector<size_t>& delays, const std::vector<float>& gains)
        : delay_times_(delays), delay_gains_(gains) {
        
        delay_buffers_.resize(delays.size());
        delay_positions_.resize(delays.size(), 0);
        
        for(size_t i = 0; i < delays.size(); ++i) {
            delay_buffers_[i].resize(delays[i], 0.0f);
        }
    }
    
    float process(float input, size_t channel) {
        if(channel >= delay_buffers_.size()) return 0.0f;
        
        float output = 0.0f;
        
        for(size_t i = 0; i < delay_buffers_[channel].size(); ++i) {
            if(i < delay_gains_.size()) {
                output += delay_buffers_[channel][i] * delay_gains_[i];
            }
        }
        
        // 更新延迟线
        delay_buffers_[channel][delay_positions_[channel]] = input;
        delay_positions_[channel] = (delay_positions_[channel] + 1) % delay_buffers_[channel].size();
        
        return output;
    }
    
    void reset() {
        for(auto& buffer : delay_buffers_) {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
        std::fill(delay_positions_.begin(), delay_positions_.end(), 0);
    }
};

// 调制器（用于混响尾音）
class modulator {
private:
    float rate_;
    float depth_;
    float phase_;
    float sample_rate_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_;
    
public:
    modulator(float rate, float depth, float sample_rate)
        : rate_(rate), depth_(depth), phase_(0.0f), sample_rate_(sample_rate),
          rng_(std::random_device{}()), dist_(-1.0f, 1.0f) {}
    
    float process() {
        // LFO调制
        float lfo = depth_ * std::sin(2.0f * M_PI * rate_ * phase_);
        phase_ += 1.0f / sample_rate_;
        if(phase_ > 1.0f) phase_ -= 1.0f;
        
        // 添加随机调制
        float random = 0.1f * depth_ * dist_(rng_);
        
        return lfo + random;
    }
    
    void reset() {
        phase_ = 0.0f;
    }
    
    void set_rate(float rate) { rate_ = rate; }
    void set_depth(float depth) { depth_ = depth; }
};

// 混响引擎基类
class reverb_engine {
protected:
    reverb_parameters params_;
    uint32_t sample_rate_;
    
public:
    reverb_engine(const reverb_parameters& params, uint32_t sample_rate)
        : params_(params), sample_rate_(sample_rate) {}
    
    virtual ~reverb_engine() = default;
    
    virtual void process(audio_chunk& chunk) = 0;
    virtual void reset() = 0;
    virtual double get_latency() const = 0;
    
    const reverb_parameters& get_params() const { return params_; }
    void set_params(const reverb_parameters& params) { params_ = params; }
};

// 房间混响引擎
class room_reverb_engine : public reverb_engine {
private:
    // 早期反射
    std::unique_ptr<early_reflections> early_reflections_;
    
    // 混响尾音
    std::vector<std::unique_ptr<comb_filter>> comb_filters_;
    std::vector<std::unique_ptr<allpass_filter>> allpass_filters_;
    
    // 调制
    std::unique_ptr<modulator> modulator_;
    
    // 参数
    std::vector<size_t> comb_delays_;
    std::vector<size_t> allpass_delays_;
    std::vector<float> comb_feedbacks_;
    
public:
    room_reverb_engine(const reverb_parameters& params, uint32_t sample_rate);
    
    void process(audio_chunk& chunk) override;
    void reset() override;
    double get_latency() const override { return params_.predelay / 1000.0; }
    
private:
    void calculate_delays();
    void initialize_filters();
    void process_early_reflections(audio_chunk& chunk);
    void process_reverb_tail(audio_chunk& chunk);
    void apply_modulation(audio_chunk& chunk);
};

// 大厅混响引擎
class hall_reverb_engine : public reverb_engine {
private:
    // 更大的延迟时间和更多的滤波器
    std::vector<std::unique_ptr<comb_filter>> comb_filters_;
    std::vector<std::unique_ptr<allpass_filter>> allpass_filters_;
    std::unique_ptr<modulator> modulator_;
    
public:
    hall_reverb_engine(const reverb_parameters& params, uint32_t sample_rate);
    
    void process(audio_chunk& chunk) override;
    void reset() override;
    double get_latency() const override { return params_.predelay / 1000.0 + 0.05; }
    
private:
    void calculate_hall_delays();
    void initialize_hall_filters();
};

// 板式混响引擎
class plate_reverb_engine : public reverb_engine {
private:
    // 高密度滤波器组
    std::vector<std::unique_ptr<allpass_filter>> allpass_filters_;
    std::unique_ptr<modulator> modulator_;
    
    // 扩散网络
    std::vector<std::vector<float>> diffusion_matrix_;
    std::vector<float> diffusion_state_;
    
public:
    plate_reverb_engine(const reverb_parameters& params, uint32_t sample_rate);
    
    void process(audio_chunk& chunk) override;
    void reset() override;
    double get_latency() const override { return params_.predelay / 1000.0; }
    
private:
    void initialize_diffusion_network();
    void process_diffusion_network(audio_chunk& chunk);
};

// 混响效果器主类
class dsp_reverb_advanced : public dsp_effect_advanced {
private:
    reverb_parameters params_;
    std::unique_ptr<reverb_engine> engine_;
    std::unique_ptr<audio_chunk> wet_buffer_;
    std::unique_ptr<audio_chunk> dry_buffer_;
    
    // 调制支持
    std::unique_ptr<modulator> modulation_;
    
    // 滤波支持
    std::unique_ptr<biquad_filter> input_filter_;
    std::unique_ptr<biquad_filter> output_filter_;
    
public:
    dsp_reverb_advanced();
    explicit dsp_reverb_advanced(const dsp_effect_params& params);
    ~dsp_reverb_advanced() override;
    
    // DSP接口实现
    bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                    uint32_t channels) override;
    void run(audio_chunk& chunk, abort_callback& abort) override;
    void reset() override;
    
    // 混响特定接口
    void set_room_size(float size);
    void set_damping(float damping);
    void set_wet_level(float level);
    void set_dry_level(float level);
    void set_width(float width);
    void set_predelay(float ms);
    void set_decay_time(float seconds);
    void set_diffusion(float diffusion);
    
    // 高级控制
    void set_modulation_rate(float rate);
    void set_modulation_depth(float depth);
    void enable_modulation(bool enable);
    void enable_filtering(bool enable);
    
    // 预设管理
    void load_room_preset(float room_size);
    void load_hall_preset(float room_size);
    void load_plate_preset();
    void load_cathedral_preset();
    
    // 房间类型预设
    void set_small_room();
    void set_medium_room();
    void set_large_room();
    void set_concert_hall();
    void set_cathedral();
    
protected:
    // dsp_effect_advanced 接口实现
    void process_chunk_internal(audio_chunk& chunk, abort_callback& abort) override;
    void update_cpu_usage(float usage) override;
    
private:
    void create_reverb_engine();
    void create_modulation();
    void create_filters();
    void calculate_delays();
    
    void apply_input_filtering(audio_chunk& chunk);
    void apply_output_filtering(audio_chunk& chunk);
    void apply_modulation(audio_chunk& chunk);
    
    void mix_wet_dry_signals(audio_chunk& chunk);
    void apply_stereo_width(audio_chunk& chunk);
};

// 混响工具函数
namespace reverb_utils {

// 混响时间计算
float calculate_reverb_time(float room_size, float damping, float diffusion);

// 混响密度计算
float calculate_reverb_density(float room_size, float diffusion);

// 延迟时间计算
std::vector<size_t> calculate_comb_delays(float room_size, float sample_rate);
std::vector<size_t> calculate_allpass_delays(float room_size, float sample_rate);

// 房间响应分析
struct room_acoustics {
    float rt60;           // 混响时间
    float clarity;        // 清晰度
    float definition;     // 定义度
    float envelopment;    // 包围感
    float warmth;         // 温暖度
    float brilliance;     // 明亮度
};

room_acoustics analyze_room_acoustics(const std::vector<float>& impulse_response,
                                     float sample_rate);

// 混响预设生成器
reverb_parameters generate_room_reverb(float room_size, float room_type);
reverb_parameters generate_hall_reverb(float hall_size, float hall_type);
reverb_parameters generate_plate_reverb(float plate_type);

// 混响质量评估
float calculate_reverb_quality(const audio_chunk& dry_signal,
                              const audio_chunk& wet_signal,
                              const reverb_parameters& params);

// 混响调试工具
std::string generate_reverb_report(const dsp_reverb_advanced& reverb);
void analyze_reverb_impulse_response(const std::vector<float>& impulse_response);

} // namespace reverb_utils

} // namespace fb2k