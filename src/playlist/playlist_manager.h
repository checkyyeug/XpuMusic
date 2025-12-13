/**
 * @file playlist_manager.h
 * @brief Playlist management for XpuMusic
 * @date 2025-12-13
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <random>
#include <chrono>
#include <functional>

namespace xpumusic {
namespace playlist {

/**
 * @brief Track information
 */
struct Track {
    std::string path;              // File path
    std::string title;             // Track title
    std::string artist;            // Artist name
    std::string album;             // Album name
    double duration;               // Duration in seconds
    std::chrono::system_clock::time_point added_time;  // When added to playlist

    Track() : duration(0.0) {
        added_time = std::chrono::system_clock::now();
    }
};

/**
 * @brief Playback modes
 */
enum class PlaybackMode {
    Sequential,    // Play tracks in order
    Random,        // Random order
    RepeatOne,     // Repeat current track
    RepeatAll,     // Repeat entire playlist
    Shuffle        // Shuffle once, then play sequentially
};

/**
 * @brief Playlist events
 */
struct PlaylistEvent {
    enum Type {
        TrackAdded,
        TrackRemoved,
        TrackMoved,
        CurrentChanged,
        PlaybackModeChanged,
        Cleared
    } type;

    size_t track_index;      // For track-specific events
    Track track;             // Track information
    std::string details;     // Additional details
};

using EventCallback = std::function<void(const PlaylistEvent&)>;

/**
 * @brief Playlist manager class
 */
class PlaylistManager {
public:
    PlaylistManager();
    ~PlaylistManager() = default;

    // Playlist operations
    void add_track(const Track& track);
    void add_track(const std::string& file_path);
    void remove_track(size_t index);
    void remove_current();
    void clear();
    void move_track(size_t from_index, size_t to_index);

    // Track access
    const Track& get_track(size_t index) const;
    Track get_current_track() const;
    size_t get_current_index() const { return current_index_; }
    size_t get_track_count() const { return tracks_.size(); }

    // Navigation
    void next();
    void previous();
    void jump_to(size_t index);
    bool has_next() const;
    bool has_previous() const;

    // Playback control
    void set_playback_mode(PlaybackMode mode);
    PlaybackMode get_playback_mode() const { return playback_mode_; }
    void set_shuffle_seed(unsigned int seed);

    // Playlist persistence
    bool save_m3u(const std::string& file_path) const;
    bool load_m3u(const std::string& file_path);
    bool save_pls(const std::string& file_path) const;
    bool load_pls(const std::string& file_path);

    // Search and filtering
    std::vector<size_t> search(const std::string& query) const;
    void filter(const std::string& query);
    void clear_filter();

    // Event system
    void set_event_callback(EventCallback callback) { event_callback_ = callback; }

    // Statistics
    double get_total_duration() const;
    size_t get_played_count() const { return played_count_; }
    void reset_statistics();

private:
    std::vector<Track> tracks_;
    std::vector<size_t> track_order_;     // For shuffle mode
    std::vector<size_t> filtered_indices_; // For filtered view
    size_t current_index_;
    PlaybackMode playback_mode_;
    bool is_filtered_;

    // Random number generation
    std::mt19937 rng_;

    // Statistics
    size_t played_count_;
    std::chrono::system_clock::time_point start_time_;

    // Events
    EventCallback event_callback_;

    // Helper methods
    void generate_shuffle_order();
    void notify_event(PlaylistEvent::Type type, size_t index = 0, const std::string& details = "");
    Track create_track_from_path(const std::string& path);

    // M3U/PLS helpers
    void write_m3u_header(std::ofstream& file) const;
    void write_m3u_track(std::ofstream& file, const Track& track) const;
    bool parse_m3u_line(const std::string& line, Track& track) const;
};

/**
 * @brief Playlist parser for different formats
 */
class PlaylistParser {
public:
    static bool is_supported_format(const std::string& file_path);
    static std::unique_ptr<PlaylistManager> parse(const std::string& file_path);

private:
    static std::unique_ptr<PlaylistManager> parse_m3u(const std::string& file_path);
    static std::unique_ptr<PlaylistManager> parse_pls(const std::string& file_path);
};

/**
 * @brief Auto-playlist generator
 */
class AutoPlaylist {
public:
    // Generate playlists based on criteria
    static std::unique_ptr<PlaylistManager> generate_recently_added(
        const std::string& music_dir, int days = 7);

    static std::unique_ptr<PlaylistManager> generate_most_played(
        const std::string& music_dir);

    static std::unique_ptr<PlaylistManager> generate_by_genre(
        const std::string& music_dir, const std::string& genre);

    static std::unique_ptr<PlaylistManager> generate_by_year(
        const std::string& music_dir, int year);
};

} // namespace playlist
} // namespace xpumusic