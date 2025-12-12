@echo off
REM 简单直接编译脚本 - 不需要 CMake
REM 编译一个基本的 WAV 播放器

echo ========================================
echo   XpuMusic 直接编译脚本
echo ========================================
echo.

REM 检查 Visual Studio 环境
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 未找到 Visual Studio 编译器
    echo 请运行 vcvarsall.bat 或使用 "Developer Command Prompt for VS"
    echo.
    echo 示例:
    echo   "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    pause
    exit /b 1
)

echo [OK] 检测到 Visual Studio 环境
echo.

REM 创建输出目录
if not exist bin mkdir bin

echo 正在编译简单 WAV 播放器...

REM 编译简单的 WAV 播放器
cl /EHsc /O2 /std:c++17 ^
   /I"src" ^
   /I"src/audio" ^
   /I"src/platform" ^
   /D"WIN32" /D"_WINDOWS" /D"NDEBUG" ^
   /Fe:"bin\simple_wav_player.exe" ^
   src\music_player_simple.cpp ^
   user32.lib winmm.lib kernel32.lib

if %ERRORLEVEL% NEQ 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo.
echo [OK] 编译成功！
echo.
echo 可执行文件: bin\simple_wav_player.exe
echo.
echo 使用方法:
echo   simple_wav_player.exe audio_file.wav
echo.
pause