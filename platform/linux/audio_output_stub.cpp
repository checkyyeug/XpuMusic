/**
 * @file audio_output_stub.cpp
 * @brief ALSA audio output stub implementation
 * @date 2025-12-10
 */

#include "../../sdk/headers/mp_audio_output.h"

namespace mp {
namespace platform {

class ALSAAudioOutput : public IAudioOutput {
public:
    ALSAAudioOutput() : is_open_(false), is_playing_(false) {}

    Result enumerate_devices(const AudioDeviceInfo** devices, size_t* count) override {
        static AudioDeviceInfo alsa_device = {
            "alsa_default",
            "Default ALSA Device",
            2,
            44100,
            true
        };
        *devices = &alsa_device;
        *count = 1;
        return Result::Success;
    }

    Result open(const AudioOutputConfig& config) override {
        (void)config;
        is_open_ = true;
        return Result::NotImplemented;
    }

    Result start() override {
        if (!is_open_) return Result::InvalidState;
        is_playing_ = true;
        return Result::NotImplemented;
    }

    Result stop() override {
        is_playing_ = false;
        return Result::Success;
    }

    void close() override {
        is_open_ = false;
        is_playing_ = false;
    }

    uint32_t get_latency() const override {
        return 100; // 100ms dummy latency
    }

    Result set_volume(float volume) override {
        if (volume < 0.0f || volume > 1.0f) return Result::InvalidParameter;
        return Result::Success;
    }

    float get_volume() const override {
        return 1.0f;
    }

private:
    bool is_open_;
    bool is_playing_;
};

extern "C" IAudioOutput* create_alsa_output() {
    return new ALSAAudioOutput();
}

} // namespace platform
} // namespace mp