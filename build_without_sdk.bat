@echo off
REM ============================================================================
REM Build Script for XpuMusic - Without problematic SDK implementations
REM ============================================================================

echo ========================================
echo Building XpuMusic (Core Components Only)
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

echo [1/3] Configuring project...
cmake -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

echo [2/3] Building core components...
cmake --build . --config Release --target core_engine
if %ERRORLEVEL% NEQ 0 (
    echo Core engine build failed!
    pause
    exit /b 1
)

cmake --build . --config Release --target platform_abstraction
if %ERRORLEVEL% NEQ 0 (
    echo Platform abstraction build failed!
    pause
    exit /b 1
)

echo [3/3] Building test executables...
cmake --build . --config Release --target test_audio_direct
if %ERRORLEVEL% NEQ 0 (
    echo test_audio_direct build failed!
    pause
    exit /b 1
)

cmake --build . --config Release --target final_wav_player
if %ERRORLEVEL% NEQ 0 (
    echo final_wav_player build failed!
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Built successfully:
if exist "build\bin\Release\*.exe" (
    echo Executables:
    dir /b build\bin\Release\*.exe
)
if exist "build\lib\Release\*.lib" (
    echo Libraries:
    dir /b build\lib\Release\*.lib
)
echo.
echo You can run the test audio player:
echo   build\bin\Release\test_audio_direct.exe
echo.
pause