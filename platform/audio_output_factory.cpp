#include "audio_output_factory.h"
#include "../core/platform_utils.h"

namespace mp {
namespace platform {

// Forward declarations of platform-specific factory functions
#ifdef MP_PLATFORM_WINDOWS
extern "C" IAudioOutput* create_wasapi_output();
#elif defined(MP_PLATFORM_MACOS)
extern "C" IAudioOutput* create_coreaudio_output();
#elif defined(MP_PLATFORM_LINUX)
extern "C" IAudioOutput* create_alsa_output();
#endif

// Fallback stub implementation for unsupported platforms
class StubAudioOutput : public IAudioOutput {
public:
    Result enumerate_devices(const AudioDeviceInfo** devices, size_t* count) override {
        static AudioDeviceInfo dummy_device = {
            "stub",
            "Stub Audio Device",
            2,
            44100,
            true
        };
        *devices = &dummy_device;
        *count = 1;
        return Result::Success;
    }

    Result open(const AudioOutputConfig& config) override {
        std::cerr << "Warning: Audio output stub - no playback available" << std::endl;
        (void)config;
        return Result::NotImplemented;
    }

    Result start() override {
        return Result::NotImplemented;
    }

    Result stop() override {
        return Result::Success;
    }

    void close() override {}

    uint32_t get_latency() const override {
        return 0;
    }

    Result set_volume(float volume) override {
        (void)volume;
        return Result::Success;
    }

    float get_volume() const override {
        return 1.0f;
    }
};

IAudioOutput* create_platform_audio_output() {
    // Try platform-specific implementation first
#ifdef MP_PLATFORM_WINDOWS
    auto* output = create_wasapi_output();
    if (output) return output;
#elif defined(MP_PLATFORM_MACOS)
    auto* output = create_coreaudio_output();
    if (output) return output;
#elif defined(MP_PLATFORM_LINUX)
    auto* output = create_alsa_output();
    if (output) return output;
#endif

    // Fallback to stub if platform implementation failed
    std::cerr << "Warning: Using stub audio output on " << MP_PLATFORM_NAME << std::endl;
    return new StubAudioOutput();
}

// Create audio output with specific backend (for testing/fallback)
IAudioOutput* create_audio_output(const std::string& backend) {
    if (backend == "auto" || backend.empty()) {
        return create_platform_audio_output();
    }

#ifdef MP_PLATFORM_WINDOWS
    if (backend == "wasapi") {
        return create_wasapi_output();
    }
#elif defined(MP_PLATFORM_MACOS)
    if (backend == "coreaudio") {
        return create_coreaudio_output();
    }
#elif defined(MP_PLATFORM_LINUX)
    if (backend == "alsa") {
        return create_alsa_output();
    }
#endif

    if (backend == "stub") {
        return new StubAudioOutput();
    }

    std::cerr << "Error: Unknown audio backend '" << backend << "'" << std::endl;
    return create_platform_audio_output();
}

// Get available audio backends
std::vector<std::string> get_available_audio_backends() {
    std::vector<std::string> backends;

    backends.push_back("auto");
    backends.push_back("stub");

#ifdef MP_PLATFORM_WINDOWS
    backends.push_back("wasapi");
#elif defined(MP_PLATFORM_MACOS)
    backends.push_back("coreaudio");
#elif defined(MP_PLATFORM_LINUX)
    backends.push_back("alsa");
#endif

    return backends;
}

}} // namespace mp::platform
