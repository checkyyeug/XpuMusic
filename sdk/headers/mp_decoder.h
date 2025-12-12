#pragma once

#include "mp_types.h"
#include <cstring>

namespace mp {

// Metadata key-value pair
struct MetadataTag {
    const char* key;
    const char* value;
};

// Common metadata keys
constexpr const char* META_TITLE = "title";
constexpr const char* META_ARTIST = "artist";
constexpr const char* META_ALBUM = "album";
constexpr const char* META_ALBUM_ARTIST = "album_artist";
constexpr const char* META_GENRE = "genre";
constexpr const char* META_DATE = "date";
constexpr const char* META_TRACK_NUMBER = "track_number";
constexpr const char* META_DISC_NUMBER = "disc_number";
constexpr const char* META_COMMENT = "comment";
constexpr const char* META_COMPOSER = "composer";

// Decoder instance handle
struct DecoderHandle {
    void* internal;
};

// Decoder plugin interface
class IDecoder {
public:
    virtual ~IDecoder() = default;
    
    // Probe file format (returns confidence score 0-100)
    virtual int probe_file(const void* header, size_t header_size) = 0;
    
    // Get supported file extensions (null-terminated array)
    virtual const char** get_extensions() const = 0;
    
    // Open audio stream from file path
    virtual Result open_stream(const char* file_path, DecoderHandle* handle) = 0;
    
    // Get stream information
    virtual Result get_stream_info(DecoderHandle handle, AudioStreamInfo* info) = 0;
    
    // Decode audio block
    // Returns number of samples decoded per channel
    virtual Result decode_block(DecoderHandle handle, void* buffer, 
                               size_t buffer_size, size_t* samples_decoded) = 0;
    
    // Seek to position (in milliseconds)
    // Returns actual position after seek
    virtual Result seek(DecoderHandle handle, uint64_t position_ms, uint64_t* actual_position) = 0;
    
    // Get metadata tags
    virtual Result get_metadata(DecoderHandle handle, const MetadataTag** tags, size_t* count) = 0;
    
    // Close stream and release resources
    virtual void close_stream(DecoderHandle handle) = 0;
};

} // namespace mp
