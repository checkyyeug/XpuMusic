/**
 * @file universal_sample_rate_converter.h
 * @brief Universal sample rate converter supporting all common audio rates
 * @date 2025-12-10
 */

#pragma once

#include "sample_rate_converter.h"
#include <vector>
#include <unordered_set>
#include <memory>
#include <string>
#include <unordered_map>

namespace audio {

/**
 * @brief Common audio sample rates supported
 */
struct AudioSampleRate {
    // Standard rates
    static constexpr int RATE_8000 = 8000;
    static constexpr int RATE_11025 = 11025;
    static constexpr int RATE_16000 = 16000;
    static constexpr int RATE_22050 = 22050;
    static constexpr int RATE_44100 = 44100;
    static constexpr int RATE_48000 = 48000;
    static constexpr int RATE_88200 = 88200;
    static constexpr int RATE_96000 = 96000;
    static constexpr int RATE_176400 = 176400;
    static constexpr int RATE_192000 = 192000;
    static constexpr int RATE_352800 = 352800;
    static constexpr int RATE_384000 = 384000;
    static constexpr int RATE_705600 = 705600;
    static constexpr int RATE_768000 = 768000;

    // Standard CD quality rates
    static constexpr int CD_RATE = RATE_44100;

    // DVD quality rates
    static constexpr int DVD_RATE = RATE_48000;

    // Professional studio rates
    static constexpr int STUDIO_RATE = RATE_96000;

    // High-definition rates
    static constexpr int HD_RATE = RATE_192000;
    static constexpr int UHD_RATE = RATE_384000;

    /**
     * Get all supported rates
     */
    static std::vector<int> get_all_rates();

    /**
     * Get rates by category
     */
    static std::vector<int> get_telephony_rates();
    static std::vector<int> get_cd_quality_rates();
    static std::vector<int> get_dvd_quality_rates();
    static std::vector<int> get_studio_rates();
    static std::vector<int> get_hd_rates();
    static std::vector<int> get_uhd_rates();

    /**
     * Check if rate is standard
     */
    static bool is_standard_rate(int rate);

    /**
     * Get rate category name
     */
    static std::string get_rate_category(int rate);

    /**
     * Get description for rate
     */
    static std::string get_rate_description(int rate);
};

/**
 * @brief Universal sample rate converter with caching
 */
class UniversalSampleRateConverter {
private:
    struct ConversionCacheKey {
        int input_rate;
        int output_rate;
        int channels;

        bool operator==(const ConversionCacheKey& other) const {
            return input_rate == other.input_rate &&
                   output_rate == other.output_rate &&
                   channels == other.channels;
        }
    };

    struct ConversionCacheHash {
        size_t operator()(const ConversionCacheKey& key) const {
            return std::hash<int>()(key.input_rate) ^
                   std::hash<int>()(key.output_rate * 17) ^
                   std::hash<int>()(key.channels * 31);
        }
    };

    std::unordered_map<ConversionCacheKey, std::unique_ptr<ISampleRateConverter>,
                       ConversionCacheHash> converter_cache_;

    int default_output_rate_;
    bool auto_optimize_;

    /**
     * Get or create converter for specific conversion
     */
    ISampleRateConverter* get_converter(int input_rate, int output_rate, int channels);

    /**
     * Find nearest standard sample rate
     */
    int find_nearest_standard_rate(int rate);

public:
    /**
     * Constructor
     * @param default_output_rate Default output rate when no specific rate is requested
     */
    UniversalSampleRateConverter(int default_output_rate = AudioSampleRate::RATE_48000);

    /**
     * Convert audio from input to output rate
     * @param input Input buffer
     * @param input_frames Number of input frames
     * @param input_rate Input sample rate
     * @param output Output buffer
     * @param max_output_frames Maximum output frames
     * @param channels Number of audio channels
     * @param output_rate Target output rate
     * @return Number of output frames generated
     */
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames,
               int input_rate, int output_rate, int channels);

    /**
     * Convert with automatic rate selection
     */
    int convert_auto(const float* input, int input_frames,
                     float* output, int max_output_frames,
                     int input_rate, int channels);

    /**
     * Select optimal output rate for input rate
     */
    int select_optimal_output_rate(int input_rate);

    /**
     * Convert between any two standard rates
     */
    int convert_standard(const float* input, int input_frames,
                       float* output, int max_output_frames,
                       int input_rate, int output_rate, int channels);

    /**
     * Set default output rate
     */
    void set_default_output_rate(int rate);

    /**
     * Enable/disable auto-optimization
     */
    void set_auto_optimize(bool enable);

    /**
     * Clear converter cache
     */
    void clear_cache();

    /**
     * Get cache size
     */
    size_t get_cache_size() const;

    /**
     * Get converter info for all cached conversions
     */
    std::vector<std::string> get_cached_conversions();
};

} // namespace audio