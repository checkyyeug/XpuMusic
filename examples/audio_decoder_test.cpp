/**
 * @file audio_decoder_test.cpp
 * @brief 测试音频解码器系统
 * @date 2025-12-10
 */

#include "../core/audio_decoder_manager.h"
#include <iostream>
#include <iomanip>

using namespace xpumusic::core;

void print_format_info(const AudioFormatInfo& info) {
    std::cout << "\n=== Audio Format Info ===" << std::endl;
    std::cout << "Format: " << info.format << std::endl;
    std::cout << "Extension: " << info.extension << std::endl;
    std::cout << "MIME Type: " << info.mime_type << std::endl;
    std::cout << "Lossless: " << (info.lossless ? "Yes" : "No") << std::endl;
    std::cout << "Codec: " << info.codec << std::endl;
    std::cout << "Container: " << info.container << std::endl;
    std::cout << "Supported: " << (info.supported ? "Yes" : "No") << std::endl;
    if (!info.possible_decoders.empty()) {
        std::cout << "Possible Decoders: ";
        for (const auto& decoder : info.possible_decoders) {
            std::cout << decoder << " ";
        }
        std::cout << std::endl;
    }
}

void print_metadata(const nlohmann::json& metadata) {
    std::cout << "\n=== Metadata ===" << std::endl;
    if (metadata.empty()) {
        std::cout << "No metadata available" << std::endl;
        return;
    }

    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        std::cout << std::setw(15) << std::left << it.key() << ": ";
        if (it.value().is_string()) {
            std::cout << it.value().get<std::string>();
        } else if (it.value().is_number()) {
            std::cout << it.value();
        } else if (it.value().is_boolean()) {
            std::cout << (it.value() ? "true" : "false");
        } else {
            std::cout << it.value().dump();
        }
        std::cout << std::endl;
    }
}

void test_decoding(const std::string& file_path) {
    std::cout << "\n\n=== Testing: " << file_path << " ===" << std::endl;

    // Initialize manager
    auto& manager = AudioDecoderManager::get_instance();
    manager.initialize();

    // Check if file is supported
    bool supported = manager.supports_file(file_path);
    std::cout << "Supported: " << (supported ? "Yes" : "No") << std::endl;

    if (!supported) {
        std::cout << "Skipping unsupported format" << std::endl;
        return;
    }

    // Detect format
    AudioFormatInfo info = manager.detect_format(file_path);
    print_format_info(info);

    // Get metadata
    nlohmann::json metadata = manager.get_metadata(file_path);
    print_metadata(metadata);

    // Get duration
    double duration = manager.get_duration(file_path);
    if (duration >= 0) {
        std::cout << "\nDuration: " << duration << " seconds (";
        int minutes = static_cast<int>(duration) / 60;
        int seconds = static_cast<int>(duration) % 60;
        std::cout << minutes << ":" << std::setw(2) << std::setfill('0') << seconds << ")" << std::endl;
    }

    // Try to open and decode a small portion
    auto decoder = manager.open_audio_file(file_path);
    if (decoder) {
        std::cout << "\nDecoder: " << decoder->get_info().name << std::endl;
        std::cout << "Version: " << decoder->get_info().version << std::endl;

        AudioFormat format = decoder->get_format();
        std::cout << "Sample Rate: " << format.sample_rate << " Hz" << std::endl;
        std::cout << "Channels: " << format.channels << std::endl;
        std::cout << "Bits per Sample: " << format.bits_per_sample << std::endl;
        std::cout << "Float: " << (format.is_float ? "Yes" : "No") << std::endl;

        // Decode a small buffer
        AudioBuffer buffer(format.channels, 1024);
        int frames = decoder->decode(buffer, 1024);
        std::cout << "\nDecoded " << frames << " frames successfully" << std::endl;

        // Test seeking (if supported)
        if (duration > 10) {
            std::cout << "\nTesting seek to 5 seconds..." << std::endl;
            if (decoder->get_info().name == "MP3 Decoder") {
                auto mp3_decoder = dynamic_cast<xpumusic::plugins::MP3Decoder*>(decoder.get());
                if (mp3_decoder) {
                    if (mp3_decoder->seek(5.0)) {
                        std::cout << "Seek successful" << std::endl;
                    } else {
                        std::cout << "Seek failed" << std::endl;
                    }
                }
            } else if (decoder->get_info().name == "FLAC Decoder") {
                auto flac_decoder = dynamic_cast<xpumusic::plugins::FLACDecoder*>(decoder.get());
                if (flac_decoder) {
                    if (flac_decoder->seek(5.0)) {
                        std::cout << "Seek successful" << std::endl;
                    } else {
                        std::cout << "Seek failed" << std::endl;
                    }
                }
            } else if (decoder->get_info().name == "OGG/Vorbis Decoder") {
                auto vorbis_decoder = dynamic_cast<xpumusic::plugins::OggVorbisDecoder*>(decoder.get());
                if (vorbis_decoder) {
                    if (vorbis_decoder->seek(5.0)) {
                        std::cout << "Seek successful" << std::endl;
                    } else {
                        std::cout << "Seek failed" << std::endl;
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== XpuMusic Audio Decoder Test ===" << std::endl;

    // Show all supported formats
    auto& manager = AudioDecoderManager::get_instance();
    manager.initialize();

    std::cout << "\n=== Supported Formats ===" << std::endl;
    auto formats = manager.get_supported_formats();
    for (const auto& format : formats) {
        std::cout << " - " << format << std::endl;
    }

    // Show available decoders
    std::cout << "\n=== Available Decoders ===" << std::endl;
    auto decoders = manager.get_available_decoders();
    for (const auto& decoder : decoders) {
        std::cout << " - " << decoder.name << " v" << decoder.version << std::endl;
        std::cout << "   Description: " << decoder.description << std::endl;
        std::cout << "   Supported formats: ";
        for (const auto& fmt : decoder.supported_formats) {
            std::cout << fmt << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Test files from command line or default files
    std::vector<std::string> test_files;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            test_files.push_back(argv[i]);
        }
    } else {
        // Default test files (if they exist)
        test_files = {
            "test_440hz.wav",
            "loud_1000hz.wav",
            "test.mp3",
            "test.flac",
            "test.ogg"
        };
    }

    // Test each file
    for (const auto& file : test_files) {
        std::ifstream f(file);
        if (f.good()) {
            f.close();
            test_decoding(file);
        } else {
            std::cout << "\n=== File not found: " << file << " ===" << std::endl;
        }
    }

    return 0;
}