#pragma once

#include "foobar2000_sdk.h"

namespace xpumusic_sdk {

// Input decoder interface (mimics foobar2000's input_decoder)
class input_decoder : public service_base {
public:
    virtual ~input_decoder() = default;
    
    // Initialize decoder with file path
    // Returns true on success
    virtual bool initialize(const char* path, abort_callback& abort) = 0;
    
    // Get file information and metadata
    virtual void get_info(file_info& info, abort_callback& abort) = 0;
    
    // Decode audio chunk
    // Returns true if chunk was decoded, false if end of stream
    virtual bool decode_run(audio_chunk& chunk, abort_callback& abort) = 0;
    
    // Seek to position in seconds
    // Returns true on success
    virtual bool seek(double seconds, abort_callback& abort) = 0;
    
    // Check if seeking is supported
    virtual bool can_seek() = 0;
    
    // Get dynamic info (for streaming sources)
    virtual bool get_dynamic_info(file_info& info, abort_callback& abort) {
        (void)info;
        (void)abort;
        return false;
    }
    
    // Get dynamic info bitrate
    virtual bool get_dynamic_info_track(file_info& info, abort_callback& abort) {
        (void)info;
        (void)abort;
        return false;
    }
    
    // Called on track change for gapless playback
    virtual void on_idle(abort_callback& abort) {
        (void)abort;
    }
};

// Input entry interface (plugin factory)
class input_entry : public service_base {
public:
    virtual ~input_entry() = default;
    
    // Get supported file extensions (semicolon-separated)
    virtual const char* get_file_extensions() = 0;
    
    // Check if this input can handle the file
    // Returns confidence level (0.0 to 1.0)
    virtual float is_our_content_type(const char* content_type) {
        (void)content_type;
        return 0.0f;
    }
    
    // Open decoder for file
    virtual bool open_for_decoding(
        service_ptr_t<input_decoder>& decoder,
        const char* path,
        abort_callback& abort) = 0;
    
    // Get input name
    virtual const char* get_name() = 0;
};

// Decoder statistics
struct decoder_stats {
    uint64_t samples_decoded;
    uint64_t bytes_read;
    uint32_t decode_calls;
    uint32_t seek_count;
    
    decoder_stats()
        : samples_decoded(0)
        , bytes_read(0)
        , decode_calls(0)
        , seek_count(0) {}
};

} // namespace xpumusic_sdk
