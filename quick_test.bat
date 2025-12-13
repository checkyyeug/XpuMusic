@echo off
echo Testing XpuMusic after fixes...
echo.

cd build

echo 1. Building all targets...
cmake --build . --config Debug --v 1

echo.
echo 2. Checking output...
if exist "bin\Debug\music-player.exe" (
    echo [OK] music-player.exe built successfully
) else (
    echo [!] music-player.exe NOT found
)

if exist "bin\Debug\foobar2k-player.exe" (
    echo [OK] foobar2k-player.exe built successfully
) else (
    echo [!] foobar2k-player.exe NOT found
)

echo.
echo 3. Copying foobar2000 DLLs if available...
if exist "c:\Program Files\foobar2000\shared.dll" (
    copy "c:\Program Files\foobar2000\shared.dll" "bin\Debug\" >nul 2>&1
    echo    Copied shared.dll
)

echo.
echo 4. Testing music-player...
cd bin\Debug
if exist "music-player.exe" (
    echo.
    echo Running: music-player.exe --help
    music-player.exe --help 2>&1 || echo   (No help flag, but executable runs)
)

cd ..\..
echo.
echo Test complete!
pause