@echo off
REM Play WAV file in background

if "%1"=="" (
    echo Usage: play_wav.bat [wav_file]
    echo.
    echo Example: play_wav.bat 1khz.wav
    goto :eof
)

set WAV_FILE=%1

echo Playing %WAV_FILE%...
echo Press any key to stop...
echo.

REM Start player in background
start /B "" "build\bin\Release\final_wav_player.exe" "%WAV_FILE%"

REM Wait for user input to stop
pause >nul

REM Kill the player process
taskkill /F /IM final_wav_player.exe >nul 2>&1

echo Playback stopped.