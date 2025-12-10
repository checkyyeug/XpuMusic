#!/bin/bash

# Install dependencies script for XpuMusic
# This script handles installation on multiple Linux distributions

set -e

echo "╔══════════════════════════════════════════════╗"
echo "║    Dependency Installer for Music Player      ║"
echo "╚══════════════════════════════════════════════╝"
echo

# Check if running as root or with sudo
if [[ $EUID -eq 0 ]]; then
    echo "✅ Running as root"
    USE_SUDO=""
elif command -v sudo >/dev/null 2>&1; then
    echo "✅ sudo is available"
    USE_SUDO="sudo"
else
    echo "⚠️  Warning: Not running as root and sudo not available"
    echo "   Please run this script as root or ensure sudo is configured"
    exit 1
fi

# Detect Linux distribution
if [ -f /etc/debian_version ]; then
    DISTRO="debian"
    INSTALL_CMD="$USE_SUDO apt-get install -y"
    UPDATE_CMD="$USE_SUDO apt-get update"
elif [ -f /etc/redhat-release ]; then
    if command -v dnf >/dev/null 2>&1; then
        DISTRO="fedora"
        INSTALL_CMD="$USE_SUDO dnf install -y"
        UPDATE_CMD="$USE_SUDO dnf makecache"
    else
        DISTRO="redhat"
        INSTALL_CMD="$USE_SUDO yum install -y"
        UPDATE_CMD="$USE_SUDO yum makecache"
    fi
elif [ -f /etc/arch-release ]; then
    DISTRO="arch"
    INSTALL_CMD="$USE_SUDO pacman -S --needed --noconfirm"
    UPDATE_CMD="$USE_SUDO pacman -Sy"
else
    echo "❌ Unsupported Linux distribution"
    echo "   Please manually install:"
    echo "   - build-essential / base-devel"
    echo "   - cmake"
    echo "   - pkg-config / pkgconf"
    echo "   - libasound2-dev / alsa-lib-devel"
    echo "   - libflac-dev / flac-devel"
    exit 1
fi

echo "📍 Detected distribution: $DISTRO"
echo

# Update package manager
echo "🔄 Updating package lists..."
$UPDATE_CMD

# Install base development tools
echo "🛠️  Installing base development tools..."
case $DISTRO in
    debian)
        $INSTALL_CMD build-essential cmake pkg-config git
        ;;
    fedora|redhat)
        $INSTALL_CMD gcc-c++ cmake pkgconfig git
        ;;
    arch)
        $INSTALL_CMD base-devel cmake pkgconf git
        ;;
esac

# Install audio libraries
echo "🔊 Installing audio libraries..."
case $DISTRO in
    debian)
        $INSTALL_CMD \
            libasound2-dev \
            libflac-dev \
            libmp3lame-dev \
            libvorbis-dev \
            libogg-dev \
            libopenmp-dev
        ;;
    fedora|redhat)
        $INSTALL_CMD \
            alsa-lib-devel \
            flac-devel \
            lame-devel \
            libvorbis-devel \
            libogg-devel \
            libgomp-devel
        ;;
    arch)
        $INSTALL_CMD \
            alsa-lib \
            flac \
            lame \
            libvorbis \
            libogg \
            openmp
        ;;
esac

# Verify installation
echo
echo "✅ Verifying installation..."
echo

# Check ALSA
if pkg-config --exists alsa; then
    echo "✓ ALSA: $(pkg-config --modversion alsa)"
else
    echo "⚠ ALSA: Not found via pkg-config (but may still be installed)"
fi

# Check FLAC
if pkg-config --exists flac; then
    echo "✓ FLAC: $(pkg-config --modversion flac)"
else
    echo "⚠ FLAC: Not found"
fi

# Check CMake
if command -v cmake >/dev/null 2>&1; then
    echo "✓ CMake: $(cmake --version | head -1)"
else
    echo "❌ CMake: Not found"
fi

# Check compiler
if command -v gcc >/dev/null 2>&1; then
    echo "✓ GCC: $(gcc --version | head -1)"
elif command -v clang >/dev/null 2>&1; then
    echo "✓ Clang: $(clang --version | head -1)"
else
    echo "❌ C++ compiler: Not found"
fi

echo
echo "🎯 Next steps:"
echo "   1. cd /data/checky/foobar/Qoder_foobar"
echo "   2. rm -rf build"
echo "   3. cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "   4. cmake --build build -j\$(nproc)"
echo "   5. ./build/bin/test_cross_platform"
echo

# Test audio permission
echo "🔍 Checking audio permissions..."
if groups $USER | grep -q audio; then
    echo "✓ User is in audio group"
else
    echo "⚠ Warning: User not in audio group"
    echo "  To fix: $USE_SUDO usermod -a -G audio $USER"
    echo "  Then logout and login again"
fi

echo
echo "╔══════════════════════════════════════════════╗"
echo "║    Installation Complete!                     ║"
echo "╚══════════════════════════════════════════════╝"
echo
echo "📚 More info:"
echo "   - README.md - Project overview"
echo "   - BUILD.md - Build instructions"
echo "   - docs/INSTALL_LINUX.md - Detailed Linux guide"