#include "dsp_equalizer.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>

namespace fb2k {

// 双二阶滤波器实现
void biquad_filter::set_coefficients(float b0, float b1, float b2, float a0, float a1, float a2) {
    // 归一化系数
    float a0_inv = 1.0f / a0;
    b0_ = b0 * a0_inv;
    b1_ = b1 * a0_inv;
    b2_ = b2 * a0_inv;
    a1_ = a1 * a0_inv;
    a2_ = a2 * a0_inv;
    a0_ = 1.0f;
}

float biquad_filter::process(float input) {
    float output = b0_ * input + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
    
    // 更新状态
    x2_ = x1_;
    x1_ = input;
    y2_ = y1_;
    y1_ = output;
    
    return output;
}

void biquad_filter::process_block(float* data, size_t samples) {
    for(size_t i = 0; i < samples; ++i) {
        data[i] = process(data[i]);
    }
}

void biquad_filter::reset() {
    x1_ = x2_ = y1_ = y2_ = 0.0f;
}

std::complex<float> biquad_filter::get_frequency_response(float frequency, float sample_rate) const {
    float omega = 2.0f * M_PI * frequency / sample_rate;
    std::complex<float> z(cos(omega), -sin(omega));
    std::complex<float> z2 = z * z;
    
    // 分子: b0 + b1*z^-1 + b2*z^-2
    std::complex<float> numerator = b0_ + b1_ / z + b2_ / z2;
    
    // 分母: a0 + a1*z^-1 + a2*z^-2
    std::complex<float> denominator = a0_ + a1_ / z + a2_ / z2;
    
    return numerator / denominator;
}

// 设计峰值滤波器
void biquad_filter::design_peaking(float frequency, float gain_db, float bandwidth, 
                                  float sample_rate, float& b0, float& b1, float& b2,
                                  float& a0, float& a1, float& a2) {
    float omega = 2.0f * M_PI * frequency / sample_rate;
    float A = std::pow(10.0f, gain_db / 40.0f);  // 幅度转换
    float alpha = std::sin(omega) * std::sinh(std::log(2.0f) / 2.0f * bandwidth * omega / std::sin(omega));
    
    float cos_omega = std::cos(omega);
    float alpha_A = alpha * A;
    float alpha_div_A = alpha / A;
    
    // 峰值滤波器系数
    b0 = 1.0f + alpha * A;
    b1 = -2.0f * cos_omega;
    b2 = 1.0f - alpha * A;
    a0 = 1.0f + alpha / A;
    a1 = -2.0f * cos_omega;
    a2 = 1.0f - alpha / A;
}

// 设计低架滤波器
void biquad_filter::design_low_shelf(float frequency, float gain_db, float slope,
                                    float sample_rate, float& b0, float& b1, float& b2,
                                    float& a0, float& a1, float& a2) {
    float omega = 2.0f * M_PI * frequency / sample_rate;
    float A = std::pow(10.0f, gain_db / 40.0f);
    float sqrt_A = std::sqrt(A);
    float alpha = std::sin(omega) / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / slope - 1.0f) + 2.0f);
    
    float cos_omega = std::cos(omega);
    float sqrt_A_alpha = sqrt_A * alpha;
    float sqrt_A_alpha_2 = 2.0f * sqrt_A * alpha;
    
    // 低架滤波器系数
    b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_omega + sqrt_A_alpha_2);
    b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_omega);
    b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_omega - sqrt_A_alpha_2);
    a0 = (A + 1.0f) + (A - 1.0f) * cos_omega + sqrt_A_alpha_2;
    a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_omega);
    a2 = (A + 1.0f) + (A - 1.0f) * cos_omega - sqrt_A_alpha_2;
}

