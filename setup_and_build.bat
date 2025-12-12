@echo off
REM XpuMusic 环境设置和构建脚本

echo ========================================
echo   XpuMusic 环境检测和设置
echo ========================================
echo.

REM 检测可能的 Visual Studio 安装路径
set VS_FOUND=0
set VS_PATH=

REM 检查 VS2022
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    set VS_FOUND=1
    echo [找到] Visual Studio 2022 Community
)

if %VS_FOUND%==0 (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        set VS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
        set VS_FOUND=1
        echo [找到] Visual Studio 2022 Community (x86)
    )
)

REM 检查 VS2019
if %VS_FOUND%==0 (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        set VS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
        set VS_FOUND=1
        echo [找到] Visual Studio 2019 Community
    )
)

if %VS_FOUND%==0 (
    echo [错误] 未找到 Visual Studio 安装
    echo 请先安装 Visual Studio (包含 C++ 开发工具)
    pause
    exit /b 1
)

echo.
echo 正在设置 Visual Studio 环境...
call %VS_PATH% x64

echo.
echo 检查编译器...
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 仍然无法找到编译器
    pause
    exit /b 1
)
echo [OK] 编译器已就绪

echo.
echo ========================================
echo   编译选项
echo ========================================
echo.
echo 1. 安装 CMake 并完整构建项目
echo 2. 直接编译简单播放器（不需要 CMake）
echo 3. 显示手动安装 CMake 的说明
echo.
set /p choice="请选择 (1/2/3): "

if "%choice%"=="1" goto :install_cmake
if "%choice%"=="2" goto :compile_direct
if "%choice%"=="3" goto :show_cmake_instructions
goto :install_cmake

:install_cmake
echo.
echo 正在下载 CMake...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/Kitware/CMake/releases/download/v3.29.0/cmake-3.29.0-windows-x86_64.msi' -OutFile 'cmake-installer.msi'}"

if exist cmake-installer.msi (
    echo [下载完成] 正在安装 CMake...
    start /wait msiexec /i cmake-installer.msi /quiet ADD_CMAKE_TO_PATH=System
    del cmake-installer.msi

    echo.
    echo CMake 安装完成！请重新打开命令提示符并运行构建脚本。
    pause
    exit /b 0
) else (
    echo [错误] 下载失败
    goto :show_cmake_instructions
)

:compile_direct
echo.
echo 正在直接编译简单播放器...
if not exist bin mkdir bin

cl /EHsc /O2 /std:c++17 ^
   /I"src" ^
   /D"WIN32" /D"_WINDOWS" ^
   /Fe:"bin\simple_player.exe" ^
   src\music_player_simple.cpp ^
   user32.lib winmm.lib kernel32.lib ole32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [成功] 编译完成！
    echo 可执行文件: %CD%\bin\simple_player.exe
    echo.
    echo 使用方法:
    echo   cd bin
    echo   simple_player.exe your_audio_file.wav
) else (
    echo [失败] 编译出错
)
pause
exit /b 0

:show_cmake_instructions
echo.
echo ========================================
echo   CMake 手动安装说明
echo ========================================
echo.
echo 1. 访问 https://cmake.org/download/
echo 2. 下载 Windows x64 Installer 版本
echo 3. 运行安装程序
echo 4. 重要：选择 "Add CMake to system PATH" 选项
echo 5. 安装完成后，重新打开命令提示符
echo.
echo 或者使用 Chocolatey 安装：
echo   choco install cmake
echo.
echo 或使用 winget 安装：
echo   winget install Kitware.CMake
echo.
pause