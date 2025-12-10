#include "playlist_manager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <iostream>

namespace mp {
namespace core {

PlaylistManager::PlaylistManager()
    : next_playlist_id_(1)
    , initialized_(false) {
}

PlaylistManager::~PlaylistManager() {
    shutdown();
}

Result PlaylistManager::initialize(const char* config_dir) {
    if (initialized_) {
        return Result::AlreadyInitialized;
    }
    
    if (!config_dir) {
        return Result::InvalidParameter;
    }
    
    config_dir_ = config_dir;
    
    // Create playlists directory if it doesn't exist
    namespace fs = std::filesystem;
    std::string playlists_dir = config_dir_ + "/playlists";
    
    try {
        if (!fs::exists(playlists_dir)) {
            fs::create_directories(playlists_dir);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create playlists directory: " << e.what() << std::endl;
        return Result::Error;
    }
    
    // Load existing playlists
    load_all_playlists();
    
    initialized_ = true;
    return Result::Success;
}

void PlaylistManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Save all playlists before shutdown
    save_all_playlists();
    
    playlists_.clear();
    initialized_ = false;
}

Result PlaylistManager::create_playlist(const char* name, uint64_t* playlist_id) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!name || !playlist_id) {
        return Result::InvalidParameter;
    }
    
    // Check for duplicate name
    for (const auto& pl : playlists_) {
        if (pl.name == name) {
            return Result::AlreadyInitialized; // Name already exists
        }
    }
    
    Playlist playlist;
    playlist.id = generate_playlist_id();
    playlist.name = name;
    playlist.creation_time = get_current_timestamp();
    playlist.modification_time = playlist.creation_time;
    
    playlists_.push_back(playlist);
    *playlist_id = playlist.id;
    
    // Save immediately
    save_playlist(playlist.id);
    
    return Result::Success;
}

Result PlaylistManager::delete_playlist(uint64_t playlist_id) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    // Delete file from disk
    namespace fs = std::filesystem;
    std::string file_path = get_playlist_file_path(playlists_[index]);
    
    try {
        if (fs::exists(file_path)) {
            fs::remove(file_path);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to delete playlist file: " << e.what() << std::endl;
    }
    
    // Remove from memory
    playlists_.erase(playlists_.begin() + index);
    
    return Result::Success;
}

Result PlaylistManager::rename_playlist(uint64_t playlist_id, const char* new_name) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!new_name) {
        return Result::InvalidParameter;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    // Delete old file
    namespace fs = std::filesystem;
    std::string old_path = get_playlist_file_path(playlists_[index]);
    
    // Update name
    playlists_[index].name = new_name;
    playlists_[index].modification_time = get_current_timestamp();
    
    // Save with new name
    Result result = save_playlist(playlist_id);
    
    // Delete old file if it exists and is different
    try {
        std::string new_path = get_playlist_file_path(playlists_[index]);
        if (old_path != new_path && fs::exists(old_path)) {
            fs::remove(old_path);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to remove old playlist file: " << e.what() << std::endl;
    }
    
    return result;
}

const Playlist* PlaylistManager::get_playlist(uint64_t playlist_id) const {
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return nullptr;
    }
    return &playlists_[index];
}

Result PlaylistManager::add_track(uint64_t playlist_id, const char* file_path) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!file_path) {
        return Result::InvalidParameter;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    TrackReference track(file_path);
    track.added_time = get_current_timestamp();
    
    playlists_[index].tracks.push_back(track);
    playlists_[index].modification_time = track.added_time;
    
    return Result::Success;
}

Result PlaylistManager::add_tracks(uint64_t playlist_id, const char** file_paths, size_t count) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!file_paths || count == 0) {
        return Result::InvalidParameter;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    uint64_t current_time = get_current_timestamp();
    
    for (size_t i = 0; i < count; ++i) {
        if (file_paths[i]) {
            TrackReference track(file_paths[i]);
            track.added_time = current_time;
            playlists_[index].tracks.push_back(track);
        }
    }
    
    playlists_[index].modification_time = current_time;
    
    return Result::Success;
}

Result PlaylistManager::remove_track(uint64_t playlist_id, size_t index_to_remove) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    if (index_to_remove >= playlists_[index].tracks.size()) {
        return Result::InvalidParameter;
    }
    
    playlists_[index].tracks.erase(playlists_[index].tracks.begin() + index_to_remove);
    playlists_[index].modification_time = get_current_timestamp();
    
    return Result::Success;
}

