@echo off
echo ========================================
echo foobar2000 兼容层简化构建脚本
echo ========================================
echo.

cd /d "%~dp0"

REM 直接使用cl编译器构建简单版本
echo [1/3] 检查编译器...
where cl >nul 2>&1
if errorlevel 1 (
    echo 错误: 找不到 Visual Studio 编译器
    echo 请运行 "Developer Command Prompt for VS" 后再执行此脚本
    pause
    exit /b 1
)

echo [2/3] 创建输出目录...
if not exist "build" mkdir build

echo [3/3] 编译项目...
cl /std:c++17 /EHsc /O2 /MD /I. /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 ^
   minihost.cpp test_minihost.cpp ^
   /Fe:build\fb2k_compat_test.exe ^
   ole32.lib oleaut32.lib uuid.lib

if errorlevel 1 (
    echo 错误: 编译失败
    pause
    exit /b 1
)

echo.
echo 编译成功！
echo 运行测试程序...
echo.
cd build
fb2k_compat_test.exe

echo.
echo ========================================
echo 构建和测试完成！
echo ========================================
pause