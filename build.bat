@echo off
setlocal enabledelayedexpansion
REM ============================================================================
REM Build Script for XpuMusic - Windows Batch File
REM Includes both Release and Debug build configurations
REM ============================================================================

echo Starting XpuMusic Build Process...
echo.

REM Check if build directory exists, create if not
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Detect available build system
echo Detecting build system...
set CMAKE_GENERATOR=
set IS_VS=0

where cl >nul 2>nul
if !ERRORLEVEL! EQU 0 (
    set CMAKE_GENERATOR="Visual Studio 16 2019"
    set IS_VS=1
    echo Found MSVC compiler, using Visual Studio generator
) else (
    where g++ >nul 2>nul
    if !ERRORLEVEL! EQU 0 (
        set CMAKE_GENERATOR="MinGW Makefiles"
        echo Found MinGW/GCC compiler, using MinGW Makefiles
    ) else (
        where clang++ >nul 2>nul
        if !ERRORLEVEL! EQU 0 (
            set CMAKE_GENERATOR="NMake Makefiles"
            echo Found Clang compiler, using NMake Makefiles
        ) else (
            echo Error: No supported C++ compiler found ^(MSVC, GCC, or Clang^)
            echo Please install a C++ compiler toolchain
            echo.
            echo Recommended options:
            echo 1. Install Visual Studio 2019/2022 with C++ development tools
            echo 2. Install MinGW-w64
            echo 3. Install LLVM/Clang
            pause
            exit /b 1
        )
    )
)

REM ============================================================================
REM Release Build
REM ============================================================================
echo ========================================
echo Building Release Configuration
echo ========================================

cd build

REM Create release build directory
if not exist "release" (
    echo Creating release build directory...
    mkdir release
)

cd release

echo Running CMake for Release build...
cmake -G %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=Release ../.. || (
    echo CMake Release configuration failed!
    pause
    exit /b 1
)

echo Building Release targets...
if !IS_VS! EQU 1 (
    cmake --build . --config Release --parallel
) else if "!CMAKE_GENERATOR!"=="MinGW Makefiles" (
    mingw32-make -j%NUMBER_OF_PROCESSORS%
) else (
    nmake
)

if !ERRORLEVEL! NEQ 0 (
    echo Release build failed!
    pause
    exit /b 1
)

echo Release build completed successfully!
echo.

REM Go back to main build directory
cd ..

REM ============================================================================
REM Debug Build
REM ============================================================================
echo ========================================
echo Building Debug Configuration
echo ========================================

REM Create debug build directory
if not exist "debug" (
    echo Creating debug build directory...
    mkdir debug
)

cd debug

echo Running CMake for Debug build...
cmake -G %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=Debug ../.. || (
    echo CMake Debug configuration failed!
    pause
    exit /b 1
)

echo Building Debug targets...
if !IS_VS! EQU 1 (
    cmake --build . --config Debug --parallel
) else if "!CMAKE_GENERATOR!"=="MinGW Makefiles" (
    mingw32-make -j%NUMBER_OF_PROCESSORS%
) else (
    nmake
)

if !ERRORLEVEL! NEQ 0 (
    echo Debug build failed!
    pause
    exit /b 1
)

echo Debug build completed successfully!
echo.

REM Go back to project root
cd ..

echo ========================================
echo Build Summary
echo ========================================
echo.
echo Release binaries available at: build\release\bin\
echo Debug binaries available at: build\debug\bin\
echo.
echo Main executables:
echo   - music-player.exe
echo   - test_audio_direct.exe
echo   - test_cross_platform.exe
echo   - final_wav_player.exe
echo.
echo Libraries:
echo   - core_engine.lib (static)
echo   - platform_abstraction.lib (static)
echo   - sdk_impl.lib (static)
echo   - plugin_*_decoder.dll (dynamic)
echo.
echo Build process completed successfully!
echo.
pause