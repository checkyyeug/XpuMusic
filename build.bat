@echo off
REM ============================================================================
REM Build Script for XpuMusic
REM ============================================================================

echo ========================================
echo XpuMusic Build Script
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

echo.
echo [1/4] Configuring Release build...
echo.

REM Try different generators in order of preference
set GENERATOR_FOUND=0

REM Try Visual Studio 2022
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release .. >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set GENERATOR_FOUND=1
    echo Using Visual Studio 2022 generator
    goto :BUILD_RELEASE
)

REM Try Visual Studio 2019
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release .. >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set GENERATOR_FOUND=1
    echo Using Visual Studio 2019 generator
    goto :BUILD_RELEASE
)

REM Try MinGW
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release .. >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set GENERATOR_FOUND=1
    echo Using MinGW Makefiles generator
    goto :BUILD_RELEASE
)

REM Try default generator
cmake -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to configure project
    echo.
    echo Please ensure you have a C++ compiler installed:
    echo   - Visual Studio 2019/2022 (recommended)
    echo   - MinGW-w64
    echo   - Or another C++17 compatible compiler
    echo.
    pause
    exit /b 1
)

:BUILD_RELEASE
echo.
echo [2/4] Building Release...
echo.

cmake --build . --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo Release build failed!
    pause
    exit /b 1
)

echo Release build completed successfully!

REM Create debug directory
cd ..
if not exist "build\debug" mkdir debug
cd debug

echo.
echo [3/4] Configuring Debug build...
echo.

REM Try the same generator that worked for Release
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug .. >nul 2>&1
if %ERRORLEVEL% EQU 0 goto :BUILD_DEBUG

cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug .. >nul 2>&1
if %ERRORLEVEL% EQU 0 goto :BUILD_DEBUG

cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug .. >nul 2>&1
if %ERRORLEVEL% EQU 0 goto :BUILD_DEBUG

cmake -DCMAKE_BUILD_TYPE=Debug ..

:BUILD_DEBUG
echo.
echo [4/4] Building Debug...
echo.

cmake --build . --config Debug --parallel
if %ERRORLEVEL% NEQ 0 (
    echo Debug build failed!
    pause
    exit /b 1
)

echo Debug build completed successfully!

REM Go back to project root
cd ..

echo.
echo ========================================
echo Build Summary
echo ========================================
echo.
echo Release build: build\Release\
if exist "build\Release" (
    dir /b build\Release\*.exe 2>nul
)
echo.
echo Debug build: build\Debug\
if exist "build\Debug" (
    dir /b build\Debug\*.exe 2>nul
)
echo.
echo Build completed successfully!
echo.
pause