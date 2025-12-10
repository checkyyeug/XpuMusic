/**
 * @file main.cpp
 * @brief Test program for audio backend auto-detection
 * @date 2025-12-10
 */

#include "audio_output.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <vector>
#include <cstring>

// Simple WAV parser
struct WAVHeader {
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
};

int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║    Audio Backend Auto-Detection Test           ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n\n";

    // Show detected backend
    std::cout << "Detected audio backend: " << audio::get_audio_backend_name() << "\n\n";

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>\n";
        return 1;
    }

    // Create audio output
    auto audio = audio::create_audio_output();

    // Load WAV file
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file " << argv[1] << "\n";
        return 1;
    }

    // Read WAV header
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0) {
        std::cerr << "Error: Not a valid WAV file\n";
        return 1;
    }

    // Display WAV info
    audio::AudioFormat format;
    format.sample_rate = header.sample_rate;
    format.channels = header.channels;
    format.bits_per_sample = header.bits;

    std::cout << "WAV File Information:\n";
    std::cout << "  Sample Rate: " << format.sample_rate << " Hz\n";
    std::cout << "  Channels: " << format.channels << "\n";
    std::cout << "  Bits: " << format.bits_per_sample << "-bit\n";
    std::cout << "  Data Size: " << header.data_size << " bytes\n\n";

    // Read audio data
    std::vector<int16_t> pcm_data(header.data_size / 2);
    file.read(reinterpret_cast<char*>(pcm_data.data()), header.data_size);

    // Convert to float
    std::vector<float> audio_data(pcm_data.size());
    for (size_t i = 0; i < pcm_data.size(); i++) {
        audio_data[i] = pcm_data[i] / 32768.0f;
    }

    std::cout << "Loaded " << audio_data.size() << " audio samples\n\n";

    // Test audio backend
    std::cout << "Testing audio backend...\n";

    if (!audio->open(format)) {
        std::cerr << "Error: Failed to open audio device\n";
        return 1;
    }

    std::cout << "✓ Audio device opened successfully\n";
    std::cout << "  Buffer Size: " << audio->get_buffer_size() << " frames\n";
    std::cout << "  Latency: " << audio->get_latency() << " ms\n\n";

    // "Play" audio for 5 seconds
    std::cout << "Processing audio for 5 seconds...\n";
    auto start_time = std::chrono::steady_clock::now();

    size_t pos = 0;
    const size_t chunk_size = 1024;
    const int duration_seconds = 5;

    while (true) {
        // Calculate elapsed time
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (elapsed >= duration_seconds) break;

        // Write chunk
        size_t frames_to_write = std::min(chunk_size, audio_data.size() - pos);
        if (frames_to_write > 0) {
            int frames_written = audio->write(
                audio_data.data() + pos,
                static_cast<int>(frames_to_write)
            );

            if (frames_written > 0) {
                pos += frames_written;
            }
        }

        // Small delay to simulate real-time processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Progress indicator
        if (elapsed > 0 && elapsed % 1 == 0) {
            std::cout << ".";
            std::cout.flush();
        }
    }

    std::cout << "\n\n";

    // Close audio device
    audio->close();
    std::cout << "✓ Audio device closed\n";

    std::cout << "Test completed successfully!\n";
    std::cout << "\nAudio backend '" << audio::get_audio_backend_name() << "' ";

    if (std::string(audio::get_audio_backend_name()) == "stub") {
        std::cout << "is the stub implementation (no actual audio output)\n";
    } else {
        std::cout << "provides real audio output capability\n";
    }

    return 0;
}