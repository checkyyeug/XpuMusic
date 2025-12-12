#pragma once

#include "../sdk/headers/mp_audio_output.h"
#include <string>
#include <vector>

namespace mp {
namespace platform {

// Factory function to create platform-specific audio output
// Returns the appropriate audio output implementation for the current platform:
// - Windows: WASAPI
// - macOS: CoreAudio
// - Linux: ALSA
// Fallback: Stub implementation for unsupported platforms
IAudioOutput* create_platform_audio_output();

// Create audio output with specific backend (for testing/fallback)
IAudioOutput* create_audio_output(const std::string& backend);

// Get available audio backends
std::vector<std::string> get_available_audio_backends();

}} // namespace mp::platform
