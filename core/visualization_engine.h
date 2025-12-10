#ifndef VISUALIZATION_ENGINE_H
#define VISUALIZATION_ENGINE_H

#include "mp_types.h"
#include <vector>
#include <complex>
#include <mutex>
#include <cstdint>

namespace mp {

// Visualization data structures

struct WaveformData {
    std::vector<float> min_values;  // Minimum amplitude per pixel
    std::vector<float> max_values;  // Maximum amplitude per pixel
    uint32_t sample_rate;
    uint16_t channels;
    float time_span_seconds;        // How many seconds of audio
};

struct SpectrumData {
    std::vector<float> magnitudes;  // Frequency bin magnitudes (dB)
    std::vector<float> frequencies; // Center frequency per bin
    uint32_t fft_size;
    uint32_t sample_rate;
    float min_frequency;
    float max_frequency;
};

struct VUMeterData {
    float peak_left;                // Peak level (0.0 - 1.0)
    float peak_right;
    float rms_left;                 // RMS level (0.0 - 1.0)
    float rms_right;
    float peak_db_left;             // Peak in dB (-inf to 0)
    float peak_db_right;
    float rms_db_left;              // RMS in dB (-inf to 0)
    float rms_db_right;
};

// Visualization engine configuration
struct VisualizationConfig {
    // Waveform settings
    uint32_t waveform_width;        // Width in pixels
    float waveform_time_span;       // Time span in seconds
    
    // Spectrum analyzer settings
    uint32_t fft_size;              // FFT size (power of 2)
    uint32_t spectrum_bars;         // Number of frequency bars
    float spectrum_min_freq;        // Minimum frequency (Hz)
    float spectrum_max_freq;        // Maximum frequency (Hz)
    float spectrum_smoothing;       // Smoothing factor (0.0 - 1.0)
    
    // VU meter settings
    float vu_peak_decay_rate;       // Peak decay in dB/second
    float vu_rms_window_ms;         // RMS averaging window in ms
    
    // General settings
    uint32_t update_rate_hz;        // Update rate (Hz)
};

class VisualizationEngine {
public:
    VisualizationEngine();
    ~VisualizationEngine();
    
    // Configuration
    Result initialize(const VisualizationConfig& config);
    void shutdown();
    
    // Audio data input (called from audio thread)
    void process_audio(const float* samples, size_t frame_count, 
                      uint16_t channels, uint32_t sample_rate);
    
    // Data retrieval (called from UI thread)
    WaveformData get_waveform_data();
    SpectrumData get_spectrum_data();
    VUMeterData get_vu_meter_data();
    
    // Configuration updates
    void set_waveform_width(uint32_t width);
    void set_fft_size(uint32_t size);
    void set_spectrum_bars(uint32_t bars);
    void set_spectrum_smoothing(float smoothing);
    
private:
    // FFT implementation (Cooley-Tukey algorithm)
    void compute_fft(const std::vector<float>& input, 
                     std::vector<std::complex<float>>& output);
    
    // Window functions
    void apply_hann_window(std::vector<float>& samples);
    
    // Frequency mapping (linear to logarithmic)
    void map_fft_to_bars(const std::vector<std::complex<float>>& fft_output,
                         std::vector<float>& bar_magnitudes);
    
    // Helper functions
    float linear_to_db(float linear);
    float db_to_linear(float db);
    uint32_t next_power_of_two(uint32_t n);
    
    // Configuration
    VisualizationConfig config_;
    bool initialized_;
    
    // Waveform data
    std::vector<float> waveform_buffer_;  // Ring buffer for waveform
    size_t waveform_write_pos_;
    std::mutex waveform_mutex_;
    
    // Spectrum data
    std::vector<float> spectrum_input_buffer_;
    std::vector<std::complex<float>> spectrum_fft_output_;
    std::vector<float> spectrum_bar_values_;
    std::vector<float> spectrum_smoothed_bars_;
    std::mutex spectrum_mutex_;
    
    // VU meter data
    VUMeterData vu_data_;
    std::vector<float> rms_buffer_left_;
    std::vector<float> rms_buffer_right_;
    size_t rms_buffer_pos_;
    float peak_hold_time_left_;
    float peak_hold_time_right_;
    std::mutex vu_mutex_;
    
    // Sample rate tracking
    uint32_t current_sample_rate_;
    uint16_t current_channels_;
};

} // namespace mp

#endif // VISUALIZATION_ENGINE_H
