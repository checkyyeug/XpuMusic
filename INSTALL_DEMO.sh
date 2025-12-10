#!/bin/bash

echo "=== ALSA Dependency Installation Demo ==="
echo
echo "On this system, we found:"
echo

# 检查 ALSA 文件
echo "📁 ALSA Headers:"
find /usr/include -name "asound.h" 2>/dev/null | while read file; do
    echo "  ✓ $file"
done

echo
echo "📚 ALSA Libraries:"
find /usr/lib -name "libasound.so" 2>/dev/null | while read lib; do
    echo "  ✓ $lib"
    ls -l "$lib" | awk '{print "    Size: " $5 ", Link: " $11}'
done

echo
echo "🔍 CMake Detection:"
cd build 2>/dev/null || mkdir -p build && cd build
cmake .. 2>&1 | grep -i alsa

echo
echo "📋 Actual Installation Commands (requires sudo):"
echo
echo "# Ubuntu/Debian:"
echo "sudo apt-get update"
echo "sudo apt-get install -y libasound2-dev libflac-dev libmp3lame-dev"
echo
echo "# CentOS/RHEL:"
echo "sudo yum install -y alsa-lib-devel flac-devel lame-devel"
echo
echo "# Fedora:"
echo "sudo dnf install -y alsa-lib-devel flac-devel lame-devel"
echo
echo "# After installation, rebuild:"
echo "rm -rf build"
echo "cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "cmake --build build -j\$(nproc)"
echo
echo "Then test:"
echo "./build/bin/test_cross_platform"
echo "./build/bin/final_wav_player test_440hz.wav"