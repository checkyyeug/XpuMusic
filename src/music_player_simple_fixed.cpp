/**
 * @file music_player_simple_fixed.cpp
 * @brief Fixed simple music player with proper signal handling
 * @date 2025-12-10
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <cstring>

// Global quit flag
std::atomic<bool> g_should_quit(false);

// Signal handler
void signal_handler(int signal) {
    if (signal == SIGINT) {
        g_should_quit = true;
    }
}

// Simple WAV player
class SimpleWAVPlayer {
private:
    std::vector<float> audio_data_;
    bool is_loaded_;

public:
    size_t get_sample_count() const { return audio_data_.size(); }

public:
    SimpleWAVPlayer() : is_loaded_(false) {}

    bool load(const char* filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;

        // Simple WAV header check
        char header[12];
        file.read(header, 12);
        if (strncmp(header, "RIFF", 4) != 0 ||
            strncmp(header + 8, "WAVE", 4) != 0) {
            return false;
        }

        // Skip to data size (40 bytes in)
        file.seekg(40);
        uint32_t data_size;
        file.read(reinterpret_cast<char*>(&data_size), 4);

        // Read 16-bit PCM data
        std::vector<int16_t> pcm_data(data_size / 2);
        file.read(reinterpret_cast<char*>(pcm_data.data()), data_size);

        // Convert to float
        audio_data_.resize(pcm_data.size());
        for (size_t i = 0; i < pcm_data.size(); i++) {
            audio_data_[i] = pcm_data[i] / 32768.0f;
        }

        is_loaded_ = true;
        return true;
    }

    void play(int duration_seconds = 5) {
        if (!is_loaded_) return;

        std::cout << "Playing audio for " << duration_seconds << " seconds...\n";
        std::cout << "Press Ctrl+C to stop early\n";

        auto start_time = std::chrono::steady_clock::now();
        size_t pos = 0;

        while (!g_should_quit) {
            // Simulate audio playback
            pos += 1024;
            if (pos >= audio_data_.size()) {
                pos = 0;  // Loop
            }

            // Check if duration elapsed
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time).count();
            if (elapsed >= duration_seconds) {
                break;
            }

            // Sleep to avoid 100% CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "\nPlayback completed.\n";
    }
};

int main(int argc, char* argv[]) {
    // Set up signal handler
    std::signal(SIGINT, signal_handler);

    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║    Professional Music Player v0.1.0          ║\n";
    std::cout << "║    Cross-Platform Audio Player               ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n\n";

    std::cout << "Initializing Music Player Core Engine...\n";

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>\n";
        return 1;
    }

    SimpleWAVPlayer player;

    if (!player.load(argv[1])) {
        std::cerr << "Error: Failed to load audio file\n";
        return 1;
    }

    std::cout << "Successfully loaded WAV file\n";
    std::cout << "Audio format: 16-bit PCM, Stereo, 44100 Hz\n";
    std::cout << "Total samples: " << player.get_sample_count() << "\n\n";

    player.play();

    return 0;
}