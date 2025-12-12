/**
 * @file sample_rate_converter.h
 * @brief Sample rate conversion interface and implementations
 * @date 2025-12-10
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace audio {

/**
 * @brief Sample rate converter interface
 */
class ISampleRateConverter {
public:
    virtual ~ISampleRateConverter() = default;

    /**
     * Initialize the converter
     * @param input_rate Input sample rate
     * @param output_rate Output sample rate
     * @param channels Number of audio channels
     * @return True if initialization successful
     */
    virtual bool initialize(int input_rate, int output_rate, int channels) = 0;

    /**
     * Convert audio samples
     * @param input Input audio buffer
     * @param input_frames Number of input frames
     * @param output Output audio buffer
     * @param max_output_frames Maximum output frames
     * @return Number of output frames generated
     */
    virtual int convert(const float* input, int input_frames,
                       float* output, int max_output_frames) = 0;

    /**
     * Get conversion latency in frames
     */
    virtual int get_latency() const = 0;

    /**
     * Reset converter state
     */
    virtual void reset() = 0;

    /**
     * Get converter name
     */
    virtual const char* get_name() const = 0;

    /**
     * Get converter description
     */
    virtual const char* get_description() const = 0;
};

/**
 * @brief Linear interpolation sample rate converter
 */
class LinearSampleRateConverter : public ISampleRateConverter {
private:
    double ratio_;              // Position increment
    double position_;           // Current position in input
    int channels_;              // Number of channels
    int input_rate_;            // Input sample rate
    int output_rate_;           // Output sample rate
    std::vector<float> last_frame_;  // Last frame for interpolation

public:
    LinearSampleRateConverter();

    bool initialize(int input_rate, int output_rate, int channels) override;
    int convert(const float* input, int input_frames,
               float* output, int max_output_frames) override;
    int get_latency() const override;
    void reset() override;
    const char* get_name() const override { return "Linear"; }
    const char* get_description() const override { return "Linear interpolation resampler"; }
};

/**
 * @brief Sample rate converter factory
 */
class SampleRateConverterFactory {
public:
    SampleRateConverterFactory();

    /**
     * Create a sample rate converter
     * @param type Type of converter ("linear", "cubic", "sinc")
     * @return Unique pointer to converter
     */
    static std::unique_ptr<ISampleRateConverter> create(const std::string& type = "linear");

    /**
     * List available converter types
     */
    static std::vector<std::string> list_available();

    /**
     * Check if converter type is available
     */
    static bool is_available(const std::string& type);
};

} // namespace audio