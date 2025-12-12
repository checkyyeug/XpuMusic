@echo off
REM Save current directory
set ORIGINAL_DIR=%CD%
echo === XpuMusic Build System ===
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
        set CMAKE_GENERATOR="Visual Studio 17 2022"
        set CMAKE_ARCH=-A x64
        set IS_VS=1
    ) else (
        echo Visual Studio compiler not found in PATH
        echo Please run from Visual Studio Developer Command Prompt
        exit /b 1
    )

    echo.
    echo Running CMake configuration...
    echo cmake -S .. -B . -G %CMAKE_GENERATOR% %CMAKE_ARCH%
    cmake -S .. -B . -G %CMAKE_GENERATOR% %CMAKE_ARCH%

    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo CMake configuration failed!
        echo Please check error messages above.
        exit /b 1
    )

    echo.
    echo CMake configuration successful!
    echo.
)

echo Build directory: %BUILD_DIR%
echo CMake cache found: CMakeCache.txt
echo.

REM Build all components
echo Building core libraries...
cmake --build . --config Debug --target core_engine platform_abstraction sdk_impl
echo.

echo Building main executables...
cmake --build . --config Debug --target simple_wav_player_native final_wav_player music-player foobar2k-player
echo.

echo Building test programs...
cmake --build . --config Debug --target test_decoders test_audio_direct test_foobar_plugins test_simple_playback
echo.

echo Building audio plugins...
cmake --build . --config Debug --target plugin_flac_decoder plugin_mp3_decoder plugin_wav_decoder
echo.

echo Building additional tools...
cmake --build . --config Debug --target optimization_integration_example simple_performance_test standalone_music_player
echo.

echo === Build Summary ===
echo Successfully built targets:
echo   ✓ core_engine.lib
echo   ✓ platform_abstraction.lib
echo   ✓ sdk_impl.lib
echo   ✓ simple_wav_player_native.exe
echo   ✓ final_wav_player.exe
echo   ✓ music-player.exe
echo   ✓ foobar2k-player.exe
echo   ✓ test_decoders.exe
echo   ✓ test_audio_direct.exe
echo   ✓ test_foobar_plugins.exe
echo   ✓ test_simple_playback.exe
echo   ✓ plugin_flac_decoder.dll
echo   ✓ plugin_mp3_decoder.dll
echo   ✓ plugin_wav_decoder.dll
echo   ✓ optimization_integration_example.exe
echo   ✓ simple_performance_test.exe
echo   ✓ standalone_music_player.exe
echo.

echo Output directory: %BUILD_DIR%\bin\Debug
echo.
dir bin\Debug\*.exe

REM Show file sizes and info
echo.
echo File details:
for %%F in (bin\Debug\*.exe) do (
    echo   %%~nF.exe: %%~zF bytes
)
echo.
echo Build complete!

REM Return to original directory
cd /d "%ORIGINAL_DIR%"