/**
 * @file sample_rate_converter_64.h
 * @brief 64-bit floating point sample rate converters
 * @date 2025-12-10
 */

#pragma once

#include <memory>
#include <vector>
#include <cmath>

namespace audio {

/**
 * @brief Interface for 64-bit sample rate converters
 */
class ISampleRateConverter64 {
public:
    virtual ~ISampleRateConverter64() = default;

    /**
     * @brief Configure the converter
     * @param input_rate Input sample rate
     * @param output_rate Output sample rate
     * @param channels Number of channels
     * @return true on success
     */
    virtual bool configure(int input_rate, int output_rate, int channels) = 0;

    /**
     * @brief Process audio samples
     * @param input Input buffer (64-bit float)
     * @param output Output buffer (64-bit float)
     * @param input_frames Number of input frames
     * @return Number of output frames
     */
    virtual int process(const double* input, double* output, int input_frames) = 0;

    /**
     * @brief Get the number of output frames for given input
     * @param input_frames Number of input frames
     * @return Expected number of output frames
     */
    virtual int get_output_latency(int input_frames) const = 0;

    /**
     * @brief Reset converter state
     */
    virtual void reset() = 0;

    /**
     * @brief Get latency in samples
     * @return Latency in samples
     */
    virtual int get_latency() const = 0;
};

/**
 * @brief 64-bit linear interpolation resampler
 * Fast but low quality
 */
class LinearSampleRateConverter64 : public ISampleRateConverter64 {
private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double phase_;
    std::vector<double> last_sample_;

public:
    LinearSampleRateConverter64();
    ~LinearSampleRateConverter64() override = default;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const double* input, double* output, int input_frames) override;
    int get_output_latency(int input_frames) const override;
    void reset() override;
    int get_latency() const override { return 1; }
};

/**
 * @brief 64-bit cubic interpolation resampler
 * Good quality with reasonable performance
 */
class CubicSampleRateConverter64 : public ISampleRateConverter64 {
private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    double phase_;
    std::vector<double> history_;  // 4 samples history per channel

    // Cubic interpolation coefficients
    double cubic_interp(double y0, double y1, double y2, double y3, double mu) const;

public:
    CubicSampleRateConverter64();
    ~CubicSampleRateConverter64() override = default;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const double* input, double* output, int input_frames) override;
    int get_output_latency(int input_frames) const override;
    void reset() override;
    int get_latency() const override { return 3; }
};

/**
 * @brief 64-bit Sinc interpolation resampler with windowing
 * High quality resampler using windowed sinc function
 */
class SincSampleRateConverter64 : public ISampleRateConverter64 {
private:
    int input_rate_;
    int output_rate_;
    int channels_;
    double ratio_;
    int taps_;
    double cutoff_;
    std::vector<double> buffer_;
    int buffer_pos_;
    double phase_;

    // Pre-computed sinc window
    std::vector<double> sinc_window_;

    // Kaiser window parameters
    double kaiser_beta_;
    double kaiser_bessel_i0(double x) const;
    void generate_sinc_table();

    // Interpolation function
    double interpolate(const double* buffer, int pos, int channels, int ch, double phase) const;

public:
    explicit SincSampleRateConverter64(int taps = 16);
    ~SincSampleRateConverter64() override = default;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const double* input, double* output, int input_frames) override;
    int get_output_latency(int input_frames) const override;
    void reset() override;
    int get_latency() const override { return taps_ / 2; }
};

/**
 * @brief 64-bit adaptive sample rate converter
 * Automatically selects quality based on CPU load and ratio
 */
class AdaptiveSampleRateConverter64 : public ISampleRateConverter64 {
private:
    std::unique_ptr<ISampleRateConverter64> converter_;
    std::string current_quality_;
    int input_rate_;
    int output_rate_;
    int channels_;
    double cpu_threshold_;
    bool enable_adaptive_;

    // Performance monitoring
    int frame_count_;
    double total_time_;

    // Select converter based on configuration
    void select_converter(const std::string& quality);

public:
    AdaptiveSampleRateConverter64();
    ~AdaptiveSampleRateConverter64() override = default;

    bool configure(int input_rate, int output_rate, int channels) override;
    int process(const double* input, double* output, int input_frames) override;
    int get_output_latency(int input_frames) const override;
    void reset() override;
    int get_latency() const override;

    // Configuration
    void set_cpu_threshold(double threshold) { cpu_threshold_ = threshold; }
    void set_adaptive_mode(bool enable) { enable_adaptive_ = enable; }
    double get_cpu_threshold() const { return cpu_threshold_; }
    bool is_adaptive_enabled() const { return enable_adaptive_; }
    const std::string& get_current_quality() const { return current_quality_; }
};

} // namespace audio