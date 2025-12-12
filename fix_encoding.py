#!/usr/bin/env python3
"""
XpuMusic Build Fix Script
Fixes encoding issues and other build problems
"""

import os
import shutil
from pathlib import Path

def fix_file_encoding(file_path):
    """Fix encoding issues by converting to UTF-8"""
    try:
        # Read file with different encodings
        content = None
        for encoding in ['utf-8', 'gb2312', 'gbk', 'latin1']:
            try:
                with open(file_path, 'r', encoding=encoding) as f:
                    content = f.read()
                break
            except UnicodeDecodeError:
                continue
        
        if content is None:
            print(f"Could not read file: {file_path}")
            return False
            
        # Write back as UTF-8
        with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
            f.write(content)
        print(f"Fixed encoding: {file_path}")
        return True
        
    except Exception as e:
        print(f"Error fixing {file_path}: {e}")
        return False

def main():
    base_dir = Path("C:/workspace/XpuMusic")
    
    # Files with C4819 warnings that need encoding fixes
    files_to_fix = [
        "core/core_engine.cpp",
        "core/playback_engine.cpp", 
        "compat/sdk_implementations/service_base.cpp",
        "compat/sdk_implementations/abort_callback.impl.cpp",
        "compat/sdk_implementations/abort_callback.implified.h",
        "compat/sdk_implementations/file_info_impl.cpp",
        "compat/sdk_implementations/file_info_impl.h",
        "compat/sdk_implementations/common_includes.h",
        "compat/sdk_implementations/audio_sample.h",
        "compat/sdk_implementations/file_info_interface.h",
        "compat/sdk_implementations/audio_chunk_impl.cpp",
        "compat/sdk_implementations/audio_chunk_impl.h",
        "compat/sdk_implementations/audio_chunk_interface.h",
        "src/play_sound.cpp"
    ]
    
    print("Fixing encoding issues in XpuMusic project...")
    fixed_count = 0
    
    for relative_path in files_to_fix:
        full_path = base_dir / relative_path
        if full_path.exists():
            if fix_file_encoding(full_path):
                fixed_count += 1
        else:
            print(f"File not found: {full_path}")
    
    print(f"\nFixed {fixed_count} files with encoding issues.")
    print("\nNext steps:")
    print("1. Run the build again: build_all.bat")
    print("2. Address remaining compilation errors")
    print("3. Fix data conversion warnings if needed")

if __name__ == "__main__":
    main()