Result PlaylistManager::remove_tracks_by_path(uint64_t playlist_id, const char* file_path) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!file_path) {
        return Result::InvalidParameter;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    auto& tracks = playlists_[index].tracks;
    auto new_end = std::remove_if(tracks.begin(), tracks.end(),
        [file_path](const TrackReference& track) {
            return track.file_path == file_path;
        });
    
    if (new_end != tracks.end()) {
        tracks.erase(new_end, tracks.end());
        playlists_[index].modification_time = get_current_timestamp();
    }
    
    return Result::Success;
}

Result PlaylistManager::clear_playlist(uint64_t playlist_id) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    playlists_[index].tracks.clear();
    playlists_[index].modification_time = get_current_timestamp();
    
    return Result::Success;
}

Result PlaylistManager::move_track(uint64_t playlist_id, size_t from_index, size_t to_index) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    auto& tracks = playlists_[index].tracks;
    
    if (from_index >= tracks.size() || to_index >= tracks.size()) {
        return Result::InvalidParameter;
    }
    
    if (from_index == to_index) {
        return Result::Success;
    }
    
    TrackReference track = tracks[from_index];
    tracks.erase(tracks.begin() + from_index);
    
    if (to_index > from_index) {
        to_index--;
    }
    
    tracks.insert(tracks.begin() + to_index, track);
    playlists_[index].modification_time = get_current_timestamp();
    
    return Result::Success;
}

size_t PlaylistManager::get_track_count(uint64_t playlist_id) const {
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return 0;
    }
    return playlists_[index].tracks.size();
}

std::vector<uint64_t> PlaylistManager::search_playlists(PlaylistSearchCallback callback) const {
    std::vector<uint64_t> results;
    
    for (const auto& playlist : playlists_) {
        if (callback(playlist)) {
            results.push_back(playlist.id);
        }
    }
    
    return results;
}

std::vector<size_t> PlaylistManager::search_tracks(uint64_t playlist_id, TrackSearchCallback callback) const {
    std::vector<size_t> results;
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return results;
    }
    
    const auto& tracks = playlists_[index].tracks;
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (callback(tracks[i])) {
            results.push_back(i);
        }
    }
    
    return results;
}

Result PlaylistManager::save_playlist(uint64_t playlist_id) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    std::string json = serialize_playlist(playlists_[index]);
    std::string file_path = get_playlist_file_path(playlists_[index]);
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to save playlist: " << file_path << std::endl;
        return Result::Error;
    }
    
    file << json;
    file.close();
    
    return Result::Success;
}

Result PlaylistManager::save_all_playlists() {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    for (const auto& playlist : playlists_) {
        save_playlist(playlist.id);
    }
    
    return Result::Success;
}

Result PlaylistManager::load_playlist(const char* file_path) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!file_path) {
        return Result::InvalidParameter;
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return Result::FileNotFound;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();
    
    Playlist playlist;
    if (!deserialize_playlist(json, playlist)) {
        return Result::InvalidFormat;
    }
    
    // Check if playlist with same ID already exists
    if (find_playlist_index(playlist.id) >= 0) {
        return Result::AlreadyInitialized;
    }
    
    playlists_.push_back(playlist);
    
    // Update next ID if needed
    if (playlist.id >= next_playlist_id_) {
        next_playlist_id_ = playlist.id + 1;
    }
    
    return Result::Success;
}

Result PlaylistManager::load_all_playlists() {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    namespace fs = std::filesystem;
    std::string playlists_dir = config_dir_ + "/playlists";
    
    try {
        if (!fs::exists(playlists_dir) || !fs::is_directory(playlists_dir)) {
            return Result::Success; // No playlists directory yet
        }
        
        for (const auto& entry : fs::directory_iterator(playlists_dir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".json") {
                    load_playlist(entry.path().string().c_str());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading playlists: " << e.what() << std::endl;
        return Result::Error;
    }
    
    return Result::Success;
}

Result PlaylistManager::import_m3u(const char* file_path, const char* playlist_name) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!file_path || !playlist_name) {
        return Result::InvalidParameter;
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return Result::FileNotFound;
    }
    
    uint64_t playlist_id;
    Result result = create_playlist(playlist_name, &playlist_id);
    if (result != Result::Success) {
        return result;
    }
    
    int playlist_index = find_playlist_index(playlist_id);
    if (playlist_index < 0) {
        return Result::Error;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Add track
        TrackReference track(line);
        track.added_time = get_current_timestamp();
        playlists_[playlist_index].tracks.push_back(track);
    }
    
    file.close();
    playlists_[playlist_index].modification_time = get_current_timestamp();
    save_playlist(playlist_id);
    
    return Result::Success;
}

