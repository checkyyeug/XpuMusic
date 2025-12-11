# ============================================================================
# Professional Music Player - Cross-Platform CMakeLists.txt
# Working version for immediate testing
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(MusicPlayer VERSION 0.3.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Threads REQUIRED)

# Platform detection and configuration
if(WIN32)
    set(MP_PLATFORM_WINDOWS TRUE)
    add_definitions(-DMP_PLATFORM_WINDOWS=1)
    message(STATUS "Platform: Windows")
    set(PLATFORM_LIBS ole32 uuid)
elseif(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(MP_PLATFORM_MACOS TRUE)
        add_definitions(-DMP_PLATFORM_MACOS=1)
        message(STATUS "Platform: macOS")
    endif()
elseif(UNIX)
    set(MP_PLATFORM_LINUX TRUE)
    add_definitions(-DMP_PLATFORM_LINUX=1)
    message(STATUS "Platform: Linux")

    # Try to find ALSA
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA QUIET alsa)
        if(ALSA_FOUND)
            set(HAVE_ALSA TRUE)
            add_definitions(-DHAVE_ALSA=1)
            set(PLATFORM_LIBS ${ALSA_LIBRARIES})
            message(STATUS "ALSA found: ${ALSA_VERSION}")
        else()
            message(WARNING "ALSA not found - using stub audio")
        endif()
    endif()
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_definitions(-DMP_ARCH_X64=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
    add_definitions(-DMP_ARCH_X86=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    add_definitions(-DMP_ARCH_ARM64=1)
endif()

# Compiler detection
if(MSVC)
    add_definitions(-DMP_COMPILER_MSVC=1)
    add_compile_options(/W4)
    add_compile_definitions(MP_EXPORTS)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-DMP_COMPILER_GCC=1)
    add_compile_options(-Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions(-DMP_COMPILER_CLANG=1)
    add_compile_options(-Wall -Wextra)
endif()

# ============================================================================
# Core Targets
# ============================================================================

# Core Engine Library
add_library(core_engine STATIC
    core/core_engine.cpp
    core/service_registry.cpp
    core/event_bus.cpp
    core/plugin_host.cpp
    core/config_manager.cpp
    core/playlist_manager.cpp
    core/playback_engine.cpp
    core/visualization_engine.cpp
)

target_include_directories(core_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/headers
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_engine PRIVATE
    ${PLATFORM_LIBS}
    Threads::Threads
)

# Platform Abstraction Layer
add_library(platform_abstraction STATIC
    platform/audio_output_factory.cpp
)

target_include_directories(platform_abstraction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(platform_abstraction PRIVATE ${PLATFORM_LIBS})

# Add Linux ALSA stub if needed
if(MP_PLATFORM_LINUX)
    target_sources(platform_abstraction PRIVATE
        platform/linux/audio_output_stub.cpp
    )
endif()

# ============================================================================
# Compatibility Layer
# ============================================================================

# SDK Implementations
add_library(sdk_impl STATIC
    compat/sdk_implementations/service_base.cpp
    compat/sdk_implementations/abort_callback.impl.cpp
    compat/sdk_implementations/file_info_impl.cpp
    compat/sdk_implementations/audio_chunk_impl.cpp
    # compat/sdk_implementations/metadb_handle_impl.cpp  # 暂时禁用，使用简化版本
)

target_include_directories(sdk_impl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/sdk_implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/foobar_sdk
)

# ============================================================================
# Plugin Ecosystem
# ============================================================================

# Always build essential plugins
add_library(plugin_wav_decoder SHARED
    plugins/decoders/wav_decoder.cpp
)

target_include_directories(plugin_wav_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/headers
)

# MP3 Decoder (always available with minimp3)
add_library(plugin_mp3_decoder SHARED
    plugins/decoders/mp3_decoder_basic.cpp
)

target_include_directories(plugin_mp3_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/external/minimp3
)

# FLAC Decoder (stub - requires FLAC dev libs)
add_library(plugin_flac_decoder SHARED
    plugins/decoders/flac_decoder_minimal.cpp
)

target_include_directories(plugin_flac_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/headers
)

# ============================================================================
# Main Executables
# ============================================================================

# Main Music Player
if(WIN32)
    add_executable(music-player
        src/music_player_legacy.cpp
    )
else()
    add_executable(music-player
        src/music_player_simple.cpp
    )
endif()

target_include_directories(music-player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
)

target_link_libraries(music-player PRIVATE
    core_engine
    platform_abstraction
    sdk_impl
    ${PLATFORM_LIBS}
    Threads::Threads
)

# Windows audio libraries
if(WIN32)
    target_link_libraries(music-player PRIVATE ole32 uuid)
endif()

# Foobar2000 Compatible Player (Experimental)
add_executable(foobar2k-player
    src/foobar2k_compatible_player.cpp
)

target_include_directories(foobar2k-player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/sdk_implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/xpumusic_sdk
)

target_link_libraries(foobar2k-player PRIVATE
    core_engine
    platform_abstraction
    sdk_impl
    ${PLATFORM_LIBS}
    Threads::Threads
)

if(WIN32)
    target_link_libraries(foobar2k-player PRIVATE ole32 uuid)
endif()

# Test audio executable
add_executable(test_audio_direct
    src/play_sound.cpp
)

# Test decoders executable
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_decoders.cpp)
    add_executable(test_decoders
        test_decoders.cpp
    )
else()
    # Use a simple test program if test_decoders.cpp doesn't exist
    add_executable(test_decoders
        test_simple.cpp
    )
endif()

target_include_directories(test_audio_direct PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

target_link_libraries(test_audio_direct PRIVATE
    platform_abstraction
    ${PLATFORM_LIBS}
)

# Cross-platform test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_cross_platform.cpp)
    add_executable(test_cross_platform
        test_cross_platform.cpp
        platform/audio_output_factory.cpp
    )
else()
    # Skip this target if test_cross_platform.cpp doesn't exist
    message(STATUS "Skipping test_cross_platform - source file not found")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_cross_platform.cpp)
    target_include_directories(test_cross_platform PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/sdk/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
        ${CMAKE_CURRENT_SOURCE_DIR}/compat
    )

    target_link_libraries(test_cross_platform PRIVATE ${PLATFORM_LIBS})

    if(MP_PLATFORM_LINUX)
        target_sources(test_cross_platform PRIVATE
            platform/linux/audio_output_stub.cpp
        )
    endif()
endif()

# WAV Player Test
add_executable(final_wav_player
    src/final_wav_player.cpp
)

target_include_directories(final_wav_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/sdk_implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/foobar_sdk
)

target_link_libraries(final_wav_player PRIVATE Threads::Threads)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "=== Cross-Platform Music Player ===")
message(STATUS "")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")

message(STATUS "Audio Backend:")
if(WIN32)
    message(STATUS "  ✓ WASAPI (Windows)")
elseif(APPLE)
    message(STATUS "  ✓ CoreAudio (macOS)")
else()
    if(HAVE_ALSA)
        message(STATUS "  ✓ ALSA (Linux)")
    else()
        message(STATUS "  ⚠ Stub (Linux - no ALSA)")
    endif()
endif()

message(STATUS "")
message(STATUS "Plugins:")
message(STATUS "  ✓ WAV decoder")
message(STATUS "  ✓ MP3 decoder (minimp3)")
message(STATUS "  ✓ FLAC decoder (stub)")
message(STATUS "")
message(STATUS "✅ Configuration complete!")