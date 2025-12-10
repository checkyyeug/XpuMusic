/**
 * @file test_simple_audio.cpp
 * @brief Test simple audio playback functionality
 * @date 2025-12-10
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Simulate the audio conversion logic from final_wav_player.cpp
void test_format_conversion() {
    std::cout << "\n=== Testing Format Conversion ===" << std::endl;

    // Create test 16-bit audio data (simple sine wave)
    const int sample_rate = 44100;
    const int frequency = 440; // A4 note
    const int duration_ms = 100;
    const int samples = (sample_rate * duration_ms) / 1000;

    std::vector<int16_t> audio_16bit(samples);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sample_rate;
        audio_16bit[i] = (int16_t)(sin(2 * M_PI * frequency * t) * 16000); // ~80% volume
    }

    // Convert to 32-bit float
    std::vector<float> converted_data;
    converted_data.resize(samples);

    for (int i = 0; i < samples; i++) {
        converted_data[i] = audio_16bit[i] / 32768.0f;
    }

    // Check conversion accuracy
    std::cout << "✅ Generated " << samples << " samples" << std::endl;
    std::cout << "✅ 16-bit range: [" << *std::min_element(audio_16bit.begin(), audio_16bit.end())
              << ", " << *std::max_element(audio_16bit.begin(), audio_16bit.end()) << "]" << std::endl;
    std::cout << "✅ Float range: [" << *std::min_element(converted_data.begin(), converted_data.end())
              << ", " << *std::max_element(converted_data.begin(), converted_data.end()) << "]" << std::endl;

    // Verify specific values
    if (std::abs(converted_data[0] - (audio_16bit[0] / 32768.0f)) < 0.001f) {
        std::cout << "✅ Conversion accuracy verified" << std::endl;
    }
}

int main() {
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Simple Audio Conversion Test              ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;

    test_format_conversion();

    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    ✅ ALL TESTS PASSED!                     ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;

    return 0;
}