Result PlaylistManager::export_m3u(uint64_t playlist_id, const char* file_path) {
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (!file_path) {
        return Result::InvalidParameter;
    }
    
    int index = find_playlist_index(playlist_id);
    if (index < 0) {
        return Result::InvalidParameter;
    }
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return Result::Error;
    }
    
    file << "#EXTM3U" << std::endl;
    
    for (const auto& track : playlists_[index].tracks) {
        file << track.file_path << std::endl;
    }
    
    file.close();
    
    return Result::Success;
}

uint64_t PlaylistManager::generate_playlist_id() {
    return next_playlist_id_++;
}

uint64_t PlaylistManager::get_current_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

int PlaylistManager::find_playlist_index(uint64_t playlist_id) const {
    for (size_t i = 0; i < playlists_.size(); ++i) {
        if (playlists_[i].id == playlist_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string PlaylistManager::serialize_playlist(const Playlist& playlist) const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"id\": " << playlist.id << ",\n";
    json << "  \"name\": \"" << playlist.name << "\",\n";
    json << "  \"creation_time\": " << playlist.creation_time << ",\n";
    json << "  \"modification_time\": " << playlist.modification_time << ",\n";
    json << "  \"tracks\": [\n";
    
    for (size_t i = 0; i < playlist.tracks.size(); ++i) {
        const auto& track = playlist.tracks[i];
        json << "    {\n";
        json << "      \"file_path\": \"" << track.file_path << "\",\n";
        json << "      \"metadata_hash\": " << track.metadata_hash << ",\n";
        json << "      \"added_time\": " << track.added_time << "\n";
        json << "    }";
        
        if (i < playlist.tracks.size() - 1) {
            json << ",";
        }
        json << "\n";
    }
    
    json << "  ]\n";
    json << "}\n";
    
    return json.str();
}

bool PlaylistManager::deserialize_playlist(const std::string& json, Playlist& playlist) const {
    // Simple JSON parser for playlist format
    // This is a basic implementation - for production use, consider a proper JSON library
    
    try {
        size_t pos = 0;
        
        // Parse ID
        pos = json.find("\"id\":", pos);
        if (pos == std::string::npos) return false;
        pos += 5;
        playlist.id = std::stoull(json.substr(pos));
        
        // Parse name
        pos = json.find("\"name\":", pos);
        if (pos == std::string::npos) return false;
        pos = json.find("\"", pos + 7) + 1;
        size_t end = json.find("\"", pos);
        playlist.name = json.substr(pos, end - pos);
        
        // Parse creation_time
        pos = json.find("\"creation_time\":", pos);
        if (pos == std::string::npos) return false;
        pos += 16;
        playlist.creation_time = std::stoull(json.substr(pos));
        
        // Parse modification_time
        pos = json.find("\"modification_time\":", pos);
        if (pos == std::string::npos) return false;
        pos += 20;
        playlist.modification_time = std::stoull(json.substr(pos));
        
        // Parse tracks array
        pos = json.find("\"tracks\":", pos);
        if (pos == std::string::npos) return false;
        
        // Find all track objects
        while (true) {
            pos = json.find("\"file_path\":", pos);
            if (pos == std::string::npos) break;
            
            TrackReference track;
            
            pos = json.find("\"", pos + 12) + 1;
            end = json.find("\"", pos);
            track.file_path = json.substr(pos, end - pos);
            
            pos = json.find("\"metadata_hash\":", pos);
            if (pos != std::string::npos) {
                pos += 16;
                track.metadata_hash = std::stoull(json.substr(pos));
            }
            
            pos = json.find("\"added_time\":", pos);
            if (pos != std::string::npos) {
                pos += 13;
                track.added_time = std::stoull(json.substr(pos));
            }
            
            playlist.tracks.push_back(track);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse playlist JSON: " << e.what() << std::endl;
        return false;
    }
}

std::string PlaylistManager::get_playlist_file_path(const Playlist& playlist) const {
    return config_dir_ + "/playlists/" + playlist.name + ".json";
}

}} // namespace mp::core
