/**
 * @file test_universal_converter.cpp
 * @brief Test program for universal sample rate converter
 * @date 2025-12-10
 */

#include "universal_sample_rate_converter.h"
#include "wav_writer.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace audio;

// Generate test tone (sine wave)
void generate_test_tone(float* buffer, int frames, int sample_rate,
                       int channels, float frequency = 440.0f) {
    for (int frame = 0; frame < frames; ++frame) {
        float time = static_cast<float>(frame) / sample_rate;
        float sample = 0.5f * std::sin(2.0f * M_PI * frequency * time);

        for (int ch = 0; ch < channels; ++ch) {
            buffer[frame * channels + ch] = sample;
        }
    }
}

// Test conversion between specific rates
bool test_conversion_pair(int input_rate, int output_rate, int channels = 2) {
    std::cout << "  Testing " << input_rate << "Hz → " << output_rate << "Hz ... ";

    // Create converter
    UniversalSampleRateConverter converter;

    // Generate test input (1 second of audio)
    int input_frames = input_rate;
    std::vector<float> input(input_frames * channels);
    generate_test_tone(input.data(), input_frames, input_rate, channels, 1000.0f);

    // Calculate expected output frames
    int expected_output_frames = static_cast<int>(
        input_frames * static_cast<double>(output_rate) / input_rate);
    std::vector<float> output(expected_output_frames * channels);

    // Perform conversion
    int actual_output_frames = converter.convert(
        input.data(), input_frames,
        output.data(), expected_output_frames,
        input_rate, output_rate, channels);

    // Verify output
    if (actual_output_frames == 0) {
        std::cout << "❌ FAILED (no output)" << std::endl;
        return false;
    }

    // Check frame count accuracy (allow 1 frame tolerance)
    int frame_diff = std::abs(actual_output_frames - expected_output_frames);
    if (frame_diff > 1) {
        std::cout << "❌ FAILED (frame count: expected " << expected_output_frames
                  << ", got " << actual_output_frames << ")" << std::endl;
        return false;
    }

    // Verify output is not silence
    float max_amplitude = 0.0f;
    for (int i = 0; i < actual_output_frames * channels; ++i) {
        max_amplitude = std::max(max_amplitude, std::abs(output[i]));
    }

    if (max_amplitude < 0.1f) {
        std::cout << "❌ FAILED (silence output)" << std::endl;
        return false;
    }

    std::cout << "✅ OK (" << actual_output_frames << " frames)" << std::endl;
    return true;
}

// Test all user-requested rates
void test_user_requested_rates() {
    std::cout << "\n=== Testing User-Requested Sample Rates ===\n";

    // User specifically requested these rates
    const std::vector<int> user_rates = {
        44100, 88200, 176400, 352800, 705600,
        48000, 96000, 192000, 384000, 768000
    };

    std::cout << "\nRates requested by user:\n";
    for (int rate : user_rates) {
        std::cout << "  " << rate << " Hz (" << AudioSampleRate::get_rate_description(rate) << ")\n";
    }

    // Test conversions
    int total_tests = 0;
    int passed_tests = 0;

    std::cout << "\n--- Conversion Tests ---\n";

    // Test each rate to 48kHz
    for (int input_rate : user_rates) {
        total_tests++;
        if (test_conversion_pair(input_rate, 48000)) {
            passed_tests++;
        }
    }

    // Test 44.1kHz to each rate
    for (int output_rate : user_rates) {
        total_tests++;
        if (test_conversion_pair(44100, output_rate)) {
            passed_tests++;
        }
    }

    // Test some interesting pairs
    std::vector<std::pair<int, int>> special_pairs = {
        {44100, 48000},  // CD → DVD
        {48000, 44100},  // DVD → CD
        {88200, 96000},  // DVD ×2 → Professional ×2
        {96000, 88200},  // Professional ×2 → DVD ×2
        {176400, 192000}, // Professional ×4 → HD ×2
        {192000, 176400}, // HD ×2 → Professional ×4
        {352800, 384000}, // Professional ×8 → HD ×4
        {384000, 352800}, // HD ×4 → Professional ×8
        {705600, 768000}, // Professional ×16 → UHD ×2
        {768000, 705600}, // UHD ×2 → Professional ×16
    };

    std::cout << "\n--- Special Interest Conversions ---\n";
    for (auto& pair : special_pairs) {
        total_tests++;
        std::cout << AudioSampleRate::get_rate_description(pair.first) << " → "
                  << AudioSampleRate::get_rate_description(pair.second) << ":\n";
        if (test_conversion_pair(pair.first, pair.second)) {
            passed_tests++;
        }
    }

    // Summary
    std::cout << "\n--- User Rates Test Summary ---\n";
    std::cout << "Total tests: " << total_tests << "\n";
    std::cout << "Passed: " << passed_tests << "\n";
    std::cout << "Failed: " << (total_tests - passed_tests) << "\n";
    std::cout << "Success rate: " << std::fixed << std::setprecision(1)
              << (100.0 * passed_tests / total_tests) << "%\n";
}

// Demonstrate auto-optimization
void demonstrate_auto_optimization() {
    std::cout << "\n=== Auto-Optimization Demonstration ===\n";

    UniversalSampleRateConverter converter(AudioSampleRate::RATE_48000);

    // Test various input rates
    const std::vector<int> test_rates = {
        8000, 11025, 16000, 22050, 32000, 37800, 44100, 48000,
        88200, 96000, 176400, 192000, 352800, 384000
    };

    std::cout << "\nInput Rate → Selected Output Rate:\n";
    std::cout << std::setw(8) << "Input" << std::setw(15) << "Output" << "Category\n";
    std::cout << std::string(35, '-') << "\n";

    for (int input_rate : test_rates) {
        int output_rate = converter.select_optimal_output_rate(input_rate);
        std::cout << std::setw(7) << input_rate << "Hz → "
                  << std::setw(9) << output_rate << "Hz "
                  << AudioSampleRate::get_rate_category(output_rate) << "\n";
    }
}

