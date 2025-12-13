<#
.SYNOPSIS
    XpuMusic 构建系统脚本
.DESCRIPTION
    现代、可配置、跨平台的构建系统
.PARAM Configuration
    构建配置 (Debug/Release)
.PARAM Platform
    目标平台 (x64/x86)
.PARAM Clean
    是否清理构建目录
.PARAM Parallel
    并行构建
.PARAM Verbose
    详细输出
.PARAM Test
    构建后运行测试
.PARAM Package
    创建发布包
#>

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("x64", "x86")]
    [string]$Platform = "x64",

    [switch]$Clean,
    [switch]$Parallel = $true,
    [switch]$Verbose,
    [switch]$Test,
    [switch]$Package
)

# 错误处理
$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  XpuMusic 现代化构建系统" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 路径标准化
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = "$ProjectRoot\build\$Platform\$Configuration"
$SourceDir = $ProjectRoot
$OutputDir = "$BuildDir\bin\$Configuration"
$LibDir = "$BuildDir\lib\$Configuration"

Write-Host "项目根目录: $ProjectRoot" -ForegroundColor Gray
Write-Host "构建目录: $BuildDir" -ForegroundColor Gray
Write-Host "输出目录: $OutputDir" -ForegroundColor Gray
Write-Host ""

# 计时开始
$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

# 函数：检查依赖
function Test-Dependencies {
    param([string[]]$Tools)

    foreach ($Tool in $Tools) {
        try {
            $null = Get-Command $Tool -ErrorAction Stop
            Write-Host "✓ 找到工具: $Tool" -ForegroundColor Green
        }
        catch {
            Write-Host "✗ 缺失工具: $Tool" -ForegroundColor Red
            Write-Host "请安装 $Tool 并添加到 PATH" -ForegroundColor Yellow
            exit 1
        }
    }
}

# 函数：清理构建目录
function Clear-BuildDirectory {
    if (Test-Path $BuildDir) {
        Write-Host "清理构建目录..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force $BuildDir
        Write-Host "✓ 清理完成" -ForegroundColor Green
    }
}

# 函数：创建构建目录
function New-BuildDirectory {
    if (!(Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
        Write-Host "✓ 创建构建目录: $BuildDir" -ForegroundColor Green
    }

    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    New-Item -ItemType Directory -Path $LibDir -Force | Out-Null
}

# 函数：检测编译器
function Test-Compiler {
    Write-Host "检测编译环境..." -ForegroundColor Gray

    # 检测 MSVC
    try {
        $null = Get-Command "cl.exe" -ErrorAction Stop
        Write-Host "✓ 找到 Microsoft Visual C++ 编译器" -ForegroundColor Green

        # 获取版本信息
        $VersionOutput = cl.exe 2>&1 | Select-String "Microsoft.*Compiler Version"
        if ($VersionOutput) {
            Write-Host "  $VersionOutput" -ForegroundColor Gray
        }

        return "MSVC"
    }
    catch {
        Write-Host "✗ 未找到 MSVC 编译器" -ForegroundColor Red

        # 检测 Clang
        try {
            $null = Get-Command "clang" -ErrorAction Stop
            Write-Host "✓ 找到 Clang 编译器" -ForegroundColor Green
            return "Clang"
        }
        catch {
            Write-Host "✗ 未找到 Clang 编译器" -ForegroundColor Red
            Write-Host "请安装 Visual Studio 或 Clang" -ForegroundColor Yellow
            exit 1
        }
    }
}

# 函数：CMake 配置
function Invoke-CMakeConfiguration {
    param([string]$Generator, [string]$Arch)

    Write-Host "运行 CMake 配置..." -ForegroundColor Gray
    Write-Host "  生成器: $Generator" -ForegroundColor Gray
    Write-Host "  架构: $Arch" -ForegroundColor Gray

    Set-Location $BuildDir

    $CMakeArgs = @(
        "-S", $SourceDir
        "-B", "."
        "-G", $Generator
        "-A", $Arch
        "-DCMAKE_CONFIGURATION_TYPES=$Configuration"
        "-DCMAKE_BUILD_TYPE=$Configuration"
        "-DXPU_BUILD_TESTS=ON"
        "-DXPU_BUILD_EXAMPLES=ON"
    )

    if ($Verbose) {
        $CMakeArgs += "-DCMAKE_VERBOSE_MAKEFILE=ON"
    }

    Write-Host "执行: cmake $($CMakeArgs -join ' ')" -ForegroundColor Gray

    $Process = Start-Process "cmake" -ArgumentList $CMakeArgs -Wait -PassThru
    $Output = $Process | Out-String

    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ CMake 配置失败！" -ForegroundColor Red
        Write-Host $Output -ForegroundColor Red
        exit 1
    }

    Write-Host "✓ CMake 配置成功" -ForegroundColor Green
}

