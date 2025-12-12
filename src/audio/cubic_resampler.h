/**
 * @file cubic_resampler.h
 * @brief Cubic interpolation sample rate converter
 * @date 2025-12-10
 */

#pragma once

#include "sample_rate_converter.h"
#include <vector>
#include <memory>
#include <cmath>

namespace audio {

/**
 * @brief Anti-aliasing low-pass filter for downsampling
 */
class AntiAliasingFilter {
private:
    std::vector<float> coefficients_;
    std::vector<float> delay_line_;
    int delay_index_;
    double cutoff_;
    int taps_;

public:
    /**
     * Constructor
     * @param cutoff Cutoff frequency as fraction of Nyquist (0.0-1.0)
     * @param taps Number of filter taps (should be odd)
     */
    AntiAliasingFilter(double cutoff = 0.45, int taps = 101);

    /**
     * Process audio samples through filter
     * @param input Input buffer
     * @param output Output buffer
     * @param frames Number of frames
     * @param channels Number of channels
     */
    void process(const float* input, float* output, int frames, int channels);

    /**
     * Reset filter state
     */
    void reset();
};

/**
 * @brief Cubic interpolation sample rate converter
 *
 * Provides better quality than linear interpolation while maintaining
 * good performance. Uses Hermite cubic interpolation with
 * anti-aliasing filtering for downsampling.
 */
class CubicSampleRateConverter : public ISampleRateConverter {
private:
    double ratio_;                     // Position increment
    double position_;                  // Current position in input
    int channels_;                    // Number of audio channels
    int input_rate_;                  // Input sample rate
    int output_rate_;                 // Output sample rate
    int history_size_;                // Number of frames to keep in history

    std::vector<float> history_buffer_; // History for continuity
    std::unique_ptr<AntiAliasingFilter> filter_; // Anti-aliasing filter

    /**
     * Perform cubic interpolation between 4 points
     * @param y0 Sample at position -1
     * @param y1 Sample at position 0
     * @param y2 Sample at position +1
     * @param y3 Sample at position +2
     * @param x Fractional position between y1 and y2 (0.0-1.0)
     * @return Interpolated sample value
     */
    float cubic_interpolate(float y0, float y1, float y2, float y3, float x);

public:
    CubicSampleRateConverter();

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override;
    void reset() override;
    const char* get_name() const override { return "Cubic"; }
    const char* get_description() const override {
        return "Cubic interpolation resampler with anti-aliasing (3x better quality than linear)";
    }
};

/**
 * @brief Factory for cubic sample rate converter
 */
class CubicSampleRateConverterFactory {
public:
    /**
     * Create cubic sample rate converter
     * @return Unique pointer to converter
     */
    static std::unique_ptr<ISampleRateConverter> create() {
        return std::make_unique<CubicSampleRateConverter>();
    }
};

} // namespace audio