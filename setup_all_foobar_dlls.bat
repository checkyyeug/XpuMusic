@echo off
echo Setting up all foobar2000 DLLs for XpuMusic...
echo.

echo 1. Copying foobar2000 DLLs to build directory...
set "FB2K_DIR=c:\Program Files\foobar2000"
set "TARGET_DIR=.\build\bin\Debug"

if not exist "%FB2K_DIR%" (
    echo [!] foobar2000 not found at %FB2K_DIR%
    echo Please install foobar2000 first: https://www.foobar2000.org/download
    pause
    exit /b 1
)

echo Found foobar2000 at: %FB2K_DIR%
echo Target directory: %TARGET_DIR%
echo.

:: Copy all relevant DLLs
echo Copying DLLs...
copy "%FB2K_DIR%\shared.dll" "%TARGET_DIR%\" >nul 2>&1
if exist "%TARGET_DIR%\shared.dll" echo    [OK] shared.dll

copy "%FB2K_DIR%\avcodec-fb2k-62.dll" "%TARGET_DIR%\" >nul 2>&1
if exist "%TARGET_DIR%\avcodec-fb2k-62.dll" echo    [OK] avcodec-fb2k-62.dll

copy "%FB2K_DIR%\avformat-fb2k-62.dll" "%TARGET_DIR%\" >nul 2>&1
if exist "%TARGET_DIR%\avformat-fb2k-62.dll" echo    [OK] avformat-fb2k-62.dll

copy "%FB2K_DIR%\avutil-fb2k-60.dll" "%TARGET_DIR%\" >nul 2>&1
if exist "%TARGET_DIR%\avutil-fb2k-60.dll" echo    [OK] avutil-fb2k-60.dll

copy "%FB2K_DIR%\sqlite3.dll" "%TARGET_DIR%\" >nul 2>&1
if exist "%TARGET_DIR%\sqlite3.dll" echo    [OK] sqlite3.dll

echo.
echo 2. Checking for foobar2000 executable...
if exist "%FB2K_DIR%\foobar2000.exe" (
    echo    [OK] foobar2000.exe found
) else (
    echo    [!] foobar2000.exe not found
)

echo.
echo 3. Setting up PATH environment...
:: Add to current session PATH
set PATH=%TARGET_DIR%;%PATH%
echo    PATH updated for this session

echo.
echo 4. Additional DLLs in components directory...
if exist "%FB2K_DIR%\components\*.dll" (
    echo    Found component DLLs:
    dir /b "%FB2K_DIR%\components\*.dll" 2>nul
    echo.
    echo    Copying key components...
    copy "%FB2K_DIR%\components\*.dll" "%TARGET_DIR%\" >nul 2>&1
) else (
    echo    No additional component DLLs found
)

echo.
echo ========================================
echo Setup complete!
echo ========================================
echo.
echo Notes:
echo 1. Modern foobar2000 uses modular architecture
echo 2. The DLLs are loaded dynamically at runtime
echo 3. Some components may require additional dependencies
echo.
echo To test the players:
echo   cd build\bin\Debug
echo   music-player.exe \music\1.wav      (Recommended)
echo   foobar2k-player.exe \music\1.wav   (Experimental)
echo.
pause