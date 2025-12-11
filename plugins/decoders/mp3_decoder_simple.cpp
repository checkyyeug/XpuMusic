/**
 * @file mp3_decoder_simple.cpp
 * @brief Simple MP3 decoder for testing
 * @date 2025-12-11
 */

// Disable minimp3 warnings for now
#pragma warning(push)
#pragma warning(disable: 4244 4267 4456)
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include "../../sdk/external/minimp3/minimp3_ex.h"
#pragma warning(pop)

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

// Plugin info structure
struct PluginInfo {
    char name[256];
    char version[64];
    char author[256];
    char description[512];
};

// Simple plugin interface
typedef struct {
    void* (*init)();
    void (*destroy)();
    bool (*can_decode)(const char* filename);
    int (*decode_file)(const char* filename);
} IPluginSimple;

// Global plugin info
static PluginInfo g_plugin_info = {
    "MP3 Decoder (minimp3)",
    "1.0.0",
    "XpuMusic Team",
    "MP3 audio decoder using minimp3 library"
};

// Check if file is MP3 by extension
bool can_decode_mp3(const char* filename) {
    if (!filename) return false;

    std::string file(filename);
    std::transform(file.begin(), file.end(), file.begin(), ::tolower);

    return (file.find(".mp3") != std::string::npos);
}

// Decode MP3 file (simplified)
int decode_mp3_file(const char* filename) {
    if (!filename) return -1;

    std::cout << "[MP3] Attempting to decode: " << filename << std::endl;

    mp3dec_ex_t decoder;
    if (mp3dec_ex_open(&decoder, filename, MP3D_SEEK_TO_SAMPLE) != 0) {
        std::cerr << "[MP3] Error: Failed to open MP3 file" << std::endl;
        return -1;
    }

    std::cout << "[MP3] Successfully opened MP3 file:" << std::endl;
    std::cout << "  Sample Rate: " << decoder.info.hz << " Hz" << std::endl;
    std::cout << "  Channels: " << decoder.info.channels << std::endl;
    std::cout << "  Duration: " << decoder.samples / (decoder.info.hz * decoder.info.channels)
              << " seconds" << std::endl;

    // For now, just read a small portion to test
    const int buffer_size = 1024;
    float buffer[buffer_size];

    size_t samples_read = mp3dec_ex_read(&decoder, buffer, buffer_size);

    std::cout << "[MP3] Read " << samples_read << " samples successfully" << std::endl;

    mp3dec_ex_close(&decoder);
    return 0;
}

// Plugin initialization
void* plugin_init() {
    std::cout << "[MP3] MP3 decoder plugin initialized" << std::endl;
    return &g_plugin_info;
}

// Plugin cleanup
void plugin_destroy() {
    std::cout << "[MP3] MP3 decoder plugin destroyed" << std::endl;
}

// Export plugin functions
extern "C" __declspec(dllexport) {
    // Get plugin info
    const PluginInfo* __cdecl GetPluginInfo() {
        return &g_plugin_info;
    }

    // Check if can decode
    bool __cdecl CanDecode(const char* filename) {
        return can_decode_mp3(filename);
    }

    // Test decode
    int __cdecl TestDecode(const char* filename) {
        return decode_mp3_file(filename);
    }

    // Simple test function
    void __cdecl TestMP3Decoder() {
        std::cout << "\n[MP3] Testing MP3 decoder..." << std::endl;
        std::cout << "  Name: " << g_plugin_info.name << std::endl;
        std::cout << "  Version: " << g_plugin_info.version << std::endl;
        std::cout << "  Author: " << g_plugin_info.author << std::endl;
        std::cout << "  Description: " << g_plugin_info.description << std::endl;
        std::cout << "  Status: Active\n" << std::endl;
    }
}