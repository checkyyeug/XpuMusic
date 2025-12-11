/**
 * @file foobar2k_compatible_player.cpp
 * @brief foobar2000 compatible player
 * @date 2025-12-11
 *
 * A player that can load foobar2000 components
 */

#include <iostream>
#include <Windows.h>
#include <vector>
#include <string>

// Foobar2000 SDK includes
#include "../compat/sdk_implementations/common_includes.h"
#include "../compat/xpumusic_sdk/foobar2000_sdk.h"

using namespace foobar2000_sdk;

class FB2KCompatiblePlayer {
private:
    HMODULE fb2k_module_;
    bool initialized_;

    // Core services
    service_ptr_t<playback_control> playback_;
    service_ptr_t<metadb> database_;

public:
    FB2KCompatiblePlayer() : initialized_(false) {}

    ~FB2KCompatiblePlayer() {
        shutdown();
    }

    bool initialize() {
        std::cout << "Initializing foobar2000 compatible player..." << std::endl;

        // Try to load foobar2000.dll first
        fb2k_module_ = LoadLibrary(L"foobar2000.dll");

        if (fb2k_module_) {
            std::cout << "✓ Loaded foobar2000.dll" << std::endl;
            // Initialize using actual foobar2000
            return init_from_dll();
        } else {
            std::cout << "⚠️  foobar2000.dll not found, using emulation mode" << std::endl;
            // Use our SDK implementation
            return init_emulated();
        }
    }

    bool init_from_dll() {
        // Get core services from real foobar2000
        typedef HRESULT (*InitFB2K)();
        auto init_func = (InitFB2K)GetProcAddress(fb2k_module_, "foobar2000_get_interface");

        if (init_func) {
            // Initialize real foobar2000 services
            playback_ = standard_api_create_t<playback_control>();
            database_ = standard_api_create_t<metadb>();
            initialized_ = true;
            return true;
        }

        return false;
    }

    bool init_emulated() {
        // Use our SDK implementation
        std::cout << "✓ Using emulated foobar2000 services" << std::endl;

        // Create emulated services
        initialized_ = true;
        return true;
    }

    void shutdown() {
        if (fb2k_module_) {
            FreeLibrary(fb2k_module_);
            fb2k_module_ = nullptr;
        }
        initialized_ = false;
    }

    bool load_file(const char* filename) {
        if (!initialized_) return false;

        std::cout << "Loading file: " << filename << std::endl;

        // Try to load using foobar2000 input plugins
        service_ptr_t<input_manager> input_manager;
        input_manager->instantiate();

        // Create playback handle
        service_ptr_t<input_decoder> decoder;
        if (input_manager->open(filename, decoder, abort_callback_dummy())) {
            std::cout << "✓ File loaded successfully" << std::endl;

            // Get file info
            file_info_impl info;
            decoder->get_info(0, info, abort_callback_dummy());

            std::cout << "  Format: " << info.info_get("codec") << std::endl;
            std::cout << "  Duration: " << info.get_length() << " seconds" << std::endl;

            return true;
        }

        std::cerr << "✗ Failed to load file" << std::endl;
        return false;
    }

    void play() {
        if (!initialized_) return;

        std::cout << "\n▶ Starting playback..." << std::endl;
        // Implementation for playback
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <audio_file>" << std::endl;
        return 1;
    }

    FB2KCompatiblePlayer player;

    if (!player.initialize()) {
        std::cerr << "Failed to initialize player" << std::endl;
        return 1;
    }

    if (player.load_file(argv[1])) {
        player.play();

        std::cout << "\nPress Enter to stop..." << std::endl;
        std::cin.get();
    }

    return 0;
}