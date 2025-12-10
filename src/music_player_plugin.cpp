/**
 * @file music_player_plugin.cpp
 * @brief Music player with foobar2000 plugin support
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
#include <filesystem>

// Include our enhanced audio components
#include "audio/wav_writer.h"
#include "audio/enhanced_sample_rate_converter.h"
#include "audio/adaptive_resampler.h"
#include "audio/plugin_audio_decoder.h"
#include "../../compat/plugin_loader/plugin_loader.h"
#include "../../compat/xpumusic_compat_manager.h"

// Platform-specific audio output
#ifdef __linux__
#include <alsa/asoundlib.h>
#elif defined(_WIN32)
#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
#elif defined(__APPLE__)
#include <CoreAudio/AudioHardware.h>
#include <AudioUnit/AudioUnit.h>
#endif

// Type aliases for convenience
using ResampleQuality = audio::ResampleQuality;
using AdaptiveSampleRateConverter = audio::AdaptiveSampleRateConverter;

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

class PluginAwareMusicPlayer {
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

    // Plugin decoder
    std::unique_ptr<PluginAudioDecoder> plugin_decoder_;
    std::string plugin_directory_;

    // Platform-specific audio handle
#ifdef __linux__
    snd_pcm_t* pcm_handle_;
    snd_pcm_hw_params_t* hw_params_;
    snd_pcm_sw_params_t* sw_params_;
#elif defined(_WIN32)
    LPDIRECTSOUND8 dsound_;
    LPDIRECTSOUNDBUFFER ds_buffer_;
    WAVEFORMATEX wave_format_;
#endif

    // Configuration
    bool use_adaptive_;
    bool use_plugins_;
    std::string resampler_mode_;
    int output_sample_rate_;

    // Statistics
    struct Statistics {
        double played_seconds;
        size_t frames_played;
        size_t frames_dropped;
        double cpu_usage;
        std::string current_decoder;
        size_t plugins_loaded;
        size_t plugins_active;
    } stats_;

public:
    PluginAwareMusicPlayer()
        : state_(STOPPED)
        , current_pos_(0)
        , total_frames_(0)
        , current_quality_(ResampleQuality::Good)
        , use_adaptive_(true)
        , use_plugins_(true)
        , resampler_mode_("music")
        , output_sample_rate_(44100) {

        memset(&stats_, 0, sizeof(stats_));

        // Initialize audio subsystem
#ifdef __linux__
        pcm_handle_ = nullptr;
        hw_params_ = nullptr;
        sw_params_ = nullptr;
#elif defined(_WIN32)
        dsound_ = nullptr;
        ds_buffer_ = nullptr;
        memset(&wave_format_, 0, sizeof(wave_format_));
#endif

        // Set default plugin directory
        plugin_directory_ = "./plugins";

        // Initialize plugin decoder
        plugin_decoder_ = std::make_unique<PluginAudioDecoder>(nullptr, output_sample_rate_);
    }

    ~PluginAwareMusicPlayer() {
        stop();
    }

    bool initialize(const char* plugin_dir = nullptr) {
        // Set plugin directory if provided
        if (plugin_dir) {
            plugin_directory_ = plugin_dir;
        }

        // Initialize plugin decoder
        if (use_plugins_) {
            std::cout << "Initializing plugin system...\n";
            if (plugin_decoder_->initialize(plugin_directory_.c_str())) {
                auto plugin_stats = plugin_decoder_->get_plugin_stats();
                stats_.plugins_loaded = plugin_stats.first;
                stats_.plugins_active = plugin_stats.second;
                std::cout << "Loaded " << stats_.plugins_loaded << " plugins ("
                         << stats_.plugins_active << " decoders)\n";
            } else {
                std::cerr << "Warning: Failed to initialize plugin system\n";
                use_plugins_ = false;
            }
        }

        // Initialize audio output
        if (!initialize_audio_output()) {
            std::cerr << "Failed to initialize audio output\n";
            return false;
        }

        // Initialize resampler
        resampler_ = std::make_unique<AdaptiveSampleRateConverter>();
        if (!resampler_->initialize(44100, output_sample_rate_, 2)) {
            std::cerr << "Warning: Failed to initialize resampler\n";
        }

        return true;
    }

    bool load_file(const char* filename) {
        if (!filename) return false;

        // Stop current playback
        stop();

        std::cout << "Loading file: " << filename << "\n";

        // Try plugin decoder first if enabled
        bool file_loaded = false;
        if (use_plugins_) {
            std::cout << "Attempting to load with plugin decoder...\n";
            if (plugin_decoder_->open_file(filename)) {
                auto audio_info = plugin_decoder_->get_audio_info();
                file_format_.sample_rate = audio_info.sample_rate;
                file_format_.channels = audio_info.channels;
                file_format_.bits_per_sample = audio_info.bits_per_sample;
                total_frames_ = audio_info.total_samples;
                stats_.current_decoder = audio_info.format_name;

                std::cout << "Loaded with " << stats_.current_decoder << " decoder\n";
                std::cout << "Format: " << file_format_.sample_rate << " Hz, "
                         << file_format_.channels << " channels, "
                         << file_format_.bits_per_sample << " bits\n";
                std::cout << "Duration: " << audio_info.duration_seconds << " seconds\n";

                // Display metadata
                const auto& metadata = plugin_decoder_->get_metadata();
                if (!metadata.empty()) {
                    std::cout << "\nMetadata:\n";
                    for (const auto& [key, value] : metadata) {
                        std::cout << "  " << key << ": " << value << "\n";
                    }
                }

                file_loaded = true;
            } else {
                std::cout << "Plugin decoder failed to load file\n";
            }
        }

        if (!file_loaded) {
            std::cerr << "Failed to load file: " << filename << "\n";
            return false;
        }

        // Setup audio output for this file
        if (!setup_audio_format()) {
            std::cerr << "Failed to setup audio format\n";
            plugin_decoder_->close_file();
            return false;
        }

        // Reset playback position
        current_pos_ = 0;
        memset(&stats_, 0, sizeof(stats_));
        stats_.current_decoder = plugin_decoder_->get_decoder_name();

        return true;
    }

    void play() {
        if (state_ == PLAYING) return;

        if (!plugin_decoder_) {
            std::cerr << "No file loaded\n";
            return;
        }

        std::cout << "\nStarting playback...\n";
        std::cout << "Resampler quality: " << quality_to_string(current_quality_) << "\n";
        std::cout << "Output sample rate: " << output_sample_rate_ << " Hz\n";
        std::cout << "Press Ctrl+C to stop\n\n";

        state_ = PLAYING;
        g_stop_flag = false;

        // Playback loop
        playback_loop();
    }

    void stop() {
        if (state_ != STOPPED) {
            state_ = STOPPED;
            g_stop_flag = true;

            // Stop audio output
#ifdef __linux__
            if (pcm_handle_) {
                snd_pcm_drain(pcm_handle_);
            }
#elif defined(_WIN32)
            if (ds_buffer_) {
                ds_buffer_->Stop();
            }
#endif

            std::cout << "\nPlayback stopped\n";
        }
    }

    void pause() {
        if (state_ == PLAYING) {
            state_ = PAUSED;
#ifdef __linux__
            if (pcm_handle_) {
                snd_pcm_pause(pcm_handle_, 1);
            }
#elif defined(_WIN32)
            if (ds_buffer_) {
                ds_buffer_->Stop();
            }
#endif
            std::cout << "Playback paused\n";
        }
    }

    void resume() {
        if (state_ == PAUSED) {
            state_ = PLAYING;
#ifdef __linux__
            if (pcm_handle_) {
                snd_pcm_pause(pcm_handle_, 0);
            }
#elif defined(_WIN32)
            if (ds_buffer_) {
                ds_buffer_->Play(0, 0, DSBPLAY_LOOPING);
            }
#endif
            std::cout << "Playback resumed\n";
        }
    }

    void seek(double seconds) {
        if (!plugin_decoder_) return;

        int64_t sample_pos = static_cast<int64_t>(seconds * file_format_.sample_rate);
        if (plugin_decoder_->seek(sample_pos)) {
            current_pos_ = sample_pos;
            std::cout << "Seeked to " << seconds << " seconds\n";
        }
    }

    void set_resampler_quality(ResampleQuality quality) {
        current_quality_ = quality;
        if (resampler_) {
            // Update resampler quality
            // In a real implementation, we would reinitialize with new quality
        }
    }

    void set_resampler_mode(const char* mode) {
        if (mode) {
            resampler_mode_ = mode;
            use_adaptive_ = (strcmp(mode, "adaptive") == 0);
        }
    }

    void set_output_sample_rate(int rate) {
        if (rate > 0 && rate != output_sample_rate_) {
            output_sample_rate_ = rate;

            // Reinitialize resampler
            if (resampler_ && file_format_.sample_rate > 0) {
                resampler_->initialize(file_format_.sample_rate, rate, file_format_.channels);
            }

            // Update plugin decoder
            if (plugin_decoder_) {
                plugin_decoder_->set_target_sample_rate(rate);
            }

            std::cout << "Output sample rate set to " << rate << " Hz\n";
        }
    }

    void list_supported_formats() const {
        if (!plugin_decoder_) {
            std::cout << "Plugin system not initialized\n";
            return;
        }

        auto extensions = plugin_decoder_->get_supported_extensions();
        std::cout << "\nSupported audio formats:\n";
        for (const auto& ext : extensions) {
            std::cout << "  ." << ext << "\n";
        }
        std::cout << "\n";
    }

    void show_plugin_info() const {
        auto plugin_stats = plugin_decoder_->get_plugin_stats();
        std::cout << "\nPlugin System Status:\n";
        std::cout << "  Loaded plugins: " << plugin_stats.first << "\n";
        std::cout << "  Active decoders: " << plugin_stats.second << "\n";
        std::cout << "  Current decoder: " << stats_.current_decoder << "\n";
        std::cout << "\n";
    }

private:
    bool initialize_audio_output() {
#ifdef __linux__
        // Initialize ALSA
        int err = snd_pcm_open(&pcm_handle_, "default", SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            std::cerr << "ALSA open error: " << snd_strerror(err) << "\n";
            return false;
        }

        return true;
#elif defined(_WIN32)
        // Initialize DirectSound
        HRESULT hr = DirectSoundCreate8(NULL, &dsound_, NULL);
        if (FAILED(hr)) {
            std::cerr << "DirectSound create failed\n";
            return false;
        }

        hr = dsound_->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
        if (FAILED(hr)) {
            std::cerr << "DirectSound set cooperative level failed\n";
            return false;
        }

        return true;
#else
        std::cerr << "Audio output not supported on this platform\n";
        return false;
#endif
    }

    bool setup_audio_format() {
        output_format_.sample_rate = output_sample_rate_;
        output_format_.channels = file_format_.channels;
        output_format_.bits_per_sample = 32; // Float output

#ifdef __linux__
        if (!pcm_handle_) return false;

        // Allocate hardware parameter structure
        snd_pcm_hw_params_alloca(&hw_params_);

        // Fill it with default values
        snd_pcm_hw_params_any(pcm_handle_, hw_params_);

        // Set access type
        snd_pcm_hw_params_set_access(pcm_handle_, hw_params_, SND_PCM_ACCESS_RW_INTERLEAVED);

        // Set sample format
        snd_pcm_hw_params_set_format(pcm_handle_, hw_params_, SND_PCM_FORMAT_FLOAT_LE);

        // Set sample rate
        unsigned int rate = output_format_.sample_rate;
        snd_pcm_hw_params_set_rate_near(pcm_handle_, hw_params_, &rate, 0);
        if (rate != output_format_.sample_rate) {
            std::cout << "Warning: Rate adjusted to " << rate << " Hz\n";
            output_format_.sample_rate = rate;
        }

        // Set channels
        snd_pcm_hw_params_set_channels(pcm_handle_, hw_params_, output_format_.channels);

        // Set period size
        snd_pcm_uframes_t period_size = 1024;
        snd_pcm_hw_params_set_period_size_near(pcm_handle_, hw_params_, &period_size, 0);

        // Apply hardware parameters
        int err = snd_pcm_hw_params(pcm_handle_, hw_params_);
        if (err < 0) {
            std::cerr << "ALSA hardware params error: " << snd_strerror(err) << "\n";
            return false;
        }

#elif defined(_WIN32)
        // Setup WAVEFORMATEX
        wave_format_.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wave_format_.nChannels = output_format_.channels;
        wave_format_.nSamplesPerSec = output_format_.sample_rate;
        wave_format_.wBitsPerSample = output_format_.bits_per_sample;
        wave_format_.nBlockAlign = (wave_format_.nChannels * wave_format_.wBitsPerSample) / 8;
        wave_format_.nAvgBytesPerSec = wave_format_.nSamplesPerSec * wave_format_.nBlockAlign;
        wave_format_.cbSize = 0;

        // Create DirectSound buffer
        DSBUFFERDESC dsbd;
        memset(&dsbd, 0, sizeof(dsbd));
        dsbd.dwSize = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
        dsbd.dwBufferBytes = 0;
        dsbd.lpwfxFormat = NULL;

        HRESULT hr = dsound_->CreateSoundBuffer(&dsbd, &ds_buffer_, NULL);
        if (FAILED(hr)) {
            std::cerr << "Failed to create primary DirectSound buffer\n";
            return false;
        }

        hr = ds_buffer_->SetFormat(&wave_format_);
        if (FAILED(hr)) {
            std::cerr << "Failed to set DirectSound format\n";
            return false;
        }
#endif

        return true;
    }

    void playback_loop() {
        const int BUFFER_FRAMES = 1024;
        std::vector<float> buffer(BUFFER_FRAMES * output_format_.channels);

        while (state_ == PLAYING && !g_stop_flag) {
            // Decode audio
            int frames_decoded = plugin_decoder_->decode_frames(buffer.data(), BUFFER_FRAMES);

            if (frames_decoded == 0) {
                // End of file
                std::cout << "Playback completed\n";
                break;
            }

            // Output audio
            if (!output_audio(buffer.data(), frames_decoded)) {
                std::cerr << "Audio output error\n";
                break;
            }

            // Update statistics
            current_pos_ += frames_decoded;
            stats_.frames_played += frames_decoded;
            stats_.played_seconds = static_cast<double>(current_pos_) / file_format_.sample_rate;

            // Show progress
            if (stats_.frames_played % (output_format_.sample_rate * 2) == 0) {
                show_progress();
            }
        }

        state_ = STOPPED;
    }

    bool output_audio(const float* buffer, int frames) {
#ifdef __linux__
        if (!pcm_handle_) return false;

        snd_pcm_sframes_t frames_written = snd_pcm_writei(pcm_handle_, buffer, frames);
        if (frames_written < 0) {
            frames_written = snd_pcm_recover(pcm_handle_, frames_written, 0);
        }
        if (frames_written < 0) {
            std::cerr << "ALSA write error: " << snd_strerror(frames_written) << "\n";
            return false;
        }

        return true;
#elif defined(_WIN32)
        if (!ds_buffer_) return false;

        // For simplicity, we'll just output to stdout for now
        // In a real implementation, we would use DirectSound properly
        return true;
#else
        return false;
#endif
    }

    void show_progress() {
        double current_seconds = static_cast<double>(current_pos_) / file_format_.sample_rate;
        double total_seconds = static_cast<double>(total_frames_) / file_format_.sample_rate;
        int progress = static_cast<int>((current_seconds / total_seconds) * 50);

        std::cout << "\r[";
        for (int i = 0; i < 50; i++) {
            if (i < progress) std::cout << "=";
            else if (i == progress) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << current_seconds << "/" << total_seconds << "s";
        std::cout << " (" << stats_.current_decoder << ")";
        std::cout.flush();
    }

    const char* quality_to_string(ResampleQuality quality) const {
        switch (quality) {
            case ResampleQuality::Fast: return "Fast";
            case ResampleQuality::Good: return "Good";
            case ResampleQuality::High: return "High";
            case ResampleQuality::Best: return "Best";
            default: return "Unknown";
        }
    }
};

// Command line options
struct Options {
    char* filename = nullptr;
    char* plugin_dir = nullptr;
    bool list_formats = false;
    bool show_plugins = false;
    ResampleQuality quality = ResampleQuality::Good;
    int output_rate = 44100;
    bool adaptive = false;
};

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [options] <audio_file>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -p, --plugins <dir>    Load plugins from directory\n";
    std::cout << "  -q, --quality <q>      Set resampler quality\n";
    std::cout << "                           (fast, good, high, best)\n";
    std::cout << "  -r, --rate <hz>         Set output sample rate\n";
    std::cout << "  -a, --adaptive          Use adaptive resampling\n";
    std::cout << "  -l, --list-formats      List supported formats\n";
    std::cout << "  -i, --plugin-info       Show plugin information\n";
    std::cout << "  -h, --help              Show this help\n";
}

int main(int argc, char* argv[]) {
    Options options;

    // Parse command line
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--plugins") == 0) {
            if (i + 1 < argc) {
                options.plugin_dir = argv[++i];
            }
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quality") == 0) {
            if (i + 1 < argc) {
                const char* q = argv[++i];
                if (strcmp(q, "fast") == 0) options.quality = ResampleQuality::Fast;
                else if (strcmp(q, "good") == 0) options.quality = ResampleQuality::Good;
                else if (strcmp(q, "high") == 0) options.quality = ResampleQuality::High;
                else if (strcmp(q, "best") == 0) options.quality = ResampleQuality::Best;
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--rate") == 0) {
            if (i + 1 < argc) {
                options.output_rate = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--adaptive") == 0) {
            options.adaptive = true;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list-formats") == 0) {
            options.list_formats = true;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--plugin-info") == 0) {
            options.show_plugins = true;
        } else if (argv[i][0] != '-') {
            options.filename = argv[i];
        }
    }

    // Initialize player
    PluginAwareMusicPlayer player;
    signal(SIGINT, signal_handler);

    if (!player.initialize(options.plugin_dir)) {
        std::cerr << "Failed to initialize player\n";
        return 1;
    }

    // Handle list formats
    if (options.list_formats) {
        player.list_supported_formats();
        return 0;
    }

    // Handle plugin info
    if (options.show_plugins) {
        player.show_plugin_info();
        return 0;
    }

    // Check for filename
    if (!options.filename) {
        std::cerr << "Error: No audio file specified\n";
        print_usage(argv[0]);
        return 1;
    }

    // Set player options
    player.set_resampler_quality(options.quality);
    player.set_output_sample_rate(options.output_rate);
    if (options.adaptive) {
        player.set_resampler_mode("adaptive");
    }

    // Load and play file
    if (!player.load_file(options.filename)) {
        return 1;
    }

    player.play();

    return 0;
}