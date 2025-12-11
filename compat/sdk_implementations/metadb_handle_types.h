/**
 * @file metadb_handle_types.h
 * @brief Concrete type definitions for metadb_handle implementation
 * @date 2025-12-11
 */

#pragma once

#include <string>
#include <chrono>

namespace foobar2000_sdk {

// Concrete playable location implementation
class playable_location_impl {
private:
    std::string path_;
    uint32_t subsong_index_;

public:
    playable_location_impl() : subsong_index_(0) {}
    playable_location_impl(const std::string& path, uint32_t subsong = 0)
        : path_(path), subsong_index_(subsong) {}

    const std::string& get_path() const { return path_; }
    void set_path(const std::string& path) { path_ = path; }

    uint32_t get_subsong_index() const { return subsong_index_; }
    void set_subsong_index(uint32_t index) { subsong_index_ = index; }

    bool operator==(const playable_location_impl& other) const {
        return path_ == other.path_ && subsong_index_ == other.subsong_index_;
    }

    bool operator!=(const playable_location_impl& other) const {
        return !(*this == other);
    }
};

// Track statistics structure
struct TrackStatistics {
    uint32_t playcount = 0;
    std::chrono::system_clock::time_point last_played;
    uint32_t rating = 0;  // 0-5 stars
    double added_timestamp = 0.0;  // Unix timestamp when added
    std::string source;  // Where the file came from
    bool skip = false;   // Whether to skip this track

    TrackStatistics() {
        last_played = std::chrono::system_clock::from_time_t(0);
    }
};

// File statistics compatibility - using xpumusic_sdk::file_stats
using xpumusic_sdk::file_stats;

// Type aliases for compatibility
using playable_location = playable_location_impl;

} // namespace foobar2000_sdk