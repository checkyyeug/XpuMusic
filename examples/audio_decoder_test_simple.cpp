/**
 * @file audio_decoder_test_simple.cpp
 * @brief 简单测试音频解码器系统
 * @date 2025-12-10
 */

#include "../core/audio_decoder_manager.h"
#include "../plugins/decoders/mp3_decoder_impl.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>

// Use simple map instead of nlohmann/json
using json_map = std::map<std::string, std::string>;

using namespace xpumusic;

void print_format_info(const xpumusic::core::AudioFormatInfo& info) {
    std::cout << "\n=== Audio Format Info ===" << std::endl;
    std::cout << "Format: " << info.format << std::endl;
    std::cout << "Extension: " << info.extension << std::endl;
    std::cout << "MIME Type: " << info.mime_type << std::endl;
    std::cout << "Lossless: " << (info.lossless ? "Yes" : "No") << std::endl;
    std::cout << "Codec: " << info.codec << std::endl;
    std::cout << "Container: " << info.container << std::endl;
    std::cout << "Supported: " << (info.supported ? "Yes" : "No") << std::endl;
}

void print_metadata(const json_map& metadata) {
    std::cout << "\n=== Metadata ===" << std::endl;
    if (metadata.empty()) {
        std::cout << "No metadata available" << std::endl;
        return;
    }

    for (const auto& [key, value] : metadata) {
        std::cout << std::setw(15) << std::left << key << ": " << value << std::endl;
    }
}

void test_decoding(const std::string& file_path) {
    std::cout << "\n=== Testing: " << file_path << " ===" << std::endl;

    // Check if file exists
    std::ifstream f(file_path);
    if (!f.good()) {
        std::cout << "Error: File not found!" << std::endl;
        return;
    }
    f.close();

    // Initialize manager
    auto& manager = xpumusic::core::AudioDecoderManager::get_instance();
    manager.initialize();

    // Manually register MP3 decoder
    auto& registry = xpumusic::core::AudioDecoderRegistry::get_instance();
    auto mp3_factory = std::make_unique<xpumusic::plugins::MP3DecoderFactory>();
    registry.register_decoder("MP3 Decoder", std::move(mp3_factory));

    // Detect format
    auto format_info = manager.detect_format(file_path);
    print_format_info(format_info);

    if (!format_info.supported) {
        std::cout << "Format not supported!" << std::endl;
        return;
    }

    // Get decoder
    auto decoder = manager.get_decoder_for_file(file_path);
    if (!decoder) {
        std::cout << "Failed to get decoder!" << std::endl;
        return;
    }

    // Open file
    if (!decoder->open(file_path)) {
        std::cout << "Failed to open file!" << std::endl;
        std::cout << "Error: " << decoder->get_last_error() << std::endl;
        return;
    }

    std::cout << "\nDecoder: " << decoder->get_info().name << std::endl;
    std::cout << "Version: " << decoder->get_info().version << std::endl;

    // Get format
    xpumusic::AudioFormat format = decoder->get_format();
    std::cout << "\nAudio Format:" << std::endl;
    std::cout << "Sample Rate: " << format.sample_rate << " Hz" << std::endl;
    std::cout << "Channels: " << format.channels << std::endl;
    std::cout << "Bits per Sample: " << format.bits_per_sample << std::endl;
    std::cout << "Is Float: " << (format.is_float ? "Yes" : "No") << std::endl;

    // Get metadata using manager
    auto metadata = manager.get_metadata(file_path);
    print_metadata(metadata);

    // Get duration
    double duration = decoder->get_duration();
    std::cout << "\nDuration: " << duration << " seconds" << std::endl;

    // Test decoding a few frames
    std::cout << "\nTesting decode..." << std::endl;
    static std::vector<float> temp_buffer(4096);
    xpumusic::AudioBuffer buffer;
    buffer.data = temp_buffer.data();
    buffer.channels = format.channels;

    int total_frames = 0;
    for (int i = 0; i < 5; i++) {
        buffer.frames = 0;
        int frames = decoder->decode(buffer, 1024);
        if (frames <= 0) {
            if (frames < 0) {
                std::cout << "Decode error at frame " << i << std::endl;
            } else {
                std::cout << "End of stream reached" << std::endl;
            }
            break;
        }
        total_frames += frames;
        std::cout << "Decoded " << frames << " frames (total: " << total_frames << ")" << std::endl;
    }

    // Test seeking
    std::cout << "\nTesting seek..." << std::endl;
    if (decoder->seek(1.0)) {
        std::cout << "Seeked to 1.0 second successfully" << std::endl;
    } else {
        std::cout << "Seek failed" << std::endl;
    }

    // Close decoder
    decoder->close();
}

int main(int argc, char* argv[]) {
    std::cout << "=== Audio Decoder Test Program ===" << std::endl;
    std::cout << "Note: This test requires MP3 files to work properly" << std::endl;

    if (argc < 2) {
        std::cout << "\nUsage: " << argv[0] << " <audio_file>" << std::endl;
        std::cout << "Example: " << argv[0] << " test.mp3" << std::endl;
        return 1;
    }

    std::string file_path = argv[1];
    test_decoding(file_path);

    return 0;
}