# XpuMusic 依赖管理指南

## 概述

XpuMusic 是一个跨平台的音频播放器项目，依赖项按平台分类。本文档提供详细的依赖安装指南。

## 核心依赖（必需）

### 构建工具
- **CMake** 3.20+ - 跨平台构建系统
- **C++ 编译器**：
  - Windows: MSVC 2019/2022 或 MinGW-w64
  - Linux: GCC 9+ 或 Clang 10+
  - macOS: Clang 10+

### 开发工具
- **Git** - 版本控制
- **Threads** - POSIX 线程支持

## 音频依赖（按功能分类）

### 1. 基础音频解码（内置）
- **WAV** - 无需额外依赖
- **MP3** - 使用 minimp3（头文件库，已包含）

### 2. 高级音频格式

#### FLAC (Free Lossless Audio Codec)
- **Windows**:
  ```bash
  choco install flac
  ```
  或从 https://xiph.org/downloads/ 下载

- **Linux**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libflac-dev

  # CentOS/RHEL
  sudo yum install flac-devel

  # Arch Linux
  sudo pacman -S flac
  ```

- **macOS**:
  ```bash
  brew install flac
  ```

#### OGG Vorbis
- **Windows**:
  ```bash
  choco install libvorbis
  ```

- **Linux**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libvorbis-dev

  # CentOS/RHEL
  sudo yum install libvorbis-devel

  # Arch Linux
  sudo pacman -S libvorbis
  ```

- **macOS**:
  ```bash
  brew install libvorbis
  ```

#### AAC (Advanced Audio Coding)
- **Windows**: 使用 fdkaac
- **Linux**: `sudo apt-get install libfdk-aac-dev`
- **macOS**: `brew install fdkaac`

### 3. 图形界面依赖（可选）

#### Qt 6.x (推荐)
- **Windows**:
  ```bash
  choco install qt6-base
  choco install qt6-tools
  ```

- **Linux**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install qt6-base-dev

  # CentOS/RHEL
  sudo yum install qt6-qtbase-devel

  # Arch Linux
  sudo pacman -S qt6-base
  ```

- **macOS**:
  ```bash
  brew install qt@6
  ```

#### imgui (轻量级GUI)
- **Windows**: 预编译版本可用
- **Linux**: `sudo apt-get install imgui`
- **macOS**: `brew install imgui`

## 平台特定依赖

### Windows
- **Visual Studio 2019/2022** (Community Edition 免费)
  - 勾选 "使用 C++ 的桌面开发" 工作负载
  - 包含 MSVC 编译器和 Windows SDK

- **Windows SDK** 10.0 或更新版本

### Linux
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake git

# 开发包
sudo apt-get install libasound2-dev libpulse-dev
sudo apt-get install libjack-jackd2-dev
sudo apt-get install libsdl2-dev
```

### macOS
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 开发工具
brew install cmake git
```

## 可选依赖

### 测试框架
- **Google Test**: `sudo apt-get install libgtest-dev` (Linux)
- **Catch2**: 头文件库（无需安装）

### 性能分析
- **Valgrind** (Linux): `sudo apt-get install valgrind`
- **Visual Studio Profiler** (Windows)
- **Instruments** (macOS)

### 文档生成
- **Doxygen**: `sudo apt-get install doxygen` (Linux)
- **Graphviz** (用于生成依赖图): `sudo apt-get install graphviz`

## 项目结构中的依赖

### 头文件库（已包含）
```
sdk/external/
├── minimp3/          # MP3 解码（头文件）
└── ...              # 其他头文件库
```

### 插件依赖
```
plugins/
├── decoders/
│   ├── mp3_decoder.cpp       # 使用 minimp3
│   ├── flac_decoder.cpp      # 需要 FLAC 库
│   └── ogg_decoder.cpp       # 需要 libvorbis
├── dsp/
│   └── resampler.cpp         # 算法实现（无外部依赖）
└── output/
    ├── wasapi_output.cpp      # Windows 原生
    ├── coreaudio_output.cpp   # macOS 原生
    └── alsa_output.cpp        # Linux ALSA
```

## 快速安装指南

### Windows
```powershell
# 使用 chocolatey (推荐)
iwr -useb c:\chocolatey\install.ps1 | iex
choco install cmake git flac qt6-base

# 或手动安装
# 1. Visual Studio 2022 Community
# 2. CMake
# 3. Git
# 4. FLAC from https://xiph.org/downloads/
```

### Ubuntu/Debian
```bash
# 基础工具
sudo apt-get update
sudo apt-get install build-essential cmake git

# 音频库
sudo apt-get install libflac-dev libvorbis-dev

# 可选：GUI
sudo apt-get install qt6-base-dev
```

### macOS
```bash
# 安装 Xcode 和 Homebrew
xcode-select --install
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 基础工具
brew install cmake git

# 音频库
brew install flac libvorbis

# 可选：GUI
brew install qt@6
```

## 环境变量设置

### Windows
```cmd
REM 设置 FLAC 环境变量
set FLAC_ROOT=C:\Program Files\FLAC
set FLAC_INCLUDE=%FLAC_ROOT%\include
set FLAC_LIB=%FLAC_ROOT%\lib
```

### Linux/macOS
```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
export FLAC_ROOT=/usr/local
export FLAC_INCLUDE=$FLAC_ROOT/include
export FLAC_LIB=$FLAC_ROOT/lib

# 或者使用 pkg-config（推荐）
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
```

## 故障排除

### 1. CMake 找不到库
```bash
# 检查 pkg-config
pkg-config --list-all | grep flac

# 手动指定路径
cmake -DFLAC_ROOT=/usr/local ..
```

### 2. Linux ALSA 错误
```bash
# 安装开发包
sudo apt-get install libasound2-dev

# 检查 ALSA
aplay -l
```

### 3. macOS CoreAudio 错误
```bash
# 确保 Xcode 命令行工具已安装
xcode-select --install

# 验证
clang --version
```

### 4. Windows Visual Studio 错误
- 确保安装了 "使用 C++ 的桌面开发"
- 使用 x64 Native Tools 命令提示符

## 依赖版本矩阵

| 组件 | 最低版本 | 推荐版本 | 备注 |
|------|----------|----------|------|
| CMake | 3.20 | 3.29+ | 3.29+ 支持更多功能 |
| GCC | 9 | 11+ | 11+ 有更好 C++20 支持 |
| Clang | 10 | 13+ | |
| MSVC | 2019 | 2022 | |
| FLAC | 1.3.0 | 1.4.2+ | |
| Qt | 5.15 | 6.4+ | 5.15+ LTS 也支持 |

## 下一步

安装完依赖后，运行构建：
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 联系支持

如果遇到依赖安装问题：
1. 检查平台的包管理器
2. 查看项目的 Issues 页面
3. 查看特定组件的官方文档

## 许可证

- 项目本身使用 MIT 许可证
- 依赖库使用各自的开源许可证