# XpuMusic 重命名总结

本文档记录了将项目从 foobar2000 相关名称重命名为 XpuMusic 的所有更改。

## 目录结构更改

### 重命名的目录
- `compat/foobar_sdk/` → `compat/xpumusic_sdk/`
- `compat/foobar2000/` → `compat/xpumusic/`

### 重命名的文件
- `compat/foobar_compat_manager.cpp` → `compat/xpumusic_compat_manager.cpp`
- `compat/foobar_compat_manager.h` → `compat/xpumusic_compat_manager.h`

## 代码更改

### CMakeLists.txt
- 项目名称从 `MusicPlayer` 改为 `XpuMusic`
- 更新了目录路径引用：
  - `compat/foobar_sdk/` → `compat/xpumusic_sdk/`
  - `compat/foobar_compat_manager.cpp` → `compat/xpumusic_compat_manager.cpp`
- 更新了描述文本：
  - "foobar2000 plugin support" → "XpuMusic plugin support"

### 类名和命名空间
- `FoobarPluginLoader` → `XpuMusicPluginLoader`
- `foobar2000_sdk` → `xpumusic_sdk`
- `foobar2000::` → `xpumusic::`
- `foobar2000_` → `xpumusic_`

### 头文件包含路径
- `../foobar_compat_manager.h` → `../xpumusic_compat_manager.h`
- `../foobar_sdk/foobar2000_sdk.h` → `../xpumusic_sdk/foobar2000_sdk.h`

## 构建脚本更新

### 更新的脚本
- `build.sh`: "XpuMusic 构建脚本" (already updated)
- `install_deps.sh`: "Install dependencies script for Cross-Platform Music Player" → "Install dependencies script for XpuMusic"
- `build_fixed_player.sh`: 更新了路径引用
- `build_macos.sh`: 更新了项目名称

## 文档更新

### README.md
已更新为使用 XpuMusic 名称，但仍保留了对 foobar2000 插件兼容性的说明。

### 其他文档
- ARCHITECTURE.md
- PLUGIN_DEVELOPMENT.md
- PROJECT_RECONSTRUCTION.md
- README_ORIGINAL.md
- 各种技术文档

## 保留的内容

### API 兼容性
以下内容保持不变，以确保 API 兼容性：
- `_foobar2000_client_entry` DLL 导出符号
- GUID 定义
- 兼容性层中的 foobar2000 命名空间（位于 `compat/xpumusic_sdk/`）
- 技术文档中关于 foobar2000 兼容性的描述

### 目的
这些保留的内容确保了 XpuMusic 能够继续兼容现有的 foobar2000 插件生态系统。

## 验证

重命名后，项目应该能够：
1. 正常编译（Windows、Linux、macOS）
2. 加载和使用 foobar2000 插件
3. 保持所有现有功能

## 注意事项

1. 如果有其他分支或 fork，需要同步这些更改
2. CI/CD 管道可能需要更新路径引用
3. 文档网站可能需要更新
4. 包管理器配置（如果使用）需要更新