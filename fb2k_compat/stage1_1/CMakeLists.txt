# 阶段1.1：真实组件集成 CMake 配置
cmake_minimum_required(VERSION 3.10)
project(fb2k_real_compat LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows特定配置
if(WIN32)
    add_definitions(-D_WIN32_DCOM)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7
    add_definitions(-DWINVER=0x0601)
    add_definitions(-DNOMINMAX)  # 避免min/max宏冲突
endif()

# 源文件
set(REAL_COMPAT_SOURCES
    real_minihost.cpp
    test_real_component.cpp
)

set(REAL_COMPAT_HEADERS
    real_minihost.h
)

# 创建可执行文件
add_executable(fb2k_real_test ${REAL_COMPAT_SOURCES} ${REAL_COMPAT_HEADERS})

# 链接库
target_link_libraries(fb2k_real_test
    ole32      # COM支持
    oleaut32   # 自动化
    uuid       # GUID支持
    shlwapi    # Windows API函数
)

# 包含目录
target_include_directories(fb2k_real_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../compat
)

# 输出目录
set_target_properties(fb2k_real_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/stage1_1"
)

# 编译选项
if(MSVC)
    target_compile_options(fb2k_real_test PRIVATE
        /W4                    # 高警告级别
        /permissive-           # 严格标准符合性
        /Zc:__cplusplus        # 正确的__cplusplus值
    )
else()
    target_compile_options(fb2k_real_test PRIVATE
        -Wall -Wextra          # 高警告级别
        -Wpedantic             # 严格标准符合性
    )
endif()

# 安装规则
install(TARGETS fb2k_real_test
    RUNTIME DESTINATION bin/stage1_1
)

# 创建测试脚本
file(WRITE "${CMAKE_BINARY_DIR}/test_real_component.bat" "
@echo off
echo === foobar2000 真实组件测试 ===
echo.
cd /d \"%~dp0\"
set FB2K_TEST_DIR=%~dp0
set PATH=%FB2K_TEST_DIR%;%PATH%
cd /d %FB2K_TEST_DIR%
fb2k_real_test.exe
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

# 创建构建脚本
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/build_real.bat" "
@echo off
echo ========================================
echo 阶段1.1：真实组件集成构建
echo ========================================
echo.

cd /d \"%~dp0\"
if not exist \"build\" mkdir build
cd build

REM 生成Visual Studio项目
cmake .. -G \"Visual Studio 16 2019\" -A x64
cmake --build . --config Release

if exist Release\\fb2k_real_test.exe (
    echo.
    echo 构建成功！运行测试...
    Release\\fb2k_real_test.exe
) else (
    echo 构建失败！
    pause
)
")