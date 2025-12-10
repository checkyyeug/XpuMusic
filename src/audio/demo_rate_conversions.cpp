/**
 * @file demo_rate_conversions.cpp
 * @brief Demonstrate specific sample rate conversions requested by user
 * @date 2025-12-10
 */

#include "universal_sample_rate_converter.h"
#include "wav_writer.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <cstdio>

using namespace audio;

// Generate test tone
void generate_test_tone(float* buffer, int frames, int sample_rate,
                       int channels, float frequency = 1000.0f) {
    for (int frame = 0; frame < frames; ++frame) {
        float time = static_cast<float>(frame) / sample_rate;
        float sample = 0.5f * std::sin(2.0f * M_PI * frequency * time);

        for (int ch = 0; ch < channels; ++ch) {
            buffer[frame * channels + ch] = sample;
        }
    }
}

int main() {
    std::cout << "=== Sample Rate Conversion Demo ===\n";
    std::cout << "Demonstrating conversions between user-requested rates:\n\n";

    // User-requested specific rates
    std::vector<int> user_rates = {
        44100, 88200, 176400, 352800, 705600, 48000, 96000, 192000, 384000, 768000
    };

    UniversalSampleRateConverter converter;
    WAVWriter wav_writer;
    const int channels = 2;
    const int duration_seconds = 3;  // 3 seconds

    // Convert from 44.1kHz to all other rates
    std::cout << "Converting from 44.1kHz to all other requested rates:\n";
    std::cout << std::setw(15) << "Input Rate" << std::setw(15) << "Output Rate"
              << std::setw(20) << "Conversion" << std::setw(10) << "Status\n";
    std::cout << std::string(60, '-') << "\n";

    int input_rate = 44100;
    int input_frames = input_rate * duration_seconds;
    std::vector<float> input_audio(input_frames * channels);
    generate_test_tone(input_audio.data(), input_frames, input_rate, channels, 440.0f);

    // Save original
    std::string input_file = "demo_44100_to_all_original.wav";
    wav_writer.write(input_file.c_str(), input_audio.data(), input_frames,
                    input_rate, channels, 24);
    std::cout << std::setw(15) << "44100 Hz" << std::setw(15) << "44100 Hz"
              << std::setw(20) << "Original" << std::setw(10) << "✓\n";

    for (int output_rate : user_rates) {
        if (output_rate == input_rate) continue;

        // Calculate output frames
        int output_frames = static_cast<int>(
            input_frames * static_cast<double>(output_rate) / input_rate);
        std::vector<float> output_audio(output_frames * channels);

        // Perform conversion
        int converted = converter.convert(
            input_audio.data(), input_frames,
            output_audio.data(), output_frames,
            input_rate, output_rate, channels);

        // Save result
        std::string output_file = "demo_44100_to_" + std::to_string(output_rate) + ".wav";
        wav_writer.write(output_file.c_str(), output_audio.data(), converted,
                        output_rate, channels, 24);

        std::cout << std::setw(14) << "44100 Hz" << " → " << std::setw(11)
                  << output_rate << " Hz" << std::setw(20)
                  << (std::to_string(output_rate) + " Hz output") << std::setw(10)
                  << "✓\n";
    }

    // Now convert from high rates back to standard rates
    std::cout << "\n\nConverting from high-resolution rates to standard:\n";
    std::cout << std::string(60, '-') << "\n";

    std::vector<std::pair<int, int>> high_to_standard = {
        {768000, 48000},  // UHD to DVD
        {705600, 44100},  // UHD to CD
        {384000, 96000},  // HD to Professional
        {352800, 88200},  // HD to DVD
        {192000, 48000},  // HD to DVD
        {176400, 44100},  // Professional to CD
    };

    for (auto& conversion : high_to_standard) {
        int high_rate = conversion.first;
        int standard_rate = conversion.second;

        // Generate high-rate test tone
        int high_frames = high_rate * duration_seconds;
        std::vector<float> high_audio(high_frames * channels);
        generate_test_tone(high_audio.data(), high_frames, high_rate, channels, 880.0f);

        // Convert to standard
        int standard_frames = static_cast<int>(
            high_frames * static_cast<double>(standard_rate) / high_rate);
        std::vector<float> standard_audio(standard_frames * channels);

        int converted = converter.convert(
            high_audio.data(), high_frames,
            standard_audio.data(), standard_frames,
            high_rate, standard_rate, channels);

        // Save both
        std::string high_file = "demo_high_" + std::to_string(high_rate) + ".wav";
        std::string std_file = "demo_high_" + std::to_string(high_rate) +
                               "_to_" + std::to_string(standard_rate) + ".wav";

        wav_writer.write(high_file.c_str(), high_audio.data(), high_rate * duration_seconds,
                        high_rate, channels, 24);
        wav_writer.write(std_file.c_str(), standard_audio.data(), converted,
                        standard_rate, channels, 24);

        std::cout << std::setw(14) << (std::to_string(high_rate) + " Hz") << " → "
                  << std::setw(11) << standard_rate << " Hz"
                  << std::setw(20) << "High → Standard" << std::setw(10) << "✓\n";
    }

    // Cross conversions within same family
    std::cout << "\n\nCross conversions within rate families:\n";
    std::cout << std::string(60, '-') << "\n";

    std::vector<std::pair<int, int>> family_conversions = {
        {44100, 88200},      // CD ×2
        {88200, 176400},     // DVD ×2
        {176400, 352800},    // Professional ×2
        {352800, 705600},    // HD ×2
        {48000, 96000},      // DVD ×2
        {96000, 192000},     // Professional ×2
        {192000, 384000},    // HD ×2
        {384000, 768000},    // HD ×2
    };

    for (auto& conversion : family_conversions) {
        int from_rate = conversion.first;
        int to_rate = conversion.second;

        // Generate input
        int from_frames = from_rate;
        std::vector<float> from_audio(from_frames * channels);
        generate_test_tone(from_audio.data(), from_frames, from_rate, channels, 660.0f);

        // Convert
        int to_frames = static_cast<int>(
            from_frames * static_cast<double>(to_rate) / from_rate);
        std::vector<float> to_audio(to_frames * channels);

        int converted = converter.convert(
            from_audio.data(), from_frames,
            to_audio.data(), to_frames,
            from_rate, to_rate, channels);

        // Save
        std::string file = "demo_family_" + std::to_string(from_rate) + "_to_" +
                          std::to_string(to_rate) + ".wav";
        wav_writer.write(file.c_str(), to_audio.data(), converted, to_rate, channels, 24);

        std::cout << std::setw(14) << (std::to_string(from_rate) + " Hz") << " → "
                  << std::setw(11) << to_rate << " Hz"
                  << std::setw(20) << "Same Family" << std::setw(10) << "✓\n";
    }

    std::cout << "\n=== Demo Complete! ===\n";
    std::cout << "✅ Generated demo WAV files for all requested conversions\n";
    std::cout << "Files created with 'demo_' prefix\n";

    return 0;
}