@echo off
REM Play music in background window

if "%1"=="" (
    echo Usage: play_music.bat [wav_file]
    echo.
    echo Example: play_music.bat 1khz.wav
    goto :eof
)

set WAV_FILE=%1

echo Starting music player in new window...
echo File: %WAV_FILE%
echo.

REM Start in a separate window
start "XpuMusic Player" build\bin\Debug\music-player.exe "%WAV_FILE%"

echo Music player started!
echo You can continue using this command prompt.
echo.
echo To stop playback, close the music player window.
echo.