/**
 * @file optimized_format_converter.h
 * @brief Header for optimized format conversion only
 * @date 2025-12-13
 */

#pragma once

#include "optimized_audio_processor.h"
#include <memory>

namespace audio {

// Sample rate converter interface
class SampleRateConverter {
public:
    virtual ~SampleRateConverter() = default;
    virtual bool initialize(int input_rate, int output_rate, int channels) = 0;
    virtual int convert(const float* input, int input_frames,
                       float* output, int max_output_frames) = 0;
    virtual void reset() = 0;
};

// Linear sample rate converter implementation
class LinearSampleRateConverter : public SampleRateConverter {
public:
    LinearSampleRateConverter();
    ~LinearSampleRateConverter() override = default;

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    void reset() override;

private:
    double ratio_;
    double position_;
    int channels_;
    aligned_vector<float> last_frame_;
};

} // namespace audio