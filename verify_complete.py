#!/usr/bin/env python3
"""
项目完整度验证脚本
"""

import os
import subprocess
import sys
from pathlib import Path

def run_command(cmd, timeout=5):
    """运行命令并返回输出"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True,
                                text=True, timeout=timeout)
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", "Timeout"

def check_file_exists(path, description):
    """检查文件是否存在"""
    if Path(path).exists():
        print(f"✅ {description}: {path}")
        return True
    else:
        print(f"❌ {description}: {path} (NOT FOUND)")
        return False

def check_build_success():
    """检查构建是否成功"""
    success, _, _ = run_command("make -C build 2>&1", timeout=30)
    if success:
        print("✅ Build successful")
        return True
    else:
        print("❌ Build failed")
        return False

def check_executable_exists():
    """检查可执行文件"""
    return check_file_exists("build/bin/music-player", "Music player executable")

def check_audio_detection():
    """检查音频后端检测"""
    success, output, _ = run_command("cmake -B build 2>&1", timeout=10)
    if "ALSA found" in output or "Audio Backend" in output:
        print("✅ Audio backend detection working")
        return True
    else:
        print("❌ Audio backend detection failed")
        return False

def check_audio_playback():
    """检查音频播放功能（通过测试文件）"""
    # 创建测试 WAV
    if not Path("test_verify.wav").exists():
        os.system("""
python3 -c "
import wave
import numpy as np
sample_rate = 44100
duration = 1
t = np.linspace(0, duration, sample_rate * duration, False)
wave_data = (np.sin(2 * np.pi * 440 * t) * 16384).astype(np.int16)
stereo = np.zeros((len(wave_data) * 2,), dtype=np.int16)
stereo[0::2] = wave_data
stereo[1::2] = wave_data
with wave.open('test_verify.wav', 'w') as f:
    f.setnchannels(2)
    f.setsampwidth(2)
    f.setframerate(sample_rate)
    f.writeframes(stereo.tobytes())
" 2>/dev/null
""")

    success, _, _ = run_command("timeout 3 ./build/bin/music-player test_verify.wav 2>&1 << EOF\nhelp\nquit\nEOF", timeout=10)
    if success:
        print("✅ Audio playback functionality")
        return True
    else:
        print("❌ Audio playback functionality")
        return False

def main():
    print("=" * 60)
    print("🔍 XpuMusic 项目完整度验证")
    print("=" * 60)
    print()

    checks = [
        ("项目结构", lambda: check_file_exists("CMakeLists.txt", "CMakeLists.txt")),
        ("音频后端检测", check_audio_detection),
        ("构建成功", check_build_success),
        ("可执行文件", check_executable_exists),
        ("音频播放", check_audio_playback),
        ("文档", lambda: check_file_exists("docs/AUDIO_BACKEND_AUTO_DETECTION.md", "Audio backend docs")),
        ("测试程序", lambda: check_file_exists("src/audio/main.cpp", "Audio test program")),
        ("CMake模块", lambda: check_file_exists("cmake/AudioBackend.cmake", "Audio backend detection")),
    ]

    passed = 0
    total = len(checks)

    for name, check in checks:
        if check():
            passed += 1
        print()

    print("=" * 60)
    print(f"📊 验证结果: {passed}/{total} 项通过")
    print(f"🎯 完成度: {passed/total*100:.1f}%")

    if passed >= total * 0.9:
        print("🎉 项目高度完整！")
    elif passed >= total * 0.7:
        print("⚠️ 项目基本完成，还有一些工作要做")
    else:
        print("❌ 项目需要更多工作")

    print("=" * 60)

if __name__ == "__main__":
    main()