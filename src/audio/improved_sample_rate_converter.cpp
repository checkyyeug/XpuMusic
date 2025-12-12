/**
 * @file improved_sample_rate_converter.cpp
 * @brief Improved sample rate converter implementation
 * @date 2025-12-10
 */

#include "improved_sample_rate_converter.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace audio {

// AntiAliasingFilter Implementation
AntiAliasingFilter::AntiAliasingFilter(int cutoff_ratio, int taps)
    : delay_index_(0)
    , cutoff_ratio_(cutoff_ratio)
    , taps_(taps) {

    coefficients_.resize(taps_);
    delay_line_.resize(taps_);

    // Generate low-pass FIR filter coefficients using Hamming window
    double cutoff = cutoff_ratio_ / 100.0;
    double m = (taps_ - 1) / 2.0;

    for (int i = 0; i < taps_; ++i) {
        double sinc;
        if (i == m) {
            sinc = 2.0 * cutoff;
        } else {
            double x = M_PI * (i - m);
            sinc = sin(2.0 * cutoff * x) / x;
        }

        // Hamming window
        double window = 0.54 - 0.46 * cos(2.0 * M_PI * i / (taps_ - 1));

        coefficients_[i] = static_cast<float>(sinc * window);
    }

    reset();
}

void AntiAliasingFilter::process(const float* input, float* output,
                                int frames, int channels) {
    for (int frame = 0; frame < frames; ++frame) {
        for (int ch = 0; ch < channels; ++ch) {
            // Update delay line
            delay_line_[delay_index_] = input[frame * channels + ch];

            // Apply FIR filter
            float sum = 0.0f;
            for (int i = 0; i < taps_; ++i) {
                int idx = (delay_index_ - i + taps_) % taps_;
                sum += delay_line_[idx] * coefficients_[i];
            }

            output[frame * channels + ch] = sum;
        }

        delay_index_ = (delay_index_ + 1) % taps_;
    }
}

void AntiAliasingFilter::reset() {
    std::fill(delay_line_.begin(), delay_line_.end(), 0.0f);
    delay_index_ = 0;
}

// CubicSampleRateConverter Implementation
CubicSampleRateConverter::CubicSampleRateConverter()
    : ratio_(1.0)
    , position_(0.0)
    , channels_(0)
    , input_rate_(0)
    , output_rate_(0) {
}

bool CubicSampleRateConverter::initialize(int input_rate, int output_rate, int channels) {
    if (input_rate <= 0 || output_rate <= 0 || channels <= 0) {
        return false;
    }

    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;
    ratio_ = static_cast<double>(input_rate) / output_rate;
    position_ = 0.0;

    // Initialize history buffer (4 frames per channel)
    history_buffer_.resize(channels_ * 4, 0.0f);

    return true;
}

float CubicSampleRateConverter::cubic_interpolate(float y0, float y1, float y2, float y3, float x) {
    // Catmull-Rom spline interpolation
    float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c = -0.5f * y0 + 0.5f * y2;
    float d = y1;

    return a * x * x * x + b * x * x + c * x + d;
}

int CubicSampleRateConverter::convert(const float* input, int input_frames,
                                     float* output, int max_output_frames) {
    if (!input || !output || input_frames <= 0 || max_output_frames <= 0) {
        return 0;
    }

    int output_frames = 0;

    // Copy input to history buffer
    int history_frames = 4;
    std::vector<float> extended_input((input_frames + history_frames) * channels_);

    // Fill with zeros and history
    std::memcpy(extended_input.data(), history_buffer_.data(),
                history_frames * channels_ * sizeof(float));

    // Copy new input
    std::memcpy(extended_input.data() + history_frames * channels_,
                input, input_frames * channels_ * sizeof(float));

    // Save last frames for next call
    if (input_frames >= 4) {
        std::memcpy(history_buffer_.data(),
                    input + (input_frames - 4) * channels_,
                    4 * channels_ * sizeof(float));
    }

    // Process each output frame
    while (output_frames < max_output_frames && position_ < input_frames + 2) {
        int pos_int = static_cast<int>(position_);
        double pos_frac = position_ - pos_int;

        // Cubic interpolation for each channel
        for (int ch = 0; ch < channels_; ++ch) {
            int idx = pos_int * channels_ + ch;

            float y0 = extended_input[idx - channels_];
            float y1 = extended_input[idx];
            float y2 = extended_input[idx + channels_];
            float y3 = extended_input[idx + channels_ * 2];

            output[output_frames * channels_ + ch] =
                cubic_interpolate(y0, y1, y2, y3, static_cast<float>(pos_frac));
        }

        output_frames++;
        position_ += ratio_;
    }

    return output_frames;
}

