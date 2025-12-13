@echo off
echo Building music-player.exe with foobar2000 support...
echo.

cd build

echo 1. Cleaning previous build...
del /Q bin\Debug\*.exe 2>nul

echo 2. Building music-player...
msbuild music-player.vcxproj /p:Configuration=Debug /p:Platform=x64 /v:minimal

if %ERRORLEVEL% EQU 0 (
    echo.
    echo 3. Build successful!
    echo.
    echo Copying DLLs...
    if exist "c:\Program Files\foobar2000\shared.dll" (
        copy "c:\Program Files\foobar2000\shared.dll" "bin\Debug\" >nul
        echo    Copied shared.dll
    )
    if exist "c:\Program Files\foobar2000\avcodec-fb2k-62.dll" (
        copy "c:\Program Files\foobar2000\avcodec-fb2k-62.dll" "bin\Debug\" >nul
        echo    Copied avcodec-fb2k-62.dll
    )

    echo.
    echo Run the player with:
    echo   cd bin\Debug
    echo   music-player.exe \music\1.wav
) else (
    echo.
    echo Build failed. Check the error messages above.
)

cd ..
pause