/**
 * @file audio_output.h
 * @brief Unified audio output interface
 * @date 2025-12-10
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace audio {

/**
 * @brief Audio format specification
 */
struct AudioFormat {
    uint32_t sample_rate;    // Sample rate in Hz
    uint16_t channels;       // Number of channels (1=mono, 2=stereo)
    uint16_t bits_per_sample; // Bits per sample (16, 24, 32)
    bool is_float;           // Whether samples are float

    AudioFormat() : sample_rate(44100), channels(2), bits_per_sample(16), is_float(false) {}
};

/**
 * @brief Audio output configuration
 */
struct AudioConfig {
    std::string device_name = "default";  // Device name or identifier
    int buffer_size = 4096;               // Buffer size in frames
    int buffer_count = 4;                 // Number of buffers
    int sample_rate = 44100;              // Default sample rate
    int channels = 2;                     // Default channels
    double volume = 1.0;                  // Volume (0.0 - 1.0)
    bool mute = false;                    // Mute state
};

/**
 * @brief Audio output interface
 *
 * All audio backends must implement this interface
 */
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    /**
     * @brief Initialize audio output with format and configuration
     * @param format Audio format
     * @param config Audio configuration
     * @return true on success
     */
    virtual bool initialize(const AudioFormat& format, const AudioConfig& config) = 0;

    /**
     * @brief Open audio device with specified format
     * @param format Audio format
     * @return true on success
     */
    virtual bool open(const AudioFormat& format) = 0;

    /**
     * @brief Close audio device
     */
    virtual void close() = 0;

    /**
     * @brief Start audio playback
     */
    virtual void start() = 0;

    /**
     * @brief Stop audio playback
     */
    virtual void stop() = 0;

    /**
     * @brief Write audio data to device
     * @param buffer Audio data (interleaved samples, format dependent)
     * @param frames Number of frames to write
     * @return Number of frames actually written
     */
    virtual int write(const void* buffer, int frames) = 0;

    /**
     * @brief Set output volume
     * @param volume Volume level (0.0 - 1.0)
     */
    virtual void set_volume(double volume) = 0;

    /**
     * @brief Get current volume
     * @return Current volume (0.0 - 1.0)
     */
    virtual double get_volume() const = 0;

    /**
     * @brief Set mute state
     * @param mute Mute state
     */
    virtual void set_mute(bool mute) = 0;

    /**
     * @brief Get mute state
     * @return Current mute state
     */
    virtual bool is_muted() const = 0;

    /**
     * @brief Get output latency in milliseconds
     * @return Latency in ms
     */
    virtual int get_latency() const = 0;

    /**
     * @brief Get buffer size in frames
     * @return Buffer size
     */
    virtual int get_buffer_size() const = 0;

    /**
     * @brief Check if device is ready
     * @return true if ready
     */
    virtual bool is_ready() const = 0;

    /**
     * @brief Cleanup resources
     */
    virtual void cleanup() = 0;
};

/**
 * @brief Factory function to create audio output
 * @return Unique pointer to audio output instance
 */
std::unique_ptr<IAudioOutput> create_audio_output();

/**
 * @brief Factory function to create audio output with configuration
 * @param config Audio configuration
 * @return Unique pointer to audio output instance
 */
std::unique_ptr<IAudioOutput> create_audio_output(const AudioConfig& config);

/**
 * @brief Get the name of the audio backend being used
 * @return Backend name (e.g., "ALSA", "WASAPI", "CoreAudio", "stub")
 */
const char* get_audio_backend_name();

} // namespace audio