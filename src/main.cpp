#include "core_engine.h"
#include "audio_output_factory.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global running flag for signal handling
static std::atomic<bool> g_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived shutdown signal, exiting..." << std::endl;
        g_running = false;
    }
}

void show_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] [audio_file]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help, -h        Show this help message" << std::endl;
    std::cout << "  --version, -v     Show version information" << std::endl;
    std::cout << "  --test            Run audio test (sine wave)" << std::endl;
    std::cout << "  --list-plugins    List available plugins" << std::endl;
    std::cout << "  --list-devices    List available audio devices" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " music.wav              # Play audio file" << std::endl;
    std::cout << "  " << program_name << " --test                  # Test audio output" << std::endl;
    std::cout << "  " << program_name << " --list-plugins          # Show loaded plugins" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "   Professional Music Player v0.2.0" << std::endl;
    std::cout << "   Microkernel Architecture" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Parse command line arguments
    if (argc < 2) {
        show_usage(argv[0]);
        return 1;
    }

    std::string command = argv[1];
    if (command == "--help" || command == "-h") {
        show_usage(argv[0]);
        return 0;
    }

    if (command == "--version" || command == "-v") {
        std::cout << "Professional Music Player v0.2.0" << std::endl;
        std::cout << "Architecture: Microkernel with Plugin System" << std::endl;
        std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
        return 0;
    }

    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Create and initialize core engine
    mp::core::CoreEngine engine;

    std::cout << "Initializing Core Engine..." << std::endl;
    mp::Result result = engine.initialize();
    if (result != mp::Result::Success) {
        std::cerr << "Failed to initialize core engine: " << static_cast<int>(result) << std::endl;
        return 1;
    }

    std::cout << "✓ Core engine initialized successfully" << std::endl;

    // Determine plugin directory
    std::string plugin_dir;
    if (argc > 2 && command != "--test") {
        plugin_dir = argv[2];
    } else {
        // Try multiple plugin directories in order
        namespace fs = std::filesystem;

        // Check common plugin locations
        std::vector<std::string> plugin_paths = {
            "./lib",              // Standard lib directory
            "./plugins",          // Plugins directory
            "./bin/Release",      // Windows build output directory
            "./bin",              // Unix build output directory
            "../lib",             // Parent lib directory
            "../plugins"          // Parent plugins directory
        };

        plugin_dir = "./lib";  // default fallback
        for (const auto& path : plugin_paths) {
            if (fs::exists(path) && fs::is_directory(path)) {
                // Check if there are any .dll/.so files in this directory
                bool has_plugins = false;
                for (const auto& entry : fs::directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == ".dll" || ext == ".so") {
                            has_plugins = true;
                            break;
                        }
                    }
                }

                if (has_plugins) {
                    plugin_dir = path;
                    break;
                }
            }
        }
    }

    // Load plugins
    std::cout << std::endl;
    std::cout << "Loading plugins from: " << plugin_dir << std::endl;
    result = engine.load_plugins(plugin_dir.c_str());
    if (result != mp::Result::Success) {
        std::cout << "⚠️  Plugin loading failed or no plugins found" << std::endl;
    }

    // Handle commands
    if (command == "--test") {
        std::cout << std::endl;
        std::cout << "Running audio test (2 second 440Hz tone)..." << std::endl;

        // Test audio output
        mp::IAudioOutput* audio_output = mp::platform::create_platform_audio_output();
        if (!audio_output) {
            std::cerr << "Failed to create audio output" << std::endl;
            return 1;
        }

        // Enumerate devices
        const mp::AudioDeviceInfo* devices = nullptr;
        size_t device_count = 0;
        result = audio_output->enumerate_devices(&devices, &device_count);

        if (result == mp::Result::Success && device_count > 0) {
            std::cout << "Found " << device_count << " audio device(s):" << std::endl;
            for (size_t i = 0; i < device_count; ++i) {
                std::cout << "  " << (i+1) << ". " << devices[i].name
                          << " (" << devices[i].max_channels << " channels, "
                          << devices[i].default_sample_rate << " Hz)" << std::endl;
            }
            std::cout << std::endl;

            // Test audio playback with a simple sine wave (440 Hz A note)
            struct AudioContext {
                float phase = 0.0f;
                float frequency = 440.0f; // A note
                float sample_rate = 44100.0f;
            };

            AudioContext audio_ctx;

            auto audio_callback = [](void* buffer, size_t frames, void* user_data) {
                AudioContext* ctx = static_cast<AudioContext*>(user_data);
                float* output = static_cast<float*>(buffer);

                float phase_increment = 2.0f * static_cast<float>(M_PI) * ctx->frequency / ctx->sample_rate;

                for (size_t i = 0; i < frames; ++i) {
                    float sample = 0.3f * std::sin(ctx->phase); // 30% volume
                    output[i * 2] = sample;     // Left channel
                    output[i * 2 + 1] = sample; // Right channel

                    ctx->phase += phase_increment;
                    if (ctx->phase > 2.0f * static_cast<float>(M_PI)) {
                        ctx->phase -= 2.0f * static_cast<float>(M_PI);
                    }
                }
            };

            mp::AudioOutputConfig audio_config = {};
            audio_config.device_id = nullptr; // Use default device
            audio_config.sample_rate = 44100;
            audio_config.channels = 2;
            audio_config.format = mp::SampleFormat::Float32;
            audio_config.buffer_frames = 1024;
            audio_config.callback = audio_callback;
            audio_config.user_data = &audio_ctx;

            audio_ctx.sample_rate = static_cast<float>(audio_config.sample_rate);

            result = audio_output->open(audio_config);
            if (result == mp::Result::Success) {
                std::cout << "✓ Audio device opened successfully" << std::endl;
                std::cout << "  Latency: " << audio_output->get_latency() << " ms" << std::endl;

                result = audio_output->start();
                if (result == mp::Result::Success) {
                    std::cout << "✓ Playback started..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(2));

                    audio_output->stop();
                    std::cout << "✓ Playback stopped" << std::endl;
                } else {
                    std::cout << "❌ Failed to start playback" << std::endl;
                }

                audio_output->close();
            } else {
                std::cout << "❌ Failed to open audio device" << std::endl;
            }
        } else {
            std::cout << "❌ No audio devices found" << std::endl;
        }

        delete audio_output;

    } else if (command == "--list-plugins") {
        std::cout << std::endl;
        std::cout << "Available plugins:" << std::endl;

        auto* plugin_host = engine.get_plugin_host();
        if (plugin_host) {
            // This would require extending the plugin host to list plugins
            std::cout << "  Plugin listing not yet implemented" << std::endl;
        }

    } else if (command == "--list-devices") {
        std::cout << std::endl;
        std::cout << "Audio devices:" << std::endl;

        mp::IAudioOutput* audio_output = mp::platform::create_platform_audio_output();
        if (audio_output) {
            const mp::AudioDeviceInfo* devices = nullptr;
            size_t device_count = 0;
            result = audio_output->enumerate_devices(&devices, &device_count);

            if (result == mp::Result::Success && device_count > 0) {
                for (size_t i = 0; i < device_count; ++i) {
                    std::cout << "  " << (i+1) << ". " << devices[i].name
                              << (devices[i].is_default ? " [DEFAULT]" : "")
                              << std::endl;
                    std::cout << "      Channels: " << devices[i].max_channels
                              << ", Sample Rate: " << devices[i].default_sample_rate << " Hz" << std::endl;
                }
            } else {
                std::cout << "  No audio devices found" << std::endl;
            }

            delete audio_output;
        }

    } else {
        // Play audio file using the proper architecture
        std::string file_path = command;

        // Check if file exists
        namespace fs = std::filesystem;
        if (!fs::exists(file_path)) {
            std::cerr << "Error: File not found: " << file_path << std::endl;
            return 1;
        }

        std::cout << std::endl;
        std::cout << "Playing: " << file_path << std::endl;

        // Use the core engine to play the file
        result = engine.play_file(file_path);
        if (result == mp::Result::Success) {
            std::cout << "✓ Playback started successfully" << std::endl;

            // Wait for playback to complete or user interrupt
            int wait_count = 0;
            while (g_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                wait_count++;

                // Check if playback is still active
                auto* playback_engine = engine.get_playback_engine();
                if (playback_engine) {
                    auto state = playback_engine->get_state();
                    if (wait_count % 50 == 0) {  // Every 5 seconds
                        std::cout << "Playback state: " << static_cast<int>(state) <<
                                     " (Waiting for 5 seconds..." << std::endl;
                    }
                    if (state == mp::core::PlaybackState::Stopped) {
                        std::cout << "Playback stopped naturally" << std::endl;
                        break;
                    }
                } else {
                    std::cout << "Playback engine is null!" << std::endl;
                    break;
                }

                // Safety timeout after 10 seconds
                if (wait_count > 100) {
                    std::cout << "Timeout: stopping playback after 10 seconds" << std::endl;
                    break;
                }
            }

            if (g_running) {
                std::cout << "Playback completed" << std::endl;
            } else {
                // User interrupted, stop playback
                engine.stop_playback();
                std::cout << "Playback stopped by user" << std::endl;
            }
        } else {
            std::cerr << "❌ Failed to start playback: " << static_cast<int>(result) << std::endl;

            // Provide helpful error information
            switch (result) {
                case mp::Result::FileNotFound:
                    std::cerr << "  File could not be opened or read" << std::endl;
                    break;
                case mp::Result::InvalidFormat:
                    std::cerr << "  Audio format not supported" << std::endl;
                    break;
                case mp::Result::FileError:
                    std::cerr << "  Audio device error" << std::endl;
                    break;
                default:
                    std::cerr << "  Unknown error occurred" << std::endl;
                    break;
            }
            return 1;
        }
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Core engine shutdown complete" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}