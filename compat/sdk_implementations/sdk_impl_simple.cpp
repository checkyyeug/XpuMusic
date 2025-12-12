/**
 * @file sdk_impl_simple.cpp
 * @brief Simplified SDK implementation without namespace conflicts
 * @date 2025-12-12
 */

#include "foobar_sdk_wrapper.h"
#include <iostream>

// Export simple functions for music player
extern "C" {

// Audio chunk functions
typedef struct {
    float* data;
    size_t frames;
    uint32_t channels;
    uint32_t sample_rate;
} audio_chunk_t;

// Create audio chunk
audio_chunk_t* create_audio_chunk() {
    audio_chunk_t* chunk = new audio_chunk_t();
    memset(chunk, 0, sizeof(audio_chunk_t));
    return chunk;
}

// Destroy audio chunk
void destroy_audio_chunk(audio_chunk_t* chunk) {
    if (chunk) {
        delete[] chunk->data;
        delete chunk;
    }
}

// Set audio chunk data
bool audio_chunk_set_data(audio_chunk_t* chunk, const float* data,
                         size_t frames, uint32_t channels, uint32_t sample_rate) {
    if (!chunk || !data || frames == 0 || channels == 0) {
        return false;
    }

    // Delete old data
    delete[] chunk->data;

    // Allocate new data
    chunk->data = new float[frames * channels];
    chunk->frames = frames;
    chunk->channels = channels;
    chunk->sample_rate = sample_rate;

    // Copy data
    memcpy(chunk->data, data, frames * channels * sizeof(float));

    return true;
}

// Get audio chunk data
const float* audio_chunk_get_data(const audio_chunk_t* chunk) {
    return chunk ? chunk->data : nullptr;
}

// Get audio chunk info
size_t audio_chunk_get_frames(const audio_chunk_t* chunk) {
    return chunk ? chunk->frames : 0;
}

uint32_t audio_chunk_get_channels(const audio_chunk_t* chunk) {
    return chunk ? chunk->channels : 0;
}

uint32_t audio_chunk_get_sample_rate(const audio_chunk_t* chunk) {
    return chunk ? chunk->sample_rate : 0;
}

// File info functions
typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bitrate;
    double length;
    uint64_t file_size;
} file_info_t;

// Create file info
file_info_t* create_file_info() {
    file_info_t* info = new file_info_t();
    memset(info, 0, sizeof(file_info_t));
    return info;
}

// Destroy file info
void destroy_file_info(file_info_t* info) {
    delete info;
}

// Set file info
void file_info_set_format(file_info_t* info, uint32_t sample_rate, uint32_t channels) {
    if (info) {
        info->sample_rate = sample_rate;
        info->channels = channels;
    }
}

void file_info_set_bitrate(file_info_t* info, uint32_t bitrate) {
    if (info) {
        info->bitrate = bitrate;
    }
}

void file_info_set_length(file_info_t* info, double length) {
    if (info) {
        info->length = length;
    }
}

void file_info_set_file_size(file_info_t* info, uint64_t size) {
    if (info) {
        info->file_size = size;
    }
}

// Get file info
uint32_t file_info_get_sample_rate(const file_info_t* info) {
    return info ? info->sample_rate : 0;
}

uint32_t file_info_get_channels(const file_info_t* info) {
    return info ? info->channels : 0;
}

double file_info_get_length(const file_info_t* info) {
    return info ? info->length : 0.0;
}

} // extern "C"