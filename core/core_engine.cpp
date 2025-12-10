#include "core_engine.h"
#include "../platform/audio_output_factory.h"
#include "mp_decoder.h"
#include "mp_plugin.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>

namespace mp {
namespace core {

CoreEngine::CoreEngine()
    : initialized_(false) {
}

CoreEngine::~CoreEngine() {
    shutdown();
}

Result CoreEngine::initialize() {
    if (initialized_) {
        return Result::AlreadyInitialized;
    }
    
    std::cout << "Initializing Music Player Core Engine..." << std::endl;
    
    // Create service registry
    service_registry_ = std::make_unique<ServiceRegistry>();
    
    // Create event bus
    event_bus_ = std::make_unique<EventBus>();
    
    // Create config manager
    config_manager_ = std::make_unique<ConfigManager>();
    std::string config_path = "music-player.json"; // Default config path
    config_manager_->initialize(config_path);
    config_manager_->set_auto_save(true);
    
    // Create playlist manager
    playlist_manager_ = std::make_unique<PlaylistManager>();
    std::string config_dir = "."; // Use current directory for now
    playlist_manager_->initialize(config_dir.c_str());
    
    // Create visualization engine
    visualization_engine_ = std::make_unique<VisualizationEngine>();
    VisualizationConfig viz_config{};
    viz_config.waveform_width = 800;
    viz_config.waveform_time_span = 5.0f;
    viz_config.fft_size = 2048;
    viz_config.spectrum_bars = 30;
    viz_config.spectrum_min_freq = 20.0f;
    viz_config.spectrum_max_freq = 20000.0f;
    viz_config.spectrum_smoothing = 0.75f;
    viz_config.vu_peak_decay_rate = 10.0f;
    viz_config.vu_rms_window_ms = 100.0f;
    viz_config.update_rate_hz = 60;
    visualization_engine_->initialize(viz_config);
    
    // Create playback engine
    playback_engine_ = std::make_unique<PlaybackEngine>();

    // Create plugin host
    plugin_host_ = std::make_unique<PluginHost>(service_registry_.get());

    // Register core services
    service_registry_->register_service(SERVICE_EVENT_BUS, event_bus_.get());
    service_registry_->register_service(SERVICE_PLUGIN_HOST, plugin_host_.get());
    service_registry_->register_service(SERVICE_CONFIG_MANAGER, config_manager_.get());
    service_registry_->register_service(SERVICE_PLAYLIST_MANAGER, playlist_manager_.get());
    service_registry_->register_service(SERVICE_VISUALIZATION, visualization_engine_.get());
    service_registry_->register_service(SERVICE_PLAYBACK_ENGINE, playback_engine_.get());

    // Initialize playback engine
    // Create audio output for playback engine
    mp::IAudioOutput* audio_output = mp::platform::create_platform_audio_output();
    if (audio_output) {
        playback_engine_->initialize(audio_output);
        delete audio_output; // Playback engine takes ownership
    }

    // Initialize XpuMusic compatibility layer (if enabled)
#ifdef ENABLE_FOOBAR_COMPAT
    if (plugin_host_) {
        mp::compat::CompatConfig compat_config;
        compat_config.enable_plugin_compat = true;
        compat_config.enable_data_migration = true;
        compat_config.adapter_logging_level = 1;

        // Try to initialize compatibility layer through plugin host
        auto* compat_service = plugin_host_->get_service("foobar_compat");
        if (compat_service) {
            std::cout << "✓ XpuMusic compatibility service available" << std::endl;
        } else {
            std::cout << "⚠ XpuMusic compatibility service not loaded" << std::endl;
        }
    }
#endif
    
    // Start event bus
    event_bus_->start();
    
    initialized_ = true;
    
    std::cout << "Core Engine initialized successfully" << std::endl;
    
    return Result::Success;
}

void CoreEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    std::cout << "Shutting down Music Player Core Engine..." << std::endl;
    
    // Shutdown plugins first
    if (plugin_host_) {
        plugin_host_->shutdown_plugins();
    }
    
    // Stop event bus
    if (event_bus_) {
        event_bus_->stop();
    }
    
    // Shutdown config manager (saves if auto-save enabled)
    if (config_manager_) {
        config_manager_->shutdown();
    }
    
    // Shutdown playlist manager (saves all playlists)
    if (playlist_manager_) {
        playlist_manager_->shutdown();
    }
    
