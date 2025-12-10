# FindDependencies.cmake - Cross-platform dependency detection
# This module detects and configures platform-specific dependencies

# Function to detect and configure platform
function(configure_platform)
    if(WIN32)
        set(MP_PLATFORM_WINDOWS TRUE PARENT_SCOPE)
        add_definitions(-DMP_PLATFORM_WINDOWS=1)
        message(STATUS "Platform: Windows")

        # Windows-specific libraries
        set(PLATFORM_LIBS ole32 uuid oleaut32 PARENT_SCOPE)

    elseif(APPLE)
        # Check if it's macOS
        if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            set(MP_PLATFORM_MACOS TRUE PARENT_SCOPE)
            add_definitions(-DMP_PLATFORM_MACOS=1)
            message(STATUS "Platform: macOS")

            # macOS-specific frameworks
            find_library(COREAUDIO_FRAMEWORK CoreAudio)
            find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
            find_library(AUDIO_TOOLBOX_FRAMEWORK AudioToolbox)

            set(PLATFORM_LIBS
                ${COREAUDIO_FRAMEWORK}
                ${AUDIOUNIT_FRAMEWORK}
                ${AUDIO_TOOLBOX_FRAMEWORK}
                PARENT_SCOPE
            )
        else()
            set(MP_PLATFORM_APPLE TRUE PARENT_SCOPE)
            add_definitions(-DMP_PLATFORM_APPLE=1)
            message(STATUS "Platform: Apple (non-macOS)")
        endif()

    elseif(UNIX)
        set(MP_PLATFORM_LINUX TRUE PARENT_SCOPE)
        add_definitions(-DMP_PLATFORM_LINUX=1)
        message(STATUS "Platform: Linux")

        # Try to find ALSA
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(ALSA QUIET alsa)
            if(ALSA_FOUND)
                set(HAVE_ALSA TRUE PARENT_SCOPE)
                add_definitions(-DHAVE_ALSA=1)
                set(PLATFORM_LIBS ${ALSA_LIBRARIES} PARENT_SCOPE)
                message(STATUS "ALSA found: ${ALSA_VERSION}")
            endif()
        endif()

        # Try to find PulseAudio as fallback
        if(NOT ALSA_FOUND)
            pkg_check_modules(PULSEAUDIO QUIET libpulse)
            if(PULSEAUDIO_FOUND)
                set(HAVE_PULSEAUDIO TRUE PARENT_SCOPE)
                add_definitions(-DHAVE_PULSEAUDIO=1)
                set(PLATFORM_LIBS ${PULSEAUDIO_LIBRARIES} PARENT_SCOPE)
                message(STATUS "PulseAudio found: ${PULSEAUDIO_VERSION}")
            endif()
        endif()
    endif()
endfunction()

# Function to detect optional libraries
function(detect_optional_libs)
    # FLAC detection
    find_path(FLAC_INCLUDE_DIR FLAC/all.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/homebrew/include
            /opt/local/include
        PATH_SUFFIXES flac
    )

    find_library(FLAC_LIBRARY
        NAMES FLAC
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/homebrew/lib
            /opt/local/lib
    )

    if(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)
        set(HAVE_FLAC TRUE PARENT_SCOPE)
        add_definitions(-DHAVE_FLAC=1)
        set(FLAC_LIBRARIES ${FLAC_LIBRARY} PARENT_SCOPE)
        message(STATUS "FLAC found: ${FLAC_LIBRARY}")
    else()
        message(STATUS "FLAC not found - using stub decoder")
    endif()

    # OGG detection (required for FLAC)
    find_library(OGG_LIBRARY
        NAMES ogg
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/homebrew/lib
            /opt/local/lib
    )

    if(OGG_LIBRARY)
        set(OGG_LIBRARIES ${OGG_LIBRARY} PARENT_SCOPE)
    endif()

    # OpenMP detection
    find_package(OpenMP QUIET)
    if(OpenMP_CXX_FOUND)
        set(HAVE_OPENMP TRUE PARENT_SCOPE)
        add_definitions(-DHAVE_OPENMP=${OpenMP_CXX_VERSION})
        message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
    endif()

    # Threading library detection
    find_package(Threads REQUIRED)
    set(THREAD_LIBS Threads::Threads PARENT_SCOPE)
endfunction()

# Function to configure compiler flags
function(configure_compiler_flags)
    # Architecture detection
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        add_definitions(-DMP_ARCH_X64=1)
        set(MP_ARCH_NAME "x64" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
        add_definitions(-DMP_ARCH_X86=1)
        set(MP_ARCH_NAME "x86" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        add_definitions(-DMP_ARCH_ARM64=1)
        set(MP_ARCH_NAME "ARM64" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        add_definitions(-DMP_ARCH_ARM32=1)
        set(MP_ARCH_NAME "ARM32" PARENT_SCOPE)
    endif()

    # Compiler detection
    if(MSVC)
        add_definitions(-DMP_COMPILER_MSVC=1)
        set(MP_COMPILER_NAME "MSVC" PARENT_SCOPE)
        # MSVC-specific flags
        add_compile_options(/W4 /permissive-)

        # Enable export for Windows DLL
        add_compile_definitions(MP_EXPORTS)

    elseif(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-DMP_COMPILER_GCC=1)
        set(MP_COMPILER_NAME "GCC" PARENT_SCOPE)
        # GCC-specific flags
        add_compile_options(-Wall -Wextra -Wpedantic)

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_definitions(-DMP_COMPILER_CLANG=1)
        set(MP_COMPILER_NAME "Clang" PARENT_SCOPE)
        # Clang-specific flags
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endfunction()

# Function to create platform summary
function(print_platform_summary)
    message(STATUS "")
    message(STATUS "=== Cross-Platform Music Player ===")
    message(STATUS "")
    message(STATUS "Platform Information:")
    message(STATUS "  OS: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "  Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "")

    message(STATUS "Audio Backend:")
    if(WIN32)
        message(STATUS "  ✓ WASAPI (Windows)")
    elseif(APPLE)
        message(STATUS "  ✓ CoreAudio (macOS)")
    else()
        if(HAVE_ALSA)
            message(STATUS "  ✓ ALSA (Linux)")
        elseif(HAVE_PULSEAUDIO)
            message(STATUS "  ✓ PulseAudio (Linux)")
        else()
            message(STATUS "  ⚠ Stub (no audio backend)")
        endif()
    endif()

    message(STATUS "")
    message(STATUS "Optional Dependencies:")

    if(HAVE_FLAC)
        message(STATUS "  ✓ FLAC decoder")
    else()
        message(STATUS "  ⚠ FLAC decoder (stub)")
    endif()

    if(HAVE_OPENMP)
        message(STATUS "  ✓ OpenMP support")
    endif()

    message(STATUS "")
endfunction()