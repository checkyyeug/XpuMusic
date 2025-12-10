#pragma once

#include "mp_types.h"

namespace mp {

// Audio device information
struct AudioDeviceInfo {
    const char* id;             // Device identifier
    const char* name;           // Human-readable device name
    uint32_t max_channels;      // Maximum supported channels
    uint32_t default_sample_rate; // Default sample rate
    bool is_default;            // Is system default device
};

// Audio output callback
// Called by audio backend when it needs more audio data
// buffer: Output buffer to fill
// frames: Number of frames (samples per channel) to write
// user_data: User-provided data
using AudioCallback = void (*)(void* buffer, size_t frames, void* user_data);

// Audio output configuration
struct AudioOutputConfig {
    const char* device_id;      // Device to use (nullptr for default)
    uint32_t sample_rate;       // Desired sample rate
    uint32_t channels;          // Number of channels
    SampleFormat format;        // Sample format
    uint32_t buffer_frames;     // Buffer size in frames
    AudioCallback callback;     // Audio callback function
    void* user_data;            // User data for callback
};

// Audio output interface
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;
    
    // Enumerate available audio devices
    virtual Result enumerate_devices(const AudioDeviceInfo** devices, size_t* count) = 0;
    
    // Open audio output
    virtual Result open(const AudioOutputConfig& config) = 0;
    
    // Start audio playback
    virtual Result start() = 0;
    
    // Stop audio playback
    virtual Result stop() = 0;
    
    // Close audio output
    virtual void close() = 0;
    
    // Get current latency in milliseconds
    virtual uint32_t get_latency() const = 0;
    
    // Set volume (0.0 to 1.0)
    virtual Result set_volume(float volume) = 0;
    
    // Get volume (0.0 to 1.0)
    virtual float get_volume() const = 0;
};

} // namespace mp