    // Shutdown visualization engine
    if (visualization_engine_) {
        visualization_engine_->shutdown();
    }
    
    // Shutdown playback engine
    if (playback_engine_) {
        playback_engine_->shutdown();
    }

    // Cleanup
    playback_engine_.reset();
    plugin_host_.reset();
    event_bus_.reset();
    visualization_engine_.reset();
    playlist_manager_.reset();
    config_manager_.reset();
    service_registry_.reset();

    initialized_ = false;

    std::cout << "Core Engine shutdown complete" << std::endl;
}

Result CoreEngine::load_plugins(const char* plugin_dir) {
    if (!initialized_) {
        return Result::NotInitialized;
    }

    std::cout << "Loading plugins from: " << plugin_dir << std::endl;

    Result result = plugin_host_->scan_directory(plugin_dir);
    if (result != Result::Success) {
        std::cerr << "Failed to scan plugin directory" << std::endl;
        return result;
    }

    result = plugin_host_->initialize_plugins();
    if (result != Result::Success) {
        std::cerr << "Failed to initialize plugins" << std::endl;
        return result;
    }

    // Playback engine will use plugins through the plugin host
    // No need for refresh_decoders as it uses the plugin host directly

    return Result::Success;
}

Result CoreEngine::play_file(const std::string& file_path) {
    if (!initialized_) {
        return Result::NotInitialized;
    }

    if (!playback_engine_) {
        return Result::Error;
    }

    if (!plugin_host_) {
        std::cerr << "Plugin host not available" << std::endl;
        return Result::Error;
    }

    std::cout << "Playing: " << file_path << std::endl;

    // Find suitable decoder for the file
    IDecoder* decoder = nullptr;

    // Get file extension
    std::filesystem::path file_path_obj(file_path);
    std::string extension = file_path_obj.extension().string();
    if (!extension.empty() && extension[0] == '.') {
        extension = extension.substr(1); // Remove the dot
    }

    // Convert to lowercase for comparison
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    std::cout << "File extension: " << extension << std::endl;

    // Search for decoder plugin that supports this file extension
    const auto& loaded_plugins = plugin_host_->get_loaded_plugins();
    for (const auto& loaded_plugin : loaded_plugins) {
        if (!loaded_plugin.plugin) {
            continue;
        }

        // Check if plugin has decoder capability
        auto capabilities = loaded_plugin.plugin->get_capabilities();
        if (!mp::has_capability(capabilities, mp::PluginCapability::Decoder)) {
            continue;
        }

        // Query decoder service from plugin
        auto* decoder_service = loaded_plugin.plugin->get_service(mp::hash_string("mp.decoder"));
        if (!decoder_service) {
            continue;
        }

        auto* candidate_decoder = static_cast<IDecoder*>(decoder_service);

        // Check if decoder supports this file extension
        const char** extensions = candidate_decoder->get_extensions();
        if (extensions) {
            for (int i = 0; extensions[i] != nullptr; ++i) {
                std::string supported_ext = extensions[i];
                std::transform(supported_ext.begin(), supported_ext.end(), supported_ext.begin(), ::tolower);

                if (supported_ext == extension) {
                    decoder = candidate_decoder;
                    std::cout << "Found decoder: " << loaded_plugin.info.name << std::endl;
                    break;
                }
            }
        }

        if (decoder) {
            break;
        }
    }

    if (!decoder) {
        std::cerr << "No decoder found for file extension: " << extension << std::endl;
        return Result::InvalidFormat;
    }

    // Load track into playback engine
    std::cout << "Loading track into playback engine..." << std::endl;
    Result result = playback_engine_->load_track(file_path, decoder);
    if (result != Result::Success) {
        std::cerr << "Failed to load track: " << static_cast<int>(result) << std::endl;
        return result;
    }
    std::cout << "✓ Track loaded successfully" << std::endl;

    // Start playback
    std::cout << "Starting playback..." << std::endl;
    result = playback_engine_->play();
    if (result != Result::Success) {
        std::cerr << "Failed to start playback: " << static_cast<int>(result) << std::endl;
        return result;
    }

    std::cout << "✓ Playback started successfully" << std::endl;

    // Add a small delay to let audio start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Audio should be playing now..." << std::endl;

    return Result::Success;
}

Result CoreEngine::stop_playback() {
    if (!initialized_) {
        return Result::NotInitialized;
    }

    if (!playback_engine_) {
        return Result::Error;
    }

    return playback_engine_->stop();
}

}} // namespace mp::core
