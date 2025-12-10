#!/bin/bash

# Quick Start Script for Cross-Platform Music Player
# This script installs dependencies and builds the project

set -e  # Exit on any error

echo "╔══════════════════════════════════════════════╗"
echo "║    Cross-Platform Music Player Quick Start    ║"
echo "╚══════════════════════════════════════════════╝"
echo

# Detect Linux distribution
if command -v apt-get >/dev/null 2>&1; then
    DISTRO="debian"
elif command -v dnf >/dev/null 2>&1; then
    DISTRO="redhat"
elif command -v pacman >/dev/null 2>&1; then
    DISTRO="arch"
else
    echo "⚠️  Unsupported distribution. Please install dependencies manually."
    exit 1
fi

echo "Detected Linux distribution: $DISTRO"
echo

# Install dependencies
echo "📦 Installing dependencies..."
case $DISTRO in
    debian)
        echo "Using apt-get..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            libasound2-dev \
            libflac-dev \
            libmp3lame-dev \
            libvorbis-dev \
            libogg-dev \
            libopenmp-dev
        ;;
    redhat)
        echo "Using dnf..."
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            pkgconfig \
            alsa-lib-devel \
            flac-devel \
            lame-devel \
            libvorbis-devel \
            libogg-devel \
            libgomp-devel
        ;;
    arch)
        echo "Using pacman..."
        sudo pacman -S --needed \
            base-devel \
            cmake \
            pkgconf \
            alsa-lib \
            flac \
            lame \
            libvorbis \
            libogg \
            openmp
        ;;
esac

echo "✅ Dependencies installed successfully!"
echo

# Clean previous build
echo "🧹 Cleaning previous build..."
rm -rf build
echo "✅ Build directory cleaned"
echo

# Configure and build
echo "🔨 Configuring and building..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
echo "✅ Configuration complete"
echo

cmake --build build -j$(nproc)
echo "✅ Build complete!"
echo

# Run tests
echo "🧪 Running tests..."
echo
echo "--- Platform Detection Test ---"
./build/bin/test_cross_platform
echo
echo "--- WAV Playback Test ---"
if [ ! -f "test_440hz.wav" ]; then
    echo "Creating test WAV file..."
    python3 -c "
import struct, math
sample_rate = 44100
duration = 2.0
frequency = 440.0
with open('test_440hz.wav', 'wb') as f:
    f.write(b'RIFF')
    f.write(struct.pack('<I', 36 + int(sample_rate * duration * 4)))
    f.write(b'WAVE')
    f.write(b'fmt ')
    f.write(struct.pack('<I', 16))
    f.write(struct.pack('<HHIIHH', 1, 2, sample_rate, sample_rate * 4, 4, 16))
    f.write(b'data')
    f.write(struct.pack('<I', int(sample_rate * duration * 4)))
    for i in range(int(sample_rate * duration)):
        t = i / sample_rate
        value = int(32767 * math.sin(2 * math.pi * frequency * t))
        f.write(struct.pack('<hh', value, value))
    "
fi

./build/bin/final_wav_player test_440hz.wav
echo

# Check if audio is working
echo "🔊 Checking audio backend..."
if ./build/bin/test_cross_platform 2>&1 | grep -q "ALSA.*Available"; then
    echo "✅ Real audio backend is active!"
    echo "   You should hear audio when playing files."
else
    echo "⚠️  Using stub audio backend."
    echo "   Audio will be processed but not played."
    echo "   This is expected if ALSA is not properly configured."
fi

echo
echo "╔══════════════════════════════════════════════╗"
echo "║    Installation Complete!                   ║"
echo "╚══════════════════════════════════════════════╝"
echo
echo "📚 Documentation:"
echo "   • README.md - Overview and features"
echo "   • BUILD.md - Detailed build instructions"
echo "   • docs/INSTALL_LINUX.md - Linux installation guide"
echo
echo "🎵 Usage:"
echo "   ./build/bin/music-player your-music-file.wav"
echo "   ./build/bin/music-player --backend alsa your-music-file.flac"
echo
echo "🔧 Troubleshooting:"
echo "   • If no audio: check 'groups \$USER' for audio group"
echo "   • Add user to audio: sudo usermod -a -G audio \$USER"
echo "   • Then logout and login again"
echo