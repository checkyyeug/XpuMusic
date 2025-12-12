#include "visualization_engine.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace mp {

namespace {
    const float PI = 3.14159265358979323846f;
    const float EPSILON = 1e-10f;
    const float MIN_DB = -80.0f;
}

VisualizationEngine::VisualizationEngine()
    : initialized_(false)
    , waveform_write_pos_(0)
    , rms_buffer_pos_(0)
    , peak_hold_time_left_(0.0f)
    , peak_hold_time_right_(0.0f)
    , current_sample_rate_(0)
    , current_channels_(0) {
    
    // Initialize VU meter data
    vu_data_.peak_left = 0.0f;
    vu_data_.peak_right = 0.0f;
    vu_data_.rms_left = 0.0f;
    vu_data_.rms_right = 0.0f;
    vu_data_.peak_db_left = MIN_DB;
    vu_data_.peak_db_right = MIN_DB;
    vu_data_.rms_db_left = MIN_DB;
    vu_data_.rms_db_right = MIN_DB;
}

VisualizationEngine::~VisualizationEngine() {
    shutdown();
}

Result VisualizationEngine::initialize(const VisualizationConfig& config) {
    if (initialized_) {
        return Result::Error;
    }
    
    config_ = config;
    
    // Ensure FFT size is power of 2
    config_.fft_size = next_power_of_two(config_.fft_size);
    
    // Initialize waveform buffer (ring buffer)
    size_t waveform_samples = static_cast<size_t>(
        config_.waveform_time_span * 48000.0f * 2); // Assume max 48kHz stereo
    waveform_buffer_.resize(waveform_samples, 0.0f);
    waveform_write_pos_ = 0;
    
    // Initialize spectrum buffers
    spectrum_input_buffer_.resize(config_.fft_size, 0.0f);
    spectrum_fft_output_.resize(config_.fft_size);
    spectrum_bar_values_.resize(config_.spectrum_bars, MIN_DB);
    spectrum_smoothed_bars_.resize(config_.spectrum_bars, MIN_DB);
    
    // Initialize VU meter buffers
    size_t rms_samples = static_cast<size_t>(
        (config_.vu_rms_window_ms / 1000.0f) * 48000.0f);
    rms_buffer_left_.resize(rms_samples, 0.0f);
    rms_buffer_right_.resize(rms_samples, 0.0f);
    rms_buffer_pos_ = 0;
    
    initialized_ = true;
    return Result::Success;
}

void VisualizationEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    waveform_buffer_.clear();
    spectrum_input_buffer_.clear();
    spectrum_fft_output_.clear();
    spectrum_bar_values_.clear();
    spectrum_smoothed_bars_.clear();
    rms_buffer_left_.clear();
    rms_buffer_right_.clear();
    
    initialized_ = false;
}

