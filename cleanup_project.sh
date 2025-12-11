#!/bin/bash

# ============================================================================
# XpuMusic Project Cleanup Script (Unix/Linux/macOS)
# Removes unnecessary files and directories while preserving all .md files
# ============================================================================

set -e

echo "========================================"
echo " XpuMusic Project Cleanup"
echo "========================================"
echo

# Show current size
echo "Current project size:"
du -sh . 2>/dev/null || echo "Size calculation failed"
echo

# Ask for confirmation
echo "This will remove:"
echo "  - All build directories (build_*)"
echo "  - Compiled binaries and executables"
echo "  - Large audio test files (.wav, .mp3)"
echo "  - Duplicate CMakeLists files"
echo "  - Old test executables"
echo "  - Temporary files"
echo
echo "All .md documentation files will be preserved."
echo
read -p "Proceed with cleanup? (y/N): " confirm
if [[ ! "$confirm" =~ ^[Yy] ]]; then
    echo "Cleanup cancelled."
    exit 0
fi

echo
echo "Starting cleanup..."

# Remove build directories
echo "[1/7] Removing build directories..."
rm -rf build_* 2>/dev/null || true

# Remove compiled executables
echo
echo "[2/7] Removing compiled executables..."
find . -type f -executable -name "*.exe" -delete 2>/dev/null || true
find . -type f -executable -name "*.out" -delete 2>/dev/null || true
find . -type f -executable -type f ! -name "*.sh" ! -name "*.py" ! -name "*.pl" -delete 2>/dev/null || true

# Remove large audio files (keep test_440hz.* for testing)
echo
echo "[3/7] Removing large audio files..."
find . -maxdepth 1 -name "*.wav" -not -name "test_440hz.wav" -delete 2>/dev/null || true
find . -maxdepth 1 -name "*.mp3" -not -name "test_440hz.mp3" -delete 2>/dev/null || true
echo "  Keeping test_440hz.wav and test_440hz.mp3 for testing"

# Remove duplicate CMakeLists files
echo
echo "[4/7] Removing duplicate CMakeLists files..."
rm -f CMakeLists_*.txt 2>/dev/null || true
rm -f CMakeLists_*.cmake 2>/dev/null || true

# Remove old test binaries in root
echo
echo "[5/7] Removing old test binaries..."
rm -f test_* 2>/dev/null || true
rm -f simple_player 2>/dev/null || true
rm -f music-player-* 2>/dev/null || true
rm -f final_wav_player 2>/dev/null || true
rm -f real_player 2>/dev/null || true
rm -f debug_audio 2>/dev/null || true

# Remove generated audio files in src/audio
echo
echo "[6/7] Removing generated audio files..."
rm -f src/audio/demo_*.wav 2>/dev/null || true

# Remove temporary and backup files
echo
echo "[7/7] Removing temporary files..."
find . -name "*.bak" -delete 2>/dev/null || true
find . -name "*.tmp" -delete 2>/dev/null || true
find . -name "*~" -delete 2>/dev/null || true
find . -name "*.orig" -delete 2>/dev/null || true
find . -name ".DS_Store" -delete 2>/dev/null || true
find . -name "Thumbs.db" -delete 2>/dev/null || true
find . -name ".gitkeep" -delete 2>/dev/null || true

# Remove specific unnecessary files
echo
echo "[Extra] Removing unnecessary files from root..."
rm -f "的声明" 2>/dev/null || true
rm -f "completion_chart.txt" 2>/dev/null || true
rm -f "rename_to_xpumusic.ps1" 2>/dev/null || true

# Clean up build artifacts
echo
echo "[Build] Cleaning build artifacts..."
find . -name "*.o" -delete 2>/dev/null || true
find . -name "*.obj" -delete 2>/dev/null || true
find . -name "*.a" -delete 2>/dev/null || true
find . -name "*.lib" -delete 2>/dev/null || true
find . -name "*.so" -delete 2>/dev/null || true
find . -name "*.dylib" -delete 2>/dev/null || true
find . -name "*.dll" -delete 2>/dev/null || true

echo
echo "========================================"
echo "Cleanup Complete!"
echo "========================================"
echo

# Show new size
echo "New project size:"
du -sh . 2>/dev/null || echo "Size calculation failed"
echo

echo "Preserved files:"
echo "  ✓ All .md documentation ($(find . -name "*.md" | wc -l) files)"
echo "  ✓ Source code (src/, core/, plugins/, etc.)"
echo "  ✓ CMakeLists.txt"
echo "  ✓ Build scripts"
echo "  ✓ Configuration files"
echo "  ✓ test_440hz.wav and test_440hz.mp3"
echo

echo "To rebuild the project, run:"
echo "  ./build.sh   (or build.bat on Windows)"
echo
echo "For a more aggressive cleanup (removing all but source code),"
echo "run: ./cleanup_project_aggressive.sh"