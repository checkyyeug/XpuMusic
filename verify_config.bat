@echo off
echo Verifying foobar2000 configuration support...
echo.

echo Current config setting:
powershell -Command "Get-Content 'config\default_config.json' | Select-String 'enable_compatibility'"

echo.
echo Building music-player.exe...
cd build
cmake --build . --config Debug --target music-player --v:minimal
cd ..

if exist "build\bin\Debug\music-player.exe" (
    echo.
    echo [OK] music-player.exe built successfully!
    echo.
    echo To test:
    echo 1. Run with foobar2000 disabled (current setting):
    echo    build\bin\Debug\music-player.exe
    echo.
    echo 2. To enable foobar2000, edit config\default_config.json:
    echo    Change: "enable_compatibility": false
    echo    To:     "enable_compatibility": true
    echo.
    echo 3. Run again to see the difference
) else (
    echo [!] Build failed!
)

pause