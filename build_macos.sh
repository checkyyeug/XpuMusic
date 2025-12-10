#!/bin/bash

# macOS 构建脚本

echo "========================================"
echo "   macOS 构建脚本 - XpuMusic"
echo "========================================"
echo

# 检查 Xcode Command Line Tools
if ! xcode-select -p &>/dev/null; then
    echo "[ERROR] 未找到 Xcode Command Line Tools"
    echo "请运行: xcode-select --install"
    exit 1
fi

echo "[OK] Xcode Command Line Tools 已安装"
echo

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] 未找到 CMake"
    echo "请安装 CMake: brew install cmake"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
echo "[OK] CMake 版本: $CMAKE_VERSION"
echo

# 创建构建目录
mkdir -p build
cd build

# 配置项目
echo "[1/2] 配置项目..."
cmake .. -DCMAKE_BUILD_TYPE=Release -G Xcode
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake 配置失败"
    exit 1
fi
echo "[OK] 项目配置成功"
echo

# 构建项目
echo "[2/2] 构建项目..."
cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "[ERROR] 构建失败"
    exit 1
fi
echo "[OK] 构建成功"
echo

# 显示结果
echo "========================================"
echo "   构建完成！"
echo "========================================"
echo
echo "可执行文件位于: build/Release/music-player"
echo
echo "运行示例:"
echo "  cd Release"
echo "  ./music-player your_audio_file.wav"
echo
echo "或者使用 Xcode 打开:"
echo "  open build/XpuMusic.xcodeproj"