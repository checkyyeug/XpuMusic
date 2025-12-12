/**
 * @file improved_sample_rate_converter.h
 * @brief Improved sample rate converter with quality options
 * @date 2025-12-10
 */

#pragma once

#include "sample_rate_converter.h"
#include <vector>
#include <memory>
#include <cmath>

namespace audio {

/**
 * @brief Quality levels for sample rate conversion
 */
enum class ResamplerQuality {
    Fast,       // Linear interpolation (current implementation)
    Good,       // Cubic interpolation
    High,       // 4-point sinc interpolation
    VeryHigh,   // 8-point sinc interpolation
    Best        // 16-point sinc interpolation
};

/**
 * @brief Low-pass filter for anti-aliasing
 */
class AntiAliasingFilter {
private:
    std::vector<float> coefficients_;
    std::vector<float> delay_line_;
    int delay_index_;
    int cutoff_ratio_;
    int taps_;

public:
    AntiAliasingFilter(int cutoff_ratio = 95, int taps = 101);

    /**
     * Apply low-pass filter to input samples
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
 */
class CubicSampleRateConverter : public ISampleRateConverter {
private:
    double ratio_;
    double position_;
    int channels_;
    int input_rate_;
    int output_rate_;
    std::vector<float> history_buffer_;  // 4 frames of history per channel

public:
    CubicSampleRateConverter();

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override { return 2; }  // 2 sample delay
    void reset() override;
    const char* get_name() const override { return "Cubic"; }
    const char* get_description() const override {
        return "Cubic interpolation resampler with improved quality";
    }

private:
    float cubic_interpolate(float y0, float y1, float y2, float y3, float x);
};

/**
 * @brief Sinc interpolation sample rate converter
 */
class SincSampleRateConverter : public ISampleRateConverter {
private:
    struct SincTable {
        std::vector<float> coefficients;
        int taps;
        double cutoff;
        bool windowed;
    };

    std::vector<SincTable> sinc_tables_;
    double ratio_;
    double position_;
    int channels_;
    int input_rate_;
    int output_rate_;
    int quality_taps_;
    std::vector<float> delay_buffer_;

    /**
     * Generate sinc windowed coefficients
     */
    void generate_sinc_table(SincTable& table, int taps, double cutoff);

    /**
     * Apply sinc interpolation
     */
    float sinc_interpolate(const float* input, double position, int taps);

public:
    SincSampleRateConverter(int taps = 8);

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override { return quality_taps_ / 2; }
    void reset() override;
    const char* get_name() const override {
        return "Sinc";
    }
    const char* get_description() const override {
        return "Sinc interpolation resampler with anti-aliasing";
    }
};

/**
 * @brief Improved sample rate converter with quality selection
 */
class ImprovedSampleRateConverter : public ISampleRateConverter {
private:
    std::unique_ptr<ISampleRateConverter> converter_;
    std::unique_ptr<AntiAliasingFilter> anti_aliasing_filter_;
    ResamplerQuality quality_;

    int input_rate_;
    int output_rate_;
    int channels_;
    bool enable_filtering_;

public:
    /**
     * Constructor
     * @param quality Quality level of resampling
     * @param enable_filtering Enable anti-aliasing filtering
     */
    ImprovedSampleRateConverter(ResamplerQuality quality = ResamplerQuality::Good,
                               bool enable_filtering = true);

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override;
    void reset() override;
    const char* get_name() const override;
    const char* get_description() const override;

    /**
     * Set quality level
     */
    void set_quality(ResamplerQuality quality);

    /**
     * Get current quality level
     */
    ResamplerQuality get_quality() const { return quality_; }

    /**
     * Enable/disable anti-aliasing filter
     */
    void enable_filtering(bool enable) { enable_filtering_ = enable; }

    /**
     * Get estimated CPU usage for current quality
     */
    double get_estimated_cpu_usage() const;

    /**
     * Get quality description
     */
    static const char* get_quality_description(ResamplerQuality quality);
};

/**
 * @brief Factory for improved sample rate converters
 */
class ImprovedSampleRateConverterFactory {
public:
    /**
     * Create improved converter with specified quality
     * @param quality Quality level
     * @param enable_filtering Enable anti-aliasing
     * @return Unique pointer to converter
     */
    static std::unique_ptr<ImprovedSampleRateConverter> create(
        ResamplerQuality quality = ResamplerQuality::Good,
        bool enable_filtering = true);

    /**
     * Get list of available quality levels
     */
    static std::vector<ResamplerQuality> get_available_qualities();

    /**
     * Get quality string representation
     */
    static std::string quality_to_string(ResamplerQuality quality);
};

} // namespace audio