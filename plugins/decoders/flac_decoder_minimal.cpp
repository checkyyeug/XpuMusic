/**
 * @file flac_decoder_minimal.cpp
 * @brief Minimal FLAC decoder stub
 * @date 2025-12-11
 */

#include <iostream>

// Simple C-style export
extern "C" {
    __declspec(dllexport) void TestFLACDecoder() {
        std::cout << "\n=== XpuMusic FLAC Decoder ===" << std::endl;
        std::cout << "Status: Stub implementation" << std::endl;
        std::cout << "Note: FLAC support requires:" << std::endl;
        std::cout << "  1. FLAC development libraries" << std::endl;
        std::cout << "  2. Integration with SDK" << std::endl;
        std::cout << "===============================\n" << std::endl;
    }

    __declspec(dllexport) int GetFLACInfo() {
        std::cerr << "[FLAC] To enable FLAC support:" << std::endl;
        std::cerr << "  Windows: Download from https://xiph.org/downloads/" << std::endl;
        std::cerr << "  Linux: sudo apt-get install libflac-dev" << std::endl;
        std::cerr << "  macOS: brew install flac" << std::endl;
        return 0; // FLAC not available
    }
}

// Minimal implementation to satisfy linking
namespace {
    struct Dummy {
        Dummy() { std::cout << "[FLAC] Decoder module loaded (stub)" << std::endl; }
        ~Dummy() { std::cout << "[FLAC] Decoder module unloaded" << std::endl; }
    };
    static Dummy dummy;
}