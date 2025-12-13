@echo off
echo Testing foobar2000.dll loading...
echo.

rem Check if DLL exists in current directory
if exist "foobar2000.dll" (
    echo [OK] foobar2000.dll found in current directory
    echo.
    foobar2k-player.exe %1
) else (
    echo [ERROR] foobar2000.dll not found!
    echo.
    echo Please install foobar2000 or copy foobar2000.dll to this directory
    echo.
    echo Installation options:
    echo 1. Download from https://www.foobar2000.org/download
    echo 2. Install to C:\Program Files\foobar2000\
    echo 3. Copy DLL to this directory
    echo.
    echo Or use the built-in player instead:
    echo   music-player.exe %1
)

pause