void VisualizationEngine::process_audio(const float* samples, size_t frame_count,
                                       uint16_t channels, uint32_t sample_rate) {
    if (!initialized_ || !samples || frame_count == 0) {
        return;
    }
    
    current_sample_rate_ = sample_rate;
    current_channels_ = channels;
    
    // Process waveform data
    {
        std::lock_guard<std::mutex> lock(waveform_mutex_);
        for (size_t i = 0; i < frame_count; ++i) {
            // Mix to mono for waveform
            float mono_sample = 0.0f;
            for (uint16_t ch = 0; ch < channels; ++ch) {
                mono_sample += samples[i * channels + ch];
            }
            mono_sample /= static_cast<float>(channels);
            
            waveform_buffer_[waveform_write_pos_] = mono_sample;
            waveform_write_pos_ = (waveform_write_pos_ + 1) % waveform_buffer_.size();
        }
    }
    
    // Process spectrum data (use first block of audio)
    {
        std::lock_guard<std::mutex> lock(spectrum_mutex_);
        size_t samples_to_copy = std::min(frame_count, spectrum_input_buffer_.size());
        
        for (size_t i = 0; i < samples_to_copy; ++i) {
            // Mix to mono for spectrum
            float mono_sample = 0.0f;
            for (uint16_t ch = 0; ch < channels; ++ch) {
                mono_sample += samples[i * channels + ch];
            }
            mono_sample /= static_cast<float>(channels);
            spectrum_input_buffer_[i] = mono_sample;
        }
        
        // Apply Hann window
        apply_hann_window(spectrum_input_buffer_);
        
        // Compute FFT
        compute_fft(spectrum_input_buffer_, spectrum_fft_output_);
        
        // Map FFT to frequency bars
        map_fft_to_bars(spectrum_fft_output_, spectrum_bar_values_);
        
        // Apply smoothing
        for (size_t i = 0; i < spectrum_bar_values_.size(); ++i) {
            spectrum_smoothed_bars_[i] = 
                config_.spectrum_smoothing * spectrum_smoothed_bars_[i] +
                (1.0f - config_.spectrum_smoothing) * spectrum_bar_values_[i];
        }
    }
    
    // Process VU meter data
    {
        std::lock_guard<std::mutex> lock(vu_mutex_);
        
        float peak_left = 0.0f;
        float peak_right = 0.0f;
        float sum_sq_left = 0.0f;
        float sum_sq_right = 0.0f;
        
        for (size_t i = 0; i < frame_count; ++i) {
            float left = samples[i * channels];
            float right = (channels > 1) ? samples[i * channels + 1] : left;
            
            // Peak detection
            peak_left = std::max(peak_left, std::abs(left));
            peak_right = std::max(peak_right, std::abs(right));
            
            // RMS calculation (store in ring buffer)
            rms_buffer_left_[rms_buffer_pos_] = left * left;
            rms_buffer_right_[rms_buffer_pos_] = right * right;
            rms_buffer_pos_ = (rms_buffer_pos_ + 1) % rms_buffer_left_.size();
        }
        
        // Calculate RMS
        for (float val : rms_buffer_left_) sum_sq_left += val;
        for (float val : rms_buffer_right_) sum_sq_right += val;
        
        float rms_left = std::sqrt(sum_sq_left / rms_buffer_left_.size());
        float rms_right = std::sqrt(sum_sq_right / rms_buffer_right_.size());
        
        // Update peak with hold
        vu_data_.peak_left = std::max(vu_data_.peak_left, peak_left);
        vu_data_.peak_right = std::max(vu_data_.peak_right, peak_right);
        
        // Update RMS
        vu_data_.rms_left = rms_left;
        vu_data_.rms_right = rms_right;
        
        // Convert to dB
        vu_data_.peak_db_left = linear_to_db(vu_data_.peak_left);
        vu_data_.peak_db_right = linear_to_db(vu_data_.peak_right);
        vu_data_.rms_db_left = linear_to_db(vu_data_.rms_left);
        vu_data_.rms_db_right = linear_to_db(vu_data_.rms_right);
    }
}

WaveformData VisualizationEngine::get_waveform_data() {
    WaveformData data;
    
    std::lock_guard<std::mutex> lock(waveform_mutex_);
    
    data.sample_rate = current_sample_rate_;
    data.channels = current_channels_;
    data.time_span_seconds = config_.waveform_time_span;
    
    // Downsample waveform to pixel width
    size_t samples_per_pixel = waveform_buffer_.size() / config_.waveform_width;
    if (samples_per_pixel == 0) samples_per_pixel = 1;
    
    data.min_values.resize(config_.waveform_width);
    data.max_values.resize(config_.waveform_width);
    
    for (uint32_t pixel = 0; pixel < config_.waveform_width; ++pixel) {
        float min_val = std::numeric_limits<float>::max();
        float max_val = std::numeric_limits<float>::lowest();
        
        size_t start_idx = pixel * samples_per_pixel;
        size_t end_idx = start_idx + samples_per_pixel;
        
        for (size_t i = start_idx; i < end_idx && i < waveform_buffer_.size(); ++i) {
            size_t actual_idx = (waveform_write_pos_ + i) % waveform_buffer_.size();
            float sample = waveform_buffer_[actual_idx];
            min_val = std::min(min_val, sample);
            max_val = std::max(max_val, sample);
        }
        
        data.min_values[pixel] = min_val;
        data.max_values[pixel] = max_val;
    }
    
    return data;
}

SpectrumData VisualizationEngine::get_spectrum_data() {
    SpectrumData data;
    
    std::lock_guard<std::mutex> lock(spectrum_mutex_);
    
    data.fft_size = config_.fft_size;
    data.sample_rate = current_sample_rate_;
    data.min_frequency = config_.spectrum_min_freq;
    data.max_frequency = config_.spectrum_max_freq;
    
    data.magnitudes = spectrum_smoothed_bars_;
    
    // Calculate center frequencies for each bar (logarithmic spacing)
    data.frequencies.resize(config_.spectrum_bars);
    float log_min = std::log10(config_.spectrum_min_freq);
    float log_max = std::log10(config_.spectrum_max_freq);
    float log_range = log_max - log_min;
    
    for (uint32_t i = 0; i < config_.spectrum_bars; ++i) {
        float t = static_cast<float>(i) / (config_.spectrum_bars - 1);
        float log_freq = log_min + t * log_range;
        data.frequencies[i] = std::pow(10.0f, log_freq);
    }
    
    return data;
}

