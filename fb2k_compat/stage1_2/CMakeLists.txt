# 阶段1.2：功能扩展 CMake 配置
cmake_minimum_required(VERSION 3.10)
project(fb2k_stage1_2 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows特定配置
if(WIN32)
    add_definitions(-D_WIN32_DCOM)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7
    add_definitions(-DWINVER=0x0601)
    add_definitions(-DNOMININMAX)  # 避免min/max宏冲突
endif()

# 源文件
set(STAGE1_2_SOURCES
    audio_chunk.cpp
    dsp_preset_impl.cpp
    dsp_chain_impl.cpp
    output_wasapi.cpp
    integration_test.cpp
)

set(STAGE1_2_HEADERS
    audio_chunk.h
    dsp_interfaces.h
    output_interfaces.h
    output_wasapi.h
)

# 创建测试可执行文件
add_executable(stage1_2_test ${STAGE1_2_SOURCES} ${STAGE1_2_HEADERS})

# 链接库
target_link_libraries(stage1_2_test
    ole32      # COM支持
    oleaut32   # 自动化
    uuid       # GUID支持
    shlwapi    # Windows API函数
    mmdevapi   # 多媒体设备API
)

# 包含目录
target_include_directories(stage1_2_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../stage1_1
    ${CMAKE_CURRENT_SOURCE_DIR}/../compat
)

# 编译选项
if(MSVC)
    target_compile_options(stage1_2_test PRIVATE
        /W4                    # 高警告级别
        /permissive-           # 严格标准符合性
        /Zc:__cplusplus        # 正确的__cplusplus值
    )
else()
    target_compile_options(stage1_2_test PRIVATE
        -Wall -Wextra          # 高警告级别
        -Wpedantic             # 严格标准符合性
    )
endif()

# 输出目录
set_target_properties(stage1_2_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/stage1_2"
)

# 安装规则
install(TARGETS stage1_2_test
    RUNTIME DESTINATION bin/stage1_2
)

# 创建构建脚本
file(WRITE "${CMAKE_BINARY_DIR}/build_stage1_2.bat" "
@echo off
echo ========================================
echo 阶段1.2：功能扩展构建
echo ========================================
echo.

cd /d \"%~dp0\"
if not exist \"build\" mkdir build
cd build

REM 生成Visual Studio项目
cmake .. -G \"Visual Studio 16 2019\" -A x64
cmake --build . --config Release

if exist Release\\stage1_2_test.exe (
    echo.
    echo 构建成功！运行测试...
    Release\\stage1_2_test.exe
) else (
    echo 构建失败！
    pause
)
")

# 创建快速测试脚本
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/quick_test.bat" "
@echo off
echo 快速运行阶段1.2测试...
cd /d \"%~dp0\"
if exist build\\Release\\stage1_2_test.exe (
    build\\Release\\stage1_2_test.exe
) else (
    echo 测试程序不存在，请先构建项目
    echo 运行: build_stage1_2.bat
)
pause
")