@echo off
echo Testing --fb2k flag for music-player.exe
echo =====================================
echo.

cd build

echo 1. Building music-player.exe...
cmake --build . --config Debug --target music-player --v:minimal

if not exist "bin\Debug\music-player.exe" (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo 2. Testing WITHOUT --fb2k flag (native only):
echo --------------------------------------------
cd bin\Debug
echo Command: music-player.exe
echo.
music-player.exe
cd ..\..

echo.
echo.
echo 3. Testing WITH --fb2k flag:
echo ----------------------------
echo Command: music-player.exe --fb2k
echo.
cd bin\Debug
music-player.exe --fb2k
cd ..\..

echo.
echo.
echo 4. Testing --fb2k with a file:
echo -------------------------------
echo Command: music-player.exe --fb2k \music\1.wav
echo.
cd bin\Debug
music-player.exe --fb2k \music\1.wav
cd ..\..

echo.
echo =====================================
echo Summary:
echo =====================================
echo.
echo - WITHOUT --fb2k: Uses native WAV decoder only
echo - WITH --fb2k: Attempts to load foobar2000 DLLs
echo.
echo Expected outputs:
echo - Native mode: "Using native decoders only"
echo - FB2K mode: "Loading foobar2000 components..."
echo.

cd ..
pause