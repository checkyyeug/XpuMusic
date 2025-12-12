/**
 * @file enhanced_sample_rate_converter.h
 * @brief Enhanced sample rate converter with quality levels
 * @date 2025-12-10
 */

#pragma once

#include "sample_rate_converter.h"
#include "cubic_resampler.h"
#include "universal_sample_rate_converter.h"
#include <memory>
#include <string>

namespace audio {

/**
 * @brief Quality levels for sample rate conversion
 */
enum class ResampleQuality {
    Fast,       // Linear interpolation (current)
    Good,       // Cubic interpolation
    High,       // High quality (placeholder for future)
    Best        // Best quality (placeholder for future)
};

/**
 * @brief Enhanced sample rate converter with quality selection
 *
 * This class provides a unified interface for different resampling
 * algorithms, allowing users to choose between speed and quality.
 */
class EnhancedSampleRateConverter : public ISampleRateConverter {
private:
    std::unique_ptr<ISampleRateConverter> converter_;
    ResampleQuality quality_;
    int input_rate_;
    int output_rate_;
    int channels_;

    /**
     * Create converter based on quality level
     */
    std::unique_ptr<ISampleRateConverter> create_converter(ResampleQuality quality);

public:
    /**
     * Constructor
     * @param quality Desired quality level
     */
    explicit EnhancedSampleRateConverter(ResampleQuality quality = ResampleQuality::Good);

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override;
    void reset() override;
    const char* get_name() const override;
    const char* get_description() const override;

    /**
     * Set quality level (re-initializes converter)
     */
    void set_quality(ResampleQuality quality);

    /**
     * Get current quality level
     */
    ResampleQuality get_quality() const { return quality_; }

    /**
     * Get quality level name
     */
    static const char* get_quality_name(ResampleQuality quality);

    /**
     * Get CPU usage estimate for quality level
     */
    static double get_cpu_usage_estimate(ResampleQuality quality);

    /**
     * Get quality description
     */
    static const char* get_quality_description(ResampleQuality quality);
};

/**
 * @brief Factory for enhanced sample rate converters
 */
class EnhancedSampleRateConverterFactory {
public:
    /**
     * Create enhanced converter with specified quality
     * @param quality Quality level
     * @return Unique pointer to converter
     */
    static std::unique_ptr<EnhancedSampleRateConverter> create(
        ResampleQuality quality = ResampleQuality::Good);

    /**
     * Create converter by quality name
     * @param quality_name String quality name
     * @return Unique pointer to converter
     */
    static std::unique_ptr<EnhancedSampleRateConverter> create_by_name(
        const std::string& quality_name);

    /**
     * Get list of available quality levels
     */
    static std::vector<std::string> get_available_qualities();

    /**
     * Parse quality string to enum
     */
    static ResampleQuality parse_quality(const std::string& quality_name);
};

} // namespace audio