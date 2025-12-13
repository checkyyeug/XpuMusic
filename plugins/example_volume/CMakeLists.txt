cmake_minimum_required(VERSION 3.20)
project(volume_plugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build plugin as shared library
add_library(volume_plugin MODULE
    volume_plugin.cpp
    volume_plugin.h
)

# Include XpuMusic headers
target_include_directories(volume_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/compat
)

# Set output properties
set_target_properties(volume_plugin PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(volume_plugin PRIVATE VOLUME_PLUGIN_EXPORTS)
endif()

# Enable optimization
if(MSVC)
    target_compile_options(volume_plugin PRIVATE /O2)
else()
    target_compile_options(volume_plugin PRIVATE -O3)
endif()