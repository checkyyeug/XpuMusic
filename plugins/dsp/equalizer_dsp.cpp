#include "mp_dsp.h"
#include <cmath>
#include <cstring>
#include <vector>
#include <cstdio>

namespace mp {
namespace dsp {

// Biquad filter implementation for peaking EQ
struct BiquadFilter {
    // Filter coefficients
    float b0, b1, b2;  // Numerator coefficients
    float a1, a2;      // Denominator coefficients (a0 is normalized to 1)
    
    // State variables (for stereo)
    float x1[2], x2[2];  // Input history
    float y1[2], y2[2];  // Output history
    
    BiquadFilter() {
        reset();
    }
    
    void reset() {
        b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
        a1 = 0.0f; a2 = 0.0f;
        std::memset(x1, 0, sizeof(x1));
        std::memset(x2, 0, sizeof(x2));
        std::memset(y1, 0, sizeof(y1));
        std::memset(y2, 0, sizeof(y2));
    }
    
    // Design peaking EQ filter
    void design_peaking(float sample_rate, float freq, float gain_db, float q) {
        const float pi = 3.14159265358979323846f;
        float A = std::pow(10.0f, gain_db / 40.0f);
        float omega = 2.0f * pi * freq / sample_rate;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float alpha = sin_omega / (2.0f * q);
        
        // Peaking EQ filter coefficients
        b0 = 1.0f + alpha * A;
        b1 = -2.0f * cos_omega;
        b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        a1 = -2.0f * cos_omega;
        a2 = 1.0f - alpha / A;
        
        // Normalize by a0
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }
    
    // Process a single sample for one channel
    inline float process(float input, int channel) {
        float output = b0 * input + b1 * x1[channel] + b2 * x2[channel]
                                   - a1 * y1[channel] - a2 * y2[channel];
        
        // Update state
        x2[channel] = x1[channel];
        x1[channel] = input;
        y2[channel] = y1[channel];
        y1[channel] = output;
        
        return output;
    }
};

// 10-band graphic equalizer DSP plugin
class EqualizerDSP : public IDSPProcessor, public IPlugin {
public:
    static constexpr int NUM_BANDS = 10;
    static constexpr float BAND_FREQUENCIES[NUM_BANDS] = {
        31.25f, 62.5f, 125.0f, 250.0f, 500.0f,
        1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
    };
    static constexpr float Q_FACTOR = 1.0f;  // Q factor for peaking filters
    
    EqualizerDSP()
        : sample_rate_(0)
        , channels_(0)
        , bypassed_(false) {
        
        // Initialize all band gains to 0 dB
        for (int i = 0; i < NUM_BANDS; ++i) {
            band_gains_db_[i] = 0.0f;
        }
    }
    
    ~EqualizerDSP() override {
        shutdown();
    }
    
    // IDSPProcessor implementation
    Result initialize(const DSPConfig* config) override {
        if (!config) {
            return Result::InvalidParameter;
        }
        
        sample_rate_ = config->sample_rate;
        channels_ = config->channels;
        
        if (channels_ > 2) {
            // Currently only support mono and stereo
            return Result::NotSupported;
        }
        
        // Design all filters
        update_filters();
        
        return Result::Success;
    }
    
    Result process(AudioBuffer* input, AudioBuffer* output) override {
        if (!input || !input->data) {
            return Result::InvalidParameter;
        }
        
        // Bypass mode - no processing
        if (bypassed_) {
            if (output && output != input) {
                std::memcpy(output->data, input->data, 
                           input->frames * channels_ * sizeof(float));
                output->frames = input->frames;
            }
            return Result::Success;
        }
        
        // Process audio
        float* buffer = static_cast<float*>(input->data);
        
        // Process each frame
        for (size_t frame = 0; frame < input->frames; ++frame) {
            for (int ch = 0; ch < channels_; ++ch) {
                size_t sample_idx = frame * channels_ + ch;
                float sample = buffer[sample_idx];
                
                // Apply all band filters in series
                for (int band = 0; band < NUM_BANDS; ++band) {
                    sample = filters_[band].process(sample, ch);
                }
                
                buffer[sample_idx] = sample;
            }
        }
        
        // If output buffer provided, copy result
        if (output && output != input) {
            std::memcpy(output->data, input->data, 
                       input->frames * channels_ * sizeof(float));
            output->frames = input->frames;
        }
        
        return Result::Success;
    }
    
    uint32_t get_latency_samples() const override {
        return 0; // Biquad filters have negligible latency
    }
    
    void reset() override {
        // Reset all filter states
        for (int i = 0; i < NUM_BANDS; ++i) {
            filters_[i].reset();
        }
    }
    