void CubicSampleRateConverter::reset() {
    position_ = 0.0;
    std::fill(history_buffer_.begin(), history_buffer_.end(), 0.0f);
}

// SincSampleRateConverter Implementation
SincSampleRateConverter::SincSampleRateConverter(int taps)
    : ratio_(1.0)
    , position_(0.0)
    , channels_(0)
    , input_rate_(0)
    , output_rate_(0)
    , quality_taps_(taps) {
}

void SincSampleRateConverter::generate_sinc_table(SincTable& table, int taps, double cutoff) {
    table.coefficients.resize(taps);
    table.taps = taps;
    table.cutoff = cutoff;
    table.windowed = true;

    double m = (taps - 1) / 2.0;

    for (int i = 0; i < taps; ++i) {
        double sinc;
        if (i == m) {
            sinc = cutoff * 2.0;
        } else {
            double x = M_PI * (i - m);
            sinc = sin(cutoff * 2.0 * x) / x;
        }

        // Kaiser window
        double beta = 6.0;
        double alpha = m;
        double arg = 1.0 - ((i - alpha) / alpha) * ((i - alpha) / alpha);
        double window = arg > 0 ?
            std::cosh(beta * std::sqrt(arg)) / std::cosh(beta) : 0.0;

        table.coefficients[i] = static_cast<float>(sinc * window);
    }
}

float SincSampleRateConverter::sinc_interpolate(const float* input, double position, int taps) {
    int pos_int = static_cast<int>(position);
    double pos_frac = position - pos_int;

    float sum = 0.0f;
    double scale = 0.0;

    for (int i = 0; i < taps; ++i) {
        double sinc_pos = (i - taps / 2) + pos_frac;

        // Skip if outside input range
        int sample_idx = pos_int + i - taps / 2 + 1;
        if (sample_idx < 0) continue;

        float coeff = sinc_coefficients_[i];
        sum += input[sample_idx] * coeff;
        scale += coeff;
    }

    return scale != 0.0f ? sum / static_cast<float>(scale) : 0.0f;
}

bool SincSampleRateConverter::initialize(int input_rate, int output_rate, int channels) {
    if (input_rate <= 0 || output_rate <= 0 || channels <= 0) {
        return false;
    }

    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;
    ratio_ = static_cast<double>(input_rate) / output_rate;
    position_ = 0.0;

    // Generate sinc coefficients
    sinc_coefficients_.resize(quality_taps_);
    double cutoff = std::min(0.45, 0.9 * output_rate_ / (2.0 * input_rate_));

    double m = (quality_taps_ - 1) / 2.0;
    for (int i = 0; i < quality_taps_; ++i) {
        double sinc;
        if (i == m) {
            sinc = cutoff * 2.0;
        } else {
            double x = M_PI * (i - m);
            sinc = sin(cutoff * 2.0 * x) / x;
        }

        // Kaiser window
        double beta = 6.0;
        double alpha = m;
        double arg = 1.0 - ((i - alpha) / alpha) * ((i - alpha) / alpha);
        double window = arg > 0 ?
            std::cosh(beta * std::sqrt(arg)) / std::cosh(beta) : 0.0;

        sinc_coefficients_[i] = static_cast<float>(sinc * window);
    }

    // Allocate delay buffer
    delay_buffer_.resize(quality_taps_ * channels_, 0.0f);

    return true;
}

