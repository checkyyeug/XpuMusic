#pragma once

// 阶段1.3：参数均衡器
// 专业级参数均衡器，支持多段频率调节

#include "dsp_manager.h"
#include <vector>
#include <complex>
#include <cmath>

namespace fb2k {

// 滤波器类型
enum class filter_type {
    low_shelf,      // 低架滤波器
    high_shelf,     // 高架滤波器
    peak,           // 峰值滤波器
    low_pass,       // 低通滤波器
    high_pass,      // 高通滤波器
    band_pass,      // 带通滤波器
    notch           // 陷波滤波器
};

// 均衡器频段参数
struct eq_band_params {
    float frequency;        // 中心频率 (Hz)
    float gain;            // 增益 (dB)
    float bandwidth;       // 带宽 (Q值或octaves)
    filter_type type;      // 滤波器类型
    bool is_enabled;       // 是否启用
    std::string name;      // 频段名称
    
    eq_band_params() : 
        frequency(1000.0f), gain(0.0f), bandwidth(1.0f), 
        type(filter_type::peak), is_enabled(true), name("") {}
    
    eq_band_params(float freq, float gain_db, float bw, filter_type t) :
        frequency(freq), gain(gain_db), bandwidth(bw), type(t), 
        is_enabled(true), name("") {}
};

// 双二阶滤波器（Biquad Filter）
class biquad_filter {
private:
    // 滤波器系数
    float a0_, a1_, a2_;  // 分母系数
    float b0_, b1_, b2_;  // 分子系数
    
    // 状态变量
    float x1_, x2_;       // 输入历史
    float y1_, y2_;       // 输出历史
    
public:
    biquad_filter() : a0_(1.0f), a1_(0.0f), a2_(0.0f), 
                     b0_(1.0f), b1_(0.0f), b2_(0.0f),
                     x1_(0.0f), x2_(0.0f), y1_(0.0f), y2_(0.0f) {}
    
    // 设置滤波器系数
    void set_coefficients(float b0, float b1, float b2, float a0, float a1, float a2);
    
    // 处理单个采样
    float process(float input);
    
    // 处理音频块
    void process_block(float* data, size_t samples);
    
    // 重置状态
    void reset();
    
    // 获取频率响应
    std::complex<float> get_frequency_response(float frequency, float sample_rate) const;
    
    // 设计滤波器
    static void design_peaking(float frequency, float gain_db, float bandwidth, 
                              float sample_rate, float& b0, float& b1, float& b2,
                              float& a0, float& a1, float& a2);
    
    static void design_low_shelf(float frequency, float gain_db, float slope,
                                float sample_rate, float& b0, float& b1, float& b2,
                                float& a0, float& a1, float& a2);
    
    static void design_high_shelf(float frequency, float gain_db, float slope,
                                 float sample_rate, float& b0, float& b1, float& b2,
                                 float& a0, float& a1, float& a2);
    
    static void design_low_pass(float frequency, float q, float sample_rate,
                               float& b0, float& b1, float& b2,
                               float& a0, float& a1, float& a2);
    
    static void design_high_pass(float frequency, float q, float sample_rate,
                                float& b0, float& b1, float& b2,
                                float& a0, float& a1, float& a2);
};

// 参数均衡器频段
class eq_band {
private:
    eq_band_params params_;
    biquad_filter filter_;
    bool needs_update_;
    
public:
    eq_band() : needs_update_(true) {}
    explicit eq_band(const eq_band_params& params) : params_(params), needs_update_(true) {}
    
    // 参数设置
    void set_params(const eq_band_params& params);
    const eq_band_params& get_params() const { return params_; }
    
    // 实时参数调节
    void set_frequency(float frequency);
    void set_gain(float gain_db);
    void set_bandwidth(float bandwidth);
    void set_enabled(bool enabled);
    
    // 音频处理
    void process(audio_chunk& chunk);
    void process_block(float* data, size_t samples);
    
    // 状态管理
    void reset();
    bool is_enabled() const { return params_.is_enabled; }
    
