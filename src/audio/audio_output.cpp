/**
 * @file audio_output.cpp
 * @brief Factory implementation for audio backends
 * @date 2025-12-10
 */

#include "audio_output.h"
#include <iostream>

namespace audio {

// Platform-specific factory function declarations
#ifdef AUDIO_BACKEND_WASAPI
std::unique_ptr<IAudioOutput> create_wasapi_audio_output();
#elif defined(AUDIO_BACKEND_COREAUDIO)
std::unique_ptr<IAudioOutput> create_coreaudio_audio_output();
#elif defined(AUDIO_BACKEND_ALSA)
std::unique_ptr<IAudioOutput> create_alsa_audio_output();
#elif defined(AUDIO_BACKEND_PULSE)
std::unique_ptr<IAudioOutput> create_pulse_audio_output();
#else
// Default to stub if no backend selected
std::unique_ptr<IAudioOutput> create_stub_audio_output();
#endif

// Factory function
std::unique_ptr<IAudioOutput> create_audio_output() {
    // Create the appropriate backend based on compile-time selection
#ifdef AUDIO_BACKEND_WASAPI
    return create_wasapi_audio_output();
#elif defined(AUDIO_BACKEND_COREAUDIO)
    return create_coreaudio_audio_output();
#elif defined(AUDIO_BACKEND_ALSA)
    return create_alsa_audio_output();
#elif defined(AUDIO_BACKEND_PULSE)
    return create_pulse_audio_output();
#else
    // Always fall back to stub
    return std::make_unique<AudioOutputStub>();
#endif
}

// Factory function with configuration
std::unique_ptr<IAudioOutput> create_audio_output(const AudioConfig& config) {
    // Create the appropriate backend based on compile-time selection
    // Note: The specific backend should honor the config in its initialize method
    std::unique_ptr<IAudioOutput> output = create_audio_output();

    // We can't call initialize here without format info
    // The caller should call initialize with both format and config

    return output;
}

// Backend name function
const char* get_audio_backend_name() {
    // Return the name of the selected backend
#ifdef AUDIO_BACKEND_WASAPI
    return "WASAPI";
#elif defined(AUDIO_BACKEND_COREAUDIO)
    return "CoreAudio";
#elif defined(AUDIO_BACKEND_ALSA)
    return "ALSA";
#elif defined(AUDIO_BACKEND_PULSE)
    return "PulseAudio";
#else
    return "stub";
#endif
}

} // namespace audio