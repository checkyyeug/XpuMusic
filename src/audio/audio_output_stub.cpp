/**
 * @file audio_output_stub.cpp
 * @brief Stub audio output implementation (always available)
 * @date 2025-12-10
 */

#include "audio_output.h"
#include <cstring>

namespace audio {

/**
 * @brief Stub audio output implementation
 *
 * This implementation does nothing but provides a valid audio output
 * that will always compile and run without any dependencies.
 */
class AudioOutputStub final : public IAudioOutput {
private:
    AudioFormat format_;
    AudioConfig config_;
    bool is_open_;
    bool is_started_;
    int latency_;
    int buffer_size_;
    double volume_;
    bool is_muted_;

public:
    AudioOutputStub()
        : is_open_(false)
        , is_started_(false)
        , latency_(100)
        , buffer_size_(1024)
        , volume_(1.0)
        , is_muted_(false) {
    }

    ~AudioOutputStub() override {
        cleanup();
    }

    bool initialize(const AudioFormat& format, const AudioConfig& config) override {
        format_ = format;
        config_ = config;
        volume_ = config.volume;
        is_muted_ = config.mute;
        buffer_size_ = config.buffer_size;

        // Calculate reasonable defaults
        latency_ = 100; // 100ms latency

        return true;
    }

    bool open(const AudioFormat& format) override {
        format_ = format;
        is_open_ = true;

        // Calculate some reasonable defaults
        latency_ = 100; // 100ms latency
        buffer_size_ = format.sample_rate / 10; // 100ms buffer

        return true;
    }

    void close() override {
        is_open_ = false;
        is_started_ = false;
    }

    void start() override {
        if (is_open_ && !is_started_) {
            is_started_ = true;
        }
    }

    void stop() override {
        is_started_ = false;
    }

    int write(const void* buffer, int frames) override {
        if (!is_open_ || !is_started_) return 0;

        // Silently discard the audio data
        // In real implementation, this would send to audio device

        // Return the number of frames "processed"
        return frames;
    }

    void set_volume(double volume) override {
        volume_ = std::max(0.0, std::min(1.0, volume));
    }

    double get_volume() const override {
        return volume_;
    }

    void set_mute(bool mute) override {
        is_muted_ = mute;
    }

    bool is_muted() const override {
        return is_muted_;
    }

    int get_latency() const override {
        return latency_;
    }

    int get_buffer_size() const override {
        return buffer_size_;
    }

    bool is_ready() const override {
        return is_open_;
    }

    void cleanup() override {
        close();
    }
};

// Factory function for stub backend (only used when no other backend is available)
std::unique_ptr<IAudioOutput> create_stub_audio_output() {
    return std::make_unique<AudioOutputStub>();
}

} // namespace audio