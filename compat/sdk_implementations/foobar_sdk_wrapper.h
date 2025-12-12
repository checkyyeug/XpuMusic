/**
 * @file foobar_sdk_wrapper.h
 * @brief Wrapper for foobar2000 SDK functionality
 * @date 2025-12-12
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>

// Forward declarations - avoid including the full SDK in headers
namespace foobar2000 {
    class audio_chunk;
    class file_info;
    class abort_callback;
    class service_base;
}

// Simple type definitions to avoid including full headers
namespace xpumusic_sdk {
    struct audio_info {
        uint32_t m_sample_rate;
        uint32_t m_channels;
        uint32_t m_bitrate;
        double m_length;
    };

    struct file_stats {
        uint64_t m_size;
        uint64_t m_timestamp;

        bool is_valid() const { return m_size > 0; }
    };
}

// Wrapper namespace
namespace xpumusic_sdk {

// Initialize Foobar2000 SDK compatibility layer
bool initialize_foobar_sdk();

// Shutdown Foobar2000 SDK compatibility layer
void shutdown_foobar_sdk();

// Create audio chunk from data
void* create_audio_chunk(const float* data, size_t frames,
                        uint32_t channels, uint32_t sample_rate);

// Destroy audio chunk
void destroy_audio_chunk(void* chunk);

}  // namespace xpumusic_sdk

namespace foobar_sdk_wrapper {

// Simple audio chunk wrapper
class AudioChunkWrapper {
public:
    AudioChunkWrapper();
    ~AudioChunkWrapper();

    // Initialize with audio data
    bool set_data(const float* data, size_t frames,
                  uint32_t channels, uint32_t sample_rate);

    // Getters
    float* get_data();
    const float* get_data() const;
    size_t get_frame_count() const;
    uint32_t get_sample_rate() const;
    uint32_t get_channel_count() const;

private:
    // Store data internally instead of using foobar2000 types
    std::vector<float> data_;
    uint32_t sample_rate_;
    uint32_t channels_;
    size_t frames_;
};

// File info wrapper
class FileInfoWrapper {
public:
    FileInfoWrapper();
    ~FileInfoWrapper();

    // Set audio information
    void set_sample_rate(uint32_t rate);
    void set_channels(uint32_t channels);
    void set_bitrate(uint32_t bitrate);
    void set_length(double length);
    void set_file_size(uint64_t size);

    // Meta information
    void set_meta(const char* field, const char* value);
    const char* get_meta(const char* field) const;

private:
    xpumusic_sdk::audio_info audio_info_;
    xpumusic_sdk::file_stats stats_;
    std::map<std::string, std::string> meta_fields_;
};

// Abort callback wrapper
class AbortCallbackWrapper {
public:
    AbortCallbackWrapper();
    ~AbortCallbackWrapper();

    // Check if operation should be aborted
    bool is_aborting() const;

    // Set abort state
    void set_aborting(bool state);

private:
    bool aborting_;
};

} // namespace foobar_sdk_wrapper