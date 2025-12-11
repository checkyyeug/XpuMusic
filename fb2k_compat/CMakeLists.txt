# foobar2000 兼容层 CMake 配置
# 阶段1：最小可运行框架

cmake_minimum_required(VERSION 3.10)
project(fb2k_compat LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows特定配置
if(WIN32)
    # 启用COM支持
    add_definitions(-D_WIN32_DCOM)
    
    # 设置Windows版本
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7
    add_definitions(-DWINVER=0x0601)
endif()

# 源文件
set(FB2K_COMPAT_SOURCES
    minihost.cpp
    test_minihost.cpp
)

set(FB2K_COMPAT_HEADERS
    minihost.h
)

# 创建可执行文件
add_executable(fb2k_compat_test ${FB2K_COMPAT_SOURCES} ${FB2K_COMPAT_HEADERS})

# 链接库
target_link_libraries(fb2k_compat_test
    ole32      # COM支持
    oleaut32   # 自动化
    uuid       # GUID支持
)

# 包含目录
target_include_directories(fb2k_compat_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../compat
)

# 输出目录
set_target_properties(fb2k_compat_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fb2k_compat"
)

# 安装规则
install(TARGETS fb2k_compat_test
    RUNTIME DESTINATION bin/fb2k_compat
)

# 创建测试脚本
file(WRITE "${CMAKE_BINARY_DIR}/test_fb2k_compat.bat" "
@echo off
echo === foobar2000 兼容层测试 ===
echo.
set FB2K_COMPAT_DIR=%~dp0
set PATH=%FB2K_COMPAT_DIR%;%PATH%
cd /d %FB2K_COMPAT_DIR%
fb2k_compat_test.exe
if errorlevel 1 (
    echo.
    echo 测试失败！
    pause
    exit /b 1
) else (
    echo.
    echo 测试成功！
    pause
)
")

# 创建简单的构建脚本
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/build_test.bat" "
@echo off
echo 正在构建 foobar2000 兼容层测试...
cd /d %~dp0
if not exist build mkdir build
cd build
cmake .. -G \"Visual Studio 16 2019\" -A x64
cmake --build . --config Release
if exist Release\\fb2k_compat_test.exe (
    echo.
    echo 构建成功！运行测试...
    Release\\fb2k_compat_test.exe
) else (
    echo 构建失败！
    pause
)
")