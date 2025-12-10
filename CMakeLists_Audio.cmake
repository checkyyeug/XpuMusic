# Top-level CMakeLists.txt with audio backend detection

cmake_minimum_required(VERSION 3.20)
project(MusicPlayer VERSION 0.4.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include audio backend detection module
include(cmake/AudioBackend.cmake)

# Find required packages
find_package(Threads REQUIRED)

# Example target that uses audio
add_executable(music-player-new
    src/audio/main.cpp  # 替换为你的主程序文件
)

# Add audio sources
target_sources(music-player-new PRIVATE ${AUDIO_SOURCES})

# Link audio libraries
target_link_libraries(music-player-new PRIVATE ${AUDIO_LIBS} Threads::Threads)

# Add audio include directories
if(AUDIO_INCLUDE_DIRS)
    target_include_directories(music-player-new PRIVATE ${AUDIO_INCLUDE_DIRS})
endif()

# Print selected backend
message(STATUS "Building with audio backend: ${AUDIO_BACKEND_NAME}")