# 函数：构建目标
function Invoke-CMakeBuild {
    param([string[]]$Targets)

    Write-Host "开始构建..." -ForegroundColor Gray
    Write-Host "配置: $Configuration" -ForegroundColor Gray
    Write-Host "目标: $($Targets -join ', ')" -ForegroundColor Gray

    Set-Location $BuildDir

    $BuildArgs = @(
        "--config", $Configuration
        "--parallel"
    )

    if ($Verbose) {
        $BuildArgs += "--verbose"
    }

    foreach ($Target in $Targets) {
        Write-Host "  构建目标: $Target" -ForegroundColor Cyan
        $Process = Start-Process "cmake" -ArgumentList ($BuildArgs + @("--target", $Target)) -Wait -PassThru
        $Output = $Process | Out-String

        if ($LASTEXITCODE -neq 0) {
            Write-Host "✗ 构建 $Target 失败！" -ForegroundColor Red
            if ($Verbose) {
                Write-Host $Output -ForegroundColor Red
            }
            exit 1
        }

        Write-Host "  ✓ $Target 构建成功" -ForegroundColor Green
    }
}

# 函数：运行测试
function Invoke-Tests {
    Write-Host "运行测试..." -ForegroundColor Gray

    $TestExes = @(
        "$OutputDir\test_decoders.exe",
        "$OutputDir\test_audio_direct.exe",
        "$OutputDir\test_simple_playback.exe"
    )

    $PassedTests = 0
    $TotalTests = $TestExes.Count

    foreach ($TestExe in $TestExes) {
        if (Test-Path $TestExe) {
            Write-Host "运行测试: $(Split-Path -Leaf $TestExe)" -ForegroundColor Cyan

            try {
                $Process = Start-Process $TestExe -Wait -PassThru
                $Output = $Process | Out-String

                if ($LASTEXITCODE -eq 0) {
                    Write-Host "  ✓ 测试通过" -ForegroundColor Green
                    $PassedTests++
                } else {
                    Write-Host "  ✗ 测试失败" -ForegroundColor Red
                    if ($Verbose) {
                        Write-Host $Output -ForegroundColor Red
                    }
                }
            }
            catch {
                Write-Host "  ✗ 测试异常" -ForegroundColor Red
                Write-Host $_" -ForegroundColor Red
            }
        }
        else {
            Write-Host "✗ 测试不存在: $TestExe" -ForegroundColor Yellow
        }
    }

    Write-Host ""
    Write-Host "测试结果: $PassedTests/$TotalTests 通过" -ForegroundColor $(if ($PassedTests -eq $TotalTests) { "Green" } else { "Yellow" })
}

# 函数：创建发布包
function New-Package {
    Write-Host "创建发布包..." -ForegroundColor Gray

    $PackageDir = "$ProjectRoot\dist\XpuMusic-$Configuration-$Platform"
    if (Test-Path $PackageDir) {
        Remove-Item -Recurse -Force $PackageDir
    }

    New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null
    New-Item -ItemType Directory -Path "$PackageDir\bin" -Force | Out-Null
    New-Item -ItemType Directory -Path "$PackageDir\plugins" -Force | Out-Null
    New-Item -ItemType Directory -Path "$PackageDir\docs" -Force | Out-Null

    # 复制可执行文件
    Get-ChildItem -Path $OutputDir -Filter "*.exe" | Copy-Item -Destination "$PackageDir\bin" -Force

    # 复制插件
    Get-ChildItem -Path $OutputDir -Filter "*.dll" | Copy-Item -Destination "$PackageDir\plugins" -Force

    # 复制库文件
    Get-ChildItem -Path $LibDir -Filter "*.lib" | Copy-Item -Destination "$PackageDir\lib" -Force

    # 创建说明文件
    $ReadmeContent = @"
XpuMusic 发布包

构建信息:
- 配置: $Configuration
- 平台: $Platform
- 日期: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
- 提交: $(git -C $ProjectRoot describe --always --tags)

使用说明:
1. 可执行文件位于 /bin 目录
2. 音频插件位于 /plugins 目录
3. 运行示例: bin/music-player.exe <audio_file>
4. 插件测试: bin/test_decoders.exe

文档:
- README.md - 项目说明
- BUILDING.md - 构建指南
- API.md - API文档
"@

    $ReadmeContent | Out-File -FilePath "$PackageDir\README.md" -Encoding UTF8

    Write-Host "✓ 发布包创建: $PackageDir" -ForegroundColor Green
    Write-Host "  包含文件: $( (Get-ChildItem -Recurse $PackageDir).Count ) 个文件" -ForegroundColor Gray
}