VUMeterData VisualizationEngine::get_vu_meter_data() {
    std::lock_guard<std::mutex> lock(vu_mutex_);
    return vu_data_;
}

void VisualizationEngine::set_waveform_width(uint32_t width) {
    config_.waveform_width = width;
}

void VisualizationEngine::set_fft_size(uint32_t size) {
    std::lock_guard<std::mutex> lock(spectrum_mutex_);
    config_.fft_size = next_power_of_two(size);
    spectrum_input_buffer_.resize(config_.fft_size, 0.0f);
    spectrum_fft_output_.resize(config_.fft_size);
}

void VisualizationEngine::set_spectrum_bars(uint32_t bars) {
    std::lock_guard<std::mutex> lock(spectrum_mutex_);
    config_.spectrum_bars = bars;
    spectrum_bar_values_.resize(bars, MIN_DB);
    spectrum_smoothed_bars_.resize(bars, MIN_DB);
}

void VisualizationEngine::set_spectrum_smoothing(float smoothing) {
    config_.spectrum_smoothing = std::max(0.0f, std::min(1.0f, smoothing));
}

// FFT implementation (Cooley-Tukey radix-2 DIT)
void VisualizationEngine::compute_fft(const std::vector<float>& input,
                                      std::vector<std::complex<float>>& output) {
    size_t N = input.size();
    output.resize(N);
    
    // Convert input to complex
    for (size_t i = 0; i < N; ++i) {
        output[i] = std::complex<float>(input[i], 0.0f);
    }
    
    // Bit-reversal permutation
    for (size_t i = 0; i < N; ++i) {
        size_t j = 0;
        size_t k = i;
        size_t m = N / 2;
        
        while (m >= 1) {
            j = (j << 1) | (k & 1);
            k >>= 1;
            m >>= 1;
        }
        
        if (j > i) {
            std::swap(output[i], output[j]);
        }
    }
    
    // FFT computation
    for (size_t s = 1; s <= std::log2(N); ++s) {
        size_t m = static_cast<size_t>(1) << s;
        size_t m2 = m >> 1;
        std::complex<float> w(1.0f, 0.0f);
        std::complex<float> wm = std::exp(std::complex<float>(0.0f, -2.0f * PI / m));
        
        for (size_t j = 0; j < m2; ++j) {
            for (size_t k = j; k < N; k += m) {
                std::complex<float> t = w * output[k + m2];
                std::complex<float> u = output[k];
                output[k] = u + t;
                output[k + m2] = u - t;
            }
            w *= wm;
        }
    }
}

void VisualizationEngine::apply_hann_window(std::vector<float>& samples) {
    size_t N = samples.size();
    for (size_t i = 0; i < N; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * PI * i / (N - 1)));
        samples[i] *= window;
    }
}

void VisualizationEngine::map_fft_to_bars(const std::vector<std::complex<float>>& fft_output,
                                          std::vector<float>& bar_magnitudes) {
    if (current_sample_rate_ == 0) {
        return;
    }
    
    size_t fft_bins = fft_output.size() / 2; // Only use positive frequencies
    float bin_frequency = static_cast<float>(current_sample_rate_) / fft_output.size();
    
    // Logarithmic frequency mapping
    float log_min = std::log10(config_.spectrum_min_freq);
    float log_max = std::log10(config_.spectrum_max_freq);
    float log_range = log_max - log_min;
    
    for (size_t bar = 0; bar < bar_magnitudes.size(); ++bar) {
        float t = static_cast<float>(bar) / (bar_magnitudes.size() - 1);
        float log_freq = log_min + t * log_range;
        float center_freq = std::pow(10.0f, log_freq);
        
        // Find FFT bin corresponding to this frequency
        size_t bin = static_cast<size_t>(center_freq / bin_frequency);
        if (bin >= fft_bins) bin = fft_bins - 1;
        
        // Calculate magnitude
        float magnitude = std::abs(fft_output[bin]);
        bar_magnitudes[bar] = linear_to_db(magnitude);
    }
}

float VisualizationEngine::linear_to_db(float linear) {
    if (linear < EPSILON) {
        return MIN_DB;
    }
    return 20.0f * std::log10(linear);
}

float VisualizationEngine::db_to_linear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

uint32_t VisualizationEngine::next_power_of_two(uint32_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

} // namespace mp
