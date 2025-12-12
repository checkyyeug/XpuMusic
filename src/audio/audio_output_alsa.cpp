/**
 * @file audio_output_alsa.cpp
 * @brief ALSA audio output implementation for Linux
 * @date 2025-12-10
 */

#ifdef AUDIO_BACKEND_ALSA

#include "audio_output.h"
#include <alsa/asoundlib.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace audio {

/**
 * @brief ALSA audio output implementation
 */
class AudioOutputALSA final : public IAudioOutput {
private:
    snd_pcm_t* pcm_;
    AudioFormat format_;
    snd_pcm_hw_params_t* hw_params_;
    snd_pcm_sw_params_t* sw_params_;
    bool is_open_;
    int buffer_size_;
    int latency_;

public:
    AudioOutputALSA()
        : pcm_(nullptr)
        , hw_params_(nullptr)
        , sw_params_(nullptr)
        , is_open_(false)
        , buffer_size_(0)
        , latency_(0) {
    }

    ~AudioOutputALSA() override {
        close();
    }

    bool open(const AudioFormat& format) override {
        format_ = format;

        // Open PCM device
        int err = snd_pcm_open(&pcm_, "default", SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            std::cerr << "Unable to open PCM device: " << snd_strerror(err) << std::endl;
            return false;
        }

        // Allocate hardware parameter structure
        err = snd_pcm_hw_params_malloc(&hw_params_);
        if (err < 0) {
            std::cerr << "Cannot allocate hardware parameter structure: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Fill with default values
        err = snd_pcm_hw_params_any(pcm_, hw_params_);
        if (err < 0) {
            std::cerr << "Cannot initialize hardware parameter structure: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Set access type
        err = snd_pcm_hw_params_set_access(pcm_, hw_params_, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) {
            std::cerr << "Cannot set access type: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Set sample format (float)
        err = snd_pcm_hw_params_set_format(pcm_, hw_params_, SND_PCM_FORMAT_FLOAT_LE);
        if (err < 0) {
            // Fallback to S16_LE
            std::cerr << "Cannot set float format, trying S16_LE: " << snd_strerror(err) << std::endl;
            err = snd_pcm_hw_params_set_format(pcm_, hw_params_, SND_PCM_FORMAT_S16_LE);
            if (err < 0) {
                std::cerr << "Cannot set sample format: " << snd_strerror(err) << std::endl;
                close();
                return false;
            }
        }

        // Set sample rate
        unsigned int rate = format_.sample_rate;
        err = snd_pcm_hw_params_set_rate_near(pcm_, hw_params_, &rate, 0);
        if (err < 0) {
            std::cerr << "Cannot set sample rate: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Set channels
        err = snd_pcm_hw_params_set_channels(pcm_, hw_params_, format_.channels);
        if (err < 0) {
            std::cerr << "Cannot set channel count: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Set buffer size
        snd_pcm_uframes_t buffer_size_frames = 1024;
        err = snd_pcm_hw_params_set_buffer_size_near(pcm_, hw_params_, &buffer_size_frames);
        if (err < 0) {
            std::cerr << "Cannot set buffer size: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Apply hardware parameters
        err = snd_pcm_hw_params(pcm_, hw_params_);
        if (err < 0) {
            std::cerr << "Cannot apply hardware parameters: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Allocate software parameter structure
        err = snd_pcm_sw_params_malloc(&sw_params_);
        if (err < 0) {
            std::cerr << "Cannot allocate software parameter structure: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Get starting threshold
        err = snd_pcm_sw_params_set_start_threshold(pcm_, sw_params_, buffer_size_ / 4);
        if (err < 0) {
            std::cerr << "Cannot set start threshold: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Apply software parameters
        err = snd_pcm_sw_params(pcm_, sw_params_);
        if (err < 0) {
            std::cerr << "Cannot apply software parameters: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        // Prepare the PCM
        err = snd_pcm_prepare(pcm_);
        if (err < 0) {
            std::cerr << "Cannot prepare audio interface: " << snd_strerror(err) << std::endl;
            close();
            return false;
        }

        is_open_ = true;
        latency_ = buffer_size_ * 1000 / format_.sample_rate; // Convert to ms

        return true;
    }

    void close() override {
        if (pcm_) {
            snd_pcm_drain(pcm_);
            snd_pcm_close(pcm_);
            pcm_ = nullptr;
        }

        if (hw_params_) {
            snd_pcm_hw_params_free(hw_params_);
            hw_params_ = nullptr;
        }

        if (sw_params_) {
            snd_pcm_sw_params_free(sw_params_);
            sw_params_ = nullptr;
        }

        is_open_ = false;
    }

    int write(const float* buffer, int frames) override {
        if (!is_open_) return 0;

        // Write audio data
        int result = snd_pcm_writei(pcm_, buffer, frames);
        if (result == -EPIPE) {
            // Buffer underrun
            std::cerr << "Buffer underrun" << std::endl;
            snd_pcm_prepare(pcm_);
            return 0;
        } else if (result < 0) {
            std::cerr << "Write error: " << snd_strerror(result) << std::endl;
            return 0;
        }

        return result;
    }

    int get_latency() const override {
        return latency_;
    }

    int get_buffer_size() const override {
        return buffer_size_;
    }

    bool is_ready() const override {
        return is_open_ && pcm_;
    }
};

// Factory function for ALSA backend
std::unique_ptr<IAudioOutput> create_alsa_audio_output() {
    return std::make_unique<AudioOutputALSA>();
}

} // namespace audio

#else // AUDIO_BACKEND_ALSA not defined

#include "audio_output.h"
#include <iostream>

namespace audio {

// Fallback to stub if ALSA not available
std::unique_ptr<IAudioOutput> create_alsa_audio_output() {
    std::cout << "ALSA backend not compiled, falling back to stub" << std::endl;
    return create_audio_output(); // This will create stub
}

#endif // AUDIO_BACKEND_ALSA