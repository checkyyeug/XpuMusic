/**
 * @file test_mp3_decoder.cpp
 * @brief Test MP3 decoder functionality
 * @date 2025-12-11
 */

#include <iostream>
#include <Windows.h>

typedef struct {
    const char* (*name)();
    int (*test)();
} PluginTest;

int main() {
    std::cout << "=== XpuMusic MP3 Decoder Test ===" << std::endl;
    std::cout << std::endl;

    // Load the MP3 decoder plugin
    HMODULE hModule = LoadLibrary(L"build/bin/Debug/plugin_mp3_decoder.dll");
    if (!hModule) {
        std::cerr << "Error: Failed to load MP3 decoder plugin" << std::endl;
        std::cerr << "Make sure the plugin is built and in the correct location" << std::endl;
        return 1;
    }

    std::cout << "✓ MP3 decoder plugin loaded successfully" << std::endl;

    // Get test function
    auto testFunc = (void(*)())GetProcAddress(hModule, "TestMP3Decoder");
    if (testFunc) {
        testFunc();
    }

    // Test with a file if available
    std::cout << "\nChecking for MP3 files..." << std::endl;

    // Try to find test_440hz.mp3
    bool mp3Found = false;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("*.mp3", &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::cout << "  Found: " << findData.cFileName << std::endl;
            mp3Found = true;
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    if (!mp3Found) {
        std::cout << "  No MP3 files found in current directory" << std::endl;
        std::cout << "  To test MP3 decoding:" << std::endl;
        std::cout << "    1. Place an MP3 file in the current directory" << std::endl;
        std::cout << "    2. Run: test_mp3_decoder.exe [filename.mp3]" << std::endl;
    }

    FreeLibrary(hModule);
    return 0;
}