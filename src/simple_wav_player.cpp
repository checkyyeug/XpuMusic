#include "../platform/audio_output_factory.h"
#include "../sdk/headers/mp_types.h"
#include "../sdk/headers/mp_decoder.h"
#include "../sdk/headers/mp_audio_output.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdint>

// Include the WAV decoder implementation directly
#define WAV_DECODER_IMPLEMENTATION
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

    class WAVDecoder : public mp::IDecoder {
    public:
        WAVDecoder() : file_(nullptr), data_start_(0), data_size_(0) {}
        ~WAVDecoder() override {
            if (file_) {
                file_->close();
                delete file_;
            }
        }

        int probe_file(const void* header, size_t header_size) override {
            if (header_size < 12) return 0;
            const char* data = static_cast<const char*>(header);
            return (std::memcmp(data, "RIFF", 4) == 0 && std::memcmp(data + 8, "WAVE", 4) == 0) ? 100 : 0;
        }

        const char** get_extensions() const override {
            static const char* extensions[] = {"wav", nullptr};
            return extensions;
        }

        mp::Result open_stream(const char* file_path, mp::DecoderHandle* handle) override {
            std::ifstream* file = new std::ifstream(file_path, std::ios::binary);
            if (!file->is_open()) {
                delete file;
                return mp::Result::FileNotFound;
            }

            // Read and validate header
            WAVHeader header;
            file->read(reinterpret_cast<char*>(&header), sizeof(header));
            if (std::memcmp(header.riff, "RIFF", 4) != 0 || std::memcmp(header.wave, "WAVE", 4) != 0) {
                delete file;
                return mp::Result::InvalidFormat;
            }

            // Skip to format chunk
            while (true) {
                char chunk_id[4];
                uint32_t chunk_size;
                file->read(chunk_id, 4);
                file->read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));

                if (std::memcmp(chunk_id, "fmt ", 4) == 0) {
                    WAVFormat format;
                    file->read(reinterpret_cast<char*>(&format), std::min(sizeof(format), chunk_size));

                    stream_info_.sample_rate = format.sample_rate;
                    stream_info_.channels = format.channels;
                    stream_info_.bits_per_sample = format.bits_per_sample;
                    stream_info_.total_samples = 0;  // Will be set from data chunk

                    break;
                } else {
                    file->seekg(chunk_size, std::ios::cur);
                }
            }

            // Find data chunk
            while (true) {
                char chunk_id[4];
                uint32_t chunk_size;
                file->read(chunk_id, 4);
                file->read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));

                if (std::memcmp(chunk_id, "data", 4) == 0) {
                    data_start_ = file->tellg();
                    data_size_ = chunk_size;
                    stream_info_.total_samples = data_size_ / (stream_info_.bits_per_sample / 8) / stream_info_.channels;
                    break;
                } else {
                    file->seekg(chunk_size, std::ios::cur);
                }
            }

            file_ = file;
            handle->internal = this;
            current_pos_ = 0;

            return mp::Result::Success;
        }

        mp::Result get_stream_info(mp::DecoderHandle handle, mp::AudioStreamInfo* info) override {
            (void)handle;
            *info = stream_info_;
            return mp::Result::Success;
        }

        mp::Result decode_block(mp::DecoderHandle handle, void* buffer, size_t buffer_size, size_t* samples_decoded) override {
            (void)handle;
            if (!file_ || current_pos_ >= data_size_) {
                *samples_decoded = 0;
                return mp::Result::Success;
            }

            size_t bytes_per_sample = stream_info_.bits_per_sample / 8;
            size_t frame_size = bytes_per_sample * stream_info_.channels;
            size_t frames_to_read = buffer_size / frame_size;
            size_t bytes_to_read = std::min(frames_to_read * frame_size, data_size_ - current_pos_);

            file_->read(static_cast<char*>(buffer), bytes_to_read);
            size_t frames_read = file_->gcount() / frame_size;

            current_pos_ += frames_read * frame_size;
            *samples_decoded = frames_read;

            return mp::Result::Success;
        }

        mp::Result seek(mp::DecoderHandle handle, uint64_t position_ms, uint64_t* actual_position) override {
            (void)handle;
            size_t frame = (position_ms * stream_info_.sample_rate) / 1000;
            size_t offset = frame * (stream_info_.bits_per_sample / 8) * stream_info_.channels;

            if (offset > data_size_) {
                offset = data_size_;
            }

            file_->seekg(data_start_ + offset);
            current_pos_ = offset;

            if (actual_position) {
                *actual_position = (current_pos_ * 1000) / (stream_info_.bits_per_sample / 8) / stream_info_.channels / stream_info_.sample_rate;
            }

            return mp::Result::Success;
        }

        mp::Result get_metadata(mp::DecoderHandle handle, const mp::MetadataTag** tags, size_t* count) override {
            (void)handle; (void)tags;
            *count = 0;
            return mp::Result::Success;
        }

        void close_stream(mp::DecoderHandle handle) override {
            (void)handle;
            if (file_) {
                file_->close();
                delete file_;
                file_ = nullptr;
            }
        }

    private:
        std::ifstream* file_;
        mp::AudioStreamInfo stream_info_;
        size_t data_start_;
        size_t data_size_;
        size_t current_pos_;
    };
}

// Simple WAV player that uses proven components

class SimpleWAVPlayer {
private:
    mp::IAudioOutput* audio_output_;
    std::unique_ptr<WAVDecoder> decoder_;
    std::vector<float> audio_buffer_;
    bool is_playing_;

public:
    SimpleWAVPlayer() : audio_output_(nullptr), is_playing_(false) {}

