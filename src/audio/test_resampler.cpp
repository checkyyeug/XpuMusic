/**
 * @file test_resampler.cpp
 * @brief Test program for sample rate conversion
 * @date 2025-12-10
 */

#include "sample_rate_converter.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Generate test sine wave
void generate_sine_wave(std::vector<float>& buffer, int sample_rate, int duration_ms, float frequency) {
    int samples = (sample_rate * duration_ms) / 1000;
    buffer.resize(samples);

    for (int i = 0; i < samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        buffer[i] = std::sin(2.0f * M_PI * frequency * t) * 0.8f;
    }
}

// Write to WAV file
void write_wav_file(const std::string& filename, const std::vector<float>& data, int sample_rate, int channels) {
    struct {
        char riff[4];
        uint32_t size;
        char wave[4];
        char fmt[4];
        uint32_t fmt_size;
        uint16_t format;
        uint16_t channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits;
        char data[4];
        uint32_t data_size;
    } header;

    // Fill header
    ::memcpy(header.riff, "RIFF", 4);
    header.size = sizeof(header) + data.size() * sizeof(int16_t) - 8;
    ::memcpy(header.wave, "WAVE", 4);
    ::memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.format = 1;
    header.channels = channels;
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * channels * sizeof(int16_t);
    header.block_align = channels * sizeof(int16_t);
    header.bits = 16;
    ::memcpy(header.data, "data", 4);
    header.data_size = data.size() * sizeof(int16_t);

    // Convert float to 16-bit PCM
    std::vector<int16_t> pcm_data(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        float sample = data[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        pcm_data[i] = static_cast<int16_t>(sample * 32767.0f);
    }

    // Write file
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<char*>(&header), sizeof(header));
    file.write(reinterpret_cast<char*>(pcm_data.data()), pcm_data.size() * sizeof(int16_t));
}

// Test sample rate conversion
void test_conversion(int input_rate, int output_rate) {
    std::cout << "\nTesting conversion: " << input_rate << "Hz → " << output_rate << "Hz\n";
    std::cout << "----------------------------------------\n";

    try {
        // Create converter
        auto converter = audio::SampleRateConverterFactory::create("linear");

        // Initialize (mono for testing)
        if (!converter->initialize(input_rate, output_rate, 1)) {
            std::cerr << "Failed to initialize converter\n";
            return;
        }

        // Generate test signal (1 second, 440Hz)
        std::vector<float> input_buffer;
        generate_sine_wave(input_buffer, input_rate, 1000, 440.0f);

        // Allocate output buffer
        std::vector<float> output_buffer;
        int max_output_frames = static_cast<int>(input_buffer.size() *
                                          static_cast<double>(output_rate) / input_rate) + 100;
        output_buffer.resize(max_output_frames);

        // Perform conversion
        int output_frames = converter->convert(
            input_buffer.data(),
            static_cast<int>(input_buffer.size()),
            output_buffer.data(),
            max_output_frames
        );

        std::cout << "Input frames:  " << input_buffer.size() << "\n";
        std::cout << "Output frames: " << output_frames << "\n";
        std::cout << "Expected:      " << static_cast<int>(input_buffer.size() *
                                               static_cast<double>(output_rate) / input_rate) << "\n";

        // Calculate ratio error
        double expected_ratio = static_cast<double>(output_rate) / input_rate;
        double actual_ratio = static_cast<double>(output_frames) / input_buffer.size();
        double ratio_error = std::abs(actual_ratio - expected_ratio) / expected_ratio * 100;

        std::cout << "Ratio error:   " << std::fixed << std::setprecision(2) << ratio_error << "%\n";

        // Write test files
        write_wav_file("input_" + std::to_string(input_rate) + "Hz.wav",
                      input_buffer, input_rate, 1);
        write_wav_file("output_" + std::to_string(output_rate) + "Hz.wav",
                      output_buffer, output_rate, 1);

        std::cout << "Files written: input_" << input_rate << "Hz.wav, output_"
                  << output_rate << "Hz.wav\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Sample Rate Converter Test\n";
    std::cout << "========================================\n";

    // Test different conversion scenarios
    test_conversion(44100, 48000);  // Up-sampling
    test_conversion(48000, 44100);  // Down-sampling
    test_conversion(44100, 22050);  // 2:1 down-sampling
    test_conversion(22050, 44100);  // 1:2 up-sampling
    test_conversion(96000, 48000);  // 2:1 down-sampling (high quality)

    std::cout << "\n========================================\n";
    std::cout << "  Test completed\n";
    std::cout << "========================================\n";

    std::cout << "\nYou can play the generated WAV files to test the conversion quality:\n";
    std::cout << "Linux:   aplay input_44100Hz.wav output_48000Hz.wav\n";
    std::cout << "macOS:   afplay input_44100Hz.wav output_48000Hz.wav\n";
    std::cout << "Windows: play input_44100Hz.wav output_48000Hz.wav\n";

    return 0;
}