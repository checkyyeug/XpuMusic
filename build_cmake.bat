@echo off
REM ============================================================================
REM Simple CMake Build Script for XpuMusic
REM ============================================================================

echo CMake Build Script for XpuMusic
echo ================================
echo.

REM Check if CMake is installed
cmake --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/download/
    echo Make sure to add CMake to system PATH during installation
    pause
    exit /b 1
)

echo CMake found successfully!
echo.

REM Create and enter build directory
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Configure with CMake
echo.
echo Configuring project with CMake...
echo.

REM Try different generators
echo Trying Visual Studio generator...
cmake -G "Visual Studio 17 2022" -A x64 .. 2>nul
if %ERRORLEVEL% EQU 0 goto :BUILD

echo Trying Visual Studio 2019 generator...
cmake -G "Visual Studio 16 2019" -A x64 .. 2>nul
if %ERRORLEVEL% EQU 0 goto :BUILD

echo Trying MinGW Makefiles...
cmake -G "MinGW Makefiles" .. 2>nul
if %ERRORLEVEL% EQU 0 goto :BUILD

echo Trying Unix Makefiles...
cmake -G "Unix Makefiles" .. 2>nul
if %ERRORLEVEL% EQU 0 goto :BUILD

echo Trying default generator...
cmake ..
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to configure project with CMake
    echo.
    echo Make sure you have a C++ compiler installed:
    echo - Visual Studio (with C++ workload)
    echo - MinGW-w64
    echo - Or another C++ compiler
    echo.
    pause
    exit /b 1
)

:BUILD
echo.
echo CMake configuration successful!
echo.

REM Build the project
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo.
echo Executables are located in:
if exist "Release" (
    echo   build\Release\
) else (
    echo   build\
)
echo.
echo Main executable: music-player.exe
echo.
pause