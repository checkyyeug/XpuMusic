@echo off
REM Save current directory
set ORIGINAL_DIR=%CD%
echo === Force Build with File Overwrite ===
echo.

REM Change to project root
cd /d "D:\workspaces\foobar\XpuMusic"

REM Check if build directory exists
if not exist "build" (
    echo Build directory not found. Running build_all.bat first...
    echo.
    call build_all.bat
    echo.
    echo Force build will now continue...
    echo.
)

REM Set target directory
set TARGET_DIR=build\bin\Debug
set TARGET_NAME=%1

if "%TARGET_NAME%"=="" set TARGET_NAME=music-player
if "%TARGET_NAME%"=="music" set TARGET_NAME=music-player
if "%TARGET_NAME%"=="wav" set TARGET_NAME=final_wav_player

echo Target: %TARGET_NAME%.exe
echo Target directory: %TARGET_DIR%
echo.

REM Step 1: Delete existing target file if it exists
echo Step 1: Deleting existing target file...
if exist "%TARGET_DIR%\%TARGET_NAME%.exe" (
    del /f /q "%TARGET_DIR%\%TARGET_NAME%.exe"
    echo   Deleted: %TARGET_DIR%\%TARGET_NAME%.exe
) else (
    echo   File not found, skipping deletion
)

REM Step 2: Delete associated PDB file for clean rebuild
if exist "%TARGET_DIR%\%TARGET_NAME%.pdb" (
    del /f /q "%TARGET_DIR%\%TARGET_NAME%.pdb"
    echo   Deleted: %TARGET_DIR%\%TARGET_NAME%.pdb
)

REM Step 3: Delete object files to force recompilation
echo.
echo Step 2: Cleaning object files...
cd build 2>nul || (
    echo ERROR: Build directory not found!
    echo Please run build_all.bat first.
    exit /b 1
)
if exist "build\%TARGET_NAME%.dir\Debug\*.obj" (
    del /f /q "%TARGET_NAME%.dir\Debug\*.obj"
    echo   Cleaned object files
)

REM Step 4: Touch source file to update timestamp
echo.
echo Step 3: Updating source timestamps...
cd /d "D:\workspaces\foobar\XpuMusic"
if "%TARGET_NAME%"=="music-player" (
    copy /b src\music_player_legacy.cpp +,,
    echo   Updated: music_player_legacy.cpp
)
if "%TARGET_NAME%"=="final_wav_player" (
    copy /b src\final_wav_player.cpp +,,
    echo   Updated: final_wav_player.cpp
)

REM Step 5: Force rebuild
echo.
echo Step 4: Building target...
cd build
cmake --build . --config Debug --target %TARGET_NAME%

REM Step 6: Verify the new file was created
echo.
echo Step 5: Verifying build result...
cd /d "D:\workspaces\foobar\XpuMusic"
if exist "%TARGET_DIR%\%TARGET_NAME%.exe" (
    echo   SUCCESS: New file created!
    echo   Location: %TARGET_DIR%\%TARGET_NAME%.exe
    for %%F in ("%TARGET_DIR%\%TARGET_NAME%.exe") do (
        echo   Size: %%~zF bytes
        echo   Modified: %%~tF
    )
) else (
    echo   ERROR: Target file was not created!
    exit /b 1
)

echo.
echo === Force Build Complete ===
echo Target %TARGET_NAME%.exe has been completely rebuilt!

REM Return to original directory
cd /d "%ORIGINAL_DIR%"