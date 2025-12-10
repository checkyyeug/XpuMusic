/**
 * @file adaptive_resampler.cpp
 * @brief Adaptive sample rate converter implementation
 * @date 2025-12-10
 */

#include "adaptive_resampler.h"
#include <algorithm>
#include <iostream>

namespace audio {

// AdaptiveSampleRateConverter Implementation
AdaptiveSampleRateConverter::AdaptiveSampleRateConverter(
    ResampleQuality min_quality,
    ResampleQuality max_quality,
    bool auto_adjust,
    double cpu_threshold)
    : current_quality_(ResampleQuality::Good)
    , min_quality_(min_quality)
    , max_quality_(max_quality)
    , auto_adjust_(auto_adjust)
    , cpu_threshold_(cpu_threshold)
    , input_rate_(0)
    , output_rate_(0)
    , channels_(0) {
}

bool AdaptiveSampleRateConverter::initialize(int input_rate, int output_rate, int channels) {
    if (input_rate <= 0 || output_rate <= 0 || channels <= 0) {
        return false;
    }

    input_rate_ = input_rate;
    output_rate_ = output_rate;
    channels_ = channels;

    // Select initial quality
    current_quality_ = select_quality();
    create_converter(current_quality_);

    return converter_ && converter_->initialize(input_rate, output_rate, channels);
}

ResampleQuality AdaptiveSampleRateConverter::select_quality() {
    // For real-time applications, prefer speed
    if (!auto_adjust_) {
        return current_quality_;
    }

    // Check current system load
    double cpu_usage = performance_monitor_.get_cpu_estimate();

    // Select quality based on CPU usage
    if (cpu_usage > cpu_threshold_) {
        // High CPU usage - use lower quality
        if (current_quality_ > min_quality_) {
            return static_cast<ResampleQuality>(static_cast<int>(current_quality_) - 1);
        }
    } else if (cpu_usage < cpu_threshold_ / 2) {
        // Low CPU usage - can use higher quality
        if (current_quality_ < max_quality_) {
            return static_cast<ResampleQuality>(static_cast<int>(current_quality_) + 1);
        }
    }

    return current_quality_;
}

bool AdaptiveSampleRateConverter::should_adjust_quality() {
    if (!auto_adjust_) {
        return false;
    }

    ResampleQuality suggested_quality = select_quality();
    return suggested_quality != current_quality_;
}

void AdaptiveSampleRateConverter::create_converter(ResampleQuality quality) {
    converter_ = EnhancedSampleRateConverterFactory::create(quality);
}

int AdaptiveSampleRateConverter::convert(const float* input, int input_frames,
                                          float* output, int max_output_frames) {
    if (!input || !output || input_frames <= 0 || max_output_frames <= 0) {
        return 0;
    }

    // Check if we need to adjust quality
    if (should_adjust_quality()) {
        ResampleQuality new_quality = select_quality();
        create_converter(new_quality);
        if (converter_) {
            converter_->initialize(input_rate_, output_rate_, channels_);
            current_quality_ = new_quality;
        }
    }

    // Start performance monitoring
    performance_monitor_.start_timing();

    // Perform conversion
    int result = converter_->convert(input, input_frames, output, max_output_frames);

    // End performance monitoring
    performance_monitor_.end_timing(result);

    return result;
}

int AdaptiveSampleRateConverter::get_latency() const {
    if (!converter_) {
        return 0;
    }
    return converter_->get_latency();
}

void AdaptiveSampleRateConverter::reset() {
    if (converter_) {
        converter_->reset();
    }
    performance_monitor_ = ResamplerPerformanceMonitor();
}

void AdaptiveSampleRateConverter::set_quality_range(
    ResampleQuality min_quality, ResampleQuality max_quality) {
    min_quality_ = min_quality;
    max_quality_ = max_quality;

    // Ensure current quality is within range
    if (current_quality_ < min_quality_) {
        current_quality_ = min_quality;
    } else if (current_quality_ > max_quality_) {
        current_quality_ = max_quality;
    }
}

AdaptiveSampleRateConverter::PerformanceStats
AdaptiveSampleRateConverter::get_performance_stats() const {
    PerformanceStats stats;
    stats.current_cpu_usage = performance_monitor_.get_cpu_estimate();
    stats.current_quality = current_quality_;
    stats.total_conversions = 0;  // Could add counter if needed
    stats.average_realtime_factor = 0.0;  // Could calculate if needed

    return stats;
}

// AdaptiveSampleRateConverterFactory Implementation
std::unique_ptr<AdaptiveSampleRateConverter>
AdaptiveSampleRateConverterFactory::create() {
    return std::make_unique<AdaptiveSampleRateConverter>();
}

std::unique_ptr<AdaptiveSampleRateConverter>
AdaptiveSampleRateConverterFactory::create_for_use_case(const std::string& use_case) {
    std::string lower_use_case = use_case;
    std::transform(lower_use_case.begin(), lower_use_case.end(),
                  lower_use_case.begin(), ::tolower);

    if (lower_use_case == "realtime" || lower_use_case == "game") {
        // For real-time applications, prioritize speed
        return create_with_settings(
            ResampleQuality::Fast,
            ResampleQuality::Good,
            true,  // Auto adjust
            70.0   // Lower CPU threshold
        );
    } else if (lower_use_case == "music" || lower_use_case == "audiophile") {
        // For music playback, balance quality and performance
        return create_with_settings(
            ResampleQuality::Good,
            ResampleQuality::High,
            true,  // Auto adjust
            85.0   // Moderate CPU threshold
        );
    } else if (lower_use_case == "professional" || lower_use_case == "studio") {
        // For professional use, prioritize quality
        return create_with_settings(
            ResampleQuality::High,
            ResampleQuality::Best,
            true,  // Auto adjust
            95.0   // High CPU threshold
        );
    } else {
        // Default case
        return create();
    }
}

std::unique_ptr<AdaptiveSampleRateConverter>
AdaptiveSampleRateConverterFactory::create_with_settings(
    ResampleQuality min_quality,
    ResampleQuality max_quality,
    bool auto_adjust,
    double cpu_threshold) {
    return std::make_unique<AdaptiveSampleRateConverter>(
        min_quality, max_quality, auto_adjust, cpu_threshold);
}

} // namespace audio