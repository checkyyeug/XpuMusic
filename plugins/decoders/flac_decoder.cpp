#include "mp_plugin.h"
#include "mp_decoder.h"

#ifndef NO_FLAC
#include <FLAC/stream_decoder.h>
#endif

#include <iostream>
#include <vector>
#include <cstring>
#include <memory>

namespace mp {
namespace plugins {

#ifndef NO_FLAC

// FLAC decoder context
struct FLACDecoderContext {
    FLAC__StreamDecoder* decoder;
    std::vector<int32_t> decode_buffer;  // Interleaved PCM buffer
    size_t buffer_position;
    size_t buffer_size;
    AudioStreamInfo stream_info;
    std::vector<MetadataTag> metadata;
    std::vector<std::string> metadata_strings;  // Storage for metadata values
    uint64_t current_sample;
    bool eos;
    
    FLACDecoderContext() 
        : decoder(nullptr)
        , buffer_position(0)
        , buffer_size(0)
        , current_sample(0)
        , eos(false) {
        std::memset(&stream_info, 0, sizeof(stream_info));
    }
};

// FLAC callbacks
static FLAC__StreamDecoderWriteStatus write_callback(
    const FLAC__StreamDecoder* decoder,
    const FLAC__Frame* frame,
    const FLAC__int32* const buffer[],
    void* client_data) {
    
    (void)decoder;
    FLACDecoderContext* ctx = static_cast<FLACDecoderContext*>(client_data);
    
    size_t samples = frame->header.blocksize;
    size_t channels = ctx->stream_info.channels;
    
    // Resize buffer if needed
    ctx->decode_buffer.resize(samples * channels);
    
    // Interleave samples
    for (size_t i = 0; i < samples; i++) {
        for (size_t ch = 0; ch < channels; ch++) {
            ctx->decode_buffer[i * channels + ch] = buffer[ch][i];
        }
    }
    
    ctx->buffer_position = 0;
    ctx->buffer_size = samples;
    
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback(
    const FLAC__StreamDecoder* decoder,
    const FLAC__StreamMetadata* metadata,
    void* client_data) {
    
    (void)decoder;
    FLACDecoderContext* ctx = static_cast<FLACDecoderContext*>(client_data);
    
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        ctx->stream_info.sample_rate = metadata->data.stream_info.sample_rate;
        ctx->stream_info.channels = metadata->data.stream_info.channels;
        ctx->stream_info.total_samples = metadata->data.stream_info.total_samples;
        ctx->stream_info.duration_ms = 
            (ctx->stream_info.total_samples * 1000) / ctx->stream_info.sample_rate;
        
        // FLAC always decodes to 32-bit signed integer
        ctx->stream_info.format = SampleFormat::Int32;
        
        // Calculate bitrate
        if (metadata->data.stream_info.total_samples > 0) {
            // This is approximate - actual bitrate varies
            ctx->stream_info.bitrate = 
                (metadata->data.stream_info.bits_per_sample * 
                 metadata->data.stream_info.sample_rate * 
                 metadata->data.stream_info.channels) / 1000;
        }
    } else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        // Parse Vorbis comments
        for (uint32_t i = 0; i < metadata->data.vorbis_comment.num_comments; i++) {
            const char* comment = 
                reinterpret_cast<const char*>(metadata->data.vorbis_comment.comments[i].entry);
            
            std::string comment_str(comment);
            size_t eq_pos = comment_str.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = comment_str.substr(0, eq_pos);
                std::string value = comment_str.substr(eq_pos + 1);
                
                // Convert to lowercase for comparison
                for (char& c : key) c = tolower(c);
                
                // Store in metadata
                ctx->metadata_strings.push_back(key);
                ctx->metadata_strings.push_back(value);
            }
        }
        
        // Build metadata tag array
        ctx->metadata.clear();
        for (size_t i = 0; i < ctx->metadata_strings.size(); i += 2) {
            MetadataTag tag;
            tag.key = ctx->metadata_strings[i].c_str();
            tag.value = ctx->metadata_strings[i + 1].c_str();
            ctx->metadata.push_back(tag);
        }
    }
}