// 设计高架滤波器
void biquad_filter::design_high_shelf(float frequency, float gain_db, float slope,
                                     float sample_rate, float& b0, float& b1, float& b2,
                                     float& a0, float& a1, float& a2) {
    float omega = 2.0f * M_PI * frequency / sample_rate;
    float A = std::pow(10.0f, gain_db / 40.0f);
    float sqrt_A = std::sqrt(A);
    float alpha = std::sin(omega) / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / slope - 1.0f) + 2.0f);
    
    float cos_omega = std::cos(omega);
    float sqrt_A_alpha = sqrt_A * alpha;
    float sqrt_A_alpha_2 = 2.0f * sqrt_A * alpha;
    
    // 高架滤波器系数
    b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_omega + sqrt_A_alpha_2);
    b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_omega);
    b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_omega - sqrt_A_alpha_2);
    a0 = (A + 1.0f) - (A - 1.0f) * cos_omega + sqrt_A_alpha_2;
    a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_omega);
    a2 = (A + 1.0f) - (A - 1.0f) * cos_omega - sqrt_A_alpha_2;
}

// 设计低通滤波器
void biquad_filter::design_low_pass(float frequency, float q, float sample_rate,
                                   float& b0, float& b1, float& b2,
                                   float& a0, float& a1, float& a2) {
    float omega = 2.0f * M_PI * frequency / sample_rate;
    float cos_omega = std::cos(omega);
    float alpha = std::sin(omega) / (2.0f * q);
    
    b0 = (1.0f - cos_omega) / 2.0f;
    b1 = 1.0f - cos_omega;
    b2 = (1.0f - cos_omega) / 2.0f;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_omega;
    a2 = 1.0f - alpha;
}

// 设计高通滤波器
void biquad_filter::design_high_pass(float frequency, float q, float sample_rate,
                                    float& b0, float& b1, float& b2,
                                    float& a0, float& a1, float& a2) {
    float omega = 2.0f * M_PI * frequency / sample_rate;
    float cos_omega = std::cos(omega);
    float alpha = std::sin(omega) / (2.0f * q);
    
    b0 = (1.0f + cos_omega) / 2.0f;
    b1 = -(1.0f + cos_omega);
    b2 = (1.0f + cos_omega) / 2.0f;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_omega;
    a2 = 1.0f - alpha;
}

// EQ频段实现
eq_band::eq_band(const eq_band_params& params) : params_(params), needs_update_(true) {}

void eq_band::set_params(const eq_band_params& params) {
    params_ = params;
    needs_update_ = true;
}

void eq_band::set_frequency(float frequency) {
    params_.frequency = std::max(10.0f, std::min(20000.0f, frequency));
    needs_update_ = true;
}

void eq_band::set_gain(float gain_db) {
    params_.gain = std::max(-24.0f, std::min(24.0f, gain_db));
    needs_update_ = true;
}

void eq_band::set_bandwidth(float bandwidth) {
    params_.bandwidth = std::max(0.1f, std::min(10.0f, bandwidth));
    needs_update_ = true;
}

void eq_band::set_enabled(bool enabled) {
    params_.is_enabled = enabled;
}

void eq_band::process(audio_chunk& chunk) {
    if(!params_.is_enabled || chunk.is_empty()) {
        return;
    }
    
    if(needs_update_) {
        update_filter_coefficients();
        needs_update_ = false;
    }
    
    // 跳过增益为0的频段
    if(std::abs(params_.gain) < 0.01f) {
        return;
    }
    
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    if(!data) return;
    
    process_block(data, total_samples);
}

void eq_band::process_block(float* data, size_t samples) {
    filter_.process_block(data, samples);
}

void eq_band::reset() {
    filter_.reset();
}

std::complex<float> eq_band::get_frequency_response(float frequency, float sample_rate) const {
    return filter_.get_frequency_response(frequency, sample_rate);
}

float eq_band::get_gain_at_frequency(float frequency, float sample_rate) const {
    auto response = get_frequency_response(frequency, sample_rate);
    return std::abs(response);
}

