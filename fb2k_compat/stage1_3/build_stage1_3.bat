@echo off
REM foobar2000兼容层 - 阶段1.3 Windows构建脚本
REM 构建标准DSP效果器和WASAPI输出设备

echo ==========================================
echo foobar2000兼容层 - 阶段1.3 Windows构建脚本
echo ==========================================
echo 构建时间: %date% %time%
echo.

REM 设置构建参数
set BUILD_TYPE=Release
set CXX_STANDARD=17
set OUTPUT_DIR=build_stage1_3_win
set TEST_OUTPUT_DIR=build_test_stage1_3_win
set CXX_COMPILER=g++

REM 检测Visual Studio环境
where cl >nul 2>nul
if %errorlevel% == 0 (
    echo 检测到Visual Studio环境，使用MSVC编译器
    set CXX_COMPILER=cl
    set MSVC_MODE=1
) else (
    echo 使用MinGW编译器
    set MSVC_MODE=0
)

echo 编译器: %CXX_COMPILER%
echo 构建类型: %BUILD_TYPE%
echo C++标准: C++%CXX_STANDARD%
echo.

REM 创建输出目录
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%TEST_OUTPUT_DIR%" mkdir "%TEST_OUTPUT_DIR%"

REM 基础编译标志
set BASE_FLAGS=-std=c++%CXX_STANDARD% -Wall -Wextra -O2
set DEBUG_FLAGS=-g -DDEBUG
set RELEASE_FLAGS=-DNDEBUG -O3

REM 根据构建类型选择标志
if "%BUILD_TYPE%" == "Debug" (
    set CXX_FLAGS=%BASE_FLAGS% %DEBUG_FLAGS%
) else (
    set CXX_FLAGS=%BASE_FLAGS% %RELEASE_FLAGS%
)

REM 包含路径
set INCLUDE_PATHS=-I. -I..\ -I..\..\

REM Windows平台库
set PLATFORM_LIBS=-lws2_32 -lole32 -loleaut32 -luuid -lavrt

echo ==========================================
echo 编译核心库...
echo ==========================================

REM DSP源文件
set DSP_SOURCES=dsp_equalizer.cpp dsp_reverb.cpp dsp_compressor.cpp dsp_manager_impl.cpp audio_block_impl.cpp

REM 输出源文件
set OUTPUT_SOURCES=output_wasapi_impl.cpp output_device_base.cpp

REM 处理器源文件
set PROCESSOR_SOURCES=audio_processor_impl.cpp ring_buffer.cpp performance_monitor.cpp

REM 通用源文件
set COMMON_SOURCES=abort_callback.cpp audio_format_utils.cpp math_utils.cpp

REM 编译DSP模块
echo 编译DSP模块...
for %%f in (%DSP_SOURCES%) do (
    if exist "%%f" (
        echo   编译: %%f
        if %MSVC_MODE% == 1 (
            cl /c /EHsc /std:c++%CXX_STANDARD% /O2 %INCLUDE_PATHS% "%%f" /Fo"%OUTPUT_DIR%\%%~nf.obj"
        ) else (
            %CXX_COMPILER% %CXX_FLAGS% %INCLUDE_PATHS% -c "%%f" -o "%OUTPUT_DIR%\%%~nf.o"
        )
        if !errorlevel! neq 0 (
            echo 错误: 编译 %%f 失败
            exit /b 1
        )
    ) else (
        echo   警告: 找不到文件 %%f
    )
)

REM 编译输出模块
echo 编译输出模块...
for %%f in (%OUTPUT_SOURCES%) do (
    if exist "%%f" (
        echo   编译: %%f
        if %MSVC_MODE% == 1 (
            cl /c /EHsc /std:c++%CXX_STANDARD% /O2 %INCLUDE_PATHS% "%%f" /Fo"%OUTPUT_DIR%\%%~nf.obj"
        ) else (
            %CXX_COMPILER% %CXX_FLAGS% %INCLUDE_PATHS% -c "%%f" -o "%OUTPUT_DIR%\%%~nf.o"
        )
        if !errorlevel! neq 0 (
            echo 错误: 编译 %%f 失败
            exit /b 1
        )
    ) else (
        echo   警告: 找不到文件 %%f
    )
)

REM 编译处理器模块
echo 编译处理器模块...
for %%f in (%PROCESSOR_SOURCES%) do (
    if exist "%%f" (
        echo   编译: %%f
        if %MSVC_MODE% == 1 (
            cl /c /EHsc /std:c++%CXX_STANDARD% /O2 %INCLUDE_PATHS% "%%f" /Fo"%OUTPUT_DIR%\%%~nf.obj"
        ) else (
            %CXX_COMPILER% %CXX_FLAGS% %INCLUDE_PATHS% -c "%%f" -o "%OUTPUT_DIR%\%%~nf.o"
        )
        if !errorlevel! neq 0 (
            echo 错误: 编译 %%f 失败
            exit /b 1
        )
    ) else (
        echo   警告: 找不到文件 %%f
    )
)

