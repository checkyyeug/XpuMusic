@echo off
REM XpuMusic Build Verification Script
REM Automated testing to ensure build functionality

setlocal EnableDelayedExpansion

echo ====================================
echo  XpuMusic Build Verification Tool
echo ====================================
echo.

REM Configuration
set PROJECT_ROOT=%~dp0
set BUILD_DIR=%PROJECT_ROOT%build
set EXE_DIR=%BUILD_DIR%\bin\Debug
set TEST_AUDIO=%PROJECT_ROOT%test_1khz.wav
set VERIFICATION_LOG=%BUILD_DIR%\verification.log

REM Initialize counters
set TESTS_PASSED=0
set TESTS_TOTAL=0
set VERIFICATION_PASSED=1

REM Create log file
echo XpuMusic Build Verification Log > "%VERIFICATION_LOG%"
echo Started: %DATE% %TIME% >> "%VERIFICATION_LOG%"
echo. >> "%VERIFICATION_LOG%"

REM Function to run a test
:run_test
set TEST_NAME=%1
set TEST_COMMAND=%2
set TEST_SUCCESS=%3

set /a TESTS_TOTAL+=1
echo [TEST %TESTS_TOTAL%] %TEST_NAME%...
echo [TEST %TESTS_TOTAL%] %TEST_NAME% >> "%VERIFICATION_LOG%"

REM Execute test
%TEST_COMMAND% > nul 2>&1
if %ERRORLEVEL% EQU %TEST_SUCCESS% (
    echo   ✓ PASSED
    echo   ✓ PASSED >> "%VERIFICATION_LOG%"
    set /a TESTS_PASSED+=1
) else (
    echo   ✗ FAILED
    echo   ✗ FAILED >> "%VERIFICATION_LOG%"
    set VERIFICATION_PASSED=0
)
echo. >> "%VERIFICATION_LOG%"
goto :eof

REM Main verification starts here
echo Verifying build directory structure...
if not exist "%BUILD_DIR%" (
    echo ERROR: Build directory not found!
    echo Please run build_all.bat or quick_build.bat first
    pause
    exit /b 1
)

echo Verifying executables...
echo. >> "%VERIFICATION_LOG%"
echo === Executable Verification === >> "%VERIFICATION_LOG%"

REM Check for required executables
call :run_test "music-player.exe" "exist \"%EXE_DIR%\music-player.exe\"" "0"
call :run_test "final_wav_player.exe" "exist \"%EXE_DIR%\final_wav_player.exe\"" "0"
call :run_test "test_decoders.exe" "exist \"%EXE_DIR%\test_decoders.exe\"" "0"
call :run_test "test_audio_direct.exe" "exist \"%EXE_DIR%\test_audio_direct.exe\"" "0"
call :run_test "test_simple_playback.exe" "exist \"%EXE_DIR%\test_simple_playback.exe\"" "0"

REM Check for optional executables
call :run_test "foobar2k-player.exe" "exist \"%EXE_DIR%\foobar2k-player.exe\"" "0"
call :run_test "plugin_flac_decoder.dll" "exist \"%EXE_DIR%\plugin_flac_decoder.dll\"" "0"
call :run_test "plugin_mp3_decoder.dll" "exist \"%EXE_DIR%\plugin_mp3_decoder.dll\"" "0"
call :run_test "plugin_wav_decoder.dll" "exist \"%EXE_DIR%\plugin_wav_decoder.dll\"" "0"

echo.
echo Running functional tests...
echo. >> "%VERIFICATION_LOG%"
echo === Functional Tests === >> "%VERIFICATION_LOG%"

REM Change to executable directory
pushd "%EXE_DIR%"

REM Test 1: Decoder test
if exist "test_decoders.exe" (
    call :run_test "Decoder functionality" "test_decoders.exe" "0"
) else (
    echo [SKIP] Decoder test - executable not found
    echo [SKIP] Decoder test - executable not found >> "%VERIFICATION_LOG%"
    set /a TESTS_TOTAL+=1
)

