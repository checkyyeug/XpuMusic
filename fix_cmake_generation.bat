@echo off
REM Save current directory
set ORIGINAL_DIR=%CD%
echo === CMake Generation Fix Script ===
echo.
echo This script fixes the CMake dependency detection issue where
echo music-player.exe is not regenerated when Debug folder exists.
echo.

REM Set paths
set BUILD_DIR=D:\workspaces\foobar\XpuMusic\build
set BIN_DIR=%BUILD_DIR%\bin\Debug
set SOURCE_FILE=D:\workspaces\foobar\XpuMusic\src\music_player_legacy.cpp

REM Method 1: Force timestamp update (most reliable)
echo Method 1: Updating source file timestamp...
copy /b "%SOURCE_FILE%" +,, >nul
echo Done.

REM Method 2: Delete specific build artifacts
echo.
echo Method 2: Cleaning build artifacts...
cd /d "%BUILD_DIR%"
if exist "music-player.dir\Debug\music_player_legacy.obj" (
    del "music-player.dir\Debug\music_player_legacy.obj"
    echo Deleted object file
)
if exist "music-player.dir\Debug\music-player.tlog" (
    rmdir /s /q "music-player.dir\Debug\music-player.tlog" 2>nul
    echo Deleted build log
)
echo Done.

REM Method 3: Force rebuild
echo.
echo Method 3: Force rebuilding music-player...
cmake --build . --target music-player --clean-first >nul
echo Done.

REM Verify result
echo.
echo Verifying result...
if exist "%BIN_DIR%\music-player.exe" (
    echo SUCCESS: music-player.exe exists!
    dir "%BIN_DIR%\music-player.exe" | find "music-player.exe"
    
    REM Show file info
    echo.
    echo File details:
    for %%F in ("%BIN_DIR%\music-player.exe") do (
        echo   Size: %%~zF bytes
        echo   Modified: %%~tF
    )
) else (
    echo ERROR: music-player.exe not found in %BIN_DIR%
)

echo.
echo === Fix Complete ===
echo.
echo QUICK FIX COMMANDS:
echo   copy /b src\music_player_legacy.cpp +,,
echo   cmake --build build --target music-player --clean-first
echo.
echo Or just run this script: fix_cmake_generation.bat

REM Return to original directory
cd /d "%ORIGINAL_DIR%"