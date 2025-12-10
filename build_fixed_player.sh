#!/bin/bash

echo "Building fixed music player..."
cd /data/checky/foobar/XpuMusic

# Clean previous build
rm -f music-player-fixed

# Compile the fixed version
g++ -std=c++17 \
    -pthread \
    -o music-player-fixed \
    src/music_player_fixed.cpp

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo ""
    echo "Usage:"
    echo "  ./music-player-fixed <wav_file>"
    echo ""
    echo "Example:"
    echo "  ./music-player-fixed test_440hz.wav"
else
    echo "❌ Build failed!"
    exit 1
fi