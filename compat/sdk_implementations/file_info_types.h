/**
 * @file file_info_types.h
 * @brief File info type definitions for foobar2000 compatibility
 * @date 2025-12-10
 */

#pragma once

#include <string>
#include <map>
#include <vector>

namespace foobar2000_sdk {

// File information field types
enum class field_type {
    field_string,
    field_integer,
    field_boolean,
    field_time,
    field_length
};

// Metadata item structure
struct MetadataItem {
    std::string name;
    std::string value;
    field_type type;

    MetadataItem() : type(field_type::field_string) {}

    MetadataItem(const std::string& n, const std::string& v, field_type t = field_type::field_string)
        : name(n), value(v), type(t) {}
};

// Replay gain information
struct replaygain_info {
    float album_gain;
    float album_peak;
    float track_gain;
    float track_peak;

    replaygain_info()
        : album_gain(0.0f), album_peak(0.0f)
        , track_gain(0.0f), track_peak(0.0f) {}
};

// File info flags
enum file_info_flags {
    flag_format_specific = 1 << 0,
    flag_test_only = 1 << 1,
    flag_lossy = 1 << 2,
    flag_gapless = 1 << 3
};

} // namespace foobar2000_sdk