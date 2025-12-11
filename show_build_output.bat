@echo off
REM ============================================================================
REM Show Build Output and Test XpuMusic
REM ============================================================================

echo ========================================
echo  XpuMusic Build Output
echo ========================================
echo.

REM Show executables
echo Executables built:
echo   ------------------
for %%F in (build\bin\Debug\*.exe) do (
    echo   ✓ %%~nF.exe ^(%%~zF bytes^)
)

echo.
echo Audio Plugins:
echo   -------------
for %%F in (build\bin\Debug\*.dll) do (
    echo   ✓ %%~nF.dll ^(%%~zF bytes^)
)

echo.
echo Libraries:
echo   ----------
for %%F in (build\lib\Debug\*.lib) do (
    echo   ✓ %%~nF.lib ^(%%~zF bytes^)
)

echo.
echo ========================================
echo  Quick Test
echo ========================================
echo.

REM Test simple program
echo Running test_decoders.exe...
build\bin\Debug\test_decoders.exe
echo.

REM Check for audio files
echo Checking for test audio files...
if exist test_440hz.wav (
    echo   ✓ Found test_440hz.wav
) else (
    echo   ✗ test_440hz.wav not found
)

if exist test_440hz.mp3 (
    echo   ✓ Found test_440hz.mp3
) else (
    echo   ✗ test_440hz.mp3 not found
)

echo.
echo ========================================
echo  Ready to Play Music!
echo ========================================
echo.
echo Usage:
echo   play_wav.bat                - Play WAV files
echo   play_music.bat              - Play music
echo   music-player.exe [file]     - Run music player
echo   final_wav_player.exe [file] - Simple WAV player
echo.
echo Plugin DLLs are located in: build\bin\Debug\