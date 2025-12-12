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

# Compiler-specific settings
if(MSVC)
    # Suppress common warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DUNICODE -D_UNICODE)

    # Disable specific warnings
    add_compile_options(/wd4996)  # Deprecated functions
    add_compile_options(/wd4100)  # Unreferenced formal parameter
    add_compile_options(/wd4189)  # Local variable is initialized but not referenced
    add_compile_options(/wd4244)  # Conversion from 'type1' to 'type2', possible loss of data
    add_compile_options(/wd4245)  # Conversion from 'type1' to 'type2', signed/unsigned mismatch
    add_compile_options(/wd4267)  # 'var' : conversion from 'size_t' to 'type', possible loss of data
    add_compile_options(/wd4995)  # '#pragma warning : disable'

    # Set warning level
    add_compile_options(/W3)
else()
    # GCC/Clang settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-conversion -Wno-shorten-64-to-32")
endif()

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
    # Audio resampling components
    src/audio/sample_rate_converter.cpp
    src/audio/cubic_resampler.cpp
    # src/audio/linear_resampler.cpp  # LinearSampleRateConverter implementation is in sample_rate_converter.cpp
    src/audio/enhanced_sample_rate_converter.cpp
    # Optimized audio processing
    src/audio/optimized_audio_processor.cpp
    src/audio/optimized_format_converter.cpp
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

# SDK Implementations - Simplified version
add_library(sdk_impl STATIC
    compat/sdk_implementations/sdk_impl_simple.cpp
    compat/sdk_implementations/foobar_sdk_wrapper.cpp
    # compat/sdk_implementations/service_base.cpp
    # compat/sdk_implementations/abort_callback.impl.cpp
    # compat/sdk_implementations/file_info_impl.cpp
    # compat/sdk_implementations/audio_chunk_impl.cpp
    # compat/sdk_implementations/metadb_handle_impl.cpp  # 暂时禁用，使用简化版本
)

# Add foobar2000 SDK path
set(FOOBAR2000_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/foobar2000_sdk")
if(EXISTS ${FOOBAR2000_SDK_PATH})
    message(STATUS "Found foobar2000 SDK at: ${FOOBAR2000_SDK_PATH}")
    set(FOOBAR2000_SDK_FOUND TRUE)
else()
    message(WARNING "foobar2000 SDK not found at: ${FOOBAR2000_SDK_PATH}")
    set(FOOBAR2000_SDK_FOUND FALSE)
endif()

target_include_directories(sdk_impl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/sdk_implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/foobar_sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/xpumusic_sdk
)

# Add SDK headers if available
if(FOOBAR2000_SDK_FOUND)
    target_include_directories(sdk_impl PUBLIC
        ${FOOBAR2000_SDK_PATH}
        ${FOOBAR2000_SDK_PATH}/foobar2000/SDK
        ${FOOBAR2000_SDK_PATH}/pfc
        ${FOOBAR2000_SDK_PATH}/libPPUI
    )
endif()

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
        src/music_player_with_resampling.cpp
        src/audio/enhanced_sample_rate_converter.cpp
        src/audio/cubic_resampler.cpp
        src/audio/sample_rate_converter.cpp
        src/audio/universal_sample_rate_converter.cpp
        src/foobar_plugin_manager.cpp
        src/plugin_error_handler.cpp
        src/plugin_config.cpp
        compat/sdk_implementations/foobar_sdk_wrapper.cpp
        # # src/audio/linear_resampler.cpp  # LinearSampleRateConverter implementation is in sample_rate_converter.cpp  # 已包含在 sample_rate_converter.cpp 中
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
    ${PLATFORM_LIBS}
    Threads::Threads
)

# Add sdk_impl dependency only if SDK is available
if(FOOBAR2000_SDK_FOUND)
    target_link_libraries(music-player PRIVATE sdk_impl)
    message(STATUS "Building music-player with foobar2000 SDK support")
else()
    message(WARNING "Building music-player without SDK support - some features may be limited")
endif()

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

# Test Foobar2000 Plugin Manager
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_foobar_plugins.cpp)
    add_executable(test_foobar_plugins
        test_foobar_plugins.cpp
        src/foobar_plugin_manager.cpp
        src/plugin_error_handler.cpp
        src/plugin_config.cpp
        compat/sdk_implementations/foobar_sdk_wrapper.cpp
    )

    target_include_directories(test_foobar_plugins PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/sdk_implementations
    )

    if(WIN32)
        target_link_libraries(test_foobar_plugins ole32)
    endif()
endif()

# Test Plugin Configuration System
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_plugin_config.cpp)
    add_executable(test_plugin_config
        test_plugin_config.cpp
        src/foobar_plugin_manager.cpp
        src/plugin_error_handler.cpp
        src/plugin_config.cpp
        compat/sdk_implementations/foobar_sdk_wrapper.cpp
    )

    target_include_directories(test_plugin_config PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/sdk_implementations
    )

    if(WIN32)
        target_link_libraries(test_plugin_config ole32)
    endif()
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

# Simple Native WAV Player (no dependencies)
add_executable(simple_wav_player_native
    src/simple_wav_player_native.cpp
)

if(WIN32)
    target_link_libraries(simple_wav_player_native winmm)
endif()

# Standalone Music Player with Resampling (no SDK dependencies)
add_executable(standalone_music_player
    src/standalone_music_player.cpp
)

if(WIN32)
    target_link_libraries(standalone_music_player ole32 uuid avrt)
endif()

# Simple WASAPI Player (simpler version)
add_executable(simple_wasapi_player
    src/simple_wasapi_player.cpp
)

if(WIN32)
    target_link_libraries(simple_wasapi_player ole32 uuid)
endif()

# Fixed Standalone Music Player
add_executable(standalone_music_player_fixed
    src/standalone_music_player_fixed.cpp
)

if(WIN32)
    target_link_libraries(standalone_music_player_fixed ole32 uuid avrt)
endif()

# Simple Playback Test
add_executable(test_simple_playback
    test_simple_playback.cpp
)

if(WIN32)
    target_link_libraries(test_simple_playback ole32 uuid)
endif()

# Simple Performance Test
add_executable(simple_performance_test
    src/simple_performance_test.cpp
)

if(WIN32)
    target_link_libraries(simple_performance_test)
endif()

# Optimization Integration Example
add_executable(optimization_integration_example
    src/optimization_integration_example.cpp
)

if(WIN32)
    target_link_libraries(optimization_integration_example)
endif()

# Quick Test Configuration
add_executable(quick_test_config
    quick_test_config.cpp
    src/foobar_plugin_manager.cpp
    src/plugin_config.cpp
    src/plugin_error_handler.cpp
    compat/sdk_implementations/foobar_sdk_wrapper.cpp
)

target_include_directories(quick_test_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    target_link_libraries(quick_test_config ole32)
endif()

# Simple Direct Music Player
add_executable(music_player_simple_direct
    src/music_player_simple_direct.cpp
)

if(WIN32)
    target_link_libraries(music_player_simple_direct ole32 uuid)
endif()

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