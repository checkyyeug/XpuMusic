#pragma once

#include "service_registry.h"
#include "event_bus.h"
#include "plugin_host.h"
#include "config_manager.h"
#include "playlist_manager.h"
#include "visualization_engine.h"
#include "playback_engine.h"

// Forward declarations for platform-specific types
namespace mp {
    class IAudioOutput;
    namespace platform {
        IAudioOutput* create_platform_audio_output();
    }
}
#include <memory>
#include <string>

namespace mp {
namespace core {

// Core engine - orchestrates all components
class CoreEngine {
public:
    CoreEngine();
    ~CoreEngine();
    
    // Initialize core engine
    Result initialize();
    
    // Shutdown core engine
    void shutdown();
    
    // Load plugins from directory
    Result load_plugins(const char* plugin_dir);
    
    // Get service registry
    ServiceRegistry* get_service_registry() {
        return service_registry_.get();
    }
    
    // Get event bus
    EventBus* get_event_bus() {
        return event_bus_.get();
    }
    
    // Get plugin host
    PluginHost* get_plugin_host() {
        return plugin_host_.get();
    }
    
    // Get config manager
    ConfigManager* get_config_manager() {
        return config_manager_.get();
    }
    
    // Get playlist manager
    PlaylistManager* get_playlist_manager() {
        return playlist_manager_.get();
    }
    
    // Get visualization engine
    VisualizationEngine* get_visualization_engine() {
        return visualization_engine_.get();
    }

    // Get playback engine
    PlaybackEngine* get_playback_engine() {
        return playback_engine_.get();
    }

    // Play a file using the plugin system
    Result play_file(const std::string& file_path);

    // Stop playback
    Result stop_playback();

    // Check if initialized
    bool is_initialized() const {
        return initialized_;
    }
    
private:
    std::unique_ptr<ServiceRegistry> service_registry_;
    std::unique_ptr<EventBus> event_bus_;
    std::unique_ptr<PluginHost> plugin_host_;
    std::unique_ptr<ConfigManager> config_manager_;
    std::unique_ptr<PlaylistManager> playlist_manager_;
    std::unique_ptr<VisualizationEngine> visualization_engine_;
    std::unique_ptr<PlaybackEngine> playback_engine_;

    bool initialized_;
};

}} // namespace mp::core
