@echo off
echo Setting up foobar2000 DLLs for XpuMusic...
echo.

echo 1. Copying foobar2000 DLLs...
if exist "c:\Program Files\foobar2000\shared.dll" (
    copy "c:\Program Files\foobar2000\shared.dll" ".\build\bin\Debug\"
    echo    Copied shared.dll
)

if exist "c:\Program Files\foobar2000\avcodec-fb2k-62.dll" (
    copy "c:\Program Files\foobar2000\avcodec-fb2k-62.dll" ".\build\bin\Debug\"
    echo    Copied avcodec-fb2k-62.dll
)

if exist "c:\Program Files\foobar2000\avformat-fb2k-62.dll" (
    copy "c:\Program Files\foobar2000\avformat-fb2k-62.dll" ".\build\bin\Debug\"
    echo    Copied avformat-fb2k-62.dll
)

if exist "c:\Program Files\foobar2000\avutil-fb2k-60.dll" (
    copy "c:\Program Files\foobar2000\avutil-fb2k-60.dll" ".\build\bin\Debug\"
    echo    Copied avutil-fb2k-60.dll
)

echo.
echo 2. DLLs copied to build\bin\Debug directory
echo.
echo 3. Now try running:
echo    foobar2k-player.exe \music\1.wav
echo.
echo Note: Modern foobar2000 uses modular architecture.
echo The old single foobar2000.dll has been split into multiple components.
echo.
pause