@echo off
REM Save current directory
set ORIGINAL_DIR=%CD%

if /i "%1"=="force" goto :force_build

:normal_build
echo === Quick Build for XpuMusic ===
echo.

REM Change to the correct build directory
cd /d "D:\workspaces\foobar\XpuMusic\build"

REM Build the main targets quickly
echo Building music-player.exe...
cmake --build . --config Debug --target music-player

echo Building final_wav_player.exe...
cmake --build . --config Debug --target final_wav_player

goto :complete

:force_build
echo === Quick Build with Force Overwrite ===
call force_build.bat music-player
call force_build.bat final_wav_player

:complete
echo.
echo === Build Complete ===
echo.
echo Available executables:
for %%F in (D:\workspaces\foobar\XpuMusic\build\bin\Debug\*.exe) do (
    echo   %%~nF.exe
)
echo.
echo Usage:
echo   music-player.exe        - Main music player
echo   final_wav_player.exe    - WAV file player (tested working)
echo   test_decoders.exe       - Decoder testing
echo   test_audio_direct.exe   - Audio system testing
echo.
echo Quick test:
echo   final_wav_player.exe 1khz.wav
echo.
echo Usage:
echo   quick_build.bat        - Normal quick build
echo   quick_build.bat force  - Force overwrite build

REM Return to original directory
cd /d "%ORIGINAL_DIR%"