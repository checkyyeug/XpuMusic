/**
 * @file linear_resampler.cpp
 * @brief Linear interpolation sample rate converter implementation
 * @date 2025-12-11
 */

#include "sample_rate_converter.h"
#include <cstring>
#include <algorithm>

namespace audio {

LinearSampleRateConverter::LinearSampleRateConverter()
    : ratio_(1.0)
    , position_(0.0)
    , channels_(0)
    , input_rate_(0)
    , output_rate_(0) {
}

bool LinearSampleRateConverter::initialize(int input_rate, int output_rate, int channels) {
    if (input_rate <= 0 || output_rate <= 0 || channels <= 0) {
        return false;
    }

    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;
    ratio_ = static_cast<double>(input_rate) / output_rate;
    position_ = 0.0;

    // Initialize last frame buffer for interpolation
    last_frame_.resize(channels_, 0.0f);

    return true;
}

int LinearSampleRateConverter::convert(const float* input, int input_frames,
                                     float* output, int max_output_frames) {
    if (!input || !output || input_frames <= 0 || max_output_frames <= 0) {
        return 0;
    }

    int output_frames = 0;
    int input_pos = 0;

    // Process each output sample
    while (output_frames < max_output_frames && input_pos < input_frames) {
        int pos_int = static_cast<int>(position_);
        double pos_frac = position_ - pos_int;

        // Linear interpolation for each channel
        for (int ch = 0; ch < channels_; ++ch) {
            float sample1, sample2;
            
            // Get samples for interpolation
            if (pos_int == 0) {
                // Use last frame for first sample
                sample1 = last_frame_[ch];
                sample2 = input[ch];
            } else if (pos_int < input_frames) {
                sample1 = input[(pos_int - 1) * channels_ + ch];
                sample2 = input[pos_int * channels_ + ch];
            } else {
                // Use last available samples
                sample1 = input[(input_frames - 2) * channels_ + ch];
                sample2 = input[(input_frames - 1) * channels_ + ch];
            }

            // Linear interpolation
            output[output_frames * channels_ + ch] = sample1 + (sample2 - sample1) * static_cast<float>(pos_frac);
        }

        output_frames++;
        position_ += ratio_;
        
        // Update input position when we advance to next input sample
        while (position_ >= input_pos + 1 && input_pos < input_frames) {
            input_pos++;
        }
    }

    // Update last frame for next call
    if (input_frames > 0) {
        for (int ch = 0; ch < channels_; ++ch) {
            last_frame_[ch] = input[(input_frames - 1) * channels_ + ch];
        }
    }

    return output_frames;
}

int LinearSampleRateConverter::get_latency() const {
    // Linear interpolation has 1 sample delay
    return 1;
}

void LinearSampleRateConverter::reset() {
    position_ = 0.0;
    std::fill(last_frame_.begin(), last_frame_.end(), 0.0f);
}

} // namespace audio