    ~SimpleWAVPlayer() {
        if (audio_output_) {
            audio_output_->stop();
            delete audio_output_;
        }
    }

    bool load_wav(const std::string& filename) {
        std::cout << "Loading WAV file: " << filename << std::endl;

        // Create decoder
        decoder_ = std::make_unique<WAVDecoder>();
        if (!decoder_) {
            std::cerr << "Failed to create decoder" << std::endl;
            return false;
        }

        // Open file
        mp::DecoderHandle handle;
        mp::Result result = decoder_->open_stream(filename.c_str(), &handle);
        if (result != mp::Result::Success) {
            std::cerr << "Failed to open WAV file: " << static_cast<int>(result) << std::endl;
            return false;
        }

        // Get stream info
        mp::AudioStreamInfo info;
        result = decoder_->get_stream_info(handle, &info);
        if (result != mp::Result::Success) {
            std::cerr << "Failed to get stream info" << std::endl;
            return false;
        }

        std::cout << "Audio info: " << info.sample_rate << " Hz, " << info.channels
                  << " channels, " << info.total_samples << " samples" << std::endl;

        // Allocate buffer for entire file (for simplicity)
        size_t total_samples = info.total_samples;
        audio_buffer_.resize(total_samples * info.channels);

        // Decode entire file
        size_t samples_decoded = 0;
        const size_t chunk_size = 4096;
        std::vector<int32_t> temp_chunk(chunk_size * info.channels);

        while (samples_decoded < total_samples) {
            size_t to_decode = std::min(chunk_size, total_samples - samples_decoded);
            size_t chunk_samples = 0;

            result = decoder_->decode_block(handle, temp_chunk.data(),
                                          to_decode * info.channels * sizeof(int32_t),
                                          &chunk_samples);

            if (result != mp::Result::Success || chunk_samples == 0) {
                break;
            }

            // Convert int32 to float and store
            for (size_t i = 0; i < chunk_samples * info.channels; ++i) {
                audio_buffer_[samples_decoded * info.channels + i] =
                    static_cast<float>(temp_chunk[i]) / 2147483648.0f;
            }

            samples_decoded += chunk_samples;
        }

        std::cout << "Decoded " << samples_decoded << " samples" << std::endl;

        // Close decoder
        decoder_->close_stream(handle);

        return samples_decoded > 0;
    }

    bool play() {
        if (audio_buffer_.empty()) {
            std::cerr << "No audio data to play" << std::endl;
            return false;
        }

        std::cout << "Starting playback..." << std::endl;

        // Create audio output with 48kHz system format per analyst report
        audio_output_ = mp::platform::create_platform_audio_output();
        if (!audio_output_) {
            std::cerr << "Failed to create audio output" << std::endl;
            return false;
        }

        mp::AudioOutputConfig config;
        config.device_id = nullptr;
        config.sample_rate = 48000;  // System preferred format
        config.channels = 2;         // Stereo
        config.format = mp::SampleFormat::Float32;
        config.buffer_frames = 1024;
        config.callback = audio_callback_static;
        config.user_data = this;

        mp::Result result = audio_output_->open(config);
        if (result != mp::Result::Success) {
            std::cerr << "Failed to open audio output: " << static_cast<int>(result) << std::endl;
            return false;
        }

        is_playing_ = true;
        result = audio_output_->start();
        if (result != mp::Result::Success) {
            std::cerr << "Failed to start audio output: " << static_cast<int>(result) << std::endl;
            is_playing_ = false;
            return false;
        }

        std::cout << "鉁?Playback started!" << std::endl;
        return true;
    }

    void stop() {
        if (audio_output_ && is_playing_) {
            is_playing_ = false;
            audio_output_->stop();
            std::cout << "Playback stopped" << std::endl;
        }
    }

private:
    static void audio_callback_static(void* buffer, size_t frames, void* user_data) {
        SimpleWAVPlayer* player = static_cast<SimpleWAVPlayer*>(user_data);
        player->audio_callback(static_cast<float*>(buffer), frames);
    }

    void audio_callback(float* buffer, size_t frames) {
        if (!is_playing_ || audio_buffer_.empty()) {
            // Fill with silence
            std::memset(buffer, 0, frames * 2 * sizeof(float));
            return;
        }

        static size_t current_sample = 0;

        for (size_t frame = 0; frame < frames; ++frame) {
            for (int channel = 0; channel < 2; ++channel) {
                if (current_sample * 2 + channel < audio_buffer_.size()) {
                    buffer[frame * 2 + channel] = audio_buffer_[current_sample * 2 + channel];
                } else {
                    buffer[frame * 2 + channel] = 0.0f;  // Silence at end
                }
            }
            current_sample++;

            // Loop the audio
            if (current_sample * 2 >= audio_buffer_.size()) {
                current_sample = 0;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "   Professional Music Player v0.5.0" << std::endl;
    std::cout << "   Simplified Direct WASAPI Architecture" << std::endl;
    std::cout << "========================================" << std::endl;

    SimpleWAVPlayer player;

    if (!player.load_wav(argv[1])) {
        std::cerr << "Failed to load WAV file" << std::endl;
        return 1;
    }

    if (!player.play()) {
        std::cerr << "Failed to start playback" << std::endl;
        return 1;
    }

    std::cout << "Playing for 10 seconds..." << std::endl;

    // Play for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    player.stop();

    return 0;
}