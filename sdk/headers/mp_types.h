#pragma once

#include <cstdint>
#include <cstddef>

namespace mp {

// Version structure
struct Version {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    
    constexpr Version(uint16_t maj, uint16_t min, uint16_t p)
        : major(maj), minor(min), patch(p) {}
    
    constexpr bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }
    
    constexpr bool operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
};

// API version
constexpr Version API_VERSION(0, 1, 0);

// Result codes
enum class Result {
    Success = 0,
    Error = 1,
    InvalidParameter = 2,
    NotImplemented = 3,
    NotSupported = 4,
    OutOfMemory = 5,
    FileNotFound = 6,
    AccessDenied = 7,
    Timeout = 8,
    NotInitialized = 9,
    AlreadyInitialized = 10,
    InvalidState = 11,
    FileError = 12,
    InvalidFormat = 13,
};

// Audio sample formats
enum class SampleFormat {
    Unknown = 0,
    Int16 = 1,      // 16-bit signed integer PCM
    Int24 = 2,      // 24-bit signed integer PCM
    Int32 = 3,      // 32-bit signed integer PCM
    Float32 = 4,    // 32-bit floating point
    Float64 = 5,    // 64-bit floating point
};

// Audio channel configurations
enum class ChannelConfig {
    Mono = 1,
    Stereo = 2,
    Surround_5_1 = 6,
    Surround_7_1 = 8,
};

// Audio stream information
struct AudioStreamInfo {
    uint32_t sample_rate;        // Sample rate in Hz
    uint32_t channels;           // Number of channels
    SampleFormat format;         // Sample format
    uint64_t total_samples;      // Total samples (0 if unknown)
    uint64_t duration_ms;        // Duration in milliseconds (0 if unknown)
    uint32_t bitrate;            // Bitrate in kbps (0 if unknown)
};

// Audio buffer for processing
struct AudioBuffer {
    void* data;                  // Pointer to sample data
    uint32_t sample_rate;        // Sample rate in Hz
    uint16_t channels;           // Number of channels
    SampleFormat format;         // Sample format
    uint32_t frames;             // Number of frames in buffer
    uint32_t capacity;           // Buffer capacity in frames
    uint64_t timestamp_us;       // Timestamp in microseconds
    uint64_t position_samples;   // Position in samples from track start
    bool end_of_stream;          // True if last buffer
    bool discontinuity;          // True if gap before this buffer
    
    AudioBuffer() 
        : data(nullptr), sample_rate(0), channels(0)
        , format(SampleFormat::Unknown), frames(0), capacity(0)
        , timestamp_us(0), position_samples(0)
        , end_of_stream(false), discontinuity(false) {}
};

// Plugin capabilities flags
enum class PluginCapability : uint32_t {
    None = 0,
    Decoder = 1 << 0,           // Audio format decoder
    Encoder = 1 << 1,           // Audio format encoder
    DSP = 1 << 2,               // DSP processor
    Visualizer = 1 << 3,        // Audio visualizer
    UIComponent = 1 << 4,       // UI component
    LibraryManager = 1 << 5,    // Media library manager
    PlaylistHandler = 1 << 6,   // Playlist format handler
    Output = 1 << 7,            // Audio output device
    Input = 1 << 8,             // Audio input device
};

inline PluginCapability operator|(PluginCapability a, PluginCapability b) {
    return static_cast<PluginCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline PluginCapability operator&(PluginCapability a, PluginCapability b) {
    return static_cast<PluginCapability>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_capability(PluginCapability caps, PluginCapability check) {
    return static_cast<uint32_t>(caps & check) != 0;
}

// Service interface IDs (using compile-time string hashing)
using ServiceID = uint64_t;

constexpr ServiceID hash_string(const char* str) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    while (*str) {
        hash ^= static_cast<uint64_t>(*str++);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// Predefined service IDs
constexpr ServiceID SERVICE_PLUGIN_HOST = hash_string("mp.service.plugin_host");
constexpr ServiceID SERVICE_EVENT_BUS = hash_string("mp.service.event_bus");
constexpr ServiceID SERVICE_CONFIG_MANAGER = hash_string("mp.service.config_manager");
constexpr ServiceID SERVICE_PLAYLIST_MANAGER = hash_string("mp.service.playlist_manager");
constexpr ServiceID SERVICE_PLAYBACK_ENGINE = hash_string("mp.service.playback_engine");
constexpr ServiceID SERVICE_VISUALIZATION = hash_string("mp.service.visualization");
constexpr ServiceID SERVICE_AUDIO_OUTPUT = hash_string("mp.service.audio_output");
constexpr ServiceID SERVICE_RESOURCE_MANAGER = hash_string("mp.service.resource_manager");

} // namespace mp