int SincSampleRateConverter::convert(const float* input, int input_frames,
                                    float* output, int max_output_frames) {
    if (!input || !output || input_frames <= 0 || max_output_frames <= 0) {
        return 0;
    }

    int output_frames = 0;
    int half_taps = quality_taps_ / 2;

    // Copy input to delay buffer with overlap
    std::vector<float> extended_input((input_frames + quality_taps_) * channels_);
    std::memcpy(extended_input.data(), delay_buffer_.data(),
                delay_buffer_.size() * sizeof(float));
    std::memcpy(extended_input.data() + delay_buffer_.size(),
                input, input_frames * channels_ * sizeof(float));

    // Save last taps for next call
    if (input_frames >= quality_taps_) {
        std::memcpy(delay_buffer_.data(),
                    input + (input_frames - quality_taps_) * channels_,
                    delay_buffer_.size() * sizeof(float));
    }

    // Process each output frame
    while (output_frames < max_output_frames && position_ < input_frames + half_taps) {
        int pos_int = static_cast<int>(position_);

        for (int ch = 0; ch < channels_; ++ch) {
            float sum = 0.0f;
            double scale = 0.0;

            for (int i = 0; i < quality_taps_; ++i) {
                int sample_idx = (pos_int + i - half_taps + 1) * channels_ + ch;
                if (sample_idx >= 0 && sample_idx < extended_input.size()) {
                    double sinc_pos = (i - half_taps) + (position_ - pos_int);
                    // Simplified sinc calculation
                    float coeff = sinc_coefficients_[i];
                    sum += extended_input[sample_idx] * coeff;
                    scale += coeff;
                }
            }

            output[output_frames * channels_ + ch] =
                scale != 0.0f ? sum / static_cast<float>(scale) : 0.0f;
        }

        output_frames++;
        position_ += ratio_;
    }

    return output_frames;
}

void SincSampleRateConverter::reset() {
    position_ = 0.0;
    std::fill(delay_buffer_.begin(), delay_buffer_.end(), 0.0f);
}

// ImprovedSampleRateConverter Implementation
ImprovedSampleRateConverter::ImprovedSampleRateConverter(
    ResamplerQuality quality, bool enable_filtering)
    : quality_(quality)
    , enable_filtering_(enable_filtering) {
}

bool ImprovedSampleRateConverter::initialize(int input_rate, int output_rate, int channels) {
    if (input_rate <= 0 || output_rate <= 0 || channels <= 0) {
        return false;
    }

    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;

    // Create converter based on quality
    switch (quality_) {
        case ResamplerQuality::Fast:
            converter_ = std::make_unique<LinearSampleRateConverter>();
            break;
        case ResamplerQuality::Good:
            converter_ = std::make_unique<CubicSampleRateConverter>();
            break;
        case ResamplerQuality::High:
            converter_ = std::make_unique<SincSampleRateConverter>(4);
            break;
        case ResamplerQuality::VeryHigh:
            converter_ = std::make_unique<SincSampleRateConverter>(8);
            break;
        case ResamplerQuality::Best:
            converter_ = std::make_unique<SincSampleRateConverter>(16);
            break;
        default:
            converter_ = std::make_unique<CubicSampleRateConverter>();
    }

    // Initialize converter
    if (!converter_->initialize(input_rate, output_rate, channels)) {
        return false;
    }

    // Create anti-aliasing filter if needed
    if (enable_filtering_ && output_rate < input_rate) {
        // Filter needed when downsampling
        anti_aliasing_filter_ = std::make_unique<AntiAliasingFilter>(
            static_cast<int>(output_rate * 100 / input_rate / 2), 101);
    }

    return true;
}

