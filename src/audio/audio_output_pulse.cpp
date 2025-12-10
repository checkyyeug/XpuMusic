/**
 * @file audio_output_pulse.cpp
 * @brief PulseAudio audio output implementation for Linux
 * @date 2025-12-10
 */

#ifdef AUDIO_BACKEND_PULSE

#include "audio_output.h"
#include <pulse/simple.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace audio {

/**
 * @brief PulseAudio audio output implementation
 */
class AudioOutputPulseAudio final : public IAudioOutput {
private:
    pa_simple* pa_;
    AudioFormat format_;
    bool is_open_;
    int buffer_size_;
    int latency_;

public:
    AudioOutputPulseAudio()
        : pa_(nullptr)
        , is_open_(false)
        , buffer_size_(0)
        , latency_(0) {
    }

    ~AudioOutputPulseAudio() override {
        close();
    }

    bool open(const AudioFormat& format) override {
        format_ = format;

        // Sample specification
        pa_sample_spec ss;
        pa_sample_spec_init(&ss);

        // Set format - try float first
        ss.format = PA_SAMPLE_FLOAT32LE;
        ss.channels = format_.channels;
        ss.rate = format_.sample_rate;

        // Create PulseAudio connection
        int error;
        pa_ = pa_simple_new(
            nullptr,                    // Server
            "Music Player",             // Application name
            "Audio Playback",           // Stream description
            &ss,                         // Sample specification
            PA_STREAM_PLAYBACK,        // Direction
            nullptr,                    // Buffer attributes
            &error                        // Error code
        );

        if (!pa_) {
            // Try with S16NE format
            ss.format = PA_SAMPLE_S16NE;
            pa_ = pa_simple_new(
                nullptr,
                "Music Player",
                "Audio Playback",
                &ss,
                PA_STREAM_PLAYBACK,
                nullptr,
                &error
            );

            if (!pa_) {
                std::cerr << "PulseAudio error: " << pa_strerror(error) << std::endl;
                return false;
            }
        }

        is_open_ = true;
        buffer_size_ = 1024; // Default buffer size
        latency_ = 100; // Default latency in ms

        // Get actual latency
        const pa_buffer_attr* attr = pa_simple_get_buffer_attr(pa_);
        if (attr) {
            latency_ = attr->maxlength / sizeof(float) * 1000 / format_.sample_rate;
        }

        return true;
    }

    void close() override {
        if (pa_) {
            pa_simple_free(pa_);
            pa_ = nullptr;
        }
        is_open_ = false;
    }

    int write(const float* buffer, int frames) override {
        if (!is_open_) return 0;

        // Calculate total bytes to write
        int bytes_to_write = frames * format_.channels * sizeof(float);

        // Write audio data
        int error;
        if (pa_simple_write(pa_, buffer, bytes_to_write, &error) < 0) {
            std::cerr << "PulseAudio write error: " << pa_strerror(error) << std::endl;
            return 0;
        }

        return frames;
    }

    int get_latency() const override {
        return latency_;
    }

    int get_buffer_size() const override {
        return buffer_size_;
    }

    bool is_ready() const override {
        return is_open_ && pa_;
    }
};

// Factory function for PulseAudio backend
std::unique_ptr<IAudioOutput> create_pulse_audio_output() {
    return std::make_unique<AudioOutputPulseAudio>();
}

} // namespace audio

#else // AUDIO_BACKEND_PULSE not defined

#include "audio_output.h"
#include <iostream>

namespace audio {

// Fallback to stub if PulseAudio not available
std::unique_ptr<IAudioOutput> create_pulse_audio_output() {
    std::cout << "PulseAudio backend not compiled, falling back to stub" << std::endl;
    return create_audio_output(); // This will create stub
}

#endif // AUDIO_BACKEND_PULSE