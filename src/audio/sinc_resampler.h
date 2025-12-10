/**
 * @file sinc_resampler.h
 * @brief High-quality sinc interpolation sample rate converter
 * @date 2025-12-10
 */

#pragma once

#include "sample_rate_converter.h"
#include <vector>
#include <memory>

namespace audio {

/**
 * @brief High-quality sinc interpolation sample rate converter
 *
 * Uses windowed sinc interpolation with configurable number of taps.
 * Provides much better quality than linear interpolation at the cost
 * of higher CPU usage.
 */
class SincSampleRateConverter : public ISampleRateConverter {
private:
    int taps_;                     // Number of filter taps
    double cutoff_;                // Cutoff frequency
    double ratio_;                 // Position increment
    double position_;              // Current position in input
    int channels_;                 // Number of audio channels
    int input_rate_;               // Input sample rate
    int output_rate_;              // Output sample rate

    std::vector<float> coefficients_;  // Sinc filter coefficients
    std::vector<float> delay_buffer_;  // Overlap buffer for continuity

    /**
     * Generate Kaiser-windowed sinc coefficients
     */
    void generate_sinc_coefficients();

    /**
     * Perform sinc interpolation at a fractional position
     * @param input Input buffer (must have sufficient padding)
     * @param position Fractional position
     * @return Interpolated sample value
     */
    float sinc_interpolate(const float* input, double position);

public:
    /**
     * Constructor
     * @param taps Number of filter taps (higher = better quality but slower)
     */
    explicit SincSampleRateConverter(int taps = 8);

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override;
    void reset() override;
    const char* get_name() const override { return "Sinc"; }
    const char* get_description() const override {
        return "Windowed sinc interpolation resampler (professional quality)";
    }
};

/**
 * @brief Factory for high quality sample rate converter (8 taps)
 */
class HighQualitySampleRateConverterFactory {
public:
    /**
     * Create high quality sample rate converter
     * @return Unique pointer to converter
     */
    static std::unique_ptr<ISampleRateConverter> create();
};

/**
 * @brief Factory for best quality sample rate converter (16 taps)
 */
class BestQualitySampleRateConverterFactory {
public:
    /**
     * Create best quality sample rate converter
     * @return Unique pointer to converter
     */
    static std::unique_ptr<ISampleRateConverter> create();
};

} // namespace audio