    void set_bypass(bool bypass) override {
        bypassed_ = bypass;
    }
    
    bool is_bypassed() const override {
        return bypassed_;
    }
    
    uint32_t get_dsp_capabilities() const {
        return static_cast<uint32_t>(DSPCapability::InPlace) |
               static_cast<uint32_t>(DSPCapability::Bypass) |
               static_cast<uint32_t>(DSPCapability::Stereo);
    }
    
    PluginCapability get_capabilities() const override {
        return PluginCapability::None;
    }
    
    uint32_t get_parameter_count() const override {
        return NUM_BANDS; // One parameter per band
    }
    
    Result get_parameter_info(uint32_t index, DSPParameter* param) const override {
        if (!param || index >= NUM_BANDS) {
            return Result::InvalidParameter;
        }
        
        char name_buf[32];
        char label_buf[64];
        
        // Format parameter name based on frequency
        if (BAND_FREQUENCIES[index] < 1000.0f) {
            snprintf(name_buf, sizeof(name_buf), "band_%dHz", 
                    static_cast<int>(BAND_FREQUENCIES[index]));
            snprintf(label_buf, sizeof(label_buf), "%d Hz", 
                    static_cast<int>(BAND_FREQUENCIES[index]));
        } else {
            snprintf(name_buf, sizeof(name_buf), "band_%dkHz", 
                    static_cast<int>(BAND_FREQUENCIES[index] / 1000.0f));
            snprintf(label_buf, sizeof(label_buf), "%.1f kHz", 
                    BAND_FREQUENCIES[index] / 1000.0f);
        }
        
        static thread_local char name_storage[32];
        static thread_local char label_storage[64];
        std::strncpy(name_storage, name_buf, sizeof(name_storage) - 1);
        std::strncpy(label_storage, label_buf, sizeof(label_storage) - 1);
        name_storage[31] = '\0';
        label_storage[63] = '\0';
        
        param->name = name_storage;
        param->label = label_storage;
        param->min_value = -12.0f;   // -12 dB
        param->max_value = 12.0f;    // +12 dB
        param->default_value = 0.0f;  // 0 dB (flat)
        param->current_value = band_gains_db_[index];
        param->unit = "dB";
        
        return Result::Success;
    }
    
    Result set_parameter(uint32_t index, float value) override {
        if (index >= NUM_BANDS) {
            return Result::InvalidParameter;
        }
        
        // Clamp value to valid range
        if (value < -12.0f) value = -12.0f;
        if (value > 12.0f) value = 12.0f;
        
        band_gains_db_[index] = value;
        
        // Update the filter for this band
        update_filter(index);
        
        return Result::Success;
    }
    
    float get_parameter(uint32_t index) const override {
        if (index >= NUM_BANDS) {
            return 0.0f;
        }
        return band_gains_db_[index];
    }
    
    void shutdown() override {
        // Nothing to cleanup
    }
    
    // IPlugin implementation
    const PluginInfo& get_plugin_info() const override {
        static PluginInfo info = {
            "10-Band Equalizer",
            "Music Player",
            "Graphic equalizer with 10 frequency bands",
            {1, 0, 0},
            API_VERSION,
            "mp.dsp.equalizer"
        };
        return info;
    }
    
    Result initialize(IServiceRegistry* services) override {
        (void)services;
        return Result::Success;
    }
    
    // Plugin type info methods (for macro)
    const char* get_uuid() const;
    const char* get_name() const;
    const char* get_author() const;
    const char* get_description() const;
    Version get_version() const;
    uint32_t get_type() const;
    
private:
    void update_filters() {
        for (int i = 0; i < NUM_BANDS; ++i) {
            update_filter(i);
        }
    }
    
    void update_filter(int band) {
        if (band < 0 || band >= NUM_BANDS || sample_rate_ == 0) {
            return;
        }
        
        filters_[band].design_peaking(
            static_cast<float>(sample_rate_),
            BAND_FREQUENCIES[band],
            band_gains_db_[band],
            Q_FACTOR
        );
    }
    
    uint32_t sample_rate_;
    uint16_t channels_;
    bool bypassed_;
    float band_gains_db_[NUM_BANDS];
    BiquadFilter filters_[NUM_BANDS];
};

}} // namespace mp::dsp

// Register plugin
MP_DEFINE_DSP_PLUGIN(
    mp::dsp::EqualizerDSP,
    "mp.dsp.equalizer",
    "10-Band Equalizer",
    "Music Player",
    "Graphic equalizer with 10 frequency bands (31Hz-16kHz)",
    1, 0, 0
)
