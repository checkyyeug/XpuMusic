@echo off
REM Windows 构建脚本

echo ========================================
echo   Windows 构建脚本 - XpuMusic
echo ========================================
echo.

REM 检查 Visual Studio 环境
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] 未找到 Visual Studio 编译器
    echo 请运行 vcvarsall.bat 或使用 "Developer Command Prompt for VS"
    echo.
    echo 示例:
    echo   "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    exit /b 1
)

echo [OK] 检测到 Visual Studio 环境
echo.

REM 创建构建目录
if not exist build (
    mkdir build
)

cd build

REM 配置项目
echo [1/2] 配置项目...
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake 配置失败
    exit /b 1
)
echo [OK] 项目配置成功
echo.

REM 构建项目
echo [2/2] 构建项目...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] 构建失败
    exit /b 1
)
echo [OK] 构建成功
echo.

REM 显示结果
echo ========================================
echo   构建完成！
echo ========================================
echo.
echo 可执行文件位于: build\Release\music-player.exe
echo.
echo 运行示例:
echo   cd Release
echo   music-player.exe your_audio_file.wav
echo.
pause