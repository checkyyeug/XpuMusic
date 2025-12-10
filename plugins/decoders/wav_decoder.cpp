#include "mp_plugin.h"
#include "mp_decoder.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <iostream>

namespace {

// WAV file header structures
#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];           // "RIFF"
    uint32_t file_size;
    char wave[4];           // "WAVE"
};

struct WAVFormat {
    char fmt[4];            // "fmt "
    uint32_t fmt_size;
    uint16_t audio_format;  // 1 = PCM
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct WAVDataChunk {
    char data[4];           // "data"
    uint32_t data_size;
};
#pragma pack(pop)

// Decoder state
struct WAVDecoderState {
    std::ifstream file;
    mp::AudioStreamInfo info;
    size_t data_start;
    size_t data_size;
    size_t current_pos;
};

class WAVDecoder : public mp::IDecoder {
public:
    WAVDecoder() {
        std::cout << "WAV Decoder created" << std::endl;
    }
    
    ~WAVDecoder() override {
        std::cout << "WAV Decoder destroyed" << std::endl;
    }
    
    int probe_file(const void* header, size_t header_size) override {
        if (header_size < 12) {
            return 0;
        }
        
        const WAVHeader* wav_header = static_cast<const WAVHeader*>(header);
        
        if (std::memcmp(wav_header->riff, "RIFF", 4) == 0 &&
            std::memcmp(wav_header->wave, "WAVE", 4) == 0) {
            return 100; // Perfect match
        }
        
        return 0;
    }
    
    const char** get_extensions() const override {
        static const char* extensions[] = { "wav", "wave", nullptr };
        return extensions;
    }
    
    mp::Result open_stream(const char* file_path, mp::DecoderHandle* handle) override {
        auto* state = new WAVDecoderState();
        
        state->file.open(file_path, std::ios::binary);
        if (!state->file.is_open()) {
            delete state;
            return mp::Result::FileNotFound;
        }
        
        // Read WAV header
        WAVHeader wav_header;
        state->file.read(reinterpret_cast<char*>(&wav_header), sizeof(WAVHeader));
        
        if (std::memcmp(wav_header.riff, "RIFF", 4) != 0 ||
            std::memcmp(wav_header.wave, "WAVE", 4) != 0) {
            delete state;
            return mp::Result::Error;
        }
        
        // Read fmt chunk
        WAVFormat fmt;
        state->file.read(reinterpret_cast<char*>(&fmt), sizeof(WAVFormat));
        
        if (std::memcmp(fmt.fmt, "fmt ", 4) != 0 || fmt.audio_format != 1) {
            delete state;
            return mp::Result::NotSupported;
        }
        
        // Read data chunk header
        WAVDataChunk data_chunk;
        state->file.read(reinterpret_cast<char*>(&data_chunk), sizeof(WAVDataChunk));
        
        if (std::memcmp(data_chunk.data, "data", 4) != 0) {
            delete state;
            return mp::Result::Error;
        }
        
        // Fill stream info
        state->info.sample_rate = fmt.sample_rate;
        state->info.channels = fmt.channels;
        
        switch (fmt.bits_per_sample) {
            case 16:
                state->info.format = mp::SampleFormat::Int16;
                break;
            case 24:
                state->info.format = mp::SampleFormat::Int24;
                break;
            case 32:
                state->info.format = mp::SampleFormat::Int32;
                break;
            default:
                delete state;
                return mp::Result::NotSupported;
        }
        
        state->data_start = state->file.tellg();
        state->data_size = data_chunk.data_size;
        state->current_pos = 0;
        
        size_t bytes_per_sample = fmt.bits_per_sample / 8;
        state->info.total_samples = data_chunk.data_size / (fmt.channels * bytes_per_sample);
        state->info.duration_ms = (state->info.total_samples * 1000) / fmt.sample_rate;
        state->info.bitrate = (fmt.sample_rate * fmt.channels * fmt.bits_per_sample) / 1000;
        
        handle->internal = state;
        
        std::cout << "WAV file opened: " << file_path << std::endl;
        std::cout << "  Sample rate: " << fmt.sample_rate << " Hz" << std::endl;
        std::cout << "  Channels: " << fmt.channels << std::endl;
        std::cout << "  Bits per sample: " << fmt.bits_per_sample << std::endl;
        std::cout << "  Duration: " << state->info.duration_ms << " ms" << std::endl;
        
        return mp::Result::Success;
    }
    