    // 频率响应分析
    std::complex<float> get_frequency_response(float frequency, float sample_rate) const;
    float get_gain_at_frequency(float frequency, float sample_rate) const;
    
private:
    void update_filter_coefficients();
    void validate_params();
};

// 参数均衡器主类
class dsp_equalizer_advanced : public dsp_effect_advanced {
private:
    std::vector<std::unique_ptr<eq_band>> bands_;
    static constexpr size_t MAX_BANDS = 32;
    
    // 预设频段频率（符合ISO标准）
    static constexpr float ISO_FREQUENCIES[10] = {
        31.25f, 62.5f, 125.0f, 250.0f, 500.0f,
        1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
    };
    
    // 状态
    bool needs_coefficient_update_;
    std::atomic<bool> is_updating_coefficients_;
    
public:
    dsp_equalizer_advanced();
    explicit dsp_equalizer_advanced(const dsp_effect_params& params);
    ~dsp_equalizer_advanced() override;
    
    // DSP接口实现
    bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                    uint32_t channels) override;
    void run(audio_chunk& chunk, abort_callback& abort) override;
    void reset() override;
    
    // 均衡器特定接口
    size_t add_band(const eq_band_params& params);
    bool remove_band(size_t index);
    void clear_bands();
    
    size_t get_band_count() const { return bands_.size(); }
    eq_band* get_band(size_t index);
    const eq_band* get_band(size_t index) const;
    
    // 预设管理
    void load_iso_preset();
    void load_classical_preset();
    void load_rock_preset();
    void load_jazz_preset();
    void load_pop_preset();
    void load_headphone_preset();
    
    // 频率响应分析
    std::vector<std::complex<float>> get_frequency_response(
        const std::vector<float>& frequencies, float sample_rate) const;
    
    float get_response_at_frequency(float frequency, float sample_rate) const;
    
    // 实时参数调节（线程安全）
    bool set_band_frequency(size_t band, float frequency);
    bool set_band_gain(size_t band, float gain_db);
    bool set_band_bandwidth(size_t band, float bandwidth);
    bool set_band_enabled(size_t band, bool enabled);
    
    // 批量操作
    void set_all_bands_gain(float gain_db);
    void reset_all_bands();
    void copy_band_settings(size_t source_band, size_t target_band);
    
    // 高级功能
    void auto_gain_compensate();
    void analyze_and_suggest_corrections(const audio_chunk& reference);
    std::vector<eq_band_params> suggest_corrections_for_room_response(
        const std::vector<float>& room_response);
    
protected:
    // dsp_effect_advanced 接口实现
    void process_chunk_internal(audio_chunk& chunk, abort_callback& abort) override;
    void update_cpu_usage(float usage) override;
    
private:
    void initialize_default_bands();
    void update_all_coefficients();
    void validate_band_index(size_t index);
    
    // 工厂方法
    static std::unique_ptr<eq_band> create_band(const eq_band_params& params);
};

// 10段参数均衡器（符合ISO标准）
class dsp_equalizer_10band : public dsp_equalizer_advanced {
public:
    dsp_equalizer_10band();
    explicit dsp_equalizer_10band(const dsp_effect_params& params);
    
    // 快速预设
    void load_flat_response();
    void load_v_shape();
    void load_smiley_curve();
    void load_loudness_contour();
};

// 图形均衡器（固定频段）
class dsp_graphic_equalizer : public dsp_effect_advanced {
private:
    std::vector<std::unique_ptr<eq_band>> iso_bands_;
    static constexpr size_t ISO_BAND_COUNT = 10;
    
public:
    dsp_graphic_equalizer();
    explicit dsp_graphic_equalizer(const dsp_effect_params& params);
    
    // DSP接口实现
    bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                    uint32_t channels) override;
    void run(audio_chunk& chunk, abort_callback& abort) override;
    void reset() override;
    
    // 图形均衡器特定接口
    void set_iso_band_gain(size_t band, float gain_db);
    float get_iso_band_gain(size_t band) const;
    void set_all_iso_bands(float gain_db);
    
    const std::vector<float>& get_iso_frequencies() const;
};

