/**
 * @file wav_writer.h
 * @brief Simple WAV file writer
 * @date 2025-12-10
 */

#pragma once

#include <string>
#include <cstdint>

class WAVWriter {
public:
    WAVWriter();
    ~WAVWriter();

    /**
     * Write audio data to WAV file
     * @param filename Output file name
     * @param data Audio data (float samples)
     * @param frames Number of frames
     * @param sample_rate Sample rate
     * @param channels Number of channels
     * @param bits_per_sample Bits per sample (16, 24, or 32)
     * @return True if successful
     */
    bool write(const char* filename, const float* data, int frames,
               int sample_rate, int channels, int bits_per_sample = 16);

private:
    struct WAVHeader {
        char riff[4];
        uint32_t file_size;
        char wave[4];
        char fmt[4];
        uint32_t fmt_size;
        uint16_t audio_format;
        uint16_t channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
        char data[4];
        uint32_t data_size;
    };

    void write_header(FILE* fp, int data_size, int sample_rate, int channels, int bits_per_sample);
    void convert_float_to_int16(const float* input, int16_t* output, int count);
    void convert_float_to_int24(const float* input, uint8_t* output, int count);
    void convert_float_to_int32(const float* input, int32_t* output, int count);
};