    mp::Result get_stream_info(mp::DecoderHandle handle, mp::AudioStreamInfo* info) override {
        auto* state = static_cast<WAVDecoderState*>(handle.internal);
        if (!state) {
            return mp::Result::InvalidParameter;
        }
        
        *info = state->info;
        return mp::Result::Success;
    }
    
    mp::Result decode_block(mp::DecoderHandle handle, void* buffer, 
                           size_t buffer_size, size_t* samples_decoded) override {
        auto* state = static_cast<WAVDecoderState*>(handle.internal);
        if (!state) {
            return mp::Result::InvalidParameter;
        }
        
        size_t bytes_to_read = std::min(buffer_size, state->data_size - state->current_pos);
        
        state->file.read(static_cast<char*>(buffer), bytes_to_read);
        size_t bytes_read = state->file.gcount();
        
        state->current_pos += bytes_read;
        
        size_t bytes_per_sample = 0;
        switch (state->info.format) {
            case mp::SampleFormat::Int16: bytes_per_sample = 2; break;
            case mp::SampleFormat::Int24: bytes_per_sample = 3; break;
            case mp::SampleFormat::Int32: bytes_per_sample = 4; break;
            default: return mp::Result::Error;
        }
        
        *samples_decoded = bytes_read / (state->info.channels * bytes_per_sample);
        
        return mp::Result::Success;
    }
    
    mp::Result seek(mp::DecoderHandle handle, uint64_t position_ms, uint64_t* actual_position) override {
        auto* state = static_cast<WAVDecoderState*>(handle.internal);
        if (!state) {
            return mp::Result::InvalidParameter;
        }
        
        size_t bytes_per_sample = 0;
        switch (state->info.format) {
            case mp::SampleFormat::Int16: bytes_per_sample = 2; break;
            case mp::SampleFormat::Int24: bytes_per_sample = 3; break;
            case mp::SampleFormat::Int32: bytes_per_sample = 4; break;
            default: return mp::Result::Error;
        }
        
        uint64_t sample_pos = (position_ms * state->info.sample_rate) / 1000;
        size_t byte_pos = sample_pos * state->info.channels * bytes_per_sample;
        
        if (byte_pos > state->data_size) {
            byte_pos = state->data_size;
        }
        
        state->file.seekg(state->data_start + byte_pos);
        state->current_pos = byte_pos;
        
        *actual_position = (byte_pos * 1000) / (state->info.sample_rate * state->info.channels * bytes_per_sample);
        
        return mp::Result::Success;
    }
    
    mp::Result get_metadata(mp::DecoderHandle handle, const mp::MetadataTag** tags, size_t* count) override {
        (void)handle;
        *tags = nullptr;
        *count = 0;
        return mp::Result::Success;
    }
    
    void close_stream(mp::DecoderHandle handle) override {
        auto* state = static_cast<WAVDecoderState*>(handle.internal);
        if (state) {
            state->file.close();
            delete state;
        }
    }
};

class WAVDecoderPlugin : public mp::IPlugin {
public:
    WAVDecoderPlugin() {
        decoder_ = new WAVDecoder();
    }
    
    ~WAVDecoderPlugin() override {
        delete decoder_;
    }
    
    const mp::PluginInfo& get_plugin_info() const override {
        static mp::PluginInfo info = {
            "WAV Decoder Plugin",
            "Music Player Team",
            "Decodes WAV/WAVE audio files",
            mp::Version(0, 1, 0),
            mp::Version(0, 1, 0),
            "com.musicplayer.decoder.wav"
        };
        return info;
    }
    
    mp::PluginCapability get_capabilities() const override {
        return mp::PluginCapability::Decoder;
    }
    
    mp::Result initialize(mp::IServiceRegistry* services) override {
        (void)services;
        std::cout << "WAV Decoder Plugin initialized" << std::endl;
        return mp::Result::Success;
    }
    
    void shutdown() override {
        std::cout << "WAV Decoder Plugin shutdown" << std::endl;
    }
    
    void* get_service(mp::ServiceID id) override {
        if (id == mp::hash_string("mp.decoder")) {
            return decoder_;
        }
        return nullptr;
    }
    
private:
    WAVDecoder* decoder_;
};

} // anonymous namespace

// Plugin exports
MP_DEFINE_PLUGIN(WAVDecoderPlugin)
