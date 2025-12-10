#pragma once

#include "mp_types.h"
#include "mp_plugin.h"

namespace mp {

// DSP plugin capabilities
enum class DSPCapability : uint32_t {
    InPlace = 0x01,         // Can process audio in-place (no output buffer needed)
    VariableLatency = 0x02, // Latency depends on parameters
    Bypass = 0x04,          // Supports bypass mode
    Stereo = 0x08,          // Supports stereo processing
    Multichannel = 0x10     // Supports more than 2 channels
};

// DSP configuration
struct DSPConfig {
    uint32_t sample_rate;   // Sample rate in Hz
    uint16_t channels;      // Number of channels
    SampleFormat format;    // Sample format
    uint32_t max_buffer_frames; // Maximum buffer size
};

// DSP parameter value
struct DSPParameter {
    const char* name;       // Parameter name
    const char* label;      // Display label
    float min_value;        // Minimum value
    float max_value;        // Maximum value
    float default_value;    // Default value
    float current_value;    // Current value
    const char* unit;       // Unit (e.g., "dB", "Hz", "%")
};

// DSP processor interface
class IDSPProcessor {
public:
    virtual ~IDSPProcessor() = default;
    
    // Initialize DSP with configuration
    virtual Result initialize(const DSPConfig* config) = 0;
    
    // Process audio buffer
    // For in-place processing: output can be nullptr, input is modified
    // For out-of-place: output must be provided, input is preserved
    virtual Result process(AudioBuffer* input, AudioBuffer* output) = 0;
    
    // Get processing latency in samples
    virtual uint32_t get_latency_samples() const = 0;
    
    // Reset internal state (e.g., on track change)
    virtual void reset() = 0;
    
    // Enable/disable processing (bypass)
    virtual void set_bypass(bool bypass) = 0;
    
    // Check if bypassed
    virtual bool is_bypassed() const = 0;
    
    // Get DSP capabilities (separate from IPlugin::get_capabilities)
    virtual uint32_t get_dsp_capabilities() const = 0;
    
    // Get parameter count
    virtual uint32_t get_parameter_count() const = 0;
    
    // Get parameter info
    virtual Result get_parameter_info(uint32_t index, DSPParameter* param) const = 0;
    
    // Set parameter value
    virtual Result set_parameter(uint32_t index, float value) = 0;
    
    // Get parameter value
    virtual float get_parameter(uint32_t index) const = 0;
    
    // Shutdown
    virtual void shutdown() = 0;
};

// DSP plugin type
constexpr uint32_t PLUGIN_TYPE_DSP = hash_string("mp.plugin.dsp");

// DSP plugin factory
using CreateDSPProcessorFunc = IDSPProcessor* (*)();
using DestroyDSPProcessorFunc = void (*)(IDSPProcessor*);

} // namespace mp

// Macro to define a DSP plugin
#define MP_DEFINE_DSP_PLUGIN(ClassName, uuid_str, name_str, author_str, desc_str, ver_major, ver_minor, ver_patch) \
    extern "C" { \
        mp::IPlugin* create_plugin() { \
            static ClassName instance; \
            return &instance; \
        } \
        void destroy_plugin(mp::IPlugin* plugin) { \
            (void)plugin; \
        } \
        mp::IDSPProcessor* create_dsp_processor() { \
            return new ClassName(); \
        } \
        void destroy_dsp_processor(mp::IDSPProcessor* processor) { \
            delete processor; \
        } \
    } \
    const char* ClassName::get_uuid() const { return uuid_str; } \
    const char* ClassName::get_name() const { return name_str; } \
    const char* ClassName::get_author() const { return author_str; } \
    const char* ClassName::get_description() const { return desc_str; } \
    mp::Version ClassName::get_version() const { return {ver_major, ver_minor, ver_patch}; } \
    uint32_t ClassName::get_type() const { return mp::PLUGIN_TYPE_DSP; }
