/**
 * @file playlist_manager.cpp
 * @brief Implementation of playlist management
 * @date 2025-12-13
 */

#include "playlist_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <random>

namespace xpumusic {
namespace playlist {

PlaylistManager::PlaylistManager()
    : current_index_(0)
    , playback_mode_(PlaybackMode::Sequential)
    , is_filtered_(false)
    , rng_(std::chrono::steady_clock::now().time_since_epoch().count())
    , played_count_(0)
    , start_time_(std::chrono::system_clock::now()) {
}

void PlaylistManager::add_track(const Track& track) {
    tracks_.push_back(track);

    // Update shuffle order if needed
    if (playback_mode_ == PlaybackMode::Random || playback_mode_ == PlaybackMode::Shuffle) {
        track_order_.push_back(tracks_.size() - 1);
        generate_shuffle_order();
    }

    notify_event(PlaylistEvent::TrackAdded, tracks_.size() - 1);
}

void PlaylistManager::add_track(const std::string& file_path) {
    Track track = create_track_from_path(file_path);
    add_track(track);
}

void PlaylistManager::remove_track(size_t index) {
    if (index >= tracks_.size()) return;

    tracks_.erase(tracks_.begin() + index);

    // Update current index
    if (current_index_ > index) {
        current_index_--;
    } else if (current_index_ == index && current_index_ >= tracks_.size()) {
        current_index_ = tracks_.size() > 0 ? tracks_.size() - 1 : 0;
    }

    // Rebuild shuffle order
    if (playback_mode_ == PlaybackMode::Random || playback_mode_ == PlaybackMode::Shuffle) {
        generate_shuffle_order();
    }

    notify_event(PlaylistEvent::TrackRemoved, index);
}

void PlaylistManager::remove_current() {
    if (current_index_ < tracks_.size()) {
        remove_track(current_index_);
    }
}

void PlaylistManager::clear() {
    tracks_.clear();
    track_order_.clear();
    filtered_indices_.clear();
    current_index_ = 0;
    is_filtered_ = false;
    notify_event(PlaylistEvent::Cleared);
}

void PlaylistManager::move_track(size_t from_index, size_t to_index) {
    if (from_index >= tracks_.size() || to_index >= tracks_.size()) return;

    Track track = tracks_[from_index];
    tracks_.erase(tracks_.begin() + from_index);
    tracks_.insert(tracks_.begin() + to_index, track);

    // Update current index if needed
    if (current_index_ == from_index) {
        current_index_ = to_index;
    } else if (from_index < current_index_ && to_index >= current_index_) {
        current_index_--;
    } else if (from_index > current_index_ && to_index <= current_index_) {
        current_index_++;
    }

    notify_event(PlaylistEvent::TrackMoved, to_index,
                "Moved from index " + std::to_string(from_index));
}

const Track& PlaylistManager::get_track(size_t index) const {
    static Track empty_track;
    return index < tracks_.size() ? tracks_[index] : empty_track;
}

Track PlaylistManager::get_current_track() const {
    static Track empty_track;
    size_t actual_index = current_index_;

    // Note: For const methods, we can't check is_filtered_ directly
    // In practice, this method is used for display, so we'll use current_index_

    // Check if we have shuffle order (from non-const context)
    // This is a simplified version - in production you'd want a better solution
    PlaylistManager* non_const_this = const_cast<PlaylistManager*>(this);
    if (non_const_this->is_filtered_ && !non_const_this->filtered_indices_.empty()) {
        actual_index = non_const_this->filtered_indices_[actual_index];
    } else if (!non_const_this->track_order_.empty() &&
              (non_const_this->playback_mode_ == PlaybackMode::Random || non_const_this->playback_mode_ == PlaybackMode::Shuffle)) {
        actual_index = non_const_this->track_order_[actual_index];
    }

    return actual_index < tracks_.size() ? tracks_[actual_index] : empty_track;
}

void PlaylistManager::next() {
    if (tracks_.empty()) return;

    played_count_++;

    size_t total_tracks = is_filtered_ ? filtered_indices_.size() : tracks_.size();

    switch (playback_mode_) {
        case PlaybackMode::Sequential:
        case PlaybackMode::Shuffle:
            current_index_ = (current_index_ + 1) % total_tracks;
            break;

        case PlaybackMode::Random:
            {
                std::uniform_int_distribution<size_t> dist(0, total_tracks - 1);
                current_index_ = dist(rng_);
            }
            break;

        case PlaybackMode::RepeatOne:
            // Don't change index
            break;

        case PlaybackMode::RepeatAll:
            if (current_index_ >= total_tracks - 1) {
                current_index_ = 0;
            } else {
                current_index_++;
            }
            break;
    }

    notify_event(PlaylistEvent::CurrentChanged, current_index_);
}

void PlaylistManager::previous() {
    if (tracks_.empty()) return;

    size_t total_tracks = is_filtered_ ? filtered_indices_.size() : tracks_.size();

    if (playback_mode_ == PlaybackMode::RepeatOne) {
        // Stay on current track
        return;
    }

    current_index_ = (current_index_ == 0) ? total_tracks - 1 : current_index_ - 1;
    notify_event(PlaylistEvent::CurrentChanged, current_index_);
}

void PlaylistManager::jump_to(size_t index) {
    size_t total_tracks = is_filtered_ ? filtered_indices_.size() : tracks_.size();

    if (index < total_tracks) {
        current_index_ = index;
        notify_event(PlaylistEvent::CurrentChanged, index);
    }
}

bool PlaylistManager::has_next() const {
    size_t total_tracks = is_filtered_ ? filtered_indices_.size() : tracks_.size();

    switch (playback_mode_) {
        case PlaybackMode::Sequential:
        case PlaybackMode::Shuffle:
            return current_index_ < total_tracks - 1;

        case PlaybackMode::RepeatAll:
        case PlaybackMode::RepeatOne:
            return true;  // Always can go next in repeat modes

        case PlaybackMode::Random:
            return true;  // Random can always go to a different track

        default:
            return false;
    }
}

bool PlaylistManager::has_previous() const {
    size_t total_tracks = is_filtered_ ? filtered_indices_.size() : tracks_.size();

    switch (playback_mode_) {
        case PlaybackMode::RepeatOne:
            return true;  // Always can repeat

        case PlaybackMode::RepeatAll:
            return true;  // Can wrap around

        case PlaybackMode::Sequential:
        case PlaybackMode::Shuffle:
            return current_index_ > 0;

        case PlaybackMode::Random:
            return true;  // Random can always go to a different track

        default:
            return false;
    }
}

void PlaylistManager::set_playback_mode(PlaybackMode mode) {
    playback_mode_ = mode;

    if (mode == PlaybackMode::Random || mode == PlaybackMode::Shuffle) {
        generate_shuffle_order();
    } else {
        track_order_.clear();
    }

    notify_event(PlaylistEvent::PlaybackModeChanged, 0);
}

void PlaylistManager::set_shuffle_seed(unsigned int seed) {
    rng_.seed(seed);
    if (playback_mode_ == PlaybackMode::Random || playback_mode_ == PlaybackMode::Shuffle) {
        generate_shuffle_order();
    }
}

bool PlaylistManager::save_m3u(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) return false;

