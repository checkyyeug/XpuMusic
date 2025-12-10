#pragma once

#include "mp_types.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>

namespace mp {
namespace core {

// Track reference in a playlist
struct TrackReference {
    std::string file_path;          // Absolute path to audio file
    uint64_t metadata_hash;         // Hash of cached metadata
    uint64_t added_time;            // Timestamp when added (seconds since epoch)
    
    TrackReference() : metadata_hash(0), added_time(0) {}
    TrackReference(const std::string& path) 
        : file_path(path), metadata_hash(0), added_time(0) {}
};

// Playlist data structure
struct Playlist {
    uint64_t id;                    // Unique playlist identifier
    std::string name;               // User-assigned playlist name
    uint64_t creation_time;         // Creation timestamp
    uint64_t modification_time;     // Last modification timestamp
    std::vector<TrackReference> tracks;  // Ordered list of tracks
    
    Playlist() : id(0), creation_time(0), modification_time(0) {}
};

// Playlist search callback
using PlaylistSearchCallback = std::function<bool(const Playlist&)>;
using TrackSearchCallback = std::function<bool(const TrackReference&)>;

// Playlist manager - manages collections of tracks
class PlaylistManager {
public:
    PlaylistManager();
    ~PlaylistManager();
    
    // Initialize playlist manager
    Result initialize(const char* config_dir);
    
    // Shutdown playlist manager
    void shutdown();
    
    // Create a new playlist
    Result create_playlist(const char* name, uint64_t* playlist_id);
    
    // Delete a playlist
    Result delete_playlist(uint64_t playlist_id);
    
    // Rename a playlist
    Result rename_playlist(uint64_t playlist_id, const char* new_name);
    
    // Get playlist by ID
    const Playlist* get_playlist(uint64_t playlist_id) const;
    
    // Get all playlists
    const std::vector<Playlist>& get_all_playlists() const {
        return playlists_;
    }
    
    // Add track to playlist
    Result add_track(uint64_t playlist_id, const char* file_path);
    
    // Add multiple tracks to playlist
    Result add_tracks(uint64_t playlist_id, const char** file_paths, size_t count);
    
    // Remove track from playlist by index
    Result remove_track(uint64_t playlist_id, size_t index);
    
    // Remove tracks from playlist by path
    Result remove_tracks_by_path(uint64_t playlist_id, const char* file_path);
    
    // Clear all tracks from playlist
    Result clear_playlist(uint64_t playlist_id);
    
    // Move track within playlist
    Result move_track(uint64_t playlist_id, size_t from_index, size_t to_index);
    
    // Get track count in playlist
    size_t get_track_count(uint64_t playlist_id) const;
    
    // Search playlists
    std::vector<uint64_t> search_playlists(PlaylistSearchCallback callback) const;
    
    // Search tracks in a playlist
    std::vector<size_t> search_tracks(uint64_t playlist_id, TrackSearchCallback callback) const;
    
    // Save playlist to disk
    Result save_playlist(uint64_t playlist_id);
    
    // Save all playlists to disk
    Result save_all_playlists();
    
    // Load playlist from disk
    Result load_playlist(const char* file_path);
    
    // Load all playlists from config directory
    Result load_all_playlists();
    
    // Import M3U playlist
    Result import_m3u(const char* file_path, const char* playlist_name);
    
    // Export playlist to M3U
    Result export_m3u(uint64_t playlist_id, const char* file_path);
    
private:
    // Generate unique playlist ID
    uint64_t generate_playlist_id();
    
    // Get current timestamp (seconds since epoch)
    uint64_t get_current_timestamp() const;
    
    // Find playlist index by ID
    int find_playlist_index(uint64_t playlist_id) const;
    
    // Serialize playlist to JSON
    std::string serialize_playlist(const Playlist& playlist) const;
    
    // Deserialize playlist from JSON
    bool deserialize_playlist(const std::string& json, Playlist& playlist) const;
    
    // Get playlist file path
    std::string get_playlist_file_path(const Playlist& playlist) const;
    
    std::vector<Playlist> playlists_;
    std::string config_dir_;
    uint64_t next_playlist_id_;
    bool initialized_;
};

}} // namespace mp::core
