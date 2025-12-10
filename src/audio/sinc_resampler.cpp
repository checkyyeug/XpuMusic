/**
 * @file sinc_resampler.cpp
 * @brief High-quality sinc interpolation sample rate converter
 * @date 2025-12-10
 */

#include "sinc_resampler.h"
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace audio {

SincSampleRateConverter::SincSampleRateConverter(int taps)
    : taps_(taps)
    , cutoff_(0.45)
    , position_(0.0)
    , channels_(0)
    , input_rate_(0)
    , output_rate_(0) {

    // Validate taps (must be odd for symmetric filter)
    if (taps_ % 2 == 0) {
        taps_++;
    }
}

bool SincSampleRateConverter::initialize(int input_rate, int output_rate, int channels) {
    if (input_rate <= 0 || output_rate <= 0 || channels <= 0) {
        return false;
    }

    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;
    position_ = 0.0;

    // Calculate conversion ratio
    ratio_ = static_cast<double>(input_rate) / output_rate;

    // Calculate cutoff frequency (for anti-aliasing)
    if (output_rate < input_rate) {
        // Downsampling - need anti-aliasing
        cutoff_ = (output_rate / 2.0) / input_rate * 0.95;  // 95% of Nyquist
    } else {
        // Upsampling - can use full bandwidth
        cutoff_ = 0.45;  // 90% of Nyquist
    }

    // Generate sinc filter coefficients
    generate_sinc_coefficients();

    // Allocate buffers
    delay_buffer_.resize(taps_ * channels_, 0.0f);

    return true;
}

void SincSampleRateConverter::generate_sinc_coefficients() {
    coefficients_.resize(taps_);

    // Generate Kaiser-windowed sinc coefficients
    double beta = 6.0;  // Kaiser window parameter for good stop-band attenuation
    double m = (taps_ - 1) / 2.0;

    for (int i = 0; i < taps_; ++i) {
        double sinc;
        if (i == m) {
            sinc = 2.0 * cutoff_;
        } else {
            double x = M_PI * (i - m);
            sinc = sin(2.0 * cutoff_ * x) / x;
        }

        // Apply Kaiser window
        double alpha = m;
        double arg = 1.0 - ((i - alpha) / alpha) * ((i - alpha) / alpha);
        double window = arg > 0 ?
            std::cosh(beta * std::sqrt(arg)) / std::cosh(beta) : 0.0;

        coefficients_[i] = static_cast<float>(sinc * window);
    }

    // Normalize coefficients
    float sum = 0.0f;
    for (float coeff : coefficients_) {
        sum += coeff;
    }
    for (float& coeff : coefficients_) {
        coeff /= sum;
    }
}

float SincSampleRateConverter::sinc_interpolate(const float* input, double position) {
    int pos_int = static_cast<int>(position);
    double pos_frac = position - pos_int;

    float sum = 0.0f;
    double scale = 0.0;

    int half_taps = taps_ / 2;

    for (int i = 0; i < taps_; ++i) {
        int sample_idx = pos_int + i - half_taps;

        // Skip if outside buffer bounds
        if (sample_idx < 0) continue;

        // Calculate fractional position for this tap
        double tap_pos = (i - half_taps) + pos_frac;

        // Calculate sinc value (simplified for performance)
        double sinc_val;
        if (std::abs(tap_pos) < 1e-6) {
            sinc_val = 1.0;
        } else {
            double x = M_PI * tap_pos;
            sinc_val = sin(2.0 * cutoff_ * x) / x;
            // Apply Kaiser window
            double arg = 1.0 - (tap_pos / half_taps) * (tap_pos / half_taps);
            double window = arg > 0 ? std::cosh(6.0 * std::sqrt(arg)) / std::cosh(6.0) : 0.0;
            sinc_val *= window;
        }

        sum += input[sample_idx] * static_cast<float>(sinc_val);
        scale += sinc_val;
    }

    return scale != 0.0f ? sum / static_cast<float>(scale) : 0.0f;
}

int SincSampleRateConverter::convert(const float* input, int input_frames,
                                     float* output, int max_output_frames) {
    if (!input || !output || input_frames <= 0 || max_output_frames <= 0) {
        return 0;
    }

    int output_frames = 0;
    int half_taps = taps_ / 2;

    // Create extended input buffer with overlap
    std::vector<float> extended_input((input_frames + taps_) * channels_);

    // Copy previous data from delay buffer
    std::memcpy(extended_input.data(), delay_buffer_.data(),
                delay_buffer_.size() * sizeof(float));

    // Copy new input data
    std::memcpy(extended_input.data() + delay_buffer_.size(),
                input, input_frames * channels_ * sizeof(float));

    // Process each output frame
    while (output_frames < max_output_frames && position_ < input_frames) {
        int pos_int = static_cast<int>(position_);

        // Sinc interpolation for each channel
        for (int ch = 0; ch < channels_; ++ch) {
            // Get pointer to channel data
            const float* channel_data = extended_input.data() + ch;

            // Perform sinc interpolation
            output[output_frames * channels_ + ch] =
                sinc_interpolate(channel_data, position_ + half_taps);
        }

        output_frames++;
        position_ += ratio_;
    }

    // Update delay buffer for next call
    if (input_frames >= taps_) {
        std::memcpy(delay_buffer_.data(),
                    input + (input_frames - taps_) * channels_,
                    taps_ * channels_ * sizeof(float));
    } else {
        // Not enough frames, shift and copy what we have
        int shift = std::max(0, taps_ - input_frames);
        if (shift > 0) {
            std::memmove(delay_buffer_.data(),
                         delay_buffer_.data() + (taps_ - shift) * channels_,
                         shift * channels_ * sizeof(float));
        }
        std::memcpy(delay_buffer_.data() + shift * channels_,
                    input, input_frames * channels_ * sizeof(float));
    }

    return output_frames;
}

int SincSampleRateConverter::get_latency() const {
    // Sinc interpolation has half the filter length delay
    return taps_ / 2;
}

void SincSampleRateConverter::reset() {
    position_ = 0.0;
    std::fill(delay_buffer_.begin(), delay_buffer_.end(), 0.0f);
}

// Factory implementations
std::unique_ptr<ISampleRateConverter> HighQualitySampleRateConverterFactory::create() {
    return std::make_unique<SincSampleRateConverter>(8);  // 8 taps for high quality
}

std::unique_ptr<ISampleRateConverter> BestQualitySampleRateConverterFactory::create() {
    return std::make_unique<SincSampleRateConverter>(16); // 16 taps for best quality
}

} // namespace audio