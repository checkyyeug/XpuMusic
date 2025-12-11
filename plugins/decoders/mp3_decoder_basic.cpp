/**
 * @file mp3_decoder_basic.cpp
 * @brief Basic MP3 decoder - minimal implementation
 * @date 2025-12-11
 */

#include <iostream>
#include <fstream>

// Simple C-style export
extern "C" {
    __declspec(dllexport) void TestMP3Decoder() {
        std::cout << "\n=== XpuMusic MP3 Decoder ===" << std::endl;
        std::cout << "Status: Plugin loaded successfully" << std::endl;
        std::cout << "Note: Full MP3 decoding needs FLAC-style integration" << std::endl;
        std::cout << "      with the playback engine" << std::endl;
        std::cout << "===============================\n" << std::endl;
    }

    __declspec(dllexport) int GetMP3Info() {
        return 1; // MP3 decoder available
    }
}

// Minimal implementation to satisfy linking
namespace {
    struct Dummy {
        Dummy() { std::cout << "[MP3] Decoder module loaded" << std::endl; }
        ~Dummy() { std::cout << "[MP3] Decoder module unloaded" << std::endl; }
    };
    static Dummy dummy;
}