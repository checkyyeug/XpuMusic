/**
 * @file sample_rate_converter_64.cpp
 * @brief 64-bit floating point sample rate converters implementation
 * @date 2025-12-10
 */

#include "sample_rate_converter_64.h"
#include <algorithm>
#include <cstring>
#include <chrono>

namespace audio {

// ============================================================================
// LinearSampleRateConverter64
// ============================================================================

LinearSampleRateConverter64::LinearSampleRateConverter64()
    : input_rate_(44100)
    , output_rate_(44100)
    , channels_(2)
    , ratio_(1.0)
    , phase_(0.0) {
}

bool LinearSampleRateConverter64::configure(int input_rate, int output_rate, int channels) {
    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;
    ratio_ = static_cast<double>(input_rate) / output_rate;
    phase_ = 0.0;

    last_sample_.resize(channels, 0.0);

    return true;
}

int LinearSampleRateConverter64::process(const double* input, double* output, int input_frames) {
    if (!input || !output || input_frames <= 0) return 0;

    const double* src = input;
    double* dst = output;
    int output_frames = 0;
    double phase = phase_;

    for (int i = 0; i < input_frames; ++i) {
        // Generate output samples while we have enough input
        while (phase < 1.0) {
            // Linear interpolation between last_sample_ and current sample
            for (int ch = 0; ch < channels_; ++ch) {
                double current = src[ch];
                double interpolated = last_sample_[ch] * (1.0 - phase) + current * phase;
                *dst++ = interpolated;
            }
            output_frames++;
            phase += ratio_;
        }

        // Store current samples for next iteration
        for (int ch = 0; ch < channels_; ++ch) {
            last_sample_[ch] = src[ch];
        }

        src += channels_;
        phase -= 1.0;
    }

    phase_ = phase;
    return output_frames;
}

int LinearSampleRateConverter64::get_output_latency(int input_frames) const {
    return static_cast<int>(input_frames / ratio_);
}

void LinearSampleRateConverter64::reset() {
    phase_ = 0.0;
    std::fill(last_sample_.begin(), last_sample_.end(), 0.0);
}

// ============================================================================
// CubicSampleRateConverter64
// ============================================================================

CubicSampleRateConverter64::CubicSampleRateConverter64()
    : input_rate_(44100)
    , output_rate_(44100)
    , channels_(2)
    , ratio_(1.0)
    , phase_(0.0) {
}

bool CubicSampleRateConverter64::configure(int input_rate, int output_rate, int channels) {
    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;
    ratio_ = static_cast<double>(input_rate) / output_rate;
    phase_ = 0.0;

    history_.resize(channels_ * 4, 0.0);  // 4 samples per channel

    return true;
}

double CubicSampleRateConverter64::cubic_interp(double y0, double y1, double y2, double y3, double mu) const {
    double mu2 = mu * mu;
    double a0 = y3 - y2 - y0 + y1;
    double a1 = y0 - y1 - a0;
    double a2 = y2 - y0;
    double a3 = y1;

    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
}

int CubicSampleRateConverter64::process(const double* input, double* output, int input_frames) {
    if (!input || !output || input_frames <= 0) return 0;

    const double* src = input;
    double* dst = output;
    int output_frames = 0;
    double phase = phase_;

    // Copy input to history buffer (sliding window)
    for (int i = 0; i < input_frames; ++i) {
        // Shift history
        std::copy(history_.begin() + channels_, history_.end(), history_.begin());

        // Add new samples
        for (int ch = 0; ch < channels_; ++ch) {
            history_[channels_ * 3 + ch] = src[ch];
        }

        // Generate output samples
        while (phase < 1.0) {
            for (int ch = 0; ch < channels_; ++ch) {
                double interpolated = cubic_interp(
                    history_[ch],
                    history_[channels_ + ch],
                    history_[channels_ * 2 + ch],
                    history_[channels_ * 3 + ch],
                    phase
                );
                *dst++ = interpolated;
            }
            output_frames++;
            phase += ratio_;
        }

        src += channels_;
        phase -= 1.0;
    }

    phase_ = phase;
    return output_frames;
}

int CubicSampleRateConverter64::get_output_latency(int input_frames) const {
    return static_cast<int>(input_frames / ratio_);
}

void CubicSampleRateConverter64::reset() {
    phase_ = 0.0;
    std::fill(history_.begin(), history_.end(), 0.0);
}

// ============================================================================
// SincSampleRateConverter64
// ============================================================================

SincSampleRateConverter64::SincSampleRateConverter64(int taps)
    : input_rate_(44100)
    , output_rate_(44100)
    , channels_(2)
    , ratio_(1.0)
    , taps_(taps)
    , cutoff_(0.45)
    , buffer_pos_(0)
    , phase_(0.0)
    , kaiser_beta_(6.0) {

    generate_sinc_table();
}

bool SincSampleRateConverter64::configure(int input_rate, int output_rate, int channels) {
    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;
    ratio_ = static_cast<double>(input_rate) / output_rate;
    phase_ = 0.0;

    // Calculate cutoff frequency
    if (ratio_ > 1.0) {
        cutoff_ = 0.45 / ratio_;  // Low-pass filter for downsampling
    } else {
        cutoff_ = 0.45;  // For upsampling
    }

    // Calculate buffer size needed
    int buffer_size = taps_ * 4;  // Extra space for smooth interpolation
    buffer_.resize(buffer_size * channels_, 0.0);
    buffer_pos_ = taps_ / 2;  // Start in middle

    return true;
}

double SincSampleRateConverter64::kaiser_bessel_i0(double x) const {
    const double epsilon = 1e-21;
    double sum = 1.0;
    double term = 1.0;
    double x_squared = x * x / 4.0;

    for (int k = 1; k < 100; ++k) {
        term *= x_squared / (k * k);
        sum += term;
        if (term < epsilon * sum) break;
    }

    return sum;
}

void SincSampleRateConverter64::generate_sinc_table() {
    sinc_window_.resize(taps_ * 4 + 1);  // 4x oversampling for smooth interpolation

    int half_taps = taps_ / 2;
    for (int i = 0; i <= taps_ * 4; ++i) {
        double x = i / 4.0 - half_taps;

        if (x == 0.0) {
            sinc_window_[i] = 1.0;
        } else {
            // Sinc function
            double sinc_x = sin(M_PI * x) / (M_PI * x);

            // Kaiser window
            double alpha = x / half_taps;
            double window = kaiser_bessel_i0(kaiser_beta_ * sqrt(1 - alpha * alpha)) /
                           kaiser_bessel_i0(kaiser_beta_);

            sinc_window_[i] = sinc_x * window;
        }
    }
}

double SincSampleRateConverter64::interpolate(const double* buffer, int pos, int channels, int ch, double phase) const {
    double sum = 0.0;
    int half_taps = taps_ / 2;

    // Use pre-computed sinc table with linear interpolation
    double table_phase = phase * 4.0;
    int table_index = static_cast<int>(table_phase);
    double frac = table_phase - table_index;

    for (int i = -half_taps; i <= half_taps; ++i) {
        int buf_pos = pos + i;
        if (buf_pos < 0 || buf_pos >= static_cast<int>(buffer_.size() / channels)) {
            continue;
        }

        double sample = buffer[buf_pos * channels + ch];

        // Interpolate sinc values
        int sinc_idx = (i + half_taps) * 4 + table_index;
        double sinc_val;

        if (sinc_idx + 1 < static_cast<int>(sinc_window_.size())) {
            sinc_val = sinc_window_[sinc_idx] * (1.0 - frac) + sinc_window_[sinc_idx + 1] * frac;
        } else {
            sinc_val = sinc_window_[sinc_idx];
        }

        sum += sample * sinc_val;
    }

    return sum;
}

int SincSampleRateConverter64::process(const double* input, double* output, int input_frames) {
    if (!input || !output || input_frames <= 0) return 0;

    const double* src = input;
    double* dst = output;
    int output_frames = 0;
    double phase = phase_;

    // Copy input samples to circular buffer
    for (int i = 0; i < input_frames; ++i) {
        for (int ch = 0; ch < channels_; ++ch) {
            buffer_[buffer_pos_ * channels_ + ch] = src[ch];
        }

        buffer_pos_++;
        if (buffer_pos_ * channels_ >= static_cast<int>(buffer_.size())) {
            buffer_pos_ = 0;
        }

        // Generate output samples
        while (phase < 1.0) {
            for (int ch = 0; ch < channels_; ++ch) {
                double interpolated = interpolate(buffer_.data(), buffer_pos_, channels_, ch, phase);
                *dst++ = interpolated;
            }
            output_frames++;
            phase += ratio_;
        }

        src += channels_;
        phase -= 1.0;
    }

    phase_ = phase;
    return output_frames;
}

int SincSampleRateConverter64::get_output_latency(int input_frames) const {
    return static_cast<int>(input_frames / ratio_);
}

void SincSampleRateConverter64::reset() {
    phase_ = 0.0;
    buffer_pos_ = taps_ / 2;
    std::fill(buffer_.begin(), buffer_.end(), 0.0);
}

// ============================================================================
// AdaptiveSampleRateConverter64
// ============================================================================

AdaptiveSampleRateConverter64::AdaptiveSampleRateConverter64()
    : current_quality_("good")
    , input_rate_(44100)
    , output_rate_(44100)
    , channels_(2)
    , cpu_threshold_(0.8)
    , enable_adaptive_(true)
    , frame_count_(0)
    , total_time_(0.0) {
}

bool AdaptiveSampleRateConverter64::configure(int input_rate, int output_rate, int channels) {
    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;

    // Select initial converter
    select_converter(current_quality_);

    if (converter_) {
        return converter_->configure(input_rate, output_rate, channels);
    }

    return false;
}

void AdaptiveSampleRateConverter64::select_converter(const std::string& quality) {
    if (current_quality_ == quality && converter_) {
        return;  // Already using this quality
    }

    current_quality_ = quality;

    if (quality == "fast") {
        converter_ = std::make_unique<LinearSampleRateConverter64>();
    } else if (quality == "good") {
        converter_ = std::make_unique<CubicSampleRateConverter64>();
    } else if (quality == "high") {
        converter_ = std::make_unique<SincSampleRateConverter64>(8);
    } else if (quality == "best") {
        converter_ = std::make_unique<SincSampleRateConverter64>(16);
    } else {
        // Default to good quality
        converter_ = std::make_unique<CubicSampleRateConverter64>();
        current_quality_ = "good";
    }
}

int AdaptiveSampleRateConverter64::process(const double* input, double* output, int input_frames) {
    if (!converter_) return 0;

    auto start = std::chrono::high_resolution_clock::now();
    int result = converter_->process(input, output, input_frames);
    auto end = std::chrono::high_resolution_clock::now();

    // Update performance metrics
    frame_count_ += input_frames;
    total_time_ += std::chrono::duration<double, std::milli>(end - start).count();

    // Check if we should adapt quality (every 10000 frames)
    if (enable_adaptive_ && frame_count_ >= 10000) {
        double avg_time_per_frame = total_time_ / frame_count_;
        double estimated_cpu = avg_time_per_frame / 10.0;  // Rough estimate

        // Adjust quality based on CPU usage
        if (estimated_cpu > cpu_threshold_) {
            // Too slow, reduce quality
            if (current_quality_ == "best") {
                select_converter("high");
            } else if (current_quality_ == "high") {
                select_converter("good");
            } else if (current_quality_ == "good") {
                select_converter("fast");
            }
        } else if (estimated_cpu < cpu_threshold_ * 0.5) {
            // Fast enough, increase quality
            if (current_quality_ == "fast") {
                select_converter("good");
            } else if (current_quality_ == "good") {
                select_converter("high");
            } else if (current_quality_ == "high") {
                select_converter("best");
            }
        }

        // Reset metrics
        frame_count_ = 0;
        total_time_ = 0.0;

        // Reconfigure with new quality
        if (converter_) {
            converter_->configure(input_rate_, output_rate_, channels_);
        }
    }

    return result;
}

int AdaptiveSampleRateConverter64::get_output_latency(int input_frames) const {
    if (converter_) {
        return converter_->get_output_latency(input_frames);
    }
    return 0;
}

void AdaptiveSampleRateConverter64::reset() {
    frame_count_ = 0;
    total_time_ = 0.0;
    if (converter_) {
        converter_->reset();
    }
}

int AdaptiveSampleRateConverter64::get_latency() const {
    if (converter_) {
        return converter_->get_latency();
    }
    return 0;
}

} // namespace audio