#!/bin/bash

echo "=== 完整音乐播放器功能测试 ==="
echo

# 生成测试文件
if [ ! -f test_short.wav ]; then
    echo "生成测试音频文件..."
    python3 -c "
import wave
import numpy as np

# 生成5秒的测试音频
sample_rate = 44100
duration = 5
frequency = 440

t = np.linspace(0, duration, sample_rate * duration, False)
wave_data = np.sin(2 * np.pi * frequency * t) * 0.5
wave_data = (wave_data * 32767).astype(np.int16)

stereo_data = np.zeros((len(wave_data) * 2,), dtype=np.int16)
stereo_data[0::2] = wave_data
stereo_data[1::2] = wave_data

with wave.open('test_short.wav', 'w') as wav_file:
    wav_file.setnchannels(2)
    wav_file.setsampwidth(2)
    wav_file.setframerate(sample_rate)
    wav_file.writeframes(stereo_data.tobytes())

print('Generated test_short.wav')
"
fi

echo "测试1: 显示帮助"
timeout 2 ./music-player-complete 2>/dev/null << EOF | grep -A 10 "COMMANDS" || true
help
quit
EOF
echo

echo "测试2: 加载文件并显示状态"
timeout 3 ./music-player-complete test_short.wav 2>/dev/null << EOF | grep -E "(LOAD|OK|File:|State:)" || true
status
quit
EOF
echo

echo "测试3: 播放控制"
timeout 5 ./music-player-complete test_short.wav 2>/dev/null << EOF | grep -E "(play|pause|stop)" || true
play
pause
play
stop
quit
EOF
echo

echo "测试完成！"