void eq_band::update_filter_coefficients() {
    float b0, b1, b2, a0, a1, a2;
    
    switch(params_.type) {
        case filter_type::peak:
            biquad_filter::design_peaking(params_.frequency, params_.gain, 
                                         params_.bandwidth, 44100.0f, // 假设标准采样率
                                         b0, b1, b2, a0, a1, a2);
            break;
            
        case filter_type::low_shelf:
            biquad_filter::design_low_shelf(params_.frequency, params_.gain, 
                                           params_.bandwidth, 44100.0f,
                                           b0, b1, b2, a0, a1, a2);
            break;
            
        case filter_type::high_shelf:
            biquad_filter::design_high_shelf(params_.frequency, params_.gain, 
                                            params_.bandwidth, 44100.0f,
                                            b0, b1, b2, a0, a1, a2);
            break;
            
        case filter_type::low_pass:
            biquad_filter::design_low_pass(params_.frequency, params_.bandwidth, 44100.0f,
                                          b0, b1, b2, a0, a1, a2);
            break;
            
        case filter_type::high_pass:
            biquad_filter::design_high_pass(params_.frequency, params_.bandwidth, 44100.0f,
                                           b0, b1, b2, a0, a1, a2);
            break;
            
        default:
            // 默认峰值滤波器
            biquad_filter::design_peaking(params_.frequency, params_.gain, 
                                         params_.bandwidth, 44100.0f,
                                         b0, b1, b2, a0, a1, a2);
            break;
    }
    
    filter_.set_coefficients(b0, b1, b2, a0, a1, a2);
}

void eq_band::validate_params() {
    // 确保参数在合理范围内
    params_.frequency = std::max(10.0f, std::min(20000.0f, params_.frequency));
    params_.gain = std::max(-24.0f, std::min(24.0f, params_.gain));
    params_.bandwidth = std::max(0.1f, std::min(10.0f, params_.bandwidth));
}

// 参数均衡器高级实现
dsp_equalizer_advanced::dsp_equalizer_advanced() 
    : dsp_effect_advanced(create_default_params()),
      needs_coefficient_update_(false),
      is_updating_coefficients_(false) {
    initialize_default_bands();
}

dsp_equalizer_advanced::dsp_equalizer_advanced(const dsp_effect_params& params)
    : dsp_effect_advanced(params),
      needs_coefficient_update_(false),
      is_updating_coefficients_(false) {
    initialize_default_bands();
}

dsp_equalizer_advanced::~dsp_equalizer_advanced() = default;

bool dsp_equalizer_advanced::instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                                        uint32_t channels) {
    if(sample_rate < 8000 || sample_rate > 192000) {
        return false;
    }
    
    if(channels < 1 || channels > 8) {
        return false;
    }
    
    // 更新所有频段的采样率
    for(auto& band : bands_) {
        if(band) {
            // 这里需要根据实际采样率重新计算系数
            band->set_params(band->get_params()); // 触发系数更新
        }
    }
    
    return true;
}

void dsp_equalizer_advanced::run(audio_chunk& chunk, abort_callback& abort) {
    if(is_bypassed() || !is_enabled() || chunk.is_empty() || abort.is_aborting()) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    process_chunk_internal(chunk, abort);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    update_cpu_usage(static_cast<float>(duration.count()) / 1000.0f); // 转换为毫秒
}

void dsp_equalizer_advanced::reset() {
    for(auto& band : bands_) {
        if(band) {
            band->reset();
        }
    }
}

size_t dsp_equalizer_advanced::add_band(const eq_band_params& params) {
    if(bands_.size() >= MAX_BANDS) {
        return static_cast<size_t>(-1); // 达到最大频段数
    }
    
    auto band = create_band(params);
    if(!band) {
        return static_cast<size_t>(-1);
    }
    
    bands_.push_back(std::move(band));
    return bands_.size() - 1;
}

bool dsp_equalizer_advanced::remove_band(size_t index) {
    if(index >= bands_.size()) {
        return false;
    }
    
    bands_.erase(bands_.begin() + index);
    return true;
}

