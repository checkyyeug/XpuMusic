@echo off
echo Testing both XpuMusic players...
echo.

cd build\bin\Debug

echo ============================================
echo 1. Testing music-player.exe (Recommended)
echo ============================================
echo.
music-player.exe \music\1.wav

echo.
echo ============================================
echo 2. Testing foobar2k-player.exe (Experimental)
echo ============================================
echo.
echo Note: This player is experimental and has limited functionality
echo      It can load files but actual playback is not fully implemented
echo.

cd ..\..
echo.
echo Summary:
echo - music-player.exe: Full-featured player with foobar2000 support
echo - foobar2k-player.exe: Experimental compatibility test
echo.
pause