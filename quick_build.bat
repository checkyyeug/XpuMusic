@echo off
REM Save current directory
set ORIGINAL_DIR=%CD%

REM Get script directory - works from anywhere
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%
set BUILD_DIR=%PROJECT_ROOT%build

if /i "%1"=="force" goto :force_build

:normal_build
echo === Quick Build for XpuMusic ===
echo.

REM Check if we're in a valid project directory
if not exist "%PROJECT_ROOT%CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found!
    echo Please ensure this script is in the XpuMusic root directory.
    echo Current project root: %PROJECT_ROOT%
    pause
    exit /b 1
)

REM Create and go to build directory
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

cd /d "%BUILD_DIR%"

REM Check if CMake has been configured
if not exist "CMakeCache.txt" (
    echo First time setup - configuring with CMake...
    cmake -S "%PROJECT_ROOT%" -B . -G "Visual Studio 17 2022" -A x64
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: CMake configuration failed!
        echo Please ensure Visual Studio is installed.
        pause
        exit /b 1
    )
)

REM Build the main targets quickly
echo Building music-player.exe...
cmake --build . --config Debug --target music-player --parallel

echo Building final_wav_player.exe...
cmake --build . --config Debug --target final_wav_player --parallel

goto :complete

:force_build
echo === Quick Build with Force Overwrite ===
cd /d "%BUILD_DIR%"
cmake --build . --config Debug --target music-player --parallel --clean-first
cmake --build . --config Debug --target final_wav_player --parallel --clean-first

:complete
echo.
echo === Build Complete ===
echo.

REM Check for executables
set EXE_DIR=%BUILD_DIR%\bin\Debug
echo Available executables:
if exist "%EXE_DIR%\*.exe" (
    for %%F in ("%EXE_DIR%\*.exe") do (
        echo   %%~nF.exe
    )
) else (
    echo No executables found in %EXE_DIR%
    echo Build may have failed.
)

echo.
echo Quick test examples:
if exist "%EXE_DIR%\final_wav_player.exe" (
    echo   final_wav_player.exe test_1khz.wav
)
if exist "%EXE_DIR%\music-player.exe" (
    echo   music-player.exe [audio_file]
)

echo.
echo Build directory: %BUILD_DIR%
echo Executable directory: %EXE_DIR%
echo.
echo Usage:
echo   quick_build.bat        - Normal quick build
echo   quick_build.bat force  - Force clean rebuild

REM Return to original directory
cd /d "%ORIGINAL_DIR%"