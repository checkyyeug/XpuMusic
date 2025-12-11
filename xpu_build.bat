@echo off
REM Save current directory
set ORIGINAL_DIR=%CD%
set FORCE_OVERWRITE=false

REM Parse command line arguments
set BUILD_TYPE=quick
set TARGET=all

if "%1"=="" goto :build
if /i "%1"=="quick" set BUILD_TYPE=quick
if /i "%1"=="full" set BUILD_TYPE=full
if /i "%1"=="clean" set BUILD_TYPE=clean
if /i "%1"=="fix" set BUILD_TYPE=fix
if /i "%1"=="force" set BUILD_TYPE=force
if /i "%1"=="music" set TARGET=music-player
if /i "%1"=="wav" set TARGET=final_wav_player
if /i "%1"=="force-music" set BUILD_TYPE=force-music
if /i "%1"=="force-wav" set BUILD_TYPE=force-wav

:build
cd /d "D:\workspaces\foobar\XpuMusic"

echo === XpuMusic Build System ===
echo Build type: %BUILD_TYPE%
echo Target: %TARGET%
echo Force overwrite: %FORCE_OVERWRITE%
echo.

if "%BUILD_TYPE%"=="quick" goto :quick_build
if "%BUILD_TYPE%"=="full" goto :full_build
if "%BUILD_TYPE%"=="clean" goto :clean_build
if "%BUILD_TYPE%"=="fix" goto :fix_build
if "%BUILD_TYPE%"=="force" goto :force_build
if "%BUILD_TYPE%"=="force-music" goto :force_music_build
if "%BUILD_TYPE%"=="force-wav" goto :force_wav_build
if "%BUILD_TYPE%"=="music" goto :target_build
if "%BUILD_TYPE%"=="wav" goto :target_build

:quick_build
echo Quick build - main executables only...
cd build
cmake --build . --config Debug --target music-player final_wav_player
goto :done

:full_build
echo Full build - all working components...
call build_all.bat
goto :done

:clean_build
echo Clean and rebuild...
cd build
cmake --build . --config Debug --target music-player --clean-first
goto :done

:fix_build
echo Fix and rebuild music-player...
call fix_cmake_generation.bat
goto :done

:target_build
echo Building specific target: %TARGET%...
cd build
cmake --build . --config Debug --target %TARGET%
goto :done

:force_build
echo Force building with file overwrite...
call force_build.bat %TARGET%
goto :done

:force_music_build
echo Force building music-player with overwrite...
call force_build.bat music-player
goto :done

:force_wav_build
echo Force building final_wav_player with overwrite...
call force_build.bat final_wav_player
goto :done

:done
echo.
echo === Build Complete ===
echo.
if exist "build\bin\Debug\music-player.exe" (
    echo ✓ music-player.exe ready
)
if exist "build\bin\Debug\final_wav_player.exe" (
    echo ✓ final_wav_player.exe ready
)
echo.
echo Usage examples:
echo   xpu_build.bat        - Quick build (default)
echo   xpu_build.bat full   - Full build
echo   xpu_build.bat clean  - Clean rebuild
echo   xpu_build.bat fix    - Fix dependencies
echo   xpu_build.bat force  - Force overwrite build
echo   xpu_build.bat music  - Build only music-player
echo   xpu_build.bat wav    - Build only final_wav_player
echo   xpu_build.bat force-music - Force music-player
echo   xpu_build.bat force-wav   - Force final_wav_player
echo.

REM Return to original directory
cd /d "%ORIGINAL_DIR%"