void dsp_equalizer_advanced::clear_bands() {
    bands_.clear();
}

eq_band* dsp_equalizer_advanced::get_band(size_t index) {
    return index < bands_.size() ? bands_[index].get() : nullptr;
}

const eq_band* dsp_equalizer_advanced::get_band(size_t index) const {
    return index < bands_.size() ? bands_[index].get() : nullptr;
}

void dsp_equalizer_advanced::load_iso_preset() {
    clear_bands();
    
    // 添加ISO标准频段
    for(size_t i = 0; i < 10; ++i) {
        eq_band_params params;
        params.frequency = ISO_FREQUENCIES[i];
        params.gain = 0.0f;
        params.bandwidth = 1.0f;
        params.type = filter_type::peak;
        params.is_enabled = true;
        params.name = "ISO_" + std::to_string(i);
        
        add_band(params);
    }
}

void dsp_equalizer_advanced::load_classical_preset() {
    load_iso_preset();
    
    // 古典音乐典型EQ设置
    if(bands_.size() >= 10) {
        bands_[2]->set_gain(2.0f);  // 125Hz: +2dB
        bands_[3]->set_gain(1.5f);  // 250Hz: +1.5dB
        bands_[6]->set_gain(1.0f);  // 2kHz: +1dB
        bands_[7]->set_gain(1.5f);  // 4kHz: +1.5dB
    }
}

void dsp_equalizer_advanced::load_rock_preset() {
    load_iso_preset();
    
    // 摇滚音乐典型EQ设置
    if(bands_.size() >= 10) {
        bands_[0]->set_gain(3.0f);  // 31.25Hz: +3dB
        bands_[1]->set_gain(2.0f);  // 62.5Hz: +2dB
        bands_[6]->set_gain(2.0f);  // 2kHz: +2dB
        bands_[7]->set_gain(1.0f);  // 4kHz: +1dB
    }
}

void dsp_equalizer_advanced::load_jazz_preset() {
    load_iso_preset();
    
    // 爵士音乐典型EQ设置
    if(bands_.size() >= 10) {
        bands_[1]->set_gain(1.5f);  // 62.5Hz: +1.5dB
        bands_[2]->set_gain(1.0f);  // 125Hz: +1dB
        bands_[5]->set_gain(1.0f);  // 1kHz: +1dB
        bands_[6]->set_gain(1.5f);  // 2kHz: +1.5dB
    }
}

void dsp_equalizer_advanced::load_pop_preset() {
    load_iso_preset();
    
    // 流行音乐典型EQ设置
    if(bands_.size() >= 10) {
        bands_[4]->set_gain(1.0f);  // 500Hz: +1dB
        bands_[5]->set_gain(2.0f);  // 1kHz: +2dB
        bands_[6]->set_gain(1.5f);  // 2kHz: +1.5dB
        bands_[8]->set_gain(1.0f);  // 8kHz: +1dB
    }
}

void dsp_equalizer_advanced::load_headphone_preset() {
    load_iso_preset();
    
    // 耳机优化EQ设置
    if(bands_.size() >= 10) {
        bands_[0]->set_gain(2.0f);  // 31.25Hz: +2dB
        bands_[1]->set_gain(1.0f);  // 62.5Hz: +1dB
        bands_[8]->set_gain(2.0f);  // 8kHz: +2dB
        bands_[9]->set_gain(1.5f);  // 16kHz: +1.5dB
    }
}

std::vector<std::complex<float>> dsp_equalizer_advanced::get_frequency_response(
    const std::vector<float>& frequencies, float sample_rate) const {
    
    std::vector<std::complex<float>> responses;
    responses.reserve(frequencies.size());
    
    for(float freq : frequencies) {
        std::complex<float> total_response(1.0f, 0.0f);
        
        for(const auto& band : bands_) {
            if(band && band->is_enabled()) {
                auto band_response = band->get_frequency_response(freq, sample_rate);
                total_response *= band_response;
            }
        }
        
        responses.push_back(total_response);
    }
    
    return responses;
}

