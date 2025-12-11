@echo off
echo ========================================
echo foobar2000 兼容层阶段1构建脚本
echo ========================================
echo.

REM 检查是否在正确的目录
cd /d "%~dp0"
if not exist "minihost.h" (
    echo 错误: 请在 fb2k_compat 目录下运行此脚本
    pause
    exit /b 1
)

echo [1/4] 创建构建目录...
if not exist "build" mkdir build
cd build

echo [2/4] 生成 Visual Studio 项目...
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo 错误: CMake 生成失败
    pause
    exit /b 1
)

echo [3/4] 构建项目...
cmake --build . --config Release
if errorlevel 1 (
    echo 错误: 构建失败
    pause
    exit /b 1
)

echo [4/4] 运行测试...
cd Release
if exist "fb2k_compat_test.exe" (
    echo.
    echo 构建成功！开始运行兼容层测试...
    echo.
    fb2k_compat_test.exe
    echo.
    echo 测试完成！
) else (
    echo 错误: 找不到测试程序
    pause
    exit /b 1
)

echo.
echo ========================================
echo 阶段1构建和测试完成！
echo ========================================
pause