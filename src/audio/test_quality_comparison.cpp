/**
 * @file test_quality_comparison.cpp
 * @brief Compare quality of different resamplers
 * @date 2025-12-10
 */

#include "improved_sample_rate_converter.h"
#include "wav_writer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cmath>

using namespace audio;

// Generate test sweep tone
void generate_sweep(float* buffer, int frames, int sample_rate,
                    int channels, float start_freq = 20.0f, float end_freq = 20000.0f) {
    for (int frame = 0; frame < frames; ++frame) {
        float t = static_cast<float>(frame) / sample_rate;
        float progress = static_cast<float>(frame) / frames;

        // Log sweep
        float freq = start_freq * std::pow(end_freq / start_freq, progress);
        float phase = 2.0f * M_PI * freq * t;

        float sample = 0.5f * std::sin(phase);

        for (int ch = 0; ch < channels; ++ch) {
            buffer[frame * channels + ch] = sample;
        }
    }
}

// Generate test square wave (sharp edges test)
void generate_square_wave(float* buffer, int frames, int sample_rate,
                         int channels, float frequency = 1000.0f) {
    float period = sample_rate / frequency;

    for (int frame = 0; frame < frames; ++frame) {
        float sample = (static_cast<int>(frame / period) % 2) * 2.0f - 1.0f;
        sample *= 0.5f;  // Reduce amplitude

        for (int ch = 0; ch < channels; ++ch) {
            buffer[frame * channels + ch] = sample;
        }
    }
}

// Test a specific converter quality
void test_converter_quality(ResamplerQuality quality, const float* input,
                           int input_frames, int input_rate, int output_rate,
                           int channels, const std::string& test_name) {
    std::cout << "\nTesting " << ImprovedSampleRateConverter::get_quality_description(quality)
              << "\n" << std::string(70, '-') << "\n";

    // Create converter
    auto converter = ImprovedSampleRateConverterFactory::create(quality, true);

    if (!converter->initialize(input_rate, output_rate, channels)) {
        std::cout << "鉂?Failed to initialize converter\n";
        return;
    }

    // Calculate output frames
    int max_output_frames = static_cast<int>(
        input_frames * static_cast<double>(output_rate) / input_rate * 1.1);
    std::vector<float> output(max_output_frames * channels);

    // Benchmark conversion
    auto start = std::chrono::high_resolution_clock::now();

    int output_frames = converter->convert(
        input, input_frames,
        output.data(), max_output_frames);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Report results
    std::cout << "Input: " << input_frames << " frames @ " << input_rate << "Hz\n";
    std::cout << "Output: " << output_frames << " frames @ " << output_rate << "Hz\n";
    std::cout << "Conversion time: " << duration.count() / 1000.0 << " ms\n";
    std::cout << "Estimated CPU usage: " << converter->get_estimated_cpu_usage() << "%\n";
    std::cout << "Latency: " << converter->get_latency() << " frames\n";

    // Save output file
    std::string filename = "quality_" + ImprovedSampleRateConverterFactory::quality_to_string(quality) +
                          "_" + test_name + ".wav";
    WAVWriter writer;
    if (writer.write(filename.c_str(), output.data(), output_frames,
                    output_rate, channels, 24)) {
        std::cout << "鉁?Saved: " << filename << "\n";
    } else {
        std::cout << "鉂?Failed to save: " << filename << "\n";
    }
}