static void error_callback(
    const FLAC__StreamDecoder* decoder,
    FLAC__StreamDecoderErrorStatus status,
    void* client_data) {
    
    (void)decoder;
    (void)client_data;
    std::cerr << "FLAC decoder error: " << FLAC__StreamDecoderErrorStatusString[status] << std::endl;
}

#endif // NO_FLAC

// FLAC decoder plugin
class FLACDecoder : public IPlugin, public IDecoder {
public:
    FLACDecoder() : services_(nullptr) {}
    
    ~FLACDecoder() override = default;
    
    // IPlugin interface
    const PluginInfo& get_plugin_info() const override {
        static const PluginInfo info = {
            "FLAC Decoder",
            "Music Player Team",
            "FLAC audio format decoder using libFLAC",
            Version(1, 0, 0),
            API_VERSION,
            "com.musicplayer.decoder.flac"
        };
        return info;
    }
    
    PluginCapability get_capabilities() const override {
        return PluginCapability::Decoder;
    }
    
    Result initialize(IServiceRegistry* services) override {
        services_ = services;
        
#ifdef NO_FLAC
        std::cerr << "FLAC decoder: libFLAC not available" << std::endl;
        return Result::NotSupported;
#else
        std::cout << "FLAC decoder initialized (libFLAC version " 
                  << FLAC__VERSION_STRING << ")" << std::endl;
        return Result::Success;
#endif
    }
    
    void shutdown() override {
        services_ = nullptr;
    }
    
    void* get_service(ServiceID id) override {
        constexpr ServiceID SERVICE_DECODER = hash_string("mp.service.decoder");
        if (id == SERVICE_DECODER) {
            return static_cast<IDecoder*>(this);
        }
        return nullptr;
    }
    
    // IDecoder interface
    int probe_file(const void* header, size_t header_size) override {
#ifdef NO_FLAC
        (void)header;
        (void)header_size;
        return 0;
#else
        // Check for FLAC signature "fLaC"
        if (header_size >= 4) {
            const uint8_t* data = static_cast<const uint8_t*>(header);
            if (data[0] == 'f' && data[1] == 'L' && 
                data[2] == 'a' && data[3] == 'C') {
                return 100; // Maximum confidence
            }
        }
        return 0;
#endif
    }
    
    const char** get_extensions() const override {
        static const char* extensions[] = { "flac", "fla", nullptr };
        return extensions;
    }
    
    Result open_stream(const char* file_path, DecoderHandle* handle) override {
#ifdef NO_FLAC
        (void)file_path;
        (void)handle;
        return Result::NotSupported;
#else
        // Create decoder context
        auto ctx = std::make_unique<FLACDecoderContext>();
        
        // Create FLAC decoder
        ctx->decoder = FLAC__stream_decoder_new();
        if (!ctx->decoder) {
            return Result::OutOfMemory;
        }
        
        // Initialize decoder
        FLAC__stream_decoder_set_md5_checking(ctx->decoder, false);
        
        FLAC__StreamDecoderInitStatus init_status = 
            FLAC__stream_decoder_init_file(
                ctx->decoder,
                file_path,
                write_callback,
                metadata_callback,
                error_callback,
                ctx.get()
            );
        
        if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            FLAC__stream_decoder_delete(ctx->decoder);
            return Result::Error;
        }
        
        // Process metadata
        if (!FLAC__stream_decoder_process_until_end_of_metadata(ctx->decoder)) {
            FLAC__stream_decoder_delete(ctx->decoder);
            return Result::Error;
        }
        
