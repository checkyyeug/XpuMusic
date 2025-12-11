#!/bin/bash

# ============================================================================
# XpuMusic Aggressive Cleanup Script
# Keeps only essential source code and all .md documentation
# ============================================================================

set -e

echo "========================================"
echo " XpuMusic Aggressive Cleanup"
echo "========================================"
echo

# Ask for confirmation
echo "⚠️  WARNING: This is an aggressive cleanup!"
echo "It will remove ALL files except:"
echo "  - All .md documentation files"
echo "  - Essential source code directories"
echo "  - Main CMakeLists.txt"
echo "  - Essential build scripts"
echo
read -p "Proceed with aggressive cleanup? (y/N): " confirm
if [[ ! "$confirm" =~ ^[Yy] ]]; then
    echo "Cleanup cancelled."
    exit 0
fi

echo
echo "Starting aggressive cleanup..."

# Create a temporary directory to preserve essential files
mkdir -p .temp_cleanup

# Backup essential configuration files
echo "[1/5] Backing up essential files..."
cp CMakeLists.txt .temp_cleanup/ 2>/dev/null || true
cp build.bat .temp_cleanup/ 2>/dev/null || true
cp build.sh .temp_cleanup/ 2>/dev/null || true
cp README.md .temp_cleanup/ 2>/dev/null || true

# Move essential source directories to temp
echo "[2/5] Preserving source directories..."
for dir in src core plugins platform sdk compat config cmake third_party; do
    if [ -d "$dir" ]; then
        mv "$dir" .temp_cleanup/
    fi
done

# Move all .md files to temp
echo "[3/5] Preserving documentation..."
find . -name "*.md" -not -path "./.temp_cleanup/*" -exec cp --parents {} .temp_cleanup/ \;

# Keep one test audio file
echo "[4/5] Preserving test audio..."
cp test_440hz.wav .temp_cleanup/ 2>/dev/null || true

# Remove everything else
echo "[5/5] Removing all other files..."
find . -maxdepth 1 -not -name "." -not -name ".." -not -name ".git" -not -name ".gitignore" -not -name ".temp_cleanup" -exec rm -rf {} + 2>/dev/null || true

# Restore preserved files
echo
echo "Restoring preserved files..."
cp -r .temp_cleanup/* . 2>/dev/null || true
rm -rf .temp_cleanup

# Set permissions
chmod +x build.sh 2>/dev/null || true

echo
echo "========================================"
echo "Aggressive Cleanup Complete!"
echo "========================================"
echo

echo "Project now contains only:"
echo "  ✓ Essential source code"
echo "  ✓ All .md documentation ($(find . -name "*.md" | wc -l) files)"
echo "  ✓ CMakeLists.txt"
echo "  ✓ Main build scripts"
echo "  ✓ test_440hz.wav (for testing)"
echo

echo "Size after cleanup:"
du -sh . 2>/dev/null || echo "Size calculation failed"

echo
echo "The project is now minimal and ready for development."
echo "To restore full functionality, you may need to:"
echo "  1. Install dependencies (see DEPENDENCIES.md)"
echo "  2. Run build.sh or build.bat"