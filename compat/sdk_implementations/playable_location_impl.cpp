/**
 * @file playable_location_impl.cpp
 * @brief Implementation of playable_location_impl class
 * @date 2025-12-11
 */

#include "playable_location_impl.h"
#include <algorithm>

namespace xpumusic_sdk {

//============================================================================
// playable_location_impl class implementation
//============================================================================

playable_location_impl::playable_location_impl()
    : subsong_index_(0), is_empty_(true) {
}

playable_location_impl::playable_location_impl(const std::string& path, uint32_t subsong_index)
    : path_(path), subsong_index_(subsong_index), is_empty_(path.empty()) {
}

playable_location_impl::playable_location_impl(const playable_location_impl& other)
    : path_(other.path_), subsong_index_(other.subsong_index_), is_empty_(other.is_empty_) {
}

playable_location_impl::playable_location_impl(playable_location_impl&& other) noexcept
    : path_(std::move(other.path_))
    , subsong_index_(other.subsong_index_)
    , is_empty_(other.is_empty_) {
    other.subsong_index_ = 0;
    other.is_empty_ = true;
}

const char* playable_location_impl::get_path() const {
    return path_.c_str();
}

void playable_location_impl::set_path(const char* path) {
    path_ = path ? path : "";
    is_empty_ = path_.empty();
}

uint32_t playable_location_impl::get_subsong_index() const {
    return subsong_index_;
}

void playable_location_impl::set_subsong_index(uint32_t index) {
    subsong_index_ = index;
}

bool playable_location_impl::is_empty() const {
    return is_empty_;
}

const char* playable_location_impl::get_subsong() const {
    static thread_local std::string subsong_str;
    if (subsong_index_ == 0) {
        return "";  // No subsong
    }
    subsong_str = std::to_string(subsong_index_);
    return subsong_str.c_str();
}

std::string playable_location_impl::get_full_location() const {
    if (subsong_index_ == 0) {
        return path_;
    }
    return path_ + ":" + std::to_string(subsong_index_);
}

void playable_location_impl::reset() {
    path_.clear();
    subsong_index_ = 0;
    is_empty_ = true;
}

playable_location_impl& playable_location_impl::operator=(const playable_location_impl& other) {
    if (this != &other) {
        path_ = other.path_;
        subsong_index_ = other.subsong_index_;
        is_empty_ = other.is_empty_;
    }
    return *this;
}

playable_location_impl& playable_location_impl::operator=(playable_location_impl&& other) noexcept {
    if (this != &other) {
        path_ = std::move(other.path_);
        subsong_index_ = other.subsong_index_;
        is_empty_ = other.is_empty_;
        
        other.subsong_index_ = 0;
        other.is_empty_ = true;
    }
    return *this;
}

bool playable_location_impl::operator==(const playable_location_impl& other) const {
    return path_ == other.path_ && subsong_index_ == other.subsong_index_;
}

bool playable_location_impl::operator!=(const playable_location_impl& other) const {
    return !(*this == other);
}

//============================================================================
// Factory function implementation
//============================================================================

std::unique_ptr<playable_location_impl> create_playable_location(
    const std::string& path, 
    uint32_t subsong_index) {
    return std::make_unique<playable_location_impl>(path, subsong_index);
}

} // namespace xpumusic_sdk