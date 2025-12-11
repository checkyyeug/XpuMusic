@echo off
REM Save current directory
set ORIGINAL_DIR=%CD%
echo === Rebuilding Music Player (Force Regeneration) ===
echo.

REM Change to build directory
cd /d "D:\workspaces\foobar\XpuMusic\build"

REM Method 1: Force rebuild specific target
echo Method 1: Force rebuilding music-player target...
cmake --build . --target music-player --clean-first

REM Method 2: Touch source files to force recompilation
echo.
echo Method 2: Touching source files to force dependency check...
copy /b "D:\workspaces\foobar\XpuMusic\src\music_player_legacy.cpp" +,,

REM Method 3: Delete specific object files
echo.
echo Method 3: Cleaning object files...
if exist "music-player.dir\Debug\music_player_legacy.obj" (
    del "music-player.dir\Debug\music_player_legacy.obj"
    echo Deleted object file
)

REM Final build
echo.
echo Final build...
cmake --build . --target music-player

echo.
echo === Build complete! ===
echo Check D:\workspaces\foobar\XpuMusic\build\bin\Debug\music-player.exe
echo.

REM Verify the file exists and show info
if exist "bin\Debug\music-player.exe" (
    echo music-player.exe successfully generated!
    dir "bin\Debug\music-player.exe" | find "music-player.exe"
) else (
    echo ERROR: music-player.exe not found!
)

REM Return to original directory
cd /d "%ORIGINAL_DIR%"