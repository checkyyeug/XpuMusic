/**
 * @file sample_rate_converter.cpp
 * @brief Sample rate conversion implementations
 * @date 2025-12-10
 */

#include "sample_rate_converter.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <fstream>

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

    // Initialize last frame with zeros
    last_frame_.resize(channels_, 0.0f);

    return true;
}

int LinearSampleRateConverter::convert(const float* input, int input_frames,
                                        float* output, int max_output_frames) {
    if (!input || !output || input_frames <= 0 || max_output_frames <= 0) {
        return 0;
    }

    int output_frames = 0;

    // Process each output sample
    while (output_frames < max_output_frames) {
        // Find current integer position in input
        int pos_int = static_cast<int>(position_);
        double pos_frac = position_ - pos_int;

        // Check if we have enough input data
        if (pos_int >= input_frames) {
            break;
        }

        // Linear interpolation for each channel
        for (int ch = 0; ch < channels_; ++ch) {
            float sample1 = input[pos_int * channels_ + ch];
            float sample2 = (pos_int < input_frames - 1) ?
                          input[(pos_int + 1) * channels_ + ch] :
                          last_frame_[ch];

            // Linear interpolation
            output[output_frames * channels_ + ch] =
                sample1 + static_cast<float>((sample2 - sample1) * pos_frac);
        }

        output_frames++;
        position_ += ratio_;
    }

    // Store last frame for next conversion
    if (input_frames > 0) {
        ::memcpy(last_frame_.data(),
                  &input[(input_frames - 1) * channels_],
                  channels_ * sizeof(float));
    }

    return output_frames;
}

int LinearSampleRateConverter::get_latency() const {
    // Linear interpolation has minimal latency
    return 0;
}

void LinearSampleRateConverter::reset() {
    position_ = 0.0;
    std::fill(last_frame_.begin(), last_frame_.end(), 0.0f);
}

SampleRateConverterFactory::SampleRateConverterFactory() {}

std::unique_ptr<ISampleRateConverter> SampleRateConverterFactory::create(const std::string& type) {
    if (type == "linear") {
        return std::make_unique<LinearSampleRateConverter>();
    }
    // TODO: Add other converter types
    // else if (type == "cubic") {
    //     return std::make_unique<CubicSampleRateConverter>();
    // }
    // else if (type == "sinc") {
    //     return std::make_unique<SincSampleRateConverter>();
    // }
    else {
        throw std::runtime_error("Unknown sample rate converter type: " + type);
    }
}

std::vector<std::string> SampleRateConverterFactory::list_available() {
    std::vector<std::string> types;
    types.push_back("linear");
    // TODO: Add other types when implemented
    // types.push_back("cubic");
    // types.push_back("sinc");
    return types;
}

bool SampleRateConverterFactory::is_available(const std::string& type) {
    auto available = list_available();
    return std::find(available.begin(), available.end(), type) != available.end();
}

} // namespace audio