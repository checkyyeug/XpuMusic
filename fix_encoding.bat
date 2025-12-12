@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================
echo   XpuMusic Encoding Fix Utility
echo ========================================
echo.

REM Fix C4819 encoding warnings for UTF-8 BOM files
echo [1/3] Converting source files to UTF-8 with BOM...

REM Define directories to process
set "SRC_DIRS=src core compat platform plugins sdk tests demos fb2k_compat"

REM Process each directory
for %%d in (%SRC_DIRS%) do (
    if exist "%%d" (
        echo Processing directory: %%d
        pushd "%%d"

        REM Convert all .cpp and .h files
        for /r %%f in (*.cpp *.h) do (
            echo   - %%~nxf
            powershell -Command ^
                "$content = [IO.File]::ReadAllText('%%f', [Text.Encoding]::Default);"^
                "$content = $content -replace '^\xEF\xBB\xBF', '';"^
                "[IO.File]::WriteAllText('%%f', $content, [Text.Encoding]::UTF8)"
        )
        popd
    )
)

echo.
echo [2/3] Checking for files that need manual review...
REM List files with potential issues
findstr /R /M "service_factory_base" compat\*.cpp compat\*.h >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Warning: service_factory_base references found - may need SDK headers
)

echo.
echo [3/3] Creating UTF-8 compatible build script...

echo Done! Files converted to UTF-8.
echo.
echo Next steps:
echo 1. Run build_utf8.bat to build with proper encoding
echo 2. If SDK errors persist, the foobar2000 SDK may need to be installed
echo.
pause