        handle->internal = ctx.release();
        return Result::Success;
#endif
    }
    
    Result get_stream_info(DecoderHandle handle, AudioStreamInfo* info) override {
#ifdef NO_FLAC
        (void)handle;
        (void)info;
        return Result::NotSupported;
#else
        if (!handle.internal) {
            return Result::InvalidParameter;
        }
        
        FLACDecoderContext* ctx = static_cast<FLACDecoderContext*>(handle.internal);
        *info = ctx->stream_info;
        return Result::Success;
#endif
    }
    
    Result decode_block(DecoderHandle handle, void* buffer, 
                       size_t buffer_size, size_t* samples_decoded) override {
#ifdef NO_FLAC
        (void)handle;
        (void)buffer;
        (void)buffer_size;
        (void)samples_decoded;
        return Result::NotSupported;
#else
        if (!handle.internal) {
            return Result::InvalidParameter;
        }
        
        FLACDecoderContext* ctx = static_cast<FLACDecoderContext*>(handle.internal);
        
        if (ctx->eos) {
            *samples_decoded = 0;
            return Result::Success;
        }
        
        int32_t* output = static_cast<int32_t*>(buffer);
        size_t samples_per_channel = buffer_size / (sizeof(int32_t) * ctx->stream_info.channels);
        size_t total_decoded = 0;
        
        while (total_decoded < samples_per_channel) {
            // If we have buffered data, copy it
            if (ctx->buffer_position < ctx->buffer_size) {
                size_t samples_available = ctx->buffer_size - ctx->buffer_position;
                size_t samples_to_copy = std::min(samples_available, 
                                                  samples_per_channel - total_decoded);
                
                size_t samples_in_bytes = samples_to_copy * ctx->stream_info.channels;
                std::memcpy(output + (total_decoded * ctx->stream_info.channels),
                           &ctx->decode_buffer[ctx->buffer_position * ctx->stream_info.channels],
                           samples_in_bytes * sizeof(int32_t));
                
                ctx->buffer_position += samples_to_copy;
                total_decoded += samples_to_copy;
                ctx->current_sample += samples_to_copy;
            } else {
                // Decode next frame
                if (!FLAC__stream_decoder_process_single(ctx->decoder)) {
                    ctx->eos = true;
                    break;
                }
                
                FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(ctx->decoder);
                if (state == FLAC__STREAM_DECODER_END_OF_STREAM) {
                    ctx->eos = true;
                    break;
                }
            }
        }
        
        *samples_decoded = total_decoded;
        return Result::Success;
#endif
    }
    
    Result seek(DecoderHandle handle, uint64_t position_ms, uint64_t* actual_position) override {
#ifdef NO_FLAC
        (void)handle;
        (void)position_ms;
        (void)actual_position;
        return Result::NotSupported;
#else
        if (!handle.internal) {
            return Result::InvalidParameter;
        }
        
        FLACDecoderContext* ctx = static_cast<FLACDecoderContext*>(handle.internal);
        
        uint64_t target_sample = (position_ms * ctx->stream_info.sample_rate) / 1000;
        
        if (!FLAC__stream_decoder_seek_absolute(ctx->decoder, target_sample)) {
            return Result::Error;
        }
        
        ctx->current_sample = target_sample;
        ctx->buffer_position = 0;
        ctx->buffer_size = 0;
        ctx->eos = false;
        
        *actual_position = (target_sample * 1000) / ctx->stream_info.sample_rate;
        return Result::Success;
#endif
    }
    
    Result get_metadata(DecoderHandle handle, const MetadataTag** tags, size_t* count) override {
#ifdef NO_FLAC
        (void)handle;
        (void)tags;
        (void)count;
        return Result::NotSupported;
#else
        if (!handle.internal) {
            return Result::InvalidParameter;
        }
        
        FLACDecoderContext* ctx = static_cast<FLACDecoderContext*>(handle.internal);
        
        if (ctx->metadata.empty()) {
            *tags = nullptr;
            *count = 0;
        } else {
            *tags = ctx->metadata.data();
            *count = ctx->metadata.size();
        }
        
        return Result::Success;
#endif
    }
    
    void close_stream(DecoderHandle handle) override {
#ifndef NO_FLAC
        if (handle.internal) {
            FLACDecoderContext* ctx = static_cast<FLACDecoderContext*>(handle.internal);
            
            if (ctx->decoder) {
                FLAC__stream_decoder_finish(ctx->decoder);
                FLAC__stream_decoder_delete(ctx->decoder);
            }
            
            delete ctx;
        }
#else
        (void)handle;
#endif
    }
    
private:
    IServiceRegistry* services_;
};

}} // namespace mp::plugins

// Plugin entry points
MP_DEFINE_PLUGIN(mp::plugins::FLACDecoder)
