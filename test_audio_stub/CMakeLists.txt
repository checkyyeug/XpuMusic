cmake_minimum_required(VERSION 3.20)
project(MusicPlayerAudioStubTest VERSION 0.4.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Force stub backend
set(AUDIO_BACKEND_NAME "stub")
set(AUDIO_LIBS "")
set(AUDIO_INCLUDE_DIRS "")

# Create test executable
add_executable(test-audio-stub
    ../src/audio/main.cpp
    ../src/audio/audio_output.cpp
    ../src/audio/audio_output_stub.cpp
)

# Use stub backend only
target_compile_definitions(test-audio-stub PRIVATE AUDIO_BACKEND_STUB)

# Link libraries
target_link_libraries(test-audio-stub PRIVATE Threads::Threads)

# Add include directories
target_include_directories(test-audio-stub PRIVATE ../src/audio)

# Print selected backend
message(STATUS "Building with audio backend: ${AUDIO_BACKEND_NAME}")