REM 编译通用模块
echo 编译通用模块...
for %%f in (%COMMON_SOURCES%) do (
    if exist "%%f" (
        echo   编译: %%f
        if %MSVC_MODE% == 1 (
            cl /c /EHsc /std:c++%CXX_STANDARD% /O2 %INCLUDE_PATHS% "%%f" /Fo"%OUTPUT_DIR%\%%~nf.obj"
        ) else (
            %CXX_COMPILER% %CXX_FLAGS% %INCLUDE_PATHS% -c "%%f" -o "%OUTPUT_DIR%\%%~nf.o"
        )
        if !errorlevel! neq 0 (
            echo 错误: 编译 %%f 失败
            exit /b 1
        )
    ) else (
        echo   警告: 找不到文件 %%f
    )
)

REM 链接核心库
echo 链接核心库...
if %MSVC_MODE% == 1 (
    REM MSVC模式 - 创建DLL
    link /DLL /OUT:"%OUTPUT_DIR%\stage1_3_core.dll" "%OUTPUT_DIR%\*.obj" %PLATFORM_LIBS%
    if !errorlevel! neq 0 (
        echo 错误: 链接核心库失败
        exit /b 1
    )
) else (
    REM MinGW模式 - 创建共享库
    set CORE_OBJECTS=
    for %%f in (%DSP_SOURCES% %OUTPUT_SOURCES% %PROCESSOR_SOURCES% %COMMON_SOURCES%) do (
        set CORE_OBJECTS=!CORE_OBJECTS! "%OUTPUT_DIR%\%%~nf.o"
    )
    
    %CXX_COMPILER% %CXX_FLAGS% -shared !CORE_OBJECTS! %PLATFORM_LIBS% -o "%OUTPUT_DIR%\libstage1_3_core.dll"
    if !errorlevel! neq 0 (
        echo 错误: 链接核心库失败
        exit /b 1
    )
)

echo 核心库构建完成: %OUTPUT_DIR%\stage1_3_core.dll
echo.

echo ==========================================
echo 编译测试程序...
echo ==========================================

REM 测试源文件
set TEST_SOURCES=test_stage1_3.cpp

REM 编译测试程序
for %%f in (%TEST_SOURCES%) do (
    if exist "%%f" (
        set test_name=%%~nf
        echo 编译测试: %%f
        
        if %MSVC_MODE% == 1 (
            cl /EHsc /std:c++%CXX_STANDARD% /O2 %INCLUDE_PATHS% "%%f" /Fe"%TEST_OUTPUT_DIR%\!test_name!.exe" /link /LIBPATH:"%OUTPUT_DIR%" stage1_3_core.lib %PLATFORM_LIBS%
        ) else (
            %CXX_COMPILER% %CXX_FLAGS% %INCLUDE_PATHS% "%%f" -L"%OUTPUT_DIR%" -lstaging1_3_core -o "%TEST_OUTPUT_DIR%\!test_name!.exe" %PLATFORM_LIBS%
        )
        
        if !errorlevel! neq 0 (
            echo 错误: 编译测试程序 %%f 失败
            exit /b 1
        )
    ) else (
        echo   警告: 找不到测试文件 %%f
    )
)

echo 测试程序构建完成
echo.

echo ==========================================
echo 构建完成！
echo ==========================================
echo 输出目录: %OUTPUT_DIR%
echo 测试目录: %TEST_OUTPUT_DIR%
echo.
echo 生成的文件:
dir /b "%OUTPUT_DIR%"
echo.
echo 测试程序:
dir /b "%TEST_OUTPUT_DIR%"
echo.
echo 运行测试:
echo   %TEST_OUTPUT_DIR%\test_stage1_3.exe
echo.
echo 构建时间: %date% %time%
echo ==========================================

REM 如果构建成功，运行基本测试
if exist "%TEST_OUTPUT_DIR%\test_stage1_3.exe" (
    echo.
    echo 运行基本功能测试...
    echo ==========================================
    
    REM 设置库路径
    set PATH=%CD%\%OUTPUT_DIR%;%PATH%
    
    REM 运行基本测试（快速模式）
    "%TEST_OUTPUT_DIR%\test_stage1_3.exe" --duration 2 --quiet
    if !errorlevel! neq 0 (
        echo 警告: 基本测试失败，但构建已完成
    )
)

exit /b 0