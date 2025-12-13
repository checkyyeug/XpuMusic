@echo off
echo Checking foobar2000.dll availability...
echo.

echo 1. Checking common foobar2000 installation paths...
if exist "C:\Program Files\foobar2000\foobar2000.dll" (
    echo Found: C:\Program Files\foobar2000\foobar2000.dll
    goto :copy_dll
)

if exist "C:\Program Files (x86)\foobar2000\foobar2000.dll" (
    echo Found: C:\Program Files ^(x86^)\foobar2000\foobar2000.dll
    goto :copy_dll
)

echo 2. foobar2000 not found in standard locations.
echo.
echo Options to get foobar2000.dll:
echo.
echo A. Install foobar2000 (recommended):
echo    1. Visit https://www.foobar2000.org/download
echo    2. Download Windows installer (about 5 MB)
echo    3. Run installer with default settings
echo    4. DLL will be at: C:\Program Files\foobar2000\foobar2000.dll
echo.
echo B. Download portable version:
echo    1. Visit https://www.foobar2000.org/download
echo    2. Download "Windows zip" version
echo    3. Extract to: C:\foobar2000_portable\
echo    4. DLL will be inside the extracted folder
echo.
echo C. Skip foobar2000 (recommended for basic playback):
echo    Use music-player.exe instead of foobar2k-player.exe
echo.
echo Current working directory:
cd
echo.
echo To continue without foobar2000, run:
echo   music-player.exe \music\1.wav
echo.
pause