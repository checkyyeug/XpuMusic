cmake_minimum_required(VERSION 3.20)
project(MusicPlayerAudioTest VERSION 0.4.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Find ALSA on Linux
if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA QUIET alsa)
        if(ALSA_FOUND)
            set(AUDIO_BACKEND_NAME "ALSA")
            set(AUDIO_LIBS ${ALSA_LIBRARIES})
            set(AUDIO_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
            message(STATUS "Audio Backend: ALSA found")
        else()
            set(AUDIO_BACKEND_NAME "stub")
            set(AUDIO_LIBS "")
            set(AUDIO_INCLUDE_DIRS "")
            message(STATUS "Audio Backend: Using stub (no ALSA)")
        endif()
    else()
        set(AUDIO_BACKEND_NAME "stub")
        message(STATUS "Audio Backend: Using stub (no PkgConfig)")
    endif()
else()
    set(AUDIO_BACKEND_NAME "stub")
    message(STATUS "Audio Backend: Using stub (not Linux)")
endif()

# Create test executable
add_executable(test-audio-backend
    ../src/audio/main.cpp
    ../src/audio/audio_output.cpp
    ../src/audio/audio_output_stub.cpp
)

# Add platform-specific sources
if(UNIX AND NOT APPLE AND ALSA_FOUND)
    target_sources(test-audio-backend PRIVATE ../src/audio/audio_output_alsa.cpp)
    target_compile_definitions(test-audio-backend PRIVATE AUDIO_BACKEND_ALSA)
else()
    target_compile_definitions(test-audio-backend PRIVATE AUDIO_BACKEND_STUB)
endif()

# Link audio libraries
target_link_libraries(test-audio-backend PRIVATE ${AUDIO_LIBS} Threads::Threads)

# Add audio include directories
if(AUDIO_INCLUDE_DIRS)
    target_include_directories(test-audio-backend PRIVATE ${AUDIO_INCLUDE_DIRS})
endif()

# Add current directory for includes
target_include_directories(test-audio-backend PRIVATE ../src/audio)

# Print selected backend
message(STATUS "Building with audio backend: ${AUDIO_BACKEND_NAME}")