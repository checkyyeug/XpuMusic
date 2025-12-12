/**
 * @file mp3_decoder_fixed.cpp
 * @brief Fixed MP3 decoder plugin using minimp3
 * @date 2025-12-11
 */

// Ensure we define implementation before including
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT

#include "../../sdk/headers/mp_types.h"
#include "../../sdk/external/minimp3/minimp3_ex.h"

#include <fstream>
#include <vector>
#include <cstring>
#include <iostream>

namespace {

// Simple MP3 decoder using minimp3
class MP3Decoder : public mp::IDecoder {
private:
    mp3dec_t decoder_;
    mp3dec_file_info_t file_info_;
    std::vector<uint8_t> file_data_;
    size_t current_sample_;
    bool is_open_;

public:
    MP3Decoder() : current_sample_(0), is_open_(false) {
        std::cout << "[MP3] MP3 Decoder initialized" << std::endl;
        mp3dec_init(&decoder_);
    }

    ~MP3Decoder() override {
        close_stream({});
    }

    bool open_stream(const char* filename, mp::DecoderHandle& handle) override {
        std::cout << "[MP3] Opening file: " << filename << std::endl;

        // Read entire file (simplified approach)
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[MP3] Failed to open file" << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        file_data_.resize(file_size);
        file.read(reinterpret_cast<char*>(file_data_.data()), file_size);
        file.close();

        // Decode MP3 info
        int samples = mp3dec_load(&decoder_, file_data_.data(), file_data_.size(),
                                &file_info_, NULL, NULL);

        if (samples <= 0) {
            std::cerr << "[MP3] Failed to decode MP3" << std::endl;
            return false;
        }

        // Fill audio stream info
        handle.info.sample_rate = file_info.hz;
        handle.info.channels = file_info.channels;
        handle.info.format = mp::SampleFormat::Float32;
        handle.info.duration = static_cast<double>(samples) / (file_info.hz * file_info.channels);

        current_sample_ = 0;
        is_open_ = true;

        std::cout << "[MP3] Successfully opened:" << std::endl;
        std::cout << "  Sample Rate: " << file_info.hz << " Hz" << std::endl;
        std::cout << "  Channels: " << file_info.channels << std::endl;
        std::cout << "  Samples: " << samples << std::endl;
        std::cout << "  Duration: " << handle.info.duration << " seconds" << std::endl;

        return true;
    }

    void close_stream(const mp::DecoderHandle& handle) override {
        if (is_open_) {
            file_data_.clear();
            is_open_ = false;
            std::cout << "[MP3] Stream closed" << std::endl;
        }
    }

    mp::Result decode_block(const mp::DecoderHandle& handle,
                           void* buffer,
                           uint64_t samples_requested,
                           uint64_t* samples_decoded) override {
        if (!is_open_ || !buffer) {
            *samples_decoded = 0;
            return mp::Result::Error;
        }

        // For simplicity, we'll decode all data at once
        // In a real implementation, you'd want streaming decode
        if (current_sample_ >= file_info.samples) {
            *samples_decoded = 0;
            return mp::Result::EndOfStream;
        }

        // Decode remaining samples
        uint64_t samples_to_decode = std::min(
            samples_requested,
            (uint64_t)(file_info.samples - current_sample_)
        );

        // Copy already decoded data
        float* output = static_cast<float*>(buffer);
        const float* input = reinterpret_cast<const float*>(file_info.buffer);

        size_t start_idx = current_sample_ * file_info.channels;
        size_t end_idx = start_idx + samples_to_decode * file_info.channels;

        if (end_idx > file_info.samples * file_info.channels) {
            end_idx = file_info.samples * file_info.channels;
            samples_to_decode = (end_idx - start_idx) / file_info.channels;
        }

        for (size_t i = start_idx; i < end_idx; i++) {
            output[i - start_idx] = input[i];
        }

        current_sample_ += samples_to_decode;
        *samples_decoded = samples_to_decode;

        return mp::Result::OK;
    }

    mp::Result seek(const mp::DecoderHandle& handle,
                   uint64_t sample_pos,
                   uint64_t* actual_pos) override {
        if (!is_open_) {
            *actual_pos = 0;
            return mp::Result::Error;
        }

        if (sample_pos > file_info.samples) {
            sample_pos = file_info.samples;
        }

        current_sample_ = sample_pos;
        *actual_pos = sample_pos;

        return mp::Result::OK;
    }
};

// Plugin factory
class MP3DecoderPlugin : public mp::IPlugin {
public:
    const char* get_name() const override {
        return "MP3 Decoder (minimp3)";
    }

    const char* get_version() const override {
        return "1.0.0";
    }

    const char* get_author() const override {
        return "XpuMusic Team";
    }

    mp::Result create_decoder(mp::IDecoder** decoder) override {
        *decoder = new MP3Decoder();
        return mp::Result::OK;
    }

    void destroy() override {
        delete this;
    }
};

} // anonymous namespace

// Plugin entry point
extern "C" __declspec(dllexport) mp::IPlugin* CreatePlugin() {
    std::cout << "[MP3] Creating MP3 decoder plugin..." << std::endl;
    return new MP3DecoderPlugin();
}