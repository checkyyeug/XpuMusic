/**
 * @file cubic_resampler.cpp
 * @brief Cubic interpolation sample rate converter implementation
 * @date 2025-12-10
 */

#include "cubic_resampler.h"
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace audio {

CubicSampleRateConverter::CubicSampleRateConverter()
    : ratio_(1.0)
    , position_(0.0)
    , channels_(0)
    , input_rate_(0)
    , output_rate_(0)
    , history_size_(4) {
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

    // Initialize history buffer (need 4 frames per channel for cubic interpolation)
    history_buffer_.resize(channels_ * history_size_, 0.0f);

    // Initialize anti-aliasing filter if downsampling
    if (output_rate < input_rate) {
        double cutoff = output_rate / (2.0 * input_rate) * 0.95;  // 95% of Nyquist
        filter_ = std::make_unique<AntiAliasingFilter>(cutoff, 101);
    }

    return true;
}

float CubicSampleRateConverter::cubic_interpolate(float y0, float y1, float y2, float y3, float x) {
    // Hermite cubic interpolation (smoother than Catmull-Rom)
    // Provides good balance between smoothness and computational efficiency
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
    int total_input_frames = input_frames + history_size_;

    // Create extended buffer with history
    std::vector<float> extended_input(total_input_frames * channels_);

    // Copy history buffer first
    std::memcpy(extended_input.data(), history_buffer_.data(),
                history_buffer_.size() * sizeof(float));

    // Copy new input
    std::memcpy(extended_input.data() + history_buffer_.size(),
                input, input_frames * channels_ * sizeof(float));

    // Apply anti-aliasing filter if downsampling
    if (filter_ && output_rate_ < input_rate_) {
        std::vector<float> filtered_input(total_input_frames * channels_);
        filter_->process(extended_input.data(), filtered_input.data(),
                        total_input_frames, channels_);
        extended_input = std::move(filtered_input);
    }

    // Process each output sample
    while (output_frames < max_output_frames && position_ < input_frames + 1) {
        int pos_int = static_cast<int>(position_);
        double pos_frac = position_ - pos_int;

        // Cubic interpolation for each channel
        for (int ch = 0; ch < channels_; ++ch) {
            // Get 4 consecutive samples for cubic interpolation
            float y0 = extended_input[(pos_int - 1) * channels_ + ch];
            float y1 = extended_input[(pos_int) * channels_ + ch];
            float y2 = extended_input[(pos_int + 1) * channels_ + ch];
            float y3 = extended_input[(pos_int + 2) * channels_ + ch];

            output[output_frames * channels_ + ch] =
                cubic_interpolate(y0, y1, y2, y3, static_cast<float>(pos_frac));
        }

        output_frames++;
        position_ += ratio_;
    }

    // Update history buffer for next call
    if (input_frames >= history_size_) {
        std::memcpy(history_buffer_.data(),
                    input + (input_frames - history_size_) * channels_,
                    history_size_ * channels_ * sizeof(float));
    } else {
        // Not enough frames, shift existing history
        int shift = std::min(input_frames, history_size_ - 1);
        std::memmove(history_buffer_.data(),
                     history_buffer_.data() + shift * channels_,
                     (history_size_ - shift) * channels_ * sizeof(float));
        std::memcpy(history_buffer_.data() + (history_size_ - shift) * channels_,
                    input, input_frames * channels_ * sizeof(float));
    }

    return output_frames;
}

int CubicSampleRateConverter::get_latency() const {
    // Cubic interpolation has 1 sample delay plus filter delay
    int filter_delay = filter_ ? 50 : 0;  // Approximate
    return 1 + filter_delay;
}

void CubicSampleRateConverter::reset() {
    position_ = 0.0;
    std::fill(history_buffer_.begin(), history_buffer_.end(), 0.0f);
    if (filter_) {
        filter_->reset();
    }
}

// AntiAliasingFilter implementation
AntiAliasingFilter::AntiAliasingFilter(double cutoff, int taps)
    : cutoff_(cutoff)
    , taps_(taps)
    , delay_index_(0) {

    coefficients_.resize(taps_);
    delay_line_.resize(taps_);

    // Generate FIR filter coefficients using Kaiser window
    double beta = 6.0;  // Kaiser window parameter
    double m = (taps_ - 1) / 2.0;

    for (int i = 0; i < taps_; ++i) {
        double sinc;
        if (i == m) {
            sinc = 2.0 * cutoff_;
        } else {
            double x = M_PI * (i - m);
            sinc = sin(2.0 * cutoff_ * x) / x;
        }

        // Kaiser window
        double alpha = m;
        double arg = 1.0 - ((i - alpha) / alpha) * ((i - alpha) / alpha);
        double window = arg > 0 ?
            std::cosh(beta * std::sqrt(arg)) / std::cosh(beta) : 0.0;

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

} // namespace audio