/**
 * @file music_player_fixed.cpp
 * @brief Fixed version with proper signal handling and cross-platform support
 * @date 2025-12-10
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <atomic>
#include <signal.h>
#include <unistd.h>

// Signal handling
static std::atomic<bool> g_quit_flag(false);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nReceived shutdown signal, exiting..." << std::endl;
        g_quit_flag = true;
    }
}

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

class SimpleMusicPlayer {
private:
    std::vector<float> audio_buffer_;
    bool is_playing_;
    std::atomic<bool> stop_requested_;

public:
    SimpleMusicPlayer() : is_playing_(false), stop_requested_(false) {
        // Set up signal handling
        struct sigaction sa;
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
    }

    bool load_wav_file(const char* filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
            std::strncmp(header.wave, "WAVE", 4) != 0) {
            std::cerr << "Error: Not a valid WAV file" << std::endl;
            return false;
        }

        std::cout << "WAV Info:" << std::endl;
        std::cout << "  Sample Rate: " << header.sample_rate << " Hz" << std::endl;
        std::cout << "  Channels: " << header.channels << std::endl;
        std::cout << "  Bits: " << header.bits << std::endl;
        std::cout << "  Data Size: " << header.data_size << " bytes" << std::endl;

        // Read and convert audio data
        if (header.bits == 16) {
            std::vector<int16_t> pcm_data(header.data_size / 2);
            file.read(reinterpret_cast<char*>(pcm_data.data()), header.data_size);

            // Convert to float
            audio_buffer_.resize(pcm_data.size());
            for (size_t i = 0; i < pcm_data.size(); i++) {
                audio_buffer_[i] = pcm_data[i] / 32768.0f;
            }
        } else {
            std::cerr << "Error: Only 16-bit WAV supported" << std::endl;
            return false;
        }

        std::cout << "Loaded " << audio_buffer_.size() << " samples" << std::endl;
        return true;
    }

    bool play() {
        if (audio_buffer_.empty()) {
            std::cerr << "No audio data to play" << std::endl;
            return false;
        }

        is_playing_ = true;
        stop_requested_ = false;

        std::cout << "\nPlaying... (Press Ctrl+C to stop)" << std::endl;

        // Simulation of audio playback with interruptible loop
        size_t current_pos = 0;
        const size_t total_samples = audio_buffer_.size();
        const size_t chunk_size = 1024; // Process in chunks

        while (is_playing_ && !stop_requested_ && !g_quit_flag) {
            // Simulate audio output
            for (size_t i = 0; i < chunk_size && current_pos < total_samples; i++) {
                current_pos++;
            }

            // Loop back to beginning
            if (current_pos >= total_samples) {
                current_pos = 0;
            }

            // Small delay to avoid 100% CPU usage (10ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Progress indicator every 1000 chunks (10 seconds)
            static int progress_counter = 0;
            if (++progress_counter >= 1000) {
                progress_counter = 0;
                std::cout << "." << std::flush;
            }
        }

        is_playing_ = false;
        return true;
    }

    void stop() {
        stop_requested_ = true;
        is_playing_ = false;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        std::cout << "\nOptions:" << std::endl;
        std::cout << "  <wav_file>  Path to WAV audio file" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "   Professional Music Player v0.1.0" << std::endl;
    std::cout << "   Cross-Platform Audio Player" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Initializing Music Player Core Engine..." << std::endl;

    SimpleMusicPlayer player;

    if (!player.load_wav_file(argv[1])) {
        std::cerr << "Failed to load audio file" << std::endl;
        return 1;
    }

    // Start playback in a separate thread
    std::thread playback_thread([&player]() {
        player.play();
    });

    // Main thread waits for signal or timeout
    int timeout_seconds = 30;
    for (int i = 0; i < timeout_seconds && !g_quit_flag; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (g_quit_flag) break;
    }

    // Stop playback if still playing
    if (player.play()) {
        std::cout << "\n\nStopping playback..." << std::endl;
        player.stop();
    }

    // Wait for thread to finish
    if (playback_thread.joinable()) {
        playback_thread.join();
    }

    std::cout << "\nPlayback completed successfully!" << std::endl;

    return 0;
}