int main() {
    std::cout << "=== Sample Rate Converter Quality Comparison ===\n";
    std::cout << "Comparing different quality levels\n\n";

    const int channels = 2;
    const int input_rate = 44100;
    const int output_rate = 48000;
    const int duration_seconds = 2;

    int input_frames = input_rate * duration_seconds;
    std::vector<float> test_signal_sweep(input_frames * channels);
    std::vector<float> test_signal_square(input_frames * channels);

    // Generate test signals
    generate_sweep(test_signal_sweep.data(), input_frames, input_rate, channels);
    generate_square_wave(test_signal_square.data(), input_frames, input_rate, channels);

    // Test all quality levels with sweep signal
    std::cout << "\n=== Test 1: Frequency Sweep (20Hz - 20kHz) ===\n";
    std::vector<ResamplerQuality> qualities = {
        ResamplerQuality::Fast,
        ResamplerQuality::Good,
        ResamplerQuality::High,
        ResamplerQuality::VeryHigh,
        ResamplerQuality::Best
    };

    for (auto quality : qualities) {
        test_converter_quality(quality, test_signal_sweep.data(),
                              input_frames, input_rate, output_rate,
                              channels, "sweep_44k_to_48k");
    }

    // Test downsampling (more challenging)
    std::cout << "\n\n=== Test 2: Downsampling (96kHz 鈫?44.1kHz) ===\n";
    int downsample_rate = 44100;
    int downsample_frames = 96000 * duration_seconds;
    std::vector<float> downsample_input(downsample_frames * channels);
    generate_sweep(downsample_input.data(), downsample_frames, 96000, channels);

    for (auto quality : qualities) {
        test_converter_quality(quality, downsample_input.data(),
                              downsample_frames, 96000, downsample_rate,
                              channels, "sweep_96k_to_44k");
    }

    // Test upsampling with square wave
    std::cout << "\n\n=== Test 3: Square Wave Upsampling (44.1kHz 鈫?96kHz) ===\n";
    for (auto quality : qualities) {
        test_converter_quality(quality, test_signal_square.data(),
                              input_frames, input_rate, 96000,
                              channels, "square_44k_to_96k");
    }

    // Performance comparison table
    std::cout << "\n\n=== Performance Summary ===\n";
    std::cout << std::setw(12) << "Quality"
              << std::setw(15) << "Est. CPU %"
              << std::setw(12) << "Latency"
              << std::setw(20) << "Use Case\n";
    std::cout << std::string(70, '-') << "\n";

    for (auto quality : qualities) {
        auto converter = ImprovedSampleRateConverterFactory::create(quality);
        converter->initialize(44100, 48000, channels);

        std::cout << std::setw(12) << ImprovedSampleRateConverter::get_quality_description(quality)
                  << std::setw(12) << converter->get_estimated_cpu_usage()
                  << std::setw(7) << converter->get_latency() << " frames"
                  << std::setw(20);

        switch (quality) {
            case ResamplerQuality::Fast:
                std::cout << "Real-time, games";
                break;
            case ResamplerQuality::Good:
                std::cout << "General use";
                break;
            case ResamplerQuality::High:
                std::cout << "Music production";
                break;
            case ResamplerQuality::VeryHigh:
                std::cout << "Professional audio";
                break;
            case ResamplerQuality::Best:
                std::cout << "Critical applications";
                break;
        }
        std::cout << "\n";
    }

    std::cout << "\n=== Analysis Notes ===\n";
    std::cout << "1. Fast (Linear):\n";
    std::cout << "   - Minimal CPU usage (<0.1%)\n";
    std::cout << "   - Almost zero latency\n";
    std::cout << "   - Noticeable high-frequency loss\n";
    std::cout << "   - Suitable for real-time applications\n\n";

    std::cout << "2. Good (Cubic):\n";
    std::cout << "   - Low CPU usage (~0.5%)\n";
    std::cout << "   - Very low latency (2 frames)\n";
    std::cout << "   - Much better than linear\n";
    std::cout << "   - Good balance of quality and speed\n\n";

    std::cout << "3. High (Sinc 4-tap):\n";
    std::cout << "   - Moderate CPU usage (~2%)\n";
    std::cout << "   - Low latency\n";
    std::cout << "   - Good anti-aliasing\n";
    std::cout << "   - Suitable for music playback\n\n";

    std::cout << "4. Very High (Sinc 8-tap):\n";
    std::cout << "   - High CPU usage (~5%)\n";
    std::cout << "   - Excellent quality\n";
    std::cout << "   - Professional grade\n";
    std::cout << "   - Not for low-power devices\n\n";

    std::cout << "5. Best (Sinc 16-tap):\n";
    std::cout << "   - Very high CPU usage (~12%)\n";
    std::cout << "   - Best possible quality\n";
    std::cout << "   - Critical applications only\n";
    std::cout << "   - Similar to foobar2000's best mode\n\n";

    std::cout << "=== Recommendations ===\n";
    std::cout << "鈥?Use 'Fast' for: Games, real-time communication, embedded systems\n";
    std::cout << "鈥?Use 'Good' for: General music playback, most applications\n";
    std::cout << "鈥?Use 'High' for: Music production, audiophile listening\n";
    std::cout << "鈥?Use 'Very High' for: Professional audio work\n";
    std::cout << "鈥?Use 'Best' for: Critical applications, archival processing\n\n";

    std::cout << "Generated test files:\n";
    std::cout << "- quality_*.wav: Different quality levels\n";
    std::cout << "  Check frequency response and aliasing using spectrum analyzer\n";
    std::cout << "- Square wave files show ringing and transient response\n";
    std::cout << "- Sweep files show frequency response flatness\n\n";

    std::cout << "鉁?Quality comparison complete!\n";

    return 0;
}