# 函数：构建摘要
function Show-BuildSummary {
    param([timespan]$Duration)

    Write-Host "" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  构建摘要" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""

    Write-Host "配置:" -ForegroundColor Gray
    Write-Host "  构建: $Configuration 模式" -ForegroundColor Gray
    Write-Host "  平台: $Platform 架构" -ForegroundColor Gray
    Write-Host ""

    if ($Duration) {
        Write-Host "耗时: $($Duration.ToString('mm\:ss'))" -ForegroundColor Gray
    }

    Write-Host "输出:" -ForegroundColor Gray
    if (Test-Path $OutputDir) {
        $ExeCount = (Get-ChildItem -Path $OutputDir -Filter "*.exe").Count
        $DllCount = (Get-ChildItem -Path $OutputDir -Filter "*.dll").Count
        Write-Host "  可执行文件: $ExeCount 个" -ForegroundColor Green
        Write-Host "  插件: $DllCount 个" -ForegroundColor Green
    }

    if (Test-Path $LibDir) {
        $LibCount = (Get-ChildItem -Path $LibDir -Filter "*.lib").Count
        Write-Host "  库文件: $LibCount 个" -ForegroundColor Green
    }

    Write-Host ""
    Write-Host "状态:" -ForegroundColor Gray
    Write-Host "  ✅ 构建完成" -ForegroundColor Green
    if ($Test) {
        $TestExes = @("$OutputDir\test_decoders.exe", "$OutputDir\test_audio_direct.exe", "$OutputDir\test_simple_playback.exe")
        $AvailableTests = $TestExes.Where({ Test-Path $_ })
        if ($AvailableTests.Count -gt 0) {
            Write-Host "  ✅ 测试可用: $($AvailableTests.Count) 个" -ForegroundColor Green
        }
    }
}

# 主执行流程

try {
    # 1. 检查依赖
    Write-Host "检查构建依赖..." -ForegroundColor Cyan
    Test-Dependencies @("cmake", "git")

    # 2. 检测编译器
    $Compiler = Test-Compiler
    $Generator = if ($Compiler -eq "MSVC") { "Visual Studio 17 2022" } else { "Unix Makefiles" }
    $Arch = if ($Platform -eq "x64") { "x64" } else { "Win32" }

    # 3. 处理构建目录
    if ($Clean) {
        Clear-BuildDirectory
    }
    New-BuildDirectory

    # 4. CMake 配置
    Invoke-CMakeConfiguration -Generator $Generator -Arch $Arch

    # 5. 确定构建目标
    $BuildTargets = @(
        "core_engine",
        "platform_abstraction",
        "sdk_impl",
        "simple_wav_player_native",
        "final_wav_player",
        "music-player",
        "foobar2k-player",
        "test_decoders",
        "test_audio_direct",
        "test_simple_playback",
        "plugin_flac_decoder",
        "plugin_mp3_decoder",
        "plugin_wav_decoder"
    )

    # 6. 执行构建
    Invoke-CMakeBuild -Targets $BuildTargets

    # 7. 运行测试
    if ($Test) {
        Invoke-Tests
    }

    # 8. 创建发布包
    if ($Package) {
        New-Package
    }

    # 9. 显示摘要
    $Stopwatch.Stop()
    Show-BuildSummary -Duration $Stopwatch.Elapsed

} catch {
    Write-Host "构建失败: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "请检查:" -ForegroundColor Yellow
    Write-Host "1. 是否安装了 Visual Studio 或其他编译器" -ForegroundColor Yellow
    Write-Host "2. 是否有足够的磁盘空间" -ForegroundColor Yellow
    Write-Host "3. 网络连接是否正常" -ForegroundColor Yellow
    exit 1
}