    write_m3u_header(file);

    for (const auto& track : tracks_) {
        write_m3u_track(file, track);
    }

    return true;
}

bool PlaylistManager::load_m3u(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) return false;

    clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        Track track;
        if (parse_m3u_line(line, track)) {
            add_track(track);
        }
    }

    return true;
}

bool PlaylistManager::save_pls(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) return false;

    file << "[playlist]\n";
    file << "NumberOfEntries=" << tracks_.size() << "\n\n";

    for (size_t i = 0; i < tracks_.size(); ++i) {
        const auto& track = tracks_[i];
        file << "File" << (i + 1) << "=" << track.path << "\n";
        if (!track.title.empty()) {
            file << "Title" << (i + 1) << "=" << track.title << "\n";
        }
        if (track.duration > 0) {
            file << "Length" << (i + 1) << "=" << static_cast<int>(track.duration) << "\n";
        }
        file << "\n";
    }

    file << "Version=2\n";
    return true;
}

bool PlaylistManager::load_pls(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) return false;

    clear();

    std::string line;
    std::unordered_map<size_t, Track> track_map;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[') continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        if (key.find("File") == 0) {
            size_t index = std::stoi(key.substr(4)) - 1;
            track_map[index].path = value;
        } else if (key.find("Title") == 0) {
            size_t index = std::stoi(key.substr(5)) - 1;
            track_map[index].title = value;
        } else if (key.find("Length") == 0) {
            size_t index = std::stoi(key.substr(6)) - 1;
            track_map[index].duration = std::stod(value);
        }
    }

    // Add tracks in order
    for (size_t i = 0; i < track_map.size(); ++i) {
        if (track_map.find(i) != track_map.end()) {
            add_track(track_map[i]);
        }
    }

    return true;
}

