/**
 * @file platform_utils.h
 * @brief Cross-platform detection and utilities
 * @date 2025-12-10
 */

#pragma once

#include <string>
#include <iostream>

namespace mp {
namespace core {

// Platform detection macros
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define MP_PLATFORM_WINDOWS 1
    #define MP_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define MP_PLATFORM_MACOS 1
        #define MP_PLATFORM_NAME "macOS"
    #elif TARGET_OS_IPHONE
        #define MP_PLATFORM_IOS 1
        #define MP_PLATFORM_NAME "iOS"
    #else
        #define MP_PLATFORM_APPLE 1
        #define MP_PLATFORM_NAME "Apple"
    #endif
#elif defined(__linux__) || defined(__linux)
    #define MP_PLATFORM_LINUX 1
    #define MP_PLATFORM_NAME "Linux"
#elif defined(__unix__) || defined(__unix)
    #define MP_PLATFORM_UNIX 1
    #define MP_PLATFORM_NAME "Unix"
#else
    #define MP_PLATFORM_UNKNOWN 1
    #define MP_PLATFORM_NAME "Unknown"
#endif

// Architecture detection
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
    #define MP_ARCH_X64 1
    #define MP_ARCH_NAME "x64"
#elif defined(_M_IX86) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    #define MP_ARCH_X86 1
    #define MP_ARCH_NAME "x86"
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define MP_ARCH_ARM64 1
    #define MP_ARCH_NAME "ARM64"
#elif defined(_M_ARM) || defined(__arm__) || defined(__thumb__)
    #define MP_ARCH_ARM32 1
    #define MP_ARCH_NAME "ARM32"
#else
    #define MP_ARCH_UNKNOWN 1
    #define MP_ARCH_NAME "Unknown"
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define MP_COMPILER_MSVC 1
    #define MP_COMPILER_NAME "MSVC"
#elif defined(__clang__)
    #define MP_COMPILER_CLANG 1
    #define MP_COMPILER_NAME "Clang"
#elif defined(__GNUC__)
    #define MP_COMPILER_GCC 1
    #define MP_COMPILER_NAME "GCC"
#else
    #define MP_COMPILER_UNKNOWN 1
    #define MP_COMPILER_NAME "Unknown"
#endif

// Platform-specific includes and exports
#ifdef MP_PLATFORM_WINDOWS
    #define MP_EXPORT __declspec(dllexport)
    #define MP_IMPORT __declspec(dllimport)
    #define MP_API_CALL __stdcall
    #include <windows.h>
#else
    #define MP_EXPORT __attribute__((visibility("default")))
    #define MP_IMPORT
    #define MP_API_CALL
#endif

// Default visibility for symbols
#ifndef MP_API
    #ifdef MP_PLATFORM_WINDOWS
        #ifdef MP_EXPORTS
            #define MP_API MP_EXPORT
        #else
            #define MP_API MP_IMPORT
        #endif
    #else
        #define MP_API __attribute__((visibility("default")))
    #endif
#endif

// Platform utilities
struct PlatformInfo {
    std::string platform;
    std::string architecture;
    std::string compiler;
    bool is_windows;
    bool is_macos;
    bool is_linux;
    bool is_unix;

    static PlatformInfo get_current() {
        PlatformInfo info;
        info.platform = MP_PLATFORM_NAME;
        info.architecture = MP_ARCH_NAME;
        info.compiler = MP_COMPILER_NAME;

#ifdef MP_PLATFORM_WINDOWS
        info.is_windows = true;
        info.is_macos = false;
        info.is_linux = false;
        info.is_unix = false;
#elif defined(MP_PLATFORM_MACOS)
        info.is_windows = false;
        info.is_macos = true;
        info.is_linux = false;
        info.is_unix = true;
#elif defined(MP_PLATFORM_LINUX)
        info.is_windows = false;
        info.is_macos = false;
        info.is_linux = true;
        info.is_unix = true;
#else
        info.is_windows = false;
        info.is_macos = false;
        info.is_linux = false;
        info.is_unix = false;
#endif

        return info;
    }

    void print_info() const {
        std::cout << "Platform Information:" << std::endl;
        std::cout << "  Platform: " << platform << std::endl;
        std::cout << "  Architecture: " << architecture << std::endl;
        std::cout << "  Compiler: " << compiler << std::endl;
        std::cout << "  Audio Backend: ";
        if (is_windows) std::cout << "WASAPI";
        else if (is_macos) std::cout << "CoreAudio";
        else if (is_linux) std::cout << "ALSA";
        else std::cout << "None";
        std::cout << std::endl;
    }
};

// Convenience macros
#define MP_IS_WINDOWS() (defined(MP_PLATFORM_WINDOWS))
#define MP_IS_MACOS() (defined(MP_PLATFORM_MACOS))
#define MP_IS_LINUX() (defined(MP_PLATFORM_LINUX))
#define MP_IS_UNIX() (defined(MP_PLATFORM_UNIX) || defined(MP_PLATFORM_MACOS) || defined(MP_PLATFORM_LINUX))

}} // namespace mp::core