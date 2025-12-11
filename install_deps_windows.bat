@echo off
REM ============================================================================
REM XpuMusic Dependencies Installer for Windows
REM ============================================================================

echo ========================================
echo  XpuMusic Dependencies Installer
echo ========================================
echo.

echo This script will install dependencies for XpuMusic on Windows.
echo It will use Chocolatey package manager for most dependencies.
echo.

REM Check if administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Please run this script as Administrator.
    echo Right-click on the script and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

echo Checking administrator privileges... OK

REM Check if chocolatey is installed
where choco >nul 2>&1
if %errorlevel% neq 0 (
    echo Installing Chocolatey package manager...
    echo.
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "iwr -useb c:\chocolatey\install.ps1 | iex"

    if %errorlevel% neq 0 (
        echo [ERROR] Failed to install Chocolatey.
        echo Please install it manually from https://chocolatey.org/install
        pause
        exit /b 1
    )
    echo Chocolatey installed successfully!
    echo Refreshing environment variables...
    set PATH=%PATH%;C:\ProgramData\chocolatey\bin
    refreshenv
    echo.
)

echo Chocolatey is installed:
choco -v

REM Update chocolatey
echo.
echo Updating Chocolatey packages...
choco upgrade -y

REM ========================================
echo Installing Required Dependencies
echo ========================================

echo.
echo [1/6] CMake...
choco install cmake -y --installargs '"ADD_CMAKE_TO_PATH=1"'

echo.
echo [2/6] Git...
choco install git -y

echo.
echo [3/6] FLAC (Free Lossless Audio Codec)...
choco install flac -y

echo.
echo [4/6] OGG Vorbis...
choco install libvorbis -y

echo.
echo [5/6] Visual Studio Build Tools 2022...
choco install visualstudio2022buildtools -y --add Microsoft.VisualStudio.Workload.VCTools -y --includeOptional

REM ========================================
echo Installing Optional Dependencies
echo ========================================

echo.
echo [Optional] Installing Qt6 for GUI development...
echo Press Y to install Qt6 or any other key to skip.
set /p installQt=Install Qt6? [y/N]:
if /i "%installQt%"=="y" (
    choco install qt6-base -y
    choco install qt6-tools -y
)

echo.
echo [Optional] Installing Doxygen for documentation...
echo Press Y to install Doxygen or any other key to skip.
set /p installDoxygen=Install Doxygen? [y/N]:
if /i "%installDoxygen%"=="y" (
    choco install doxygen.install -y
)

REM ========================================
echo Setting Environment Variables
echo ========================================

echo.
echo Setting up environment variables...

REM Try to find FLAC installation
if exist "C:\Program Files\FLAC" (
    set "FLAC_PATH=C:\Program Files\FLAC"
) else if exist "C:\Program Files (x86)\FLAC" (
    set "FLAC_PATH=C:\Program Files (x86)\FLAC"
) else (
    echo [WARNING] FLAC not found in default locations.
    echo Please ensure FLAC is installed or set environment variables manually.
    goto :skip_env
)

echo Setting FLAC environment variables...
setx FLAC_ROOT "%FLAC_PATH%" /M
setx FLAC_INCLUDE "%FLAC_PATH%\include" /M
setx FLAC_LIB "%FLAC_PATH%\lib" /M

:skip_env

REM ========================================
echo Verifying Installation
echo ========================================

echo.
echo Verifying installed components...
echo.

echo Checking CMake...
cmake --version

echo.
echo Checking Git...
git --version

echo.
echo Checking FLAC...
where flac >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ FLAC found at:
    where flac
) else (
    echo ✗ FLAC not found in PATH
)

echo.
echo ========================================
echo Installation Complete!
echo ========================================
echo.

echo The following dependencies have been installed:
echo  ✓ CMake
echo  ✓ Git
echo  ✓ FLAC (Free Lossless Audio Codec)
echo  ✓ OGG Vorbis
echo  ✓ Visual Studio Build Tools 2022

if /i "%installQt%"=="y" (
    echo  ✓ Qt6
)

if /i "%installDoxygen%"=="y" (
    echo  ✓ Doxygen
)

echo.
echo Environment variables have been set for FLAC.
echo.
echo IMPORTANT: Please restart your command prompt or IDE
echo to use the new environment variables.
echo.

echo Next steps:
echo 1. Restart your command prompt/IDE
echo 2. Navigate to project directory
echo 3. Run: mkdir build && cd build && cmake .. && cmake --build . --config Release
echo.
pause