#!/bin/bash

# ============================================================================
# XpuMusic Dependencies Installer for Linux/macOS
# ============================================================================

set -e

echo "========================================"
echo "  XpuMusic Dependencies Installer"
echo "========================================"
echo

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    PKG_MANAGER="apt"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    PKG_MANAGER="brew"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Detected OS: $OS"
echo "Package manager: $PKG_MANAGER"
echo

# Function to install with confirmation
install_with_confirm() {
    local package=$1
    local description=$2

    echo -n "  Installing $package ($description)... "

    if [ "$OS" = "linux" ]; then
        if command -v apt-get >/dev/null; then
            sudo apt-get install -y "$package" >/dev/null 2>&1
        else
            echo "ERROR: apt-get not found. Please use your distribution's package manager."
            return 1
        fi
    else
        if command -v brew >/dev/null; then
            brew install "$package" >/dev/null 2>&1
        else
            echo "ERROR: Homebrew not found. Please install from https://brew.sh/"
            echo "  /bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
            return 1
        fi
    fi

    echo "✓ Done"
}

# ========================================
echo "Installing Required Dependencies"
echo "========================================"

echo
echo "[1/5] Build tools..."
if [ "$OS" = "linux" ]; then
    sudo apt-get update
    install_with_confirm "build-essential" "C/C++ compiler and tools"
    install_with_confirm "cmake" "Cross-platform build system"
    install_with_confirm "git" "Version control"
else
    # macOS: Xcode command line tools
    if ! xcode-select -p /usr/bin/git 1>/dev/null; then
        echo "  Installing Xcode command line tools..."
        xcode-select --install
    fi

    # Install Homebrew if not present
    if ! command -v brew >/dev/null; then
        echo "  Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    install_with_confirm "cmake" "Cross-platform build system"
    install_with_confirm "git" "Version control"
fi

echo
echo "[2/5] Audio development libraries..."
if [ "$OS" = "linux" ]; then
    install_with_confirm "libflac-dev" "FLAC encoder/decoder"
    install_with_confirm "libvorbis-dev" "OGG Vorbis encoder/decoder"
    install_with_confirm "libsdl2-dev" "Dynamic linking"
    install_with_confirm "libasound2-dev" "ALSA sound system"
else
    install_with_confirm "flac" "FLAC encoder/decoder"
    install_with_confirm "libvorbis" "OGG Vorbis encoder/decoder"
    install_with_confirm "sdl2" "Simple DirectMedia Layer"
fi

echo
echo "[3/5] Additional audio libraries..."
if [ "$OS" = "linux" ]; then
    install_with_confirm "libpulse-dev" "PulseAudio sound server" 2>/dev/null || true
    install_with_confirm "libjack-jackd2-dev" "JACK Audio Connection Kit" 2>/dev/null || true
else
    install_with_confirm "portaudio" "Cross-platform audio I/O"
fi

# ========================================
echo "Installing Optional Dependencies"
echo "========================================"

echo
echo "[Optional] Install Qt6 for GUI development? (y/n)"
read -r response
if [[ $response =~ ^[Yy] ]]; then
    if [ "$OS" = "linux" ]; then
        install_with_confirm "qt6-base-dev" "Qt6 GUI framework"
        install_with_confirm "qt6-tools-dev" "Qt6 development tools"
    else
        install_with_confirm "qt@6" "Qt6 GUI framework"
    fi
fi

echo
echo "[Optional] Install documentation tools? (y/n)"
read -r response
if [[ $response =~ ^[Yy] ]]; then
    install_with_confirm "doxygen" "Documentation generator"
    install_with_confirm "graphviz" "Dependency graph generator"
fi

# ========================================
echo "Platform-Specific Setup"
echo "========================================"

if [ "$OS" = "linux" ]; then
    echo
    echo "Linux detected. Setting up for audio..."
    echo "  - ALSA: $(aplay -l 2>/dev/null | grep "card" | wc -l) audio cards found"

    # Check if user is in audio group
    if groups $USER | grep -q audio; then
        echo "  - User $USER is in the audio group ✓"
    else
        echo "  - Adding user to audio group..."
        sudo usermod -aG audio $USER
        echo "  ✓ Added to audio group. Please log out and log back in for changes to take effect."
    fi
else
    echo
    echo "macOS detected. CoreAudio is built-in."
fi

# ========================================
echo "Environment Setup"
echo "======================================="

echo
echo "Creating pkg-config files for FLAC..."
if [ ! -f /usr/local/lib/pkgconfig/flac.pc ] && [ -d /usr/local/lib ]; then
    sudo mkdir -p /usr/local/lib/pkgconfig
    cat << EOF | sudo tee /usr/local/lib/pkgconfig/flac.pc
prefix=/usr/local
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: FLAC
Description: Free Lossless Audio Codec
Version: 1.4.2
Requires:
Libs: -L\${libdir} -lFLAC -lm
Cflags: -I\${includedir}
EOF
    echo "  ✓ Created /usr/local/lib/pkgconfig/flac.pc"
fi

# ========================================
echo "Verification"
echo "=======================================

echo
echo "Verifying installation..."
echo

echo "CMake:"
cmake --version | head -n 1

echo
echo "Compiler:"
if [ "$OS" = "linux" ]; then
    gcc --version | head -n 1
    g++ --version | head -n 1
else
    clang --version | head -n 1
fi

echo
echo "FLAC:"
if command -v flac >/dev/null; then
    echo "  ✓ $(flac --version | head -n 1)"
else
    echo "  ✗ flac not found"
fi

if [ "$OS" = "linux" ] && command -v pkg-config >/dev/null; then
    echo
    echo "PKG-Config FLAC:"
    if pkg-config --exists flac 2>/dev/null; then
        echo "  ✓ $(pkg-config --modversion flac)"
    else
        echo "  ✗ flac.pc not found"
    fi
fi

echo
echo "========================================"
echo "Installation Complete!"
echo "========================================"
echo

echo "Successfully installed dependencies for XpuMusic on $OS."
echo
echo "Next steps:"
echo "  1. If you added yourself to audio group (Linux), log out and log back in"
echo " 2. Navigate to the project directory"
echo " 3. Run:"
echo "     mkdir build"
echo "     cd build"
echo "     cmake .."
echo "     cmake --build . --config Release"
echo
echo "For build troubleshooting, see: ./BUILD.md"
echo

# Save environment variables to .env file
echo "export FLAC_ROOT=/usr/local" > .env
echo "export FLAC_INCLUDE=/usr/local/include" >> .env
echo "export FLAC_LIB=/usr/local/lib" >> .env
echo "export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:\$PKG_CONFIG_PATH" >> .env

echo
echo "Environment variables saved to .env file."
echo "Source it with: source .env (Linux) or use the env variables above."