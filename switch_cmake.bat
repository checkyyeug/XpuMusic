@echo off
REM ============================================================================
REM Switch between different CMakeLists.txt configurations
REM ============================================================================

echo Available CMakeLists.txt versions:
echo.
echo 1. CMakeLists.txt           (Current)
echo 2. CMakeLists_WORKING.txt   (Working version)
echo 3. CMakeLists_test_audio.txt (Audio test version)
echo 4. CMakeLists_CLEAN.txt     (Clean version)
echo 5. CMakeLists_original.txt  (Original version)
echo.

set /p choice="Select version to switch to (1-5): "

if "%choice%"=="1" (
    echo Already using current CMakeLists.txt
    goto :end
)

if "%choice%"=="2" (
    set target=CMakeLists_WORKING.txt
)
if "%choice%"=="3" (
    set target=CMakeLists_test_audio.txt
)
if "%choice%"=="4" (
    set target=CMakeLists_CLEAN.txt
)
if "%choice%"=="5" (
    set target=CMakeLists_original.txt
)

if not defined target (
    echo Invalid choice!
    goto :end
)

REM Backup current
if exist "CMakeLists_backup.txt" del CMakeLists_backup.txt
ren CMakeLists.txt CMakeLists_backup.txt

REM Switch to selected
ren %target% CMakeLists.txt

echo Switched to %target%
echo.
echo Don't forget to reconfigure CMake:
echo   cd build
echo   cmake ..
echo.

:end
pause