float dsp_equalizer_advanced::get_response_at_frequency(float frequency, float sample_rate) const {
    std::complex<float> total_response(1.0f, 0.0f);
    
    for(const auto& band : bands_) {
        if(band && band->is_enabled()) {
            auto band_response = band->get_frequency_response(frequency, sample_rate);
            total_response *= band_response;
        }
    }
    
    return std::abs(total_response);
}

void dsp_equalizer_advanced::process_chunk_internal(audio_chunk& chunk, abort_callback& abort) {
    if(bands_.empty() || abort.is_aborting()) {
        return;
    }
    
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    if(!data) return;
    
    // 处理每个频段
    for(const auto& band : bands_) {
        if(band && band->is_enabled()) {
            band->process_block(data, total_samples);
            
            if(abort.is_aborting()) {
                return;
            }
        }
    }
}

void dsp_equalizer_advanced::update_cpu_usage(float usage) {
    // 这里可以实现更复杂的CPU使用率计算
    // 目前直接使用传入的值
}

void dsp_equalizer_advanced::initialize_default_bands() {
    // 默认加载ISO预设
    load_iso_preset();
}

std::unique_ptr<eq_band> dsp_equalizer_advanced::create_band(const eq_band_params& params) {
    return std::make_unique<eq_band>(params);
}

// 10段参数均衡器
dsp_equalizer_10band::dsp_equalizer_10band() : dsp_equalizer_advanced() {
    // 默认就是ISO预设
}

dsp_equalizer_10band::dsp_equalizer_10band(const dsp_effect_params& params) 
    : dsp_equalizer_advanced(params) {
    // 默认就是ISO预设
}

void dsp_equalizer_10band::load_flat_response() {
    // 平直响应（所有增益为0）
    for(auto& band : bands_) {
        if(band) {
            band->set_gain(0.0f);
        }
    }
}

void dsp_equalizer_10band::load_v_shape() {
    if(bands_.size() >= 10) {
        bands_[0]->set_gain(3.0f);   // 31.25Hz: +3dB
        bands_[1]->set_gain(2.0f);   // 62.5Hz: +2dB
        bands_[8]->set_gain(3.0f);   // 8kHz: +3dB
        bands_[9]->set_gain(2.0f);   // 16kHz: +2dB
    }
}

void dsp_equalizer_10band::load_smiley_curve() {
    if(bands_.size() >= 10) {
        bands_[0]->set_gain(4.0f);   // 31.25Hz: +4dB
        bands_[1]->set_gain(2.0f);   // 62.5Hz: +2dB
        bands_[8]->set_gain(4.0f);   // 8kHz: +4dB
        bands_[9]->set_gain(2.0f);   // 16kHz: +2dB
    }
}

void dsp_equalizer_10band::load_loudness_contour() {
    if(bands_.size() >= 10) {
        bands_[0]->set_gain(2.0f);   // 31.25Hz: +2dB
        bands_[1]->set_gain(1.0f);   // 62.5Hz: +1dB
        bands_[8]->set_gain(1.5f);   // 8kHz: +1.5dB
        bands_[9]->set_gain(1.0f);   // 16kHz: +1dB
    }
}

// 图形均衡器
dsp_graphic_equalizer::dsp_graphic_equalizer() : dsp_effect_advanced(create_default_params()) {
    initialize_iso_bands();
}

dsp_graphic_equalizer::dsp_graphic_equalizer(const dsp_effect_params& params) 
    : dsp_effect_advanced(params) {
    initialize_iso_bands();
}

void dsp_graphic_equalizer::initialize_iso_bands() {
    iso_bands_.clear();
    iso_bands_.reserve(ISO_BAND_COUNT);
    
    for(size_t i = 0; i < ISO_BAND_COUNT; ++i) {
        eq_band_params params;
        params.frequency = ISO_FREQUENCIES[i];
        params.gain = 0.0f;
        params.bandwidth = 1.0f; // Q = 1.0 for ISO bands
        params.type = filter_type::peak;
        params.is_enabled = true;
        params.name = "ISO_" + std::to_string(i);
        
        iso_bands_.push_back(std::make_unique<eq_band>(params));
    }
}

