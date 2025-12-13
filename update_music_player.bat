@echo off
echo Updating music-player.exe with foobar2000 modern DLL support...
echo.

echo 1. Copying foobar2000 DLLs to build directory...
if exist "c:\Program Files\foobar2000\shared.dll" (
    copy "c:\Program Files\foobar2000\shared.dll" ".\build\bin\Debug\" >nul 2>&1
    echo    ✓ shared.dll
)

if exist "c:\Program Files\foobar2000\avcodec-fb2k-62.dll" (
    copy "c:\Program Files\foobar2000\avcodec-fb2k-62.dll" ".\build\bin\Debug\" >nul 2>&1
    echo    ✓ avcodec-fb2k-62.dll
)

if exist "c:\Program Files\foobar2000\avformat-fb2k-62.dll" (
    copy "c:\Program Files\foobar2000\avformat-fb2k-62.dll" ".\build\bin\Debug\" >nul 2>&1
    echo    ✓ avformat-fb2k-62.dll
)

if exist "c:\Program Files\foobar2000\avutil-fb2k-60.dll" (
    copy "c:\Program Files\foobar2000\avutil-fb2k-60.dll" ".\build\bin\Debug\" >nul 2>&1
    echo    ✓ avutil-fb2k-60.dll
)

echo.
echo 2. Rebuilding music-player.exe...
cd build

if exist XpuMusic.sln (
    echo    Using Visual Studio solution...
    msbuild XpuMusic.sln /p:Configuration=Debug /p:Platform=x64 /target:music-player /v:minimal
) else (
    echo    Using CMake...
    cmake --build . --config Debug --target music-player
)

cd ..
echo.
echo 3. Update complete!
echo.
echo Now run:
echo   cd build\bin\Debug
echo   music-player.exe \music\1.wav
echo.
echo The player will automatically detect and use foobar2000 DLLs if available.
echo.
pause