@echo off
echo Testing foobar2000 configuration control...
echo.

cd build

echo 1. Building updated music-player.exe...
cmake --build . --config Debug --target music-player --v minimal

echo.
echo 2. Testing with foobar2000 DISABLED in config...
echo.
cd bin\Debug
echo [Config file: foobar2000.enable_compatibility = false]
echo.
music-player.exe

echo.
echo.
echo 3. Enabling foobar2000 in config...
echo.
cd ..\..
powershell -Command "(Get-Content 'config\default_config.json') -replace '\"enable_compatibility\": false', '\"enable_compatibility\": true' | Set-Content 'config\default_config.json'"
echo [Config updated: foobar2000.enable_compatibility = true]
echo.

echo 4. Testing with foobar2000 ENABLED in config...
echo.
cd bin\Debug
echo [Config file: foobar2000.enable_compatibility = true]
echo.
music-player.exe

echo.
echo.
echo 5. Restoring original config (foobar2000 disabled)...
cd ..\..
powershell -Command "(Get-Content 'config\default_config.json') -replace '\"enable_compatibility\": true', '\"enable_compatibility\": false' | Set-Content 'config\default_config.json'"

echo.
echo ========================================
echo Test Complete!
echo ========================================
echo.
echo You should see different messages:
echo - When disabled: "Using native decoders only (foobar2000 disabled in config)"
echo - When enabled: "foobar2000 modern components loaded" (if DLLs are present)
echo.

cd ..
pause