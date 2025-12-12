/**
 * @file adaptive_resampler.h
 * @brief Adaptive sample rate converter with automatic quality selection
 * @date 2025-12-10
 */

#pragma once

#include "enhanced_sample_rate_converter.h"
#include <memory>
#include <chrono>

namespace audio {

/**
 * @brief Performance monitoring for resampler
 */
class ResamplerPerformanceMonitor {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    double total_conversion_time_;
    int total_frames_;
    int update_interval_;
    double last_cpu_estimate_;

public:
    ResamplerPerformanceMonitor()
        : total_conversion_time_(0.0)
        , total_frames_(0)
        , update_interval_(1000)  // Update every 1000 frames
        , last_cpu_estimate_(0.0) {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * Start timing conversion
     */
    void start_timing() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * End timing conversion and update metrics
     */
    void end_timing(int frames) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start_time_);
        total_conversion_time_ += duration.count() / 1000.0;  // Convert to ms
        total_frames_ += frames;

        // Update CPU estimate every interval frames
        if (total_frames_ >= update_interval_) {
            update_cpu_estimate();
            total_frames_ = 0;
            total_conversion_time_ = 0.0;
        }
    }

    /**
     * Get current CPU usage estimate (percentage)
     */
    double get_cpu_estimate() const {
        return last_cpu_estimate_;
    }

private:
    void update_cpu_estimate() {
        if (total_conversion_time_ > 0) {
            // Estimate based on conversion time vs real time
            last_cpu_estimate_ = std::min(100.0, total_conversion_time_ / 10.0);
        }
    }
};

/**
 * @brief Adaptive sample rate converter that adjusts quality based on performance
 *
 * This class automatically selects the best quality level based on:
 * - System performance (CPU usage)
 * - Audio parameters (sample rates, channels)
 * - User preferences
 */
class AdaptiveSampleRateConverter : public ISampleRateConverter {
private:
    std::unique_ptr<EnhancedSampleRateConverter> converter_;
    ResamplerPerformanceMonitor performance_monitor_;
    ResampleQuality current_quality_;
    ResampleQuality max_quality_;
    ResampleQuality min_quality_;
    bool auto_adjust_;
    double cpu_threshold_;
    int input_rate_;
    int output_rate_;
    int channels_;

    /**
     * Select quality based on current conditions
     */
    ResampleQuality select_quality();

    /**
     * Check if quality adjustment is needed
     */
    bool should_adjust_quality();

    /**
     * Create converter with specified quality
     */
    void create_converter(ResampleQuality quality);

public:
    /**
     * Constructor
     * @param min_quality Minimum quality to use
     * @param max_quality Maximum quality to use
     * @param auto_adjust Enable automatic quality adjustment
     * @param cpu_threshold CPU usage threshold for quality adjustment (%)
     */
    AdaptiveSampleRateConverter(
        ResampleQuality min_quality = ResampleQuality::Fast,
        ResampleQuality max_quality = ResampleQuality::Best,
        bool auto_adjust = true,
        double cpu_threshold = 80.0);

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override;
    void reset() override;
    const char* get_name() const override { return "Adaptive"; }
    const char* get_description() const override {
        return "Adaptive resampler with automatic quality selection";
    }

    /**
     * Set quality range
     */
    void set_quality_range(ResampleQuality min_quality, ResampleQuality max_quality);

    /**
     * Enable/disable automatic quality adjustment
     */
    void set_auto_adjust(bool enable) { auto_adjust_ = enable; }

    /**
     * Set CPU usage threshold for quality adjustment
     */
    void set_cpu_threshold(double threshold) { cpu_threshold_ = threshold; }

    /**
     * Get current quality level
     */
    ResampleQuality get_current_quality() const { return current_quality_; }

    /**
     * Get performance statistics
     */
    struct PerformanceStats {
        double current_cpu_usage;
        ResampleQuality current_quality;
        int total_conversions;
        double average_realtime_factor;
    };

    PerformanceStats get_performance_stats() const;
};

/**
 * @brief Factory for adaptive sample rate converters
 */
class AdaptiveSampleRateConverterFactory {
public:
    /**
     * Create adaptive converter with default settings
     */
    static std::unique_ptr<AdaptiveSampleRateConverter> create();

    /**
     * Create adaptive converter for specific use case
     * @param use_case Application type (realtime, music, professional)
     */
    static std::unique_ptr<AdaptiveSampleRateConverter> create_for_use_case(
        const std::string& use_case);

    /**
     * Create adaptive converter with custom settings
     */
    static std::unique_ptr<AdaptiveSampleRateConverter> create_with_settings(
        ResampleQuality min_quality,
        ResampleQuality max_quality,
        bool auto_adjust = true,
        double cpu_threshold = 80.0);
};

} // namespace audio