bool dsp_graphic_equalizer::instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                                       uint32_t channels) {
    // 更新所有ISO频段的采样率
    for(auto& band : iso_bands_) {
        if(band) {
            band->set_params(band->get_params()); // 触发系数更新
        }
    }
    
    return true;
}

void dsp_graphic_equalizer::run(audio_chunk& chunk, abort_callback& abort) {
    if(is_bypassed() || !is_enabled() || chunk.is_empty() || abort.is_aborting()) {
        return;
    }
    
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    if(!data) return;
    
    // 处理每个ISO频段
    for(auto& band : iso_bands_) {
        if(band && band->is_enabled()) {
            band->process_block(data, total_samples);
            
            if(abort.is_aborting()) {
                return;
            }
        }
    }
}

void dsp_graphic_equalizer::reset() {
    for(auto& band : iso_bands_) {
        if(band) {
            band->reset();
        }
    }
}

void dsp_graphic_equalizer::set_iso_band_gain(size_t band, float gain_db) {
    if(band < iso_bands_.size() && iso_bands_[band]) {
        iso_bands_[band]->set_gain(gain_db);
    }
}

float dsp_graphic_equalizer::get_iso_band_gain(size_t band) const {
    if(band < iso_bands_.size() && iso_bands_[band]) {
        return iso_bands_[band]->get_params().gain;
    }
    return 0.0f;
}

void dsp_graphic_equalizer::set_all_iso_bands(float gain_db) {
    for(auto& band : iso_bands_) {
        if(band) {
            band->set_gain(gain_db);
        }
    }
}

const std::vector<float>& dsp_graphic_equalizer::get_iso_frequencies() const {
    static std::vector<float> freqs(ISO_FREQUENCIES, 
                                    ISO_FREQUENCIES + ISO_BAND_COUNT);
    return freqs;
}

// 均衡器工具函数
namespace eq_utils {

float frequency_to_midi(float frequency) {
    return 69.0f + 12.0f * std::log2(frequency / 440.0f);
}

float midi_to_frequency(float midi_note) {
    return 440.0f * std::pow(2.0f, (midi_note - 69.0f) / 12.0f);
}

float frequency_to_octave(float frequency, float reference_freq) {
    return std::log2(frequency / reference_freq);
}

float q_to_bandwidth(float q) {
    return 1.0f / q;
}

float bandwidth_to_q(float bandwidth) {
    return 1.0f / bandwidth;
}

float octave_to_q(float octaves) {
    return std::sqrt(std::pow(2.0f, octaves)) / (std::pow(2.0f, octaves) - 1.0f);
}

float q_to_octave(float q) {
    return std::log2((q * q + std::sqrt(q * q * q * q + 4.0f * q * q)) / (2.0f * q * q));
}

float db_to_linear(float gain_db) {
    return std::pow(10.0f, gain_db / 20.0f);
}

float linear_to_db(float gain_linear) {
    return 20.0f * std::log10(gain_linear);
}

std::vector<std::complex<float>> calculate_combined_response(
    const std::vector<eq_band>& bands, 
    const std::vector<float>& frequencies, 
    float sample_rate) {
    
    std::vector<std::complex<float>> responses;
    responses.reserve(frequencies.size());
    
    for(float freq : frequencies) {
        std::complex<float> total_response(1.0f, 0.0f);
        
        for(const auto& band : bands) {
            if(band.is_enabled()) {
                auto band_response = band.get_frequency_response(freq, sample_rate);
                total_response *= band_response;
            }
        }
        
        responses.push_back(total_response);
    }
    
    return responses;
}

} // namespace eq_utils

} // namespace fb2k