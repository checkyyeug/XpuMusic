#pragma once

#include "mp_types.h"
#include <functional>

namespace mp {

// Event type IDs
using EventID = uint64_t;

// Predefined event IDs
constexpr EventID EVENT_PLAYBACK_STARTED = hash_string("mp.event.playback_started");
constexpr EventID EVENT_PLAYBACK_STOPPED = hash_string("mp.event.playback_stopped");
constexpr EventID EVENT_PLAYBACK_PAUSED = hash_string("mp.event.playback_paused");
constexpr EventID EVENT_PLAYBACK_RESUMED = hash_string("mp.event.playback_resumed");
constexpr EventID EVENT_TRACK_CHANGED = hash_string("mp.event.track_changed");
constexpr EventID EVENT_SEEK = hash_string("mp.event.seek");
constexpr EventID EVENT_VOLUME_CHANGED = hash_string("mp.event.volume_changed");
constexpr EventID EVENT_CONFIG_CHANGED = hash_string("mp.event.config_changed");
constexpr EventID EVENT_LIBRARY_UPDATED = hash_string("mp.event.library_updated");
constexpr EventID EVENT_PLAYLIST_CHANGED = hash_string("mp.event.playlist_changed");
constexpr EventID EVENT_METADATA_LOADED = hash_string("mp.event.metadata_loaded");

// Event data structure
struct Event {
    EventID id;                 // Event identifier
    void* data;                 // Event-specific data
    size_t data_size;           // Size of data
    uint64_t timestamp;         // Event timestamp (milliseconds since epoch)
    
    Event() : id(0), data(nullptr), data_size(0), timestamp(0) {}
    Event(EventID evt_id, void* evt_data = nullptr, size_t size = 0)
        : id(evt_id), data(evt_data), data_size(size), timestamp(0) {}
};

// Event listener callback
using EventCallback = std::function<void(const Event&)>;

// Event subscription handle
using SubscriptionHandle = uint64_t;

// Event bus interface
class IEventBus {
public:
    virtual ~IEventBus() = default;
    
    // Subscribe to an event
    virtual SubscriptionHandle subscribe(EventID event_id, EventCallback callback) = 0;
    
    // Unsubscribe from an event
    virtual Result unsubscribe(SubscriptionHandle handle) = 0;
    
    // Publish an event (asynchronous)
    virtual Result publish(const Event& event) = 0;
    
    // Publish an event (synchronous - blocks until all handlers complete)
    virtual Result publish_sync(const Event& event) = 0;
};

} // namespace mp
