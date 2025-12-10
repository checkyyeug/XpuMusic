/**
 * @file universal_sample_rate_converter.cpp
 * @brief Universal sample rate converter implementation
 * @date 2025-12-10
 */

#include "universal_sample_rate_converter.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstring>

namespace audio {

// AudioSampleRate implementations
std::vector<int> AudioSampleRate::get_all_rates() {
    return {
        RATE_8000,
        RATE_11025,
        RATE_16000,
        RATE_22050,
        RATE_44100,
        RATE_48000,
        RATE_88200,
        RATE_96000,
        RATE_176400,
        RATE_192000,
        RATE_352800,
        RATE_384000,
        RATE_705600,
        RATE_768000
    };
}

std::vector<int> AudioSampleRate::get_telephony_rates() {
    return {RATE_8000, RATE_11025, RATE_16000};
}

std::vector<int> AudioSampleRate::get_cd_quality_rates() {
    return {RATE_22050, RATE_44100, RATE_48000};
}

std::vector<int> AudioSampleRate::get_dvd_quality_rates() {
    return {RATE_44100, RATE_48000, RATE_88200, RATE_96000};
}

std::vector<int> AudioSampleRate::get_studio_rates() {
    return {RATE_44100, RATE_48000, RATE_88200, RATE_96000, RATE_176400, RATE_192000};
}

std::vector<int> AudioSampleRate::get_hd_rates() {
    return {RATE_88200, RATE_96000, RATE_176400, RATE_192000, RATE_352800, RATE_384000};
}

std::vector<int> AudioSampleRate::get_uhd_rates() {
    return {RATE_176400, RATE_192000, RATE_352800, RATE_384000, RATE_705600, RATE_768000};
}

bool AudioSampleRate::is_standard_rate(int rate) {
    const std::vector<int>& all_rates = get_all_rates();
    return std::find(all_rates.begin(), all_rates.end(), rate) != all_rates.end();
}

std::string AudioSampleRate::get_rate_category(int rate) {
    if (rate <= RATE_16000) return "Telephony";
    if (rate <= RATE_22050) return "Consumer";
    if (rate <= RATE_48000) return "CD";
    if (rate <= RATE_96000) return "DVD";
    if (rate <= RATE_192000) return "Studio";
    if (rate <= RATE_384000) return "HD";
    return "UHD";
}

std::string AudioSampleRate::get_rate_description(int rate) {
    switch (rate) {
        case RATE_8000: return "8 kHz (Telephony)";
        case RATE_11025: return "11.025 kHz (CD-XA)";
        case RATE_16000: return "16 kHz (Telephony)";
        case RATE_22050: return "22.05 kHz (CD-XA)";
        case RATE_44100: return "44.1 kHz (CD)";
        case RATE_48000: return "48 kHz (DVD)";
        case RATE_88200: return "88.2 kHz (DVD)";
        case RATE_96000: return "96 kHz (Professional)";
        case RATE_176400: return "176.4 kHz (Professional)";
        case RATE_192000: return "192 kHz (HD)";
        case RATE_352800: return "352.8 kHz (HD)";
        case RATE_384000: return "384 kHz (HD)";
        case RATE_705600: return "705.6 kHz (UHD)";
        case RATE_768000: return "768 kHz (UHD)";
        default: return std::to_string(rate) + " Hz";
    }
}

// UniversalSampleRateConverter implementations
UniversalSampleRateConverter::UniversalSampleRateConverter(int default_output_rate)
    : default_output_rate_(default_output_rate)
    , auto_optimize_(true) {
}

ISampleRateConverter* UniversalSampleRateConverter::get_converter(int input_rate, int output_rate, int channels) {
    ConversionCacheKey key{input_rate, output_rate, channels};

    auto it = converter_cache_.find(key);
    if (it != converter_cache_.end()) {
        return it->second.get();
    }

    // Create new converter
    auto converter = audio::SampleRateConverterFactory::create("linear");

    if (converter && converter->initialize(input_rate, output_rate, channels)) {
        it = converter_cache_.emplace(key, std::move(converter)).first;
        return it->second.get();
    }

    return nullptr;
}

int UniversalSampleRateConverter::convert(const float* input, int input_frames,
                                        float* output, int max_output_frames,
                                        int input_rate, int output_rate, int channels) {
    // Fast path: same rate
    if (input_rate == output_rate) {
        int frames_to_copy = std::min(input_frames, max_output_frames);
        ::memcpy(output, input, frames_to_copy * channels * sizeof(float));
        return frames_to_copy;
    }

    // Get or create converter
    ISampleRateConverter* converter = get_converter(input_rate, output_rate, channels);
    if (!converter) {
        return 0;
    }

    // Perform conversion
    return converter->convert(input, input_frames, output, max_output_frames);
}

int UniversalSampleRateConverter::convert_auto(const float* input, int input_frames,
                                             float* output, int max_output_frames,
                                             int input_rate, int channels) {
    // Auto-select best output rate
    int output_rate = select_optimal_output_rate(input_rate);

    return convert(input, input_frames, output, max_output_frames,
                   input_rate, output_rate, channels);
}

int UniversalSampleRateConverter::select_optimal_output_rate(int input_rate) {
    if (!auto_optimize_) {
        return default_output_rate_;
    }

    // If input rate is already standard and high quality, keep it
    if (input_rate >= AudioSampleRate::RATE_48000) {
        return input_rate;
    }

    // Otherwise, map to nearest standard rate
    const std::vector<int> priority_rates = {
        AudioSampleRate::RATE_48000,  // DVD quality (most common)
        AudioSampleRate::RATE_44100,  // CD quality
        AudioSampleRate::RATE_96000,  // Professional
        AudioSampleRate::RATE_88200,  // DVD
        AudioSampleRate::RATE_192000, // HD
        AudioSampleRate::RATE_384000, // HD
    };

    int best_rate = priority_rates[0];
    int min_diff = std::abs(input_rate - best_rate);

    for (int rate : priority_rates) {
        int diff = std::abs(input_rate - rate);
        if (diff < min_diff) {
            min_diff = diff;
            best_rate = rate;
        }
    }

    return best_rate;
}

int UniversalSampleRateConverter::convert_standard(const float* input, int input_frames,
                                                    float* output, int max_output_frames,
                                                    int input_rate, int output_rate, int channels) {
    // Validate input rate
    if (!AudioSampleRate::is_standard_rate(input_rate)) {
        // Find nearest standard rate
        int nearest = find_nearest_standard_rate(input_rate);
        if (nearest != input_rate) {
            // First convert to nearest standard, then to target
            std::vector<float> temp_buffer;
            int temp_frames = static_cast<int>(
                input_frames * static_cast<double>(nearest) / input_rate);
            temp_buffer.resize(temp_frames * channels);

            // Convert to nearest standard
            int converted = convert_standard(input, input_frames,
                                            temp_buffer.data(), temp_frames,
                                            input_rate, nearest, channels);

            if (converted > 0) {
                // Convert from nearest to target
                int final_frames = convert_standard(temp_buffer.data(), converted,
                                                        output, max_output_frames,
                                                        nearest, output_rate, channels);
                return final_frames;
            }
        }

        return 0;
    }

    // Validate output rate
    if (!AudioSampleRate::is_standard_rate(output_rate)) {
        int nearest = find_nearest_standard_rate(output_rate);
        return convert_standard(input, input_frames, output, max_output_frames,
                               input_rate, nearest, channels);
    }

    return convert(input, input_frames, output, max_output_frames,
                   input_rate, output_rate, channels);
}

void UniversalSampleRateConverter::set_default_output_rate(int rate) {
    if (AudioSampleRate::is_standard_rate(rate)) {
        default_output_rate_ = rate;
    }
}

void UniversalSampleRateConverter::set_auto_optimize(bool enable) {
    auto_optimize_ = enable;
}

void UniversalSampleRateConverter::clear_cache() {
    converter_cache_.clear();
}

size_t UniversalSampleRateConverter::get_cache_size() const {
    return converter_cache_.size();
}

std::vector<std::string> UniversalSampleRateConverter::get_cached_conversions() {
    std::vector<std::string> conversions;
    for (const auto& [key, _] : converter_cache_) {
        conversions.push_back(
            std::to_string(key.input_rate) + "Hz → " +
            std::to_string(key.output_rate) + "Hz (" +
            std::to_string(key.channels) + " channels)"
        );
    }
    return conversions;
}

int UniversalSampleRateConverter::find_nearest_standard_rate(int rate) {
    const std::vector<int>& rates = AudioSampleRate::get_all_rates();
    int nearest = rates[0];
    int min_diff = std::abs(rate - nearest);

    for (int r : rates) {
        int diff = std::abs(rate - r);
        if (diff < min_diff) {
            min_diff = diff;
            nearest = r;
        }
    }

    return nearest;
}

} // namespace audio