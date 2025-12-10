# DSP Resampler Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.20)
project(dsp_resampler_plugin LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件
set(PLUGIN_SOURCES
    dsp_resampler.cpp
)

# 创建插件库
add_library(dsp_resampler_plugin SHARED
    ${PLUGIN_SOURCES}
)

# 包含目录
target_include_directories(dsp_resampler_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_SOURCE_DIR}/src
)

# 链接库
target_link_libraries(dsp_resampler_plugin PRIVATE
    xpumusic_sdk
)

# 设置输出目录 - 将插件放在统一的插件目录下
set_target_properties(dsp_resampler_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)

# 平台特定设置
if(WIN32)
    # Windows特定设置
    set_target_properties(dsp_resampler_plugin PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
    # macOS特定设置
    set_target_properties(dsp_resampler_plugin PROPERTIES
        PREFIX ""
        SUFFIX ".dylib"
    )
else()
    # Linux和其他Unix系统
    set_target_properties(dsp_resampler_plugin PROPERTIES
        PREFIX ""
        SUFFIX ".so"
    )
endif()

# 输出信息
message(STATUS "  ✓ DSP Resampler Plugin")