// 房间响应均衡器
class dsp_room_equalizer : public dsp_equalizer_advanced {
private:
    std::vector<float> room_response_;
    std::vector<float> target_response_;
    
public:
    dsp_room_equalizer();
    explicit dsp_room_equalizer(const dsp_effect_params& params);
    
    // 房间响应设置
    void set_room_response(const std::vector<float>& response, 
                          const std::vector<float>& frequencies);
    void set_target_response(const std::vector<float>& response);
    
    // 自动校正
    void auto_calculate_corrections();
    std::vector<eq_band_params> calculate_optimal_corrections();
    
    // 测量支持
    bool can_measure_room_response() const;
    void start_room_measurement();
    void stop_room_measurement();
    std::vector<float> get_measurement_progress() const;
};

// DSP均衡器工具函数
namespace eq_utils {

// 频率转换工具
float frequency_to_midi(float frequency);
float midi_to_frequency(float midi_note);
float frequency_to_octave(float frequency, float reference_freq = 1000.0f);

// Q值和带宽转换
float q_to_bandwidth(float q);
float bandwidth_to_q(float bandwidth);
float octave_to_q(float octaves);
float q_to_octave(float q);

// 增益转换
db_to_linear(float gain_db);
linear_to_db(float gain_linear);

// 频率响应计算
std::vector<std::complex<float>> calculate_combined_response(
    const std::vector<eq_band>& bands, 
    const std::vector<float>& frequencies, 
    float sample_rate);

// 最优频段放置
std::vector<float> optimize_band_placement(
    const std::vector<float>& target_frequencies,
    size_t num_bands,
    float min_frequency,
    float max_frequency);

// 房间响应分析
struct room_analysis_result {
    std::vector<float> measured_response;
    std::vector<float> recommended_corrections;
    std::vector<float> problematic_frequencies;
    float overall_gain_adjustment;
};

room_analysis_result analyze_room_response(
    const std::vector<float>& measured_response,
    const std::vector<float>& frequencies,
    const std::vector<float>& target_response);

} // namespace eq_utils

// 标准DSP效果器工厂
class dsp_std_effect_factory {
public:
    // 标准效果器
    static std::unique_ptr<dsp_equalizer_advanced> create_equalizer_10band();
    static std::unique_ptr<dsp_equalizer_advanced> create_equalizer_31band();
    
    static std::unique_ptr<dsp_effect_advanced> create_reverb_basic();
    static std::unique_ptr<dsp_effect_advanced> create_reverb_room();
    static std::unique_ptr<dsp_effect_advanced> create_reverb_hall();
    
    static std::unique_ptr<dsp_effect_advanced> create_compressor_basic();
    static std::unique_ptr<dsp_effect_advanced> create_compressor_multiband();
    
    static std::unique_ptr<dsp_effect_advanced> create_limiter_basic();
    static std::unique_ptr<dsp_effect_advanced> create_limiter_lookahead();
    
    static std::unique_ptr<dsp_effect_advanced> create_volume_control_advanced();
    static std::unique_ptr<dsp_effect_advanced> create_crossfeed_natural();
    static std::unique_ptr<dsp_effect_advanced> create_crossfeed_jmeier();
    
    // 预设DSP链
    static std::vector<std::unique_ptr<dsp_effect_advanced>> 
        create_hifi_effect_chain();
    
    static std::vector<std::unique_ptr<dsp_effect_advanced>> 
        create_headphone_effect_chain();
    
    static std::vector<std::unique_ptr<dsp_effect_advanced>> 
        create_room_correction_chain(const std::vector<float>& room_response);
    
    // 根据foobar2000配置创建效果器
    static std::unique_ptr<dsp_effect_advanced> 
        create_from_fb2k_config(const std::string& config_name);
};

} // namespace fb2k