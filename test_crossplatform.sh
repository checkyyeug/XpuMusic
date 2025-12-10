#!/bin/bash

echo "========================================"
echo "🌐 跨平台音频后端检测测试"
echo "========================================"
echo

# 测试 Linux 平台
echo "测试 1: Linux 平台 (当前)"
echo "----------------------------------------"
cmake -B build_linux 2>&1 | grep -E "(Audio Backend|selected|→)" || true
echo

# 模拟 Windows 平台
echo "测试 2: Windows 平台 (模拟)"
echo "----------------------------------------"
# 创建临时的 Windows 测试环境
export CMAKE_SYSTEM_NAME=Windows
cmake -B build_windows -DCMAKE_SYSTEM_NAME=Windows 2>&1 | grep -E "(Audio Backend|selected|→)" || true
unset CMAKE_SYSTEM_NAME
echo

# 模拟 macOS 平台
echo "测试 3: macOS 平台 (模拟)"
echo "----------------------------------------"
# 创建临时的 macOS 测试环境
export CMAKE_SYSTEM_NAME=Darwin
cmake -B build_macos -DCMAKE_SYSTEM_NAME=Darwin 2>&1 | grep -E "(Audio Backend|selected|→)" || true
unset CMAKE_SYSTEM_NAME
echo

echo "========================================"
echo "✅ 跨平台检测完成！"
echo "========================================"