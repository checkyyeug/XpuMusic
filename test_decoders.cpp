/**
 * @file test_decoders.cpp
 * @brief Test audio decoders
 * @date 2025-12-11
 */

#include <iostream>
#include <Windows.h>

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  XpuMusic Audio Decoder Test" << std::endl;
    std::cout << "========================================" << std::endl;

    // Test MP3 decoder
    std::cout << "\n1. Testing MP3 Decoder..." << std::endl;
    HMODULE hMP3 = LoadLibraryA("bin/Debug/plugin_mp3_decoder.dll");
    if (hMP3) {
        std::cout << "✓ MP3 decoder plugin loaded" << std::endl;
        auto testMP3 = (void(*)())GetProcAddress(hMP3, "TestMP3Decoder");
        if (testMP3) {
            testMP3();
        }
        FreeLibrary(hMP3);
    } else {
        std::cout << "✗ Failed to load MP3 decoder" << std::endl;
    }

    // Test FLAC decoder
    std::cout << "\n2. Testing FLAC Decoder..." << std::endl;
    HMODULE hFLAC = LoadLibraryA("bin/Debug/plugin_flac_decoder.dll");
    if (hFLAC) {
        std::cout << "✓ FLAC decoder plugin loaded" << std::endl;
        auto testFLAC = (void(*)())GetProcAddress(hFLAC, "TestFLACDecoder");
        if (testFLAC) {
            testFLAC();
        }
        FreeLibrary(hFLAC);
    } else {
        std::cout << "✗ Failed to load FLAC decoder" << std::endl;
    }

    // Test WAV decoder
    std::cout << "\n3. Testing WAV Decoder..." << std::endl;
    HMODULE hWAV = LoadLibraryA("bin/Debug/plugin_wav_decoder.dll");
    if (hWAV) {
        std::cout << "✓ WAV decoder plugin loaded" << std::endl;
        FreeLibrary(hWAV);
    } else {
        std::cout << "✗ Failed to load WAV decoder" << std::endl;
    }

    // Check for audio files
    std::cout << "\n4. Checking Audio Files..." << std::endl;

    WIN32_FIND_DATAA findData;

    // Check MP3 files
    HANDLE hFind = FindFirstFileA("*.mp3", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::cout << "  Found MP3: " << findData.cFileName << std::endl;
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    } else {
        std::cout << "  No MP3 files found" << std::endl;
    }

    // Check FLAC files
    hFind = FindFirstFileA("*.flac", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::cout << "  Found FLAC: " << findData.cFileName << std::endl;
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    } else {
        std::cout << "  No FLAC files found" << std::endl;
    }

    // Check WAV files
    hFind = FindFirstFileA("*.wav", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        int count = 0;
        do {
            if (count < 5) {  // Show only first 5
                std::cout << "  Found WAV: " << findData.cFileName << std::endl;
                count++;
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    } else {
        std::cout << "  No WAV files found" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Test Complete" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}