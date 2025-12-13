@echo off
echo Quick verification of config support...
echo.

cd build
cmake --build . --config Debug --target music-player --v:minimal
cd ..

if exist "build\bin\Debug\music-player.exe" (
    echo [OK] music-player.exe built successfully!
    echo.
    echo Current config:
    powershell -Command "Get-Content 'config\default_config.json' | Select-String 'enable_compatibility'"
    echo.
    echo To test the player:
    echo   cd build\bin\Debug
    echo   music-player.exe
    echo.
    echo Expected output when enable_compatibility=false:
    echo   "[Config] foobar2000 compatibility: disabled"
    echo   "[OK] Using native decoders only (foobar2000 disabled in config)"
) else (
    echo [!] Build still failed!
    echo.
    echo Check the error messages above for:
    echo   - Missing source files
    echo   - Compilation errors
)

pause