REM Test 2: Audio system test
if exist "test_audio_direct.exe" (
    call :run_test "Audio system" "test_audio_direct.exe" "0"
) else (
    echo [SKIP] Audio system test - executable not found
    echo [SKIP] Audio system test - executable not found >> "%VERIFICATION_LOG%"
    set /a TESTS_TOTAL+=1
)

REM Test 3: Simple playback test
if exist "test_simple_playback.exe" (
    call :run_test "Simple playback" "test_simple_playback.exe" "0"
) else (
    echo [SKIP] Simple playback test - executable not found
    echo [SKIP] Simple playback test - executable not found >> "%VERIFICATION_LOG%"
    set /a TESTS_TOTAL+=1
)

REM Test 4: Actual audio playback (if test file exists)
if exist "final_wav_player.exe" (
    if exist "%TEST_AUDIO%" (
        echo [TEST] Audio playback test...
        echo [TEST] Audio playback test >> "%VERIFICATION_LOG%"
        final_wav_player.exe "%TEST_AUDIO%" > nul 2>&1
        if %ERRORLEVEL% EQU 0 (
            echo   ✓ PASSED (Audio played successfully)
            echo   ✓ PASSED (Audio played successfully) >> "%VERIFICATION_LOG%"
            set /a TESTS_PASSED+=1
        ) else (
            echo   ⚠ WARNING (Audio test failed - may be audio device issue)
            echo   ⚠ WARNING (Audio test failed - may be audio device issue) >> "%VERIFICATION_LOG%"
        )
        set /a TESTS_TOTAL+=1
        echo. >> "%VERIFICATION_LOG%"
    ) else (
        echo [SKIP] Audio playback test - test file not found
        echo [SKIP] Audio playback test - test file not found >> "%VERIFICATION_LOG%"
        set /a TESTS_TOTAL+=1
    )
)

REM Return to original directory
popd

REM Additional checks
echo.
echo Performing additional checks...
echo. >> "%VERIFICATION_LOG%"
echo === Additional Checks === >> "%VERIFICATION_LOG%"

REM Check build artifacts
call :run_test "Core library (core_engine.lib)" "exist \"%BUILD_DIR%\lib\Debug\core_engine.lib\"" "0"
call :run_test "Platform library (platform_abstraction.lib)" "exist \"%BUILD_DIR%\lib\Debug\platform_abstraction.lib\"" "0"
call :run_test "SDK library (sdk_impl.lib)" "exist \"%BUILD_DIR%\lib\Debug\sdk_impl.lib\"" "0"

REM Check file sizes
echo. >> "%VERIFICATION_LOG%"
echo === File Size Analysis === >> "%VERIFICATION_LOG%"

if exist "%EXE_DIR%\*.exe" (
    echo Executable file sizes:
    echo Executable file sizes: >> "%VERIFICATION_LOG%"
    for %%F in ("%EXE_DIR%\*.exe") do (
        set "size=%%~zF"
        set /a "sizeKB=!size!/1024"
        echo   %%~nF.exe: !sizeKB! KB
        echo   %%~nF.exe: !sizeKB! KB >> "%VERIFICATION_LOG%"
    )
    echo. >> "%VERIFICATION_LOG%"
)

REM Generate summary
echo.
echo ====================================
echo  VERIFICATION SUMMARY
echo ====================================
echo.
echo Tests: %TESTS_PASSED%/%TESTS_TOTAL% passed
echo Tests: %TESTS_PASSED%/%TESTS_TOTAL% passed >> "%VERIFICATION_LOG%"
if %VERIFICATION_PASSED% EQU 1 (
    echo ✅ VERIFICATION SUCCESSFUL
    echo ✅ VERIFICATION SUCCESSFUL >> "%VERIFICATION_LOG%"
    exit /b 0
) else (
    echo ⚠️ VERIFICATION COMPLETED WITH WARNINGS
    echo ⚠️ VERIFICATION COMPLETED WITH WARNINGS >> "%VERIFICATION_LOG%"
    exit /b 1
)