@echo off
echo Testing foobar2000 config control in music-player.exe
echo ================================================
echo.

echo Step 1: Rebuilding music-player.exe...
cd build
cmake --build . --config Debug --target music-player --v:minimal
cd ..

if not exist "build\bin\Debug\music-player.exe" (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo Step 2: Testing with foobar2000 DISABLED in config...
echo.
echo Current config setting:
powershell -Command "$content = Get-Content 'config\default_config.json'; $content | Select-String 'enable_compatibility'"

echo.
echo Running music-player.exe (should NOT load foobar2000 DLLs):
echo --------------------------------------------------------
cd build\bin\Debug
music-player.exe
cd ..\..

echo.
echo.
echo Step 3: Enabling foobar2000 in config...
echo.
powershell -Command "(Get-Content 'config\default_config.json') -replace '\"enable_compatibility\": false', '\"enable_compatibility\": true' | Set-Content 'config\default_config.json'"
echo [Updated] foobar2000.enable_compatibility = true

echo.
echo Step 4: Testing with foobar2000 ENABLED in config...
echo.
echo Updated config setting:
powershell -Command "$content = Get-Content 'config\default_config.json'; $content | Select-String 'enable_compatibility'"

echo.
echo Running music-player.exe (should ATTEMPT to load foobar2000 DLLs):
echo ----------------------------------------------------------------
cd build\bin\Debug
music-player.exe
cd ..\..

echo.
echo.
echo Step 5: Restoring original config (foobar2000 disabled)...
powershell -Command "(Get-Content 'config\default_config.json') -replace '\"enable_compatibility\": true', '\"enable_compatibility\": false' | Set-Content 'config\default_config.json'"
echo [Restored] foobar2000.enable_compatibility = false

echo.
echo ================================================
echo Test Summary:
echo ================================================
echo.
echo - When enable_compatibility=false:
echo   Should see: "Using native decoders only (foobar2000 disabled in config)"
echo.
echo - When enable_compatibility=true:
echo   Should see: Either "foobar2000 modern components loaded" (if DLLs exist)
echo              OR "Failed to load foobar2000 components" (if DLLs missing)
echo.
pause