@echo off
echo Testing XpuMusic build...
echo.

cd build\bin\Debug

echo Checking executables:
if exist music-player.exe (
    echo [OK] music-player.exe found
) else (
    echo [!] music-player.exe NOT found
)

if exist final_wav_player.exe (
    echo [OK] final_wav_player.exe found
) else (
    echo [!] final_wav_player.exe NOT found
)

if exist foobar2k-player.exe (
    echo [OK] foobar2k-player.exe found
) else (
    echo [!] foobar2k-player.exe NOT found
)

echo.
echo Checking foobar2000 DLLs:
if exist shared.dll (
    echo [OK] shared.dll found
) else (
    echo [!] shared.dll NOT found - copying...
    if exist "c:\Program Files\foobar2000\shared.dll" (
        copy "c:\Program Files\foobar2000\shared.dll" .
    )
)

echo.
echo To test the player:
echo   music-player.exe \music\1.wav
echo.

cd ..\..
pause