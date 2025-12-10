# WAV Decoder Plugin CMakeLists.txt

cmake_minimum_required(VERSION 3.20)
project(wav_decoder_plugin VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# 输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# 编译插件
add_library(wav_decoder MODULE
    wav_decoder.cpp
)

# 包含目录
target_include_directories(wav_decoder PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_SOURCE_DIR}/src
)

# 链接库
target_link_libraries(wav_decoder PRIVATE
    ${CMAKE_DL_LIBS}
)

# 编译定义
target_compile_definitions(wav_decoder PRIVATE
    QODER_PLUGIN_BUILD=1
)

# 设置输出名称
set_target_properties(wav_decoder PROPERTIES
    PREFIX ""
    OUTPUT_NAME "wav_decoder"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)

# 安装规则
install(TARGETS wav_decoder
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)