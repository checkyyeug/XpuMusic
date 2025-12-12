@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM Set UTF-8 code page to handle Chinese characters
set ORIGINAL_DIR=%CD%
echo === XpuMusic UTF-8 Build System ===
echo.

REM Set the correct build directory
set BUILD_DIR=%~dp0build

REM Check if build directory exists
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

REM Go to build directory
cd /d "%BUILD_DIR%"

REM Check if this is a valid CMake build directory
if not exist "CMakeCache.txt" (
    echo CMake cache not found. Running configuration...
    echo.
    echo Detecting compiler...

    REM Detect Visual Studio
    where cl >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo Found: Microsoft Visual C++ Compiler
        set CMAKE_GENERATOR="Visual Studio 16 2019"
        set CMAKE_ARCH=-A x64
        set IS_VS=1
    ) else (
        echo Visual Studio compiler not found in PATH
        echo Please run from Visual Studio Developer Command Prompt
        pause
        exit /b 1
    )

    echo.
    echo Running CMake configuration...
    echo cmake -S .. -B . -G %CMAKE_GENERATOR% %CMAKE_ARCH% -DCMAKE_CXX_STANDARD=17
    cmake -S .. -B . -G %CMAKE_GENERATOR% %CMAKE_ARCH% -DCMAKE_CXX_STANDARD=17

    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo CMake configuration failed!
        echo Please check error messages above.
        pause
        exit /b 1
    )

    echo.
    echo CMake configuration successful!
    echo.
)

echo Build directory: %BUILD_DIR%
echo CMake cache found: CMakeCache.txt
echo.

REM Skip problematic targets by default
set SKIP_PROBLEMATIC=1

echo Building core components...
cmake --build . --config Debug --target core_engine platform_abstraction
if %ERRORLEVEL% NEQ 0 (
    echo Warning: Core components build had issues
)
echo.

echo Building test programs...
cmake --build . --config Debug --target test_decoders test_audio_direct
if %ERRORLEVEL% NEQ 0 (
    echo Warning: Test programs build had issues
)
echo.

echo Building audio plugins...
cmake --build . --config Debug --target plugin_flac_decoder plugin_mp3_decoder plugin_wav_decoder
if %ERRORLEVEL% NEQ 0 (
    echo Warning: Plugin build had issues
)
echo.

echo ========================================
echo   Build Summary
echo ========================================
echo.

echo Built targets:
if exist "lib\Debug\core_engine.lib" echo   [OK] core_engine.lib
if exist "lib\Debug\platform_abstraction.lib" echo   [OK] platform_abstraction.lib
if exist "bin\Debug\test_decoders.exe" echo   [OK] test_decoders.exe
if exist "bin\Debug\test_audio_direct.exe" echo   [OK] test_audio_direct.exe
if exist "bin\Debug\plugin_flac_decoder.dll" echo   [OK] plugin_flac_decoder.dll
if exist "bin\Debug\plugin_mp3_decoder.dll" echo   [OK] plugin_mp3_decoder.dll
if exist "bin\Debug\plugin_wav_decoder.dll" echo   [OK] plugin_wav_decoder.dll

echo.
echo Skipped targets due to dependencies:
echo   - music-player.exe (SDK dependencies)
echo   - sdk_impl.lib (Missing foobar2000 SDK headers)
echo   - foobar2k-player.exe (SDK dependencies)
echo.

echo Output directory: %BUILD_DIR%\bin\Debug
echo.
if exist "bin\Debug\*.exe" (
    echo Executables found:
    dir bin\Debug\*.exe
)

echo.
echo Build complete!
echo.
echo To run test programs:
echo   cd bin\Debug
echo   test_decoders.exe
echo   test_audio_direct.exe
echo.

REM Return to original directory
cd /d "%ORIGINAL_DIR%"
pause