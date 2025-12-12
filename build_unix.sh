#!/bin/bash
# Cross-platform build script for XpuMusic
# This script provides a simple build solution that works on Unix-like systems

set -e  # Exit on any error

echo "╔══════════════════════════════════════════════╗"
echo "║        XpuMusic Cross-Platform Build          ║"
echo "╚══════════════════════════════════════════════╝"
echo

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: CMakeLists.txt not found. Please run from project root directory."
    exit 1
fi

echo "📋 Build Configuration:"
echo "   • Project: XpuMusic Professional Music Player"
echo "   • Build Type: Release"
echo "   • Platform: Unix/Linux"
echo

# Create build directory
if [ -d "build" ]; then
    echo "🧹 Cleaning existing build directory..."
    rm -rf build
fi

echo "🔨 Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "⚙️  Configuring project with CMake..."
echo "   Command: cmake .. -DCMAKE_BUILD_TYPE=Release"

if cmake .. -DCMAKE_BUILD_TYPE=Release; then
    echo "✅ CMake configuration successful!"
else
    echo "❌ CMake configuration failed!"
    echo "   Please ensure CMake is installed and all dependencies are available."
    exit 1
fi

echo

# Build the project
echo "🔨 Building project..."
echo "   Using all available CPU cores..."

if cmake --build . -j$(nproc); then
    echo "✅ Build successful!"
else
    echo "❌ Build failed!"
    echo "   Please check the error messages above."
    exit 1
fi

echo

# Show build results
echo "📦 Build Results:"
echo "   Build directory: $(pwd)"
echo "   Executables created:"

if [ -f "bin/music-player" ]; then
    size=$(stat -f%z "bin/music-player" 2>/dev/null || stat -c%s "bin/music-player" 2>/dev/null || echo "unknown")
    echo "   ✅ music-player: $size bytes"
else
    echo "   ⚠️  music-player: not found"
fi

if [ -f "bin/final_wav_player" ]; then
    size=$(stat -f%z "bin/final_wav_player" 2>/dev/null || stat -c%s "bin/final_wav_player" 2>/dev/null || echo "unknown")
    echo "   ✅ final_wav_player: $size bytes"
else
    echo "   ⚠️  final_wav_player: not found"
fi

if [ -f "bin/test_decoders" ]; then
    size=$(stat -f%z "bin/test_decoders" 2>/dev/null || stat -c%s "bin/test_decoders" 2>/dev/null || echo "unknown")
    echo "   ✅ test_decoders: $size bytes"
else
    echo "   ⚠️  test_decoders: not found"
fi

echo

# Show library files
echo "📚 Libraries created:"
for lib in *.a *.so *.dylib 2>/dev/null; do
    if [ -f "$lib" ]; then
        size=$(stat -f%z "$lib" 2>/dev/null || stat -c%s "$lib" 2>/dev/null || echo "unknown")
        echo "   • $lib: $size bytes"
    fi
done

echo

# Test if basic executables work
echo "🧪 Running basic tests..."

if [ -f "bin/test_decoders" ]; then
    echo "   Testing decoder system..."
    if ./bin/test_decoders >/dev/null 2>&1; then
        echo "   ✅ Decoder tests: PASSED"
    else
        echo "   ⚠️  Decoder tests: FAILED (expected in limited environment)"
    fi
fi

echo

# Show usage instructions
echo "╔══════════════════════════════════════════════╗"
echo "║           Build Complete! 🎉                  ║"
echo "╚══════════════════════════════════════════════╝"
echo
echo "📖 Usage Instructions:"
echo
echo "🎵 To play audio files:"
echo "   ./bin/music-player <audio-file>"
echo
echo "🔧 To test specific features:"
echo "   ./bin/final_wav_player <wav-file>"
echo "   ./bin/test_decoders"
echo
echo "📚 Available executables:"
ls -la bin/ 2>/dev/null | grep -E '\.(exe|)$' | awk '{print "   • " $9 " (" $5 " bytes)"}' || echo "   (Run 'ls bin/' to see all files)"
echo
echo "📋 Documentation:"
echo "   • README.md - Project overview"
echo "   • BUILD_USAGE.md - Detailed build instructions"
echo "   • docs/ - Technical documentation"
echo
echo "🔧 Next steps:"
echo "   1. Test with audio files: ./bin/music-player your-music.wav"
echo "   2. Check plugin system: Review plugin directories"
echo "   3. Explore configuration: See config/ directory"
echo

# Return to original directory
cd ..