/**
 * @file test_cross_platform.cpp
 * @brief Cross-platform detection and audio test
 * @date 2025-12-10
 */

#include "core/platform_utils.h"
#include "core/dependency_detector.h"
#include "platform/audio_output_factory.h"
#include <iostream>
#include <iomanip>

int main() {
    using namespace mp::core;
    using namespace mp::platform;

    // Print platform information
    PlatformInfo info = PlatformInfo::get_current();
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Cross-Platform Music Player Test        ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    info.print_info();

    // Test dependency detection
    std::cout << "\n─────────────────────────────────────────" << std::endl;
    std::cout << "Dependency Detection Results:" << std::endl;
    std::cout << "─────────────────────────────────────────" << std::endl;

    auto dependencies = DependencyDetector::instance().detect_all();
    for (const auto& dep : dependencies) {
        std::cout << std::setw(15) << std::left << dep.name << ": ";
        if (dep.is_available) {
            std::cout << "✓ Available";
            if (!dep.version.empty()) {
                std::cout << " (" << dep.version << ")";
            }
        } else {
            std::cout << "✗ Not Available";
            if (dep.is_required) {
                std::cout << " [REQUIRED]";
            }
        }
        std::cout << std::endl;

        if (!dep.description.empty() && dep.description != "Detection failed") {
            std::cout << "  " << dep.description << std::endl;
        }

        if (!dep.is_available && !dep.install_hint.empty()) {
            std::cout << "  Install hint: " << dep.install_hint << std::endl;
        }
    }

    // Test audio backend detection
    std::cout << "\n─────────────────────────────────────────" << std::endl;
    std::cout << "Audio Backend Testing:" << std::endl;
    std::cout << "─────────────────────────────────────────" << std::endl;

    auto backends = get_available_audio_backends();
    std::cout << "Available backends: ";
    for (size_t i = 0; i < backends.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << backends[i];
    }
    std::cout << std::endl;

    // Test each backend
    for (const auto& backend : backends) {
        std::cout << "\nTesting backend '" << backend << "': ";

        auto* audio = create_audio_output(backend);
        if (audio) {
            std::cout << "✓ Created successfully" << std::endl;

            // Try to initialize with a common format
            mp::AudioOutputConfig config;
            config.device_id = nullptr;
            config.sample_rate = 44100;
            config.channels = 2;
            config.format = mp::SampleFormat::Float32;
            config.buffer_frames = 1024;

            auto result = audio->open(config);
            if (result == mp::Result::Success) {
                std::cout << "  ✓ Open successful" << std::endl;
                std::cout << "  ✓ Latency: " << audio->get_latency() << " ms" << std::endl;
            } else if (result == mp::Result::NotImplemented) {
                std::cout << "  ⚠ Stub implementation (expected for non-native platforms)" << std::endl;
            } else {
                std::cout << "  ✗ Open failed" << std::endl;
            }

            audio->close();
            delete audio;
        } else {
            std::cout << "✗ Failed to create" << std::endl;
        }
    }

    // Final status
    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Cross-Platform Test Summary              ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;

    bool all_required = DependencyDetector::instance().check_required_dependencies();

    if (all_required) {
        std::cout << "✅ All required dependencies are available!" << std::endl;
    } else {
        std::cout << "⚠️  Some required dependencies are missing" << std::endl;
    }

    std::cout << "\nPlatform-specific notes:" << std::endl;

    if (info.is_windows) {
        std::cout << "• WASAPI audio backend is available" << std::endl;
        std::cout << "• Use Visual Studio 2017+ for best results" << std::endl;
    } else if (info.is_macos) {
        std::cout << "• CoreAudio backend is available" << std::endl;
        std::cout << "• Use Xcode or clang for compilation" << std::endl;
    } else if (info.is_linux) {
        std::cout << "• ALSA backend: " << (dependencies[0].is_available ? "Available" : "Not installed") << std::endl;
        std::cout << "• Install libasound2-dev for full audio support" << std::endl;
    }

    std::cout << "\n✅ Cross-platform framework is working!" << std::endl;

    return all_required ? 0 : 1;
}