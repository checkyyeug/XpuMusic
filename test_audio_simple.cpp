/**
 * @file test_audio_simple.cpp
 * @brief Simple test to demonstrate audio backend detection
 * @date 2025-12-10
 */

#include <iostream>
#include <fstream>
#include <vector>

int main() {
    std::cout << "=== 音频后端检测测试 ===\n\n";

    // 检测当前系统
#ifdef __linux__
    std::cout << "操作系统: Linux\n";
    #ifdef HAVE_ALSA
        std::cout << "音频后端: ✓ ALSA 已检测到\n";
        std::cout << "状态: 可以使用原生Linux音频\n";
    #else
        std::cout << "音频后端: ⚠ 使用Stub模式（静音）\n";
        std::cout << "状态: 需要安装ALSA开发库\n";
        std::cout << "安装命令: sudo apt-get install libasound2-dev\n";
    #endif
#elif _WIN32
    std::cout << "操作系统: Windows\n";
    std::cout << "音频后端: ✓ WASAPI\n";
#elif __APPLE__
    std::cout << "操作系统: macOS\n";
    std::cout << "音频后端: ✓ CoreAudio\n";
#else
    std::cout << "操作系统: 其他\n";
    std::cout << "音频后端: 使用Stub模式\n";
#endif

    std::cout << "\n=== 测试完成 ===\n";
    return 0;
}