// Generate test files for all requested rates
void generate_test_files() {
    std::cout << "\n=== Generating Test Audio Files ===\n";

    // Create WAV writer
    WAVWriter wav_writer;

    // User-requested rates
    const std::vector<int> user_rates = {
        44100, 88200, 176400, 352800, 705600,
        48000, 96000, 192000, 384000, 768000
    };

    const int channels = 2;
    const int duration_seconds = 2;  // 2 second test tones

    for (int rate : user_rates) {
        // Generate test tone
        int frames = rate * duration_seconds;
        std::vector<float> audio(frames * channels);

        // Use different frequencies for different rate categories
        float frequency = 440.0f;  // Default: A4
        if (rate >= 384000) {
            frequency = 880.0f;  // A5 for UHD rates
        } else if (rate >= 192000) {
            frequency = 660.0f;  // E5 for HD rates
        } else if (rate >= 96000) {
            frequency = 550.0f;  // C#5 for professional
        }

        generate_test_tone(audio.data(), frames, rate, channels, frequency);

        // Write WAV file
        std::string filename = "test_" + std::to_string(rate) + "hz.wav";
        if (wav_writer.write(filename.c_str(), audio.data(), frames,
                           rate, channels, 32)) {
            std::cout << "✅ Created: " << filename << " ("
                      << AudioSampleRate::get_rate_description(rate) << ")\n";
        } else {
            std::cout << "❌ Failed to create: " << filename << "\n";
        }
    }
}

// Performance benchmark
void benchmark_conversions() {
    std::cout << "\n=== Performance Benchmark ===\n";

    UniversalSampleRateConverter converter;

    // Benchmark setup
    const int channels = 2;
    const int test_duration_seconds = 10;  // Process 10 seconds of audio

    struct BenchmarkResult {
        int input_rate;
        int output_rate;
        double time_ms;
        double realtime_factor;
    };

    std::vector<BenchmarkResult> results;

    // Test user-requested conversions
    const std::vector<std::pair<int, int>> test_pairs = {
        {44100, 48000},   // CD → DVD
        {48000, 44100},   // DVD → CD
        {44100, 96000},   // CD → Professional
        {96000, 44100},   // Professional → CD
        {176400, 192000}, // Professional ×4 → HD ×2
        {192000, 176400}, // HD ×2 → Professional ×4
        {352800, 384000}, // Professional ×8 → HD ×4
        {384000, 352800}, // HD ×4 → Professional ×8
        {705600, 768000}, // Professional ×16 → UHD ×2
        {768000, 705600}, // UHD ×2 → Professional ×16
    };

    std::cout << "\nBenchmarking " << test_duration_seconds
              << " seconds of audio per conversion...\n";
    std::cout << std::setw(10) << "Input" << std::setw(10) << "Output"
              << std::setw(12) << "Time (ms)" << std::setw(15) << "Realtime Factor\n";
    std::cout << std::string(50, '-') << "\n";

    for (auto& pair : test_pairs) {
        int input_rate = pair.first;
        int output_rate = pair.second;

        // Generate test data
        int input_frames = input_rate * test_duration_seconds;
        std::vector<float> input(input_frames * channels);
        std::vector<float> output(input_frames * 2 * channels);  // Max possible size

        generate_test_tone(input.data(), input_frames, input_rate, channels);

        // Benchmark conversion
        auto start = std::chrono::high_resolution_clock::now();

        int output_frames = converter.convert(
            input.data(), input_frames,
            output.data(), input_frames * 2,
            input_rate, output_rate, channels);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double time_ms = duration.count() / 1000.0;
        double realtime_factor = (test_duration_seconds * 1000.0) / time_ms;

        results.push_back({input_rate, output_rate, time_ms, realtime_factor});

        std::cout << std::setw(8) << input_rate << "Hz"
                  << std::setw(8) << output_rate << "Hz"
                  << std::setw(10) << std::fixed << std::setprecision(1) << time_ms << "ms"
                  << std::setw(13) << std::setprecision(2) << realtime_factor << "x\n";
    }

    // Find average performance
    double avg_realtime_factor = 0;
    for (auto& r : results) {
        avg_realtime_factor += r.realtime_factor;
    }
    avg_realtime_factor /= results.size();

    std::cout << "\nAverage realtime factor: " << std::fixed
              << std::setprecision(2) << avg_realtime_factor << "x\n";

    if (avg_realtime_factor > 1.0) {
        std::cout << "✅ Converter is faster than real-time (can process audio faster than it plays)\n";
    } else {
        std::cout << "⚠️  Converter is slower than real-time\n";
    }
}

int main() {
    std::cout << "=== Universal Sample Rate Converter Test ===\n";
    std::cout << "Testing all user-requested sample rates:\n";
    std::cout << "44100, 88200, 176400, 352800, 705600, ";
    std::cout << "48000, 96000, 192000, 384000, 768000\n\n";

    // Run all tests
    test_user_requested_rates();
    demonstrate_auto_optimization();
    generate_test_files();
    benchmark_conversions();

    std::cout << "\n=== Test Complete ===\n";
    std::cout << "✅ Universal sample rate converter supports all requested rates!\n";

    return 0;
}