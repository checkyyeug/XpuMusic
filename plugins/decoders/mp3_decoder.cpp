#include "mp_plugin.h"
#include "mp_decoder.h"

#define MINIMP3_FLOAT_OUTPUT  // We want float output for DSP processing
#include "../../sdk/external/minimp3/minimp3_ex.h"

#include <fstream>
#include <vector>
#include <cstring>
#include <iostream>

namespace {

// Decoder state
struct MP3DecoderState {
    mp3dec_ex_t decoder;
    mp::AudioStreamInfo info;
    std::string file_path;
    bool is_open;
    uint64_t current_sample;
};

class MP3Decoder : public mp::IDecoder {
public:
    MP3Decoder() {
        std::cout << "MP3 Decoder created" << std::endl;
    }
    
    ~MP3Decoder() override {
        std::cout << "MP3 Decoder destroyed" << std::endl;
    }
    
    int probe_file(const void* header, size_t header_size) override {
        if (header_size < 3) {
            return 0;
        }
        
        const uint8_t* bytes = static_cast<const uint8_t*>(header);
        
        // Check for ID3 tag
        if (header_size >= 10 && 
            bytes[0] == 'I' && bytes[1] == 'D' && bytes[2] == '3') {
            return 90; // Strong match
        }
        
        // Check for MP3 frame sync (11 bits set: 0xFF 0xFx or 0xFF 0xEx)
        if (header_size >= 2 &&
            bytes[0] == 0xFF && 
            (bytes[1] & 0xE0) == 0xE0) {
            return 85; // Good match
        }
        
        return 0;
    }
    
    const char** get_extensions() const override {
        static const char* extensions[] = { "mp3", nullptr };
        return extensions;
    }
    
    mp::Result open_stream(const char* file_path, mp::DecoderHandle* handle) override {
        auto* state = new MP3DecoderState();
        state->is_open = false;
        state->current_sample = 0;
        state->file_path = file_path;
        
        // Open the MP3 file using minimp3_ex
        if (mp3dec_ex_open(&state->decoder, file_path, MP3D_SEEK_TO_SAMPLE) != 0) {
            delete state;
            return mp::Result::FileNotFound;
        }
        
        state->is_open = true;
        
        // Fill stream info
        state->info.sample_rate = state->decoder.info.hz;
        state->info.channels = state->decoder.info.channels;
        state->info.format = mp::SampleFormat::Float32; // minimp3 with MINIMP3_FLOAT_OUTPUT
        state->info.total_samples = state->decoder.samples / state->info.channels;
        state->info.duration_ms = (state->info.total_samples * 1000) / state->info.sample_rate;
        state->info.bitrate = state->decoder.info.bitrate_kbps;
        
        handle->internal = state;
        
        std::cout << "MP3 file opened: " << file_path << std::endl;
        std::cout << "  Sample rate: " << state->info.sample_rate << " Hz" << std::endl;
        std::cout << "  Channels: " << state->info.channels << std::endl;
        std::cout << "  Duration: " << state->info.duration_ms << " ms" << std::endl;
        std::cout << "  Bitrate: " << state->info.bitrate << " kbps" << std::endl;
        std::cout << "  Layer: " << state->decoder.info.layer << std::endl;
        
        return mp::Result::Success;
    }
    
    mp::Result get_stream_info(mp::DecoderHandle handle, mp::AudioStreamInfo* info) override {
        auto* state = static_cast<MP3DecoderState*>(handle.internal);
        if (!state || !state->is_open) {
            return mp::Result::InvalidParameter;
        }
        
        *info = state->info;
        return mp::Result::Success;
    }
    
    mp::Result decode_block(mp::DecoderHandle handle, void* buffer, 
                           size_t buffer_size, size_t* samples_decoded) override {
        auto* state = static_cast<MP3DecoderState*>(handle.internal);
        if (!state || !state->is_open) {
            return mp::Result::InvalidParameter;
        }
        
        // Calculate how many samples we can decode
        size_t samples_per_channel = buffer_size / (sizeof(float) * state->info.channels);
        
        // Read samples from minimp3
        size_t samples_read = mp3dec_ex_read(&state->decoder, 
                                             static_cast<mp3d_sample_t*>(buffer), 
                                             samples_per_channel * state->info.channels);
        
        // samples_read is total samples (interleaved), convert to samples per channel
        *samples_decoded = samples_read / state->info.channels;
        state->current_sample += *samples_decoded;
        
        return mp::Result::Success;
    }
    
    mp::Result seek(mp::DecoderHandle handle, uint64_t position_ms, uint64_t* actual_position) override {
        auto* state = static_cast<MP3DecoderState*>(handle.internal);
        if (!state || !state->is_open) {
            return mp::Result::InvalidParameter;
        }
        
        // Convert milliseconds to sample position
        uint64_t sample_pos = (position_ms * state->info.sample_rate) / 1000;
        
        // Seek to the sample position
        if (mp3dec_ex_seek(&state->decoder, sample_pos * state->info.channels) != 0) {
            return mp::Result::Error;
        }
        
        state->current_sample = sample_pos;
        *actual_position = (state->current_sample * 1000) / state->info.sample_rate;
        
        return mp::Result::Success;
    }
    
    mp::Result get_metadata(mp::DecoderHandle handle, const mp::MetadataTag** tags, size_t* count) override {
        (void)handle;
        // minimp3 doesn't provide ID3 tag parsing in the basic API
        // For now, return no metadata (could be enhanced later)
        *tags = nullptr;
        *count = 0;
        return mp::Result::Success;
    }
    
    void close_stream(mp::DecoderHandle handle) override {
        auto* state = static_cast<MP3DecoderState*>(handle.internal);
        if (state) {
            if (state->is_open) {
                mp3dec_ex_close(&state->decoder);
            }
            delete state;
        }
    }
};

class MP3DecoderPlugin : public mp::IPlugin {
public:
    MP3DecoderPlugin() {
        decoder_ = new MP3Decoder();
    }
    
    ~MP3DecoderPlugin() override {
        delete decoder_;
    }
    
    const mp::PluginInfo& get_plugin_info() const override {
        static mp::PluginInfo info = {
            "MP3 Decoder Plugin",
            "Music Player Team",
            "Decodes MP3 audio files using minimp3",
            mp::Version(1, 0, 0),
            mp::Version(0, 1, 0),
            "com.musicplayer.decoder.mp3"
        };
        return info;
    }
    
    mp::PluginCapability get_capabilities() const override {
        return mp::PluginCapability::Decoder;
    }
    
    mp::Result initialize(mp::IServiceRegistry* services) override {
        (void)services;
        std::cout << "MP3 Decoder Plugin initialized" << std::endl;
        return mp::Result::Success;
    }
    
    void shutdown() override {
        std::cout << "MP3 Decoder Plugin shutdown" << std::endl;
    }
    
    void* get_service(mp::ServiceID id) override {
        if (id == mp::hash_string("mp.decoder")) {
            return decoder_;
        }
        return nullptr;
    }
    
private:
    MP3Decoder* decoder_;
};

} // anonymous namespace

// Plugin exports
MP_DEFINE_PLUGIN(MP3DecoderPlugin)
