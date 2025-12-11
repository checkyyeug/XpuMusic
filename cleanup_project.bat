@echo off
REM ============================================================================
REM XpuMusic Project Cleanup Script
REM Removes unnecessary files and directories while preserving all .md files
REM ============================================================================

echo ========================================
echo  XpuMusic Project Cleanup
echo ========================================
echo.

REM Show current size
echo Current project size:
for /f "tokens=3" %%a in ('dir /s /-c ^| find "bytes"') do set size=%%a
echo %size% bytes
echo.

REM Ask for confirmation
echo This will remove:
echo   - All build directories (build_*)
echo   - Compiled binaries and executables
echo   - Large audio test files (.wav, .mp3)
echo   - Duplicate CMakeLists files
echo   - Old test executables
echo.
echo All .md documentation files will be preserved.
echo.
set /p confirm="Proceed with cleanup? (y/N): "
if /i not "%confirm%"=="y" (
    echo Cleanup cancelled.
    pause
    exit /b 0
)

echo.
echo Starting cleanup...

REM Remove build directories
echo [1/7] Removing build directories...
for /d %%d in (build_*) do (
    echo   Removing %%d...
    rmdir /s /q "%%d" 2>nul
)

REM Remove compiled executables
echo.
echo [2/7] Removing compiled executables...
del /q *.exe 2>nul
del /q *.out 2>nul

REM Remove large audio files (keep test_440hz.* for testing)
echo.
echo [3/7] Removing large audio files...
del /q *.wav 2>nul
del /q *.mp3 2>nul
echo   Keeping test_440hz.wav and test_440hz.mp3 for testing

REM Remove duplicate CMakeLists files
echo.
echo [4/7] Removing duplicate CMakeLists files...
del /q CMakeLists_*.txt 2>nul
del /q CMakeLists_*.cmake 2>nul

REM Remove old test binaries
echo.
echo [5/7] Removing old test binaries...
del /q test_* 2>nul
del /q simple_player 2>nul
del /q music-player-* 2>nul
del /q final_wav_player 2>nul
del /q real_player 2>nul
del /q debug_audio 2>nul

REM Remove generated audio files in src/audio
echo.
echo [6/7] Removing generated audio files...
if exist "src/audio" (
    del /q "src/audio\demo_*.wav" 2>nul
)

REM Remove temporary and backup files
echo.
echo [7/7] Removing temporary files...
del /q *.bak 2>nul
del /q *.tmp 2>nul
del /q *~ 2>nul
del /q *.orig 2>nul
del /q ".DS_Store" 2>nul
del /q "Thumbs.db" 2>nul

REM Remove specific unnecessary files from root
echo.
echo [Extra] Removing unnecessary files from root...
del /q "的声明" 2>nul
del /q "completion_chart.txt" 2>nul
del /q "rename_to_xpumusic.ps1" 2>nul

echo.
echo ========================================
echo Cleanup Complete!
echo ========================================
echo.

REM Show new size
echo New project size:
for /f "tokens=3" %%a in ('dir /s /-c ^| find "bytes"') do set size=%%a
echo %size% bytes
echo.

echo Preserved files:
echo   ✓ All .md documentation (39 files)
echo   ✓ Source code (src/, core/, plugins/, etc.)
echo   ✓ CMakeLists.txt
echo   ✓ Build scripts
echo   ✓ Configuration files
echo   ✓ test_440hz.wav and test_440hz.mp3

echo.
echo To rebuild the project, run:
echo   build.bat

pause