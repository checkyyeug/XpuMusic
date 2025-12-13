/**
 * @file foobar2000_modern_loader.cpp
 * @brief Modern foobar2000 DLL loader for music-player.exe
 * @date 2025-12-13
 */

#include "foobar2000_modern_loader.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <delayimp.h>
#pragma comment(lib, "delayimp.lib")
#else
#include <dlfcn.h>
#endif

Foobar2000ModernLoader::Foobar2000ModernLoader()
    : initialized_(false) {
#ifdef _WIN32
    shared_lib_ = nullptr;
#endif
}

Foobar2000ModernLoader::~Foobar2000ModernLoader() {
    shutdown();
}

bool Foobar2000ModernLoader::initialize() {
    std::cout << "Initializing foobar2000 modern DLL loader..." << std::endl;

    // Try to load modern foobar2000 components
    if (load_modern_components()) {
        std::cout << "[OK] Successfully loaded foobar2000 modern components" << std::endl;
        initialized_ = true;
        return true;
    }

    std::cout << "[!] foobar2000 components not loaded, using native decoders" << std::endl;
    return false;
}

bool Foobar2000ModernLoader::load_modern_components() {
    const std::vector<std::string> required_dlls = {
        "shared.dll",
        "avcodec-fb2k-62.dll",
        "avformat-fb2k-62.dll",
        "avutil-fb2k-60.dll"
    };

    // Search paths for foobar2000 DLLs
    std::vector<std::string> search_paths = {
        "c:\\Program Files\\foobar2000\\",
        "c:\\Program Files (x86)\\foobar2000\\",
        "./",  // Current directory
        "./components/",
        "../components/"
    };

    // Try each DLL
    for (const auto& dll : required_dlls) {
        bool loaded = false;

        for (const auto& path : search_paths) {
            std::string full_path = path + dll;

#ifdef _WIN32
            HMODULE hModule = LoadLibraryA(full_path.c_str());
            if (hModule) {
                loaded_dlls_.push_back({dll, hModule});
                std::cout << "  [OK] Loaded: " << full_path << std::endl;
                loaded = true;
                break;
            }
#endif
        }

        if (!loaded) {
            std::cout << "  [!] Failed to load: " << dll << std::endl;
            // For some DLLs, it's ok if they're not found
            if (dll == "shared.dll") {
                return false; // shared.dll is essential
            }
        }
    }

    return !loaded_dlls_.empty();
}

bool Foobar2000ModernLoader::can_decode_format(const std::string& extension) const {
    if (!initialized_) return false;

    // Modern foobar2000 supports many formats through its components
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    const std::vector<std::string> supported_formats = {
        "mp3", "flac", "ogg", "m4a", "aac", "wma", "wav",
        "ape", "tak", "wv", "tta", "mpc", "opus", "dsd", "dsf"
    };

    return std::find(supported_formats.begin(), supported_formats.end(), ext)
           != supported_formats.end();
}

Foobar2000ModernLoader::DecoderPtr Foobar2000ModernLoader::create_decoder(
    const std::string& filename) {

    if (!initialized_ || !can_decode_format(get_extension(filename))) {
        return nullptr;
    }

    // Create a wrapper decoder that will use foobar2000 components
    return std::make_unique<Foobar2000DecoderWrapper>(filename, this);
}

std::string Foobar2000ModernLoader::get_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        return filename.substr(dot_pos + 1);
    }
    return "";
}

void Foobar2000ModernLoader::shutdown() {
#ifdef _WIN32
    for (auto& [name, handle] : loaded_dlls_) {
        if (handle) {
            FreeLibrary((HMODULE)handle);
            std::cout << "Unloaded: " << name << std::endl;
        }
    }
    if (shared_lib_) {
        FreeLibrary((HMODULE)shared_lib_);
        shared_lib_ = nullptr;
    }
#endif
    loaded_dlls_.clear();
    initialized_ = false;
}

// Decoder wrapper implementation
Foobar2000DecoderWrapper::Foobar2000DecoderWrapper(
    const std::string& filename,
    Foobar2000ModernLoader* loader)
    : filename_(filename), loader_(loader) {
}

bool Foobar2000DecoderWrapper::open() {
    std::cout << "Opening file with foobar2000: " << filename_ << std::endl;

    // TODO: Implement actual foobar2000 decoding
    // For now, we'll return false to fall back to native decoder

    std::cout << "[!] foobar2000 decoding not implemented yet" << std::endl;
    return false;
}

bool Foobar2000DecoderWrapper::read(char* buffer, size_t size, size_t& bytes_read) {
    // TODO: Implement actual read from foobar2000
    return false;
}

void Foobar2000DecoderWrapper::close() {
    std::cout << "Closed foobar2000 decoder" << std::endl;
}

AudioFormat Foobar2000DecoderWrapper::get_format() const {
    // TODO: Return actual format from foobar2000
    return AudioFormat{44100, 2, 16};
}
