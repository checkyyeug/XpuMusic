/**
 * @file flac_decoder_stub.cpp
 * @brief FLAC decoder stub (placeholder until FLAC is installed)
 * @date 2025-12-11
 */

#include "../../sdk/headers/mp_types.h"
#include <iostream>
#include <fstream>

namespace {

class FLACDecoder : public mp::IDecoder {
private:
    bool is_open_;

public:
    FLACDecoder() : is_open_(false) {
        std::cout << "[FLAC] FLAC decoder initialized (stub version)" << std::endl;
    }

    ~FLACDecoder() override {
        close_stream({});
    }

    bool open_stream(const char* filename, mp::DecoderHandle& handle) override {
        std::cout << "[FLAC] Attempting to open: " << filename << std::endl;

        // Check if it's a FLAC file by extension
        std::string file_str(filename);
        if (file_str.size() < 5 ||
            file_str.substr(file_str.size() - 5) != ".flac") {
            std::cerr << "[FLAC] Not a FLAC file" << std::endl;
            return false;
        }

        // For now, just show a message
        std::cerr << "[FLAC] ERROR: FLAC support not available" << std::endl;
        std::cerr << "[FLAC] To enable FLAC support:" << std::endl;
        std::cerr << "[FLAC]   1. Install FLAC development libraries" << std::endl;
        std::cerr << "[FLAC]   2. On Windows: download from https://xiph.org/downloads/" << std::endl;
        std::cerr << "[FLAC]   3. On Linux: sudo apt-get install libflac-dev" << std::endl;
        std::cerr << "[FLAC]   4. On macOS: brew install flac" << std::endl;

        return false;
    }

    void close_stream(const mp::DecoderHandle& handle) override {
        is_open_ = false;
    }

    mp::Result decode_block(const mp::DecoderHandle& handle,
                           void* buffer,
                           uint64_t samples_requested,
                           uint64_t* samples_decoded) override {
        *samples_decoded = 0;
        return mp::Result::Error;
    }

    mp::Result seek(const mp::DecoderHandle& handle,
                   uint64_t sample_pos,
                   uint64_t* actual_pos) override {
        *actual_pos = 0;
        return mp::Result::Error;
    }
};

class FLACDecoderPlugin : public mp::IPlugin {
public:
    const char* get_name() const override {
        return "FLAC Decoder (Stub)";
    }

    const char* get_version() const override {
        return "0.1.0";
    }

    const char* get_author() const override {
        return "XpuMusic Team";
    }

    mp::Result create_decoder(mp::IDecoder** decoder) override {
        *decoder = new FLACDecoder();
        return mp::Result::OK;
    }

    void destroy() override {
        delete this;
    }
};

} // anonymous namespace

// Plugin entry point
extern "C" __declspec(dllexport) mp::IPlugin* CreatePlugin() {
    std::cout << "[FLAC] Creating FLAC decoder plugin (stub)..." << std::endl;
    return new FLACDecoderPlugin();
}