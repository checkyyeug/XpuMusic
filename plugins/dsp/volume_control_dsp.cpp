#include "mp_dsp.h"
#include <cmath>
#include <cstring>

namespace mp {
namespace dsp {

// Simple volume control DSP plugin
class VolumeControlDSP : public IDSPProcessor, public IPlugin {
public:
    VolumeControlDSP()
        : sample_rate_(0)
        , channels_(0)
        , bypassed_(false)
        , volume_db_(0.0f)
        , volume_linear_(1.0f) {
    }
    
    ~VolumeControlDSP() override {
        shutdown();
    }
    
    // IDSPProcessor implementation
    Result initialize(const DSPConfig* config) override {
        if (!config) {
            return Result::InvalidParameter;
        }
        
        sample_rate_ = config->sample_rate;
        channels_ = config->channels;
        
        // Initialize with 0 dB (unity gain)
        set_parameter(0, 0.0f);
        
        return Result::Success;
    }
    
    Result process(AudioBuffer* input, AudioBuffer* output) override {
        if (!input || !input->data) {
            return Result::InvalidParameter;
        }
        
        // Bypass mode - no processing
        if (bypassed_) {
            if (output && output != input) {
                // Copy input to output if separate buffers
                std::memcpy(output->data, input->data, 
                           input->frames * channels_ * sizeof(float));
                output->frames = input->frames;
            }
            return Result::Success;
        }
        
        // In-place processing
        float* buffer = static_cast<float*>(input->data);
        size_t sample_count = input->frames * channels_;
        
        // Apply volume multiplication
        for (size_t i = 0; i < sample_count; ++i) {
            buffer[i] *= volume_linear_;
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
        return 0; // No latency for volume control
    }
    
    void reset() override {
        // No internal state to reset
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
               static_cast<uint32_t>(DSPCapability::Stereo) |
               static_cast<uint32_t>(DSPCapability::Multichannel);
    }
    
    PluginCapability get_capabilities() const override {
        return PluginCapability::None; // IPlugin capabilities
    }
    
    uint32_t get_parameter_count() const override {
        return 1; // Only volume parameter
    }
    
    Result get_parameter_info(uint32_t index, DSPParameter* param) const override {
        if (!param || index >= get_parameter_count()) {
            return Result::InvalidParameter;
        }
        
        param->name = "volume";
        param->label = "Volume";
        param->min_value = -60.0f;   // -60 dB minimum
        param->max_value = 12.0f;    // +12 dB maximum
        param->default_value = 0.0f;  // 0 dB (unity gain)
        param->current_value = volume_db_;
        param->unit = "dB";
        
        return Result::Success;
    }
    
    Result set_parameter(uint32_t index, float value) override {
        if (index >= get_parameter_count()) {
            return Result::InvalidParameter;
        }
        
        // Clamp value to valid range
        if (value < -60.0f) value = -60.0f;
        if (value > 12.0f) value = 12.0f;
        
        volume_db_ = value;
        
        // Convert dB to linear gain: linear = 10^(dB/20)
        volume_linear_ = std::pow(10.0f, volume_db_ / 20.0f);
        
        return Result::Success;
    }
    
    float get_parameter(uint32_t index) const override {
        if (index >= get_parameter_count()) {
            return 0.0f;
        }
        return volume_db_;
    }
    
    void shutdown() override {
        // Nothing to cleanup
    }
    
    // IPlugin implementation
    const PluginInfo& get_plugin_info() const override {
        static PluginInfo info = {
            "Volume Control",
            "Music Player",
            "Simple volume control DSP",
            {1, 0, 0},
            API_VERSION,
            "mp.dsp.volume_control"
        };
        return info;
    }
    
    Result initialize(IServiceRegistry* services) override {
        (void)services;
        return Result::Success;
    }
    
    void shutdown_plugin() {
        shutdown();
    }
    
    // Plugin type info methods (for macro)
    const char* get_uuid() const;
    const char* get_name() const;
    const char* get_author() const;
    const char* get_description() const;
    Version get_version() const;
    uint32_t get_type() const;
    
private:
    uint32_t sample_rate_;
    uint16_t channels_;
    bool bypassed_;
    float volume_db_;      // Volume in dB
    float volume_linear_;  // Volume as linear multiplier
};

}} // namespace mp::dsp

// Register plugin
MP_DEFINE_DSP_PLUGIN(
    mp::dsp::VolumeControlDSP,
    "mp.dsp.volume_control",
    "Volume Control",
    "Music Player",
    "Simple volume control DSP",
    1, 0, 0
)
