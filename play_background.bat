@echo off
REM Play audio in background using start command

if "%1"=="" (
    echo Usage: play_background.bat [audio_file]
    echo.
    echo Example: play_background.bat 1khz.wav
    goto :eof
)

set AUDIO_FILE=%1

echo Playing %AUDIO_FILE% in background...
echo The player will run in a separate window.
echo.

REM Start in new window
start "Music Player" /D "%CD%" build/bin/Debug/music-player.exe "%AUDIO_FILE%"

echo Player started in separate window.
echo You can continue using this command prompt.
echo.
pause