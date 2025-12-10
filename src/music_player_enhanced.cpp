/**
 * @file music_player_enhanced.cpp
 * @brief Enhanced music player with quality-adjustable sample rate conversion
 * @date 2025-12-10
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <signal.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <algorithm>
#include <cmath>

// Include our enhanced audio components
#include "audio/wav_writer.h"
#include "audio/enhanced_sample_rate_converter.h"
#include "audio/adaptive_resampler.h"

// Platform-specific audio output
#ifdef __linux__
#include <alsa/asoundlib.h>
#endif

// Audio format structure
struct AudioFormat {
    int sample_rate;
    int channels;
    int bits_per_sample;
};

// WAV file header
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

// Playback state
enum PlaybackState {
    STOPPED,
    PLAYING,
    PAUSED
};

// Global stop flag for signal handling
std::atomic<bool> g_stop_flag(false);

void signal_handler(int sig) {
    if (sig == SIGINT) {
        g_stop_flag = true;
        std::cout << "\nReceived SIGINT, stopping playback...\n";
    }
}

class EnhancedMusicPlayer {
private:
    PlaybackState state_;
    std::vector<float> audio_buffer_;
    size_t current_pos_;
    size_t total_frames_;
    AudioFormat file_format_;
    AudioFormat output_format_;

    // Enhanced resampler
    std::unique_ptr<AdaptiveSampleRateConverter> resampler_;
    ResampleQuality current_quality_;

    // Platform-specific audio handle
#ifdef __linux__
    snd_pcm_t* pcm_handle_;
#endif

    // Configuration
    bool use_adaptive_;
    std::string resampler_mode_;

public:
    EnhancedMusicPlayer()
        : state_(STOPPED)
        , current_pos_(0)
        , total_frames_(0)
        , current_quality_(ResampleQuality::Good)
        , use_adaptive_(true)
        , resampler_mode_("music") {
#ifdef __linux__
        pcm_handle_ = nullptr;
#endif

        // Initialize output format (default to 48kHz)
        output_format_ = {48000, 2, 32};
    }

    ~EnhancedMusicPlayer() {
        close();
    }

    bool load_wav(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Cannot open file " << filename << "\n";
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        // Verify WAV format
        if (strncmp(header.riff, "RIFF", 4) != 0 ||
            strncmp(header.wave, "WAVE", 4) != 0) {
            std::cerr << "Error: Not a valid WAV file\n";
            return false;
        }

        // Store file format
        file_format_.sample_rate = header.sample_rate;
        file_format_.channels = header.channels;
        file_format_.bits_per_sample = header.bits;

        // Read audio data
        total_frames_ = header.data_size / (header.channels * header.bits / 8);
        audio_buffer_.resize(total_frames_ * header.channels);

        if (header.bits == 16) {
            // Convert 16-bit to float
            std::vector<int16_t> temp(total_frames_ * header.channels);
            file.read(reinterpret_cast<char*>(temp.data()), header.data_size);

            for (size_t i = 0; i < temp.size(); ++i) {
                audio_buffer_[i] = static_cast<float>(temp[i]) / 32768.0f;
            }
        } else if (header.bits == 24) {
            // Convert 24-bit to float
            std::vector<uint8_t> temp(header.data_size);
            file.read(reinterpret_cast<char*>(temp.data()), header.data_size);

            for (size_t i = 0; i < total_frames_ * header.channels; ++i) {
                int32_t sample = (temp[i*3] << 8) | (temp[i*3+1] << 16) | (temp[i*3+2] << 24);
                sample >>= 8;  // Sign extend
                audio_buffer_[i] = static_cast<float>(sample) / 8388608.0f;
            }
        } else if (header.bits == 32) {
            // Convert 32-bit to float
            file.read(reinterpret_cast<char*>(audio_buffer_.data()), header.data_size);
        } else {
            std::cerr << "Error: Unsupported bit depth " << header.bits << "\n";
            return false;
        }

        current_pos_ = 0;
        std::cout << "Loaded: " << filename << "\n";
        std::cout << "  Format: " << file_format_.sample_rate << "Hz, "
                  << file_format_.channels << " channels, "
                  << file_format_.bits_per_sample << " bits\n";
        std::cout << "  Duration: " << static_cast<double>(total_frames_) / file_format_.sample_rate
                  << " seconds\n";

        return true;
    }

    bool initialize_audio() {
        // Create resampler based on mode
        if (resampler_mode_ == "adaptive") {
            resampler_ = AdaptiveSampleRateConverterFactory::create_for_use_case("music");
            std::cout << "Using adaptive resampler\n";
        } else {
            // Use fixed quality resampler
            ResampleQuality quality = EnhancedSampleRateConverterFactory::parse_quality(resampler_mode_);
            resampler_ = std::make_unique<AdaptiveSampleRateConverter>(
                quality, quality, false, 100.0);
            std::cout << "Using fixed quality resampler: "
                      << EnhancedSampleRateConverter::get_quality_name(quality) << "\n";
        }

        // Initialize resampler
        if (!resampler_->initialize(file_format_.sample_rate,
                                   output_format_.sample_rate,
                                   file_format_.channels)) {
            std::cerr << "Error: Failed to initialize resampler\n";
            return false;
        }

        // Initialize audio output
#ifdef __linux__
        return initialize_alsa();
#else
        std::cout << "Audio output initialized (stub implementation)\n";
        return true;
#endif
    }

    bool play() {
        if (state_ == PLAYING) {
            return true;
        }

        if (!resampler_) {
            std::cerr << "Error: Resampler not initialized\n";
            return false;
        }

        state_ = PLAYING;
        g_stop_flag = false;

        std::cout << "Starting playback at " << output_format_.sample_rate << "Hz\n";

        // Calculate resampled buffer size
        int resampled_frames = static_cast<int>(
            total_frames_ * static_cast<double>(output_format_.sample_rate) /
            file_format_.sample_rate);
        std::vector<float> resampled_buffer(resampled_frames * output_format_.channels);

        // Perform sample rate conversion
        std::cout << "Resampling audio (" << file_format_.sample_rate
                  << "Hz → " << output_format_.sample_rate << "Hz)... ";
        auto start = std::chrono::high_resolution_clock::now();

        int converted_frames = resampler_->convert(
            audio_buffer_.data(), total_frames_,
            resampled_buffer.data(), resampled_frames);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Done in " << duration.count() << "ms\n";
        std::cout << "Output frames: " << converted_frames << "\n";

        if (use_adaptive_ && resampler_mode_ == "adaptive") {
            auto stats = resampler_->get_performance_stats();
            std::cout << "Final quality used: "
                      << EnhancedSampleRateConverter::get_quality_name(stats.current_quality)
                      << "\n";
        }

        // Play the resampled audio
#ifdef __linux__
        return play_alsa(resampled_buffer.data(), converted_frames);
#else
        // Simulate playback
        std::cout << "Simulating playback...\n";
        std::this_thread::sleep_for(std::chrono::seconds(
            static_cast<int>(converted_frames / output_format_.sample_rate)));
        state_ = STOPPED;
        return true;
#endif
    }

    void pause() {
        if (state_ == PLAYING) {
            state_ = PAUSED;
            std::cout << "Playback paused\n";
        }
    }

    void stop() {
        state_ = STOPPED;
        current_pos_ = 0;
        std::cout << "Playback stopped\n";
    }

    void set_resampler_mode(const std::string& mode) {
        resampler_mode_ = mode;
        use_adaptive_ = (mode == "adaptive");
        std::cout << "Resampler mode set to: " << mode << "\n";
    }

    void set_quality(ResampleQuality quality) {
        current_quality_ = quality;
        if (resampler_ && resampler_mode_ != "adaptive") {
            resampler_ = std::make_unique<AdaptiveSampleRateConverter>(
                quality, quality, false, 100.0);
            resampler_->initialize(file_format_.sample_rate,
                                  output_format_.sample_rate,
                                  file_format_.channels);
        }
        std::cout << "Quality set to: "
                  << EnhancedSampleRateConverter::get_quality_name(quality) << "\n";
    }

    void show_info() {
        std::cout << "\n=== Player Information ===\n";
        std::cout << "State: " << (state_ == PLAYING ? "Playing" :
                               state_ == PAUSED ? "Paused" : "Stopped") << "\n";
        std::cout << "Position: " << current_pos_ << " / " << total_frames_ << " frames\n";
        std::cout << "File format: " << file_format_.sample_rate << "Hz, "
                  << file_format_.channels << " ch, " << file_format_.bits_per_sample << " bit\n";
        std::cout << "Output format: " << output_format_.sample_rate << "Hz, "
                  << output_format_.channels << " ch\n";
        std::cout << "Resampler mode: " << resampler_mode_ << "\n";
        if (resampler_mode_ != "adaptive") {
            std::cout << "Quality: " << EnhancedSampleRateConverter::get_quality_name(current_quality_) << "\n";
        }
    }

private:
#ifdef __linux__
    bool initialize_alsa() {
        int err = snd_pcm_open(&pcm_handle_, "default", SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            std::cerr << "Error: Cannot open PCM device: " << snd_strerror(err) << "\n";
            return false;
        }

        snd_pcm_hw_params_t* hw_params;
        err = snd_pcm_hw_params_malloc(&hw_params);
        if (err < 0) return false;

        err = snd_pcm_hw_params_any(pcm_handle_, hw_params);
        if (err < 0) return false;

        err = snd_pcm_hw_params_set_access(pcm_handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) return false;

        err = snd_pcm_hw_params_set_format(pcm_handle_, hw_params, SND_PCM_FORMAT_FLOAT_LE);
        if (err < 0) {
            // Fall back to 16-bit
            err = snd_pcm_hw_params_set_format(pcm_handle_, hw_params, SND_PCM_FORMAT_S16_LE);
        }

        unsigned int rate = output_format_.sample_rate;
        err = snd_pcm_hw_params_set_rate_near(pcm_handle_, hw_params, &rate, 0);
        if (err < 0) return false;

        err = snd_pcm_hw_params_set_channels(pcm_handle_, hw_params, output_format_.channels);
        if (err < 0) return false;

        err = snd_pcm_hw_params(pcm_handle_, hw_params);
        if (err < 0) return false;

        snd_pcm_hw_params_free(hw_params);

        std::cout << "ALSA initialized at " << rate << "Hz\n";
        return true;
    }

    bool play_alsa(const float* buffer, int frames) {
        int err = snd_pcm_prepare(pcm_handle_);
        if (err < 0) {
            std::cerr << "Error: Cannot prepare PCM: " << snd_strerror(err) << "\n";
            return false;
        }

        snd_pcm_sframes_t frames_written = snd_pcm_writei(
            pcm_handle_, buffer, frames);

        if (frames_written < 0) {
            frames_written = snd_pcm_recover(pcm_handle_, frames_written, 0);
        }

        if (frames_written < 0) {
            std::cerr << "Error: Cannot write to PCM: " << snd_strerror(frames_written) << "\n";
            return false;
        }

        state_ = STOPPED;
        return true;
    }
#endif
};

void print_help() {
    std::cout << "\nEnhanced Music Player Commands:\n";
    std::cout << "  play/p   - Start playback\n";
    std::cout << "  pause    - Pause playback\n";
    std::cout << "  stop/s   - Stop playback\n";
    std::cout << "  info/i   - Show player information\n";
    std::cout << "  quality <fast|good|high|best> - Set quality level\n";
    std::cout << "  mode <adaptive|fixed> - Set resampler mode\n";
    std::cout << "  help/h   - Show this help\n";
    std::cout << "  quit/q   - Exit player\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <wav_file> [options]\n";
        std::cerr << "\nOptions:\n";
        std::cerr << "  --mode <adaptive|fixed>  Set resampler mode (default: adaptive)\n";
        std::cerr << "  --quality <fast|good|high|best>  Set fixed quality (default: good)\n";
        return 1;
    }

    // Set up signal handler
    signal(SIGINT, signal_handler);

    EnhancedMusicPlayer player;
    std::string filename = argv[1];

    // Parse command line options
    for (int i = 2; i < argc; i++) {
        if (std::string(argv[i]) == "--mode" && i + 1 < argc) {
            player.set_resampler_mode(argv[++i]);
        } else if (std::string(argv[i]) == "--quality" && i + 1 < argc) {
            ResampleQuality quality = EnhancedSampleRateConverterFactory::parse_quality(argv[++i]);
            player.set_quality(quality);
            player.set_resampler_mode("fixed");
        }
    }

    // Load and play
    if (!player.load_wav(filename)) {
        return 1;
    }

    if (!player.initialize_audio()) {
        return 1;
    }

    print_help();

    // Interactive loop
    std::string command;
    while (!g_stop_flag) {
        std::cout << "\n> ";
        std::getline(std::cin, command);

        if (command.empty() || command == "play" || command == "p") {
            player.play();
        } else if (command == "pause") {
            player.pause();
        } else if (command == "stop" || command == "s") {
            player.stop();
        } else if (command == "info" || command == "i") {
            player.show_info();
        } else if (command.find("quality") == 0) {
            size_t space = command.find(' ');
            if (space != std::string::npos) {
                std::string quality_str = command.substr(space + 1);
                ResampleQuality quality = EnhancedSampleRateConverterFactory::parse_quality(quality_str);
                player.set_quality(quality);
            }
        } else if (command.find("mode") == 0) {
            size_t space = command.find(' ');
            if (space != std::string::npos) {
                std::string mode = command.substr(space + 1);
                player.set_resampler_mode(mode);
            }
        } else if (command == "help" || command == "h") {
            print_help();
        } else if (command == "quit" || command == "q") {
            break;
        } else if (!command.empty()) {
            std::cout << "Unknown command: " << command << "\n";
            print_help();
        }
    }

    std::cout << "\nGoodbye!\n";
    return 0;
}