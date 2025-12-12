/**
 * @file dependency_detector.h
 * @brief Cross-platform dependency detection
 * @date 2025-12-10
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace mp {
namespace core {

// Dependency information structure
struct DependencyInfo {
    std::string name;
    std::string version;
    std::string description;
    bool is_available;
    bool is_required;
    std::string install_hint;
};

// Detector function signature
using DetectorFunc = std::function<DependencyInfo()>;

class DependencyDetector {
public:
    // Singleton instance
    static DependencyDetector& instance() {
        static DependencyDetector detector;
        return detector;
    }

    // Register a dependency detector
    void register_detector(const std::string& name, DetectorFunc detector) {
        detectors_[name] = detector;
    }

    // Detect all dependencies
    std::vector<DependencyInfo> detect_all() {
        std::vector<DependencyInfo> results;

        for (const auto& [name, detector] : detectors_) {
            try {
                results.push_back(detector());
            } catch (const std::exception& e) {
                DependencyInfo info;
                info.name = name;
                info.is_available = false;
                info.description = "Detection failed: " + std::string(e.what());
                results.push_back(info);
            }
        }

        return results;
    }

    // Get specific dependency
    DependencyInfo detect(const std::string& name) {
        auto it = detectors_.find(name);
        if (it != detectors_.end()) {
            return it->second();
        }

        DependencyInfo info;
        info.name = name;
        info.is_available = false;
        info.description = "Unknown dependency";
        return info;
    }

    // Check if all required dependencies are available
    bool check_required_dependencies() {
        for (const auto& [name, detector] : detectors_) {
            DependencyInfo info = detector();
            if (info.is_required && !info.is_available) {
                return false;
            }
        }
        return true;
    }

private:
    std::map<std::string, DetectorFunc> detectors_;
};

// Helper macros for easy registration
#define MP_REGISTER_DEPENDENCY(name, detector_func) \
    namespace { \
        struct DependencyRegistrar_##name { \
            DependencyRegistrar_##name() { \
                mp::core::DependencyDetector::instance().register_detector(#name, detector_func); \
            } \
        }; \
        static DependencyRegistrar_##name g_dependency_registrar_##name; \
    }

// Common dependency detectors
namespace detectors {

// ALSA detection
inline DependencyInfo detect_alsa() {
    DependencyInfo info;
    info.name = "ALSA";
    info.description = "Advanced Linux Sound Architecture";
    info.is_required = false;

#ifdef MP_PLATFORM_LINUX
    // Try to find ALSA headers
    #ifdef HAVE_ALSA
        info.is_available = true;
        #ifdef ALSA_VERSION_STRING
            info.version = ALSA_VERSION_STRING;
        #else
            info.version = "Unknown";
        #endif
    #else
        info.is_available = false;
        info.install_hint = "Install ALSA development libraries:\n"
                           "  Ubuntu/Debian: sudo apt-get install libasound2-dev\n"
                           "  CentOS/RHEL: sudo yum install alsa-lib-devel\n"
                           "  Fedora: sudo dnf install alsa-lib-devel";
    #endif
#else
    info.is_available = false;
    info.description = "ALSA is Linux-specific";
#endif

    return info;
}

// FLAC detection
inline DependencyInfo detect_flac() {
    DependencyInfo info;
    info.name = "FLAC";
    info.description = "Free Lossless Audio Codec library";
    info.is_required = false;

#ifdef HAVE_FLAC
    info.is_available = true;
    #ifdef FLAC_VERSION_STRING
        info.version = FLAC_VERSION_STRING;
    #else
        info.version = "1.x.x";
    #endif
#else
    info.is_available = false;
    info.install_hint = "Install FLAC development libraries:\n"
                       "  Ubuntu/Debian: sudo apt-get install libflac-dev\n"
                       "  CentOS/RHEL: sudo yum install flac-devel\n"
                       "  Fedora: sudo dnf install flac-devel\n"
                       "  macOS: brew install flac\n"
                       "  Windows: vcpkg install flac";
#endif

    return info;
}

// OpenMP detection
inline DependencyInfo detect_openmp() {
    DependencyInfo info;
    info.name = "OpenMP";
    info.description = "OpenMP parallel programming API";
    info.is_required = false;

#ifdef _OPENMP
    info.is_available = true;
    info.version = std::to_string(_OPENMP / 100) + "." +
                   std::to_string((_OPENMP / 10) % 10) + "." +
                   std::to_string(_OPENMP % 10);
#else
    info.is_available = false;
    info.install_hint = "Enable OpenMP support:\n"
                       "  GCC/Clang: add -fopenmp flag\n"
                       "  MSVC: add /openmp flag";
#endif

    return info;
}

// GPU detection (placeholder)
inline DependencyInfo detect_gpu_support() {
    DependencyInfo info;
    info.name = "GPU Support";
    info.description = "GPU acceleration support (Vulkan/OpenGL)";
    info.is_required = false;

#ifdef HAVE_VULKAN
    info.is_available = true;
    info.version = "Vulkan";
#elif defined(HAVE_OPENGL)
    info.is_available = true;
    info.version = "OpenGL";
#else
    info.is_available = false;
    info.description = "Not implemented yet";
    info.install_hint = "GPU support is planned for future versions";
#endif

    return info;
}

} // namespace detectors

// Register common detectors
MP_REGISTER_DEPENDENCY(alsa, detectors::detect_alsa)
MP_REGISTER_DEPENDENCY(flac, detectors::detect_flac)
MP_REGISTER_DEPENDENCY(openmp, detectors::detect_openmp)
MP_REGISTER_DEPENDENCY(gpu, detectors::detect_gpu_support)

} // namespace core
} // namespace mp