int ImprovedSampleRateConverter::convert(const float* input, int input_frames,
                                        float* output, int max_output_frames) {
    if (!input || !output || input_frames <= 0 || max_output_frames <= 0) {
        return 0;
    }

    std::vector<float> filtered_input;
    const float* process_input = input;

    // Apply anti-aliasing filter if downsampling
    if (anti_aliasing_filter_ && output_rate_ < input_rate_) {
        filtered_input.resize(input_frames * channels_);
        anti_aliasing_filter_->process(input, filtered_input.data(),
                                       input_frames, channels_);
        process_input = filtered_input.data();
    }

    // Perform conversion
    return converter_->convert(process_input, input_frames,
                              output, max_output_frames);
}

int ImprovedSampleRateConverter::get_latency() const {
    int converter_latency = converter_->get_latency();

    // Add filter delay if present
    if (anti_aliasing_filter_) {
        converter_latency += 50;  // Approximate filter delay
    }

    return converter_latency;
}

void ImprovedSampleRateConverter::reset() {
    if (converter_) {
        converter_->reset();
    }
    if (anti_aliasing_filter_) {
        anti_aliasing_filter_->reset();
    }
}

const char* ImprovedSampleRateConverter::get_name() const {
    switch (quality_) {
        case ResamplerQuality::Fast: return "Linear (Fast)";
        case ResamplerQuality::Good: return "Cubic (Good)";
        case ResamplerQuality::High: return "Sinc 4-tap (High)";
        case ResamplerQuality::VeryHigh: return "Sinc 8-tap (Very High)";
        case ResamplerQuality::Best: return "Sinc 16-tap (Best)";
        default: return "Unknown";
    }
}

const char* ImprovedSampleRateConverter::get_description() const {
    return get_quality_description(quality_);
}

void ImprovedSampleRateConverter::set_quality(ResamplerQuality quality) {
    if (quality_ != quality) {
        quality_ = quality;

        // Recreate converter with new quality
        int ir = input_rate_;
        int or_ = output_rate_;
        int ch = channels_;

        converter_.reset();
        anti_aliasing_filter_.reset();

        initialize(ir, or_, ch);
    }
}

double ImprovedSampleRateConverter::get_estimated_cpu_usage() const {
    switch (quality_) {
        case ResamplerQuality::Fast: return 0.1;      // Very low
        case ResamplerQuality::Good: return 0.5;      // Low
        case ResamplerQuality::High: return 2.0;      // Medium
        case ResamplerQuality::VeryHigh: return 5.0;  // High
        case ResamplerQuality::Best: return 12.0;     // Very High
        default: return 1.0;
    }
}

const char* ImprovedSampleRateConverter::get_quality_description(ResamplerQuality quality) {
    switch (quality) {
        case ResamplerQuality::Fast:
            return "Fast linear interpolation for real-time applications";
        case ResamplerQuality::Good:
            return "Cubic interpolation with good quality for general use";
        case ResamplerQuality::High:
            return "4-point sinc interpolation for high quality audio";
        case ResamplerQuality::VeryHigh:
            return "8-point sinc interpolation for professional use";
        case ResamplerQuality::Best:
            return "16-point sinc interpolation for critical applications";
        default:
            return "Unknown quality level";
    }
}

// ImprovedSampleRateConverterFactory Implementation
std::unique_ptr<ImprovedSampleRateConverter>
ImprovedSampleRateConverterFactory::create(ResamplerQuality quality, bool enable_filtering) {
    return std::make_unique<ImprovedSampleRateConverter>(quality, enable_filtering);
}

std::vector<ResamplerQuality> ImprovedSampleRateConverterFactory::get_available_qualities() {
    return {
        ResamplerQuality::Fast,
        ResamplerQuality::Good,
        ResamplerQuality::High,
        ResamplerQuality::VeryHigh,
        ResamplerQuality::Best
    };
}

std::string ImprovedSampleRateConverterFactory::quality_to_string(ResamplerQuality quality) {
    switch (quality) {
        case ResamplerQuality::Fast: return "fast";
        case ResamplerQuality::Good: return "good";
        case ResamplerQuality::High: return "high";
        case ResamplerQuality::VeryHigh: return "very_high";
        case ResamplerQuality::Best: return "best";
        default: return "unknown";
    }
}

} // namespace audio