std::vector<size_t> PlaylistManager::search(const std::string& query) const {
    std::vector<size_t> results;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    for (size_t i = 0; i < tracks_.size(); ++i) {
        const auto& track = tracks_[i];

        // Search in title, artist, album, and path
        std::string title = track.title;
        std::string artist = track.artist;
        std::string album = track.album;
        std::string path = track.path;

        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        std::transform(artist.begin(), artist.end(), artist.begin(), ::tolower);
        std::transform(album.begin(), album.end(), album.begin(), ::tolower);
        std::transform(path.begin(), path.end(), path.begin(), ::tolower);

        if (title.find(lower_query) != std::string::npos ||
            artist.find(lower_query) != std::string::npos ||
            album.find(lower_query) != std::string::npos ||
            path.find(lower_query) != std::string::npos) {
            results.push_back(i);
        }
    }

    return results;
}

void PlaylistManager::filter(const std::string& query) {
    filtered_indices_ = search(query);
    is_filtered_ = true;
    current_index_ = 0;
}

void PlaylistManager::clear_filter() {
    is_filtered_ = false;
    filtered_indices_.clear();
    current_index_ = 0;
}

double PlaylistManager::get_total_duration() const {
    double total = 0.0;
    for (const auto& track : tracks_) {
        total += track.duration;
    }
    return total;
}

void PlaylistManager::reset_statistics() {
    played_count_ = 0;
    start_time_ = std::chrono::system_clock::now();
}

// Private methods
void PlaylistManager::generate_shuffle_order() {
    track_order_.clear();
    for (size_t i = 0; i < tracks_.size(); ++i) {
        track_order_.push_back(i);
    }
    std::shuffle(track_order_.begin(), track_order_.end(), rng_);
}

void PlaylistManager::notify_event(PlaylistEvent::Type type, size_t index, const std::string& details) {
    if (event_callback_) {
        PlaylistEvent event;
        event.type = type;
        event.track_index = index;
        event.details = details;

        if (index < tracks_.size()) {
            event.track = tracks_[index];
        }

        event_callback_(event);
    }
}

Track PlaylistManager::create_track_from_path(const std::string& path) {
    Track track;
    track.path = path;

    // Extract filename as title if no metadata available
    std::filesystem::path p(path);
    track.title = p.stem().string();

    // In a real implementation, you would:
    // 1. Read metadata from the audio file
    // 2. Extract title, artist, album, duration
    // 3. Use audio libraries like taglib or ffmpeg

    return track;
}

void PlaylistManager::write_m3u_header(std::ofstream& file) const {
    file << "#EXTM3U\n";
    file << "# XpuMusic Playlist\n";
    file << "# Created: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n\n";
}

void PlaylistManager::write_m3u_track(std::ofstream& file, const Track& track) const {
    if (!track.title.empty() || !track.artist.empty()) {
        file << "#EXTINF:" << static_cast<int>(track.duration);
        if (!track.artist.empty()) {
            file << " " << track.artist;
        }
        if (!track.title.empty()) {
            if (!track.artist.empty()) file << " - ";
            file << track.title;
        }
        file << "\n";
    }
    file << track.path << "\n";
}

bool PlaylistManager::parse_m3u_line(const std::string& line, Track& track) const {
    if (line.substr(0, 8) == "#EXTINF:") {
        // Parse extended info: #EXTINF:duration,title
        // This would be handled in conjunction with the next line
        return false;  // Not a file path
    }

    if (line[0] == '#') {
        return false;  // Comment or metadata
    }

    track.path = line;
    return true;
}

// PlaylistParser implementation
bool PlaylistParser::is_supported_format(const std::string& file_path) {
    std::filesystem::path p(file_path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".m3u" || ext == ".m3u8" || ext == ".pls";
}

std::unique_ptr<PlaylistManager> PlaylistParser::parse(const std::string& file_path) {
    std::filesystem::path p(file_path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".m3u" || ext == ".m3u8") {
        return parse_m3u(file_path);
    } else if (ext == ".pls") {
        return parse_pls(file_path);
    }

    return nullptr;
}

std::unique_ptr<PlaylistManager> PlaylistParser::parse_m3u(const std::string& file_path) {
    auto playlist = std::make_unique<PlaylistManager>();

    if (!playlist->load_m3u(file_path)) {
        return nullptr;
    }

    return playlist;
}

std::unique_ptr<PlaylistManager> PlaylistParser::parse_pls(const std::string& file_path) {
    auto playlist = std::make_unique<PlaylistManager>();

    if (!playlist->load_pls(file_path)) {
        return nullptr;
    }

    return playlist;
}

} // namespace playlist
} // namespace xpumusic