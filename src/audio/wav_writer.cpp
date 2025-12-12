/**
 * @file wav_writer.cpp
 * @brief Simple WAV file writer implementation
 * @date 2025-12-10
 */

#include "wav_writer.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdint>

WAVWriter::WAVWriter() {
}

WAVWriter::~WAVWriter() {
}

bool WAVWriter::write(const char* filename, const float* data, int frames,
                      int sample_rate, int channels, int bits_per_sample) {
    if (!data || frames <= 0 || sample_rate <= 0 || channels <= 0) {
        return false;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Calculate data size
    int bytes_per_sample = bits_per_sample / 8;
    int data_size = frames * channels * bytes_per_sample;

    // Write header (with placeholder for file size)
    write_header(fp, data_size, sample_rate, channels, bits_per_sample);

    // Convert and write audio data
    int total_samples = frames * channels;

    if (bits_per_sample == 16) {
        int16_t* int16_data = new int16_t[total_samples];
        convert_float_to_int16(data, int16_data, total_samples);
        fwrite(int16_data, sizeof(int16_t), total_samples, fp);
        delete[] int16_data;
    }
    else if (bits_per_sample == 24) {
        uint8_t* int24_data = new uint8_t[total_samples * 3];
        convert_float_to_int24(data, int24_data, total_samples);
        fwrite(int24_data, 3, total_samples, fp);
        delete[] int24_data;
    }
    else if (bits_per_sample == 32) {
        int32_t* int32_data = new int32_t[total_samples];
        convert_float_to_int32(data, int32_data, total_samples);
        fwrite(int32_data, sizeof(int32_t), total_samples, fp);
        delete[] int32_data;
    }
    else {
        fclose(fp);
        return false;
    }

    // Update file size in header
    uint32_t file_size = sizeof(WAVHeader) - 8 + data_size;
    fseek(fp, 4, SEEK_SET);
    fwrite(&file_size, sizeof(uint32_t), 1, fp);

    fclose(fp);
    return true;
}

void WAVWriter::write_header(FILE* fp, int data_size, int sample_rate,
                             int channels, int bits_per_sample) {
    WAVHeader header;

    // RIFF header
    memcpy(header.riff, "RIFF", 4);
    header.file_size = sizeof(WAVHeader) - 8 + data_size;
    memcpy(header.wave, "WAVE", 4);

    // fmt chunk
    memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1;  // PCM
    header.channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = bits_per_sample;
    header.block_align = channels * bits_per_sample / 8;
    header.byte_rate = sample_rate * header.block_align;

    // data chunk
    memcpy(header.data, "data", 4);
    header.data_size = data_size;

    // Write header
    fwrite(&header, sizeof(WAVHeader), 1, fp);
}

void WAVWriter::convert_float_to_int16(const float* input, int16_t* output, int count) {
    for (int i = 0; i < count; ++i) {
        float sample = input[i];
        // Clamp to [-1.0, 1.0]
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        // Convert to int16
        output[i] = static_cast<int16_t>(sample * 32767.0f);
    }
}

void WAVWriter::convert_float_to_int24(const float* input, uint8_t* output, int count) {
    for (int i = 0; i < count; ++i) {
        float sample = input[i];
        // Clamp to [-1.0, 1.0]
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        // Convert to int24
        int32_t int24 = static_cast<int32_t>(sample * 8388607.0f);
        // Write little-endian 24-bit
        output[i * 3] = int24 & 0xFF;
        output[i * 3 + 1] = (int24 >> 8) & 0xFF;
        output[i * 3 + 2] = (int24 >> 16) & 0xFF;
    }
}

void WAVWriter::convert_float_to_int32(const float* input, int32_t* output, int count) {
    for (int i = 0; i < count; ++i) {
        float sample = input[i];
        // Clamp to [-1.0, 1.0]
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        // Convert to int32
        output[i] = static_cast<int32_t>(sample * 2147483647.0f);
    }
}