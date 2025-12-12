/**
 * @file foobar_sdk_wrapper.cpp
 * @brief Implementation of foobar2000 SDK wrapper
 * @date 2025-12-12
 */

#include "foobar_sdk_wrapper.h"
#include <algorithm>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#endif

// Global initialization state
static bool sdk_initialized = false;

namespace foobar_sdk_wrapper {

// AudioChunkWrapper implementation
AudioChunkWrapper::AudioChunkWrapper() : sample_rate_(44100), channels_(2), frames_(0) {
}

AudioChunkWrapper::~AudioChunkWrapper() {
}

bool AudioChunkWrapper::set_data(const float* data, size_t frames,
                                uint32_t channels, uint32_t sample_rate) {
    if (!data || frames == 0 || channels == 0) {
        data_.clear();
        frames_ = 0;
        return false;
    }

    sample_rate_ = sample_rate;
    channels_ = channels;
    frames_ = frames;

    data_.resize(frames * channels);
    std::copy(data, data + frames * channels, data_.begin());

    return true;
}

float* AudioChunkWrapper::get_data() {
    return data_.empty() ? nullptr : data_.data();
}

const float* AudioChunkWrapper::get_data() const {
    return data_.empty() ? nullptr : data_.data();
}

size_t AudioChunkWrapper::get_frame_count() const {
    return frames_;
}

uint32_t AudioChunkWrapper::get_sample_rate() const {
    return sample_rate_;
}

uint32_t AudioChunkWrapper::get_channel_count() const {
    return channels_;
}

// FileInfoWrapper implementation
FileInfoWrapper::FileInfoWrapper() {
    memset(&audio_info_, 0, sizeof(audio_info_));
    memset(&stats_, 0, sizeof(stats_));
}

FileInfoWrapper::~FileInfoWrapper() {
}

void FileInfoWrapper::set_sample_rate(uint32_t rate) {
    audio_info_.m_sample_rate = rate;
}

void FileInfoWrapper::set_channels(uint32_t channels) {
    audio_info_.m_channels = channels;
}

void FileInfoWrapper::set_bitrate(uint32_t bitrate) {
    audio_info_.m_bitrate = bitrate;
}

void FileInfoWrapper::set_length(double length) {
    audio_info_.m_length = length;
}

void FileInfoWrapper::set_file_size(uint64_t size) {
    stats_.m_size = size;
}

void FileInfoWrapper::set_meta(const char* field, const char* value) {
    if (field && value) {
        meta_fields_[field] = value;
    }
}

const char* FileInfoWrapper::get_meta(const char* field) const {
    auto it = meta_fields_.find(field);
    return it != meta_fields_.end() ? it->second.c_str() : nullptr;
}

// AbortCallbackWrapper implementation
AbortCallbackWrapper::AbortCallbackWrapper() : aborting_(false) {
}

AbortCallbackWrapper::~AbortCallbackWrapper() {
}

bool AbortCallbackWrapper::is_aborting() const {
    return aborting_;
}

void AbortCallbackWrapper::set_aborting(bool state) {
    aborting_ = state;
}

} // namespace foobar_sdk_wrapper

// xpumusic_sdk namespace implementation
namespace xpumusic_sdk {

bool initialize_foobar_sdk() {
    if (sdk_initialized) {
        return true;
    }

    std::cout << "Initializing Foobar2000 SDK compatibility layer..." << std::endl;

    // Initialize COM on Windows
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM" << std::endl;
        return false;
    }
#endif

    sdk_initialized = true;
    std::cout << "Foobar2000 SDK compatibility layer initialized!" << std::endl;
    return true;
}

void shutdown_foobar_sdk() {
    if (!sdk_initialized) {
        return;
    }

    // Cleanup COM on Windows
#ifdef _WIN32
    CoUninitialize();
#endif

    sdk_initialized = false;
    std::cout << "Foobar2000 SDK compatibility layer shutdown." << std::endl;
}

void* create_audio_chunk(const float* data, size_t frames,
                        uint32_t channels, uint32_t sample_rate) {
    // Create a simple audio chunk structure
    struct simple_audio_chunk {
        float* data;
        size_t frames;
        uint32_t channels;
        uint32_t sample_rate;
    };

    simple_audio_chunk* chunk = new simple_audio_chunk;
    chunk->frames = frames;
    chunk->channels = channels;
    chunk->sample_rate = sample_rate;

    if (data && frames > 0 && channels > 0) {
        size_t total_samples = frames * channels;
        chunk->data = new float[total_samples];
        memcpy(chunk->data, data, total_samples * sizeof(float));
    } else {
        chunk->data = nullptr;
    }

    return chunk;
}

void destroy_audio_chunk(void* chunk) {
    if (chunk) {
        struct simple_audio_chunk {
            float* data;
            size_t frames;
            uint32_t channels;
            uint32_t sample_rate;
        };
        simple_audio_chunk* ac = static_cast<simple_audio_chunk*>(chunk);
        delete[] ac->data;
        delete ac;
    }
}

}  // namespace xpumusic_sdk