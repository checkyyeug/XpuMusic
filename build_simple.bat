@echo off
REM ============================================================================
REM Simple Build Script for XpuMusic
REM ============================================================================

echo.
echo ========================================
echo XpuMusic Simple Build Script
echo ========================================
echo.

REM Check if CMake is installed
cmake --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

echo [1/2] Configuring project...
cmake ..
if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

echo [2/2] Building project...
cmake --build .
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo.
echo Executables created:
dir /b *.exe 2>nul
echo.
cd ..
pause