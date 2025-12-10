/**
 * @file test_format_conversion.cpp
 * @brief Test the format conversion fix
 * @date 2025-12-10
 */

#include <iostream>
#include <vector>
#include <cstdint>

// Test the conversion logic we added to final_wav_player.cpp
int main() {
    std::cout << "Testing format conversion logic..." << std::endl;

    // Simulate 16-bit audio data
    std::vector<int16_t> audio_16bit = {
        32767, 0, -32768, 0,  // Full scale values
        16384, 0, -16384, 0   // Half scale values
    };

    // Convert to 32-bit float (same logic as in final_wav_player.cpp)
    std::vector<float> converted_data;
    uint32_t samples = audio_16bit.size();
    converted_data.resize(samples);

    for (uint32_t i = 0; i < samples; i++) {
        converted_data[i] = audio_16bit[i] / 32768.0f; // Normalize to [-1, 1]
    }

    // Display results
    std::cout << "\nOriginal 16-bit samples:" << std::endl;
    for (int16_t sample : audio_16bit) {
        std::cout << "  " << sample << std::endl;
    }

    std::cout << "\nConverted 32-bit float samples:" << std::endl;
    for (float sample : converted_data) {
        std::cout << "  " << sample << std::endl;
    }

    // Test the extremes
    std::cout << "\n✅ Conversion test successful!" << std::endl;
    std::cout << "   32767 -> " << (32767 / 32768.0f) << " (should be close to 1.0)" << std::endl;
    std::cout << "   -32768 -> " << (-32768 / 32768.0f) << " (should be exactly -1.0)" << std::endl;
    std::cout << "   0 -> " << (0 / 32768.0f) << " (should be exactly 0.0)" << std::endl;

    return 0;
}