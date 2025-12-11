# XpuMusic 项目清理指南

## 概述

本指南提供了三种不同的清理选项来精简 XpuMusic 项目，同时保留所有重要的 markdown 文档。

## 清理选项

### 1. 标准清理 (推荐)

**脚本**: `cleanup_project.bat` (Windows) 或 `cleanup_project.sh` (Unix/Linux/macOS)

**保留内容**:
- 所有 `.md` 文档文件
- 源代码目录 (`src/`, `core/`, `plugins/`, `platform/`, `sdk/`, `compat/`)
- `CMakeLists.txt`
- 构建脚本
- 配置文件
- `test_440hz.wav` 和 `test_440hz.mp3` (用于测试)

**删除内容**:
- 所有构建目录 (`build_*`)
- 编译后的可执行文件
- 大型音频测试文件 (除测试文件外)
- 重复的 CMakeLists 文件
- 旧的测试二进制文件
- 临时文件

**使用方法**:
```bash
# Windows
cleanup_project.bat

# Linux/macOS
chmod +x cleanup_project.sh
./cleanup_project.sh
```

### 2. 激进清理

**脚本**: `cleanup_project_aggressive.sh` (仅限 Unix 系统)

**保留内容**:
- 所有 `.md` 文档文件
- 核心源代码目录
- 主 CMakeLists.txt
- 主要构建脚本
- 一个测试音频文件

**删除内容**: 除上述保留内容外的所有其他文件

**使用方法**:
```bash
chmod +x cleanup_project_aggressive.sh
./cleanup_project_aggressive.sh
```

## 清理前后对比

### 清理前项目大小
- 总大小: ~300MB+
- 最大文件:
  - `src/audio/demo_*.wav`: ~10MB (多个文件)
  - 各种构建目录: ~60MB
  - 编译后的二进制文件: ~50MB

### 标准清理后
- 预计大小: ~50-100MB
- 减少约 60-80% 的磁盘空间

### 激进清理后
- 预计大小: ~10-20MB
- 减少约 90-95% 的磁盘空间

## 被删除的文件类型详解

### 构建产物
- `build_*/` - 各种构建目录
- `*.exe`, `*.out` - 可执行文件
- `*.o`, `*.obj` - 目标文件
- `*.a`, `*.lib`, `*.so`, `*.dll` - 库文件

### 测试音频文件
- `1khz.wav` - 10MB 测试音频
- `input_*.wav` - 各种采样率的输入测试文件
- `output_*.wav` - 输出测试文件
- `loud_1000hz.wav`, `test_verify.wav` 等

### 临时和重复文件
- `CMakeLists_*.txt` - 备用的 CMakeLists 版本
- `*.bak`, `*.tmp`, `*~` - 备份和临时文件
- `.DS_Store`, `Thumbs.db` - 系统生成的文件

## 恢复被删除的文件

如果您需要恢复被删除的文件：

### 1. 从 Git 恢复
```bash
# 恢复所有已删除的文件
git checkout .

# 恢复特定文件
git checkout path/to/file
```

### 2. 重新生成
```bash
# 重新构建项目
./build.sh  # 或 build.bat

# 重新生成测试音频文件
python create_test_wav.py
```

## 注意事项

1. **备份重要文件**: 在运行激进清理之前，确保已备份任何未提交的更改
2. **依赖关系**: 激进清理后需要重新安装依赖（参考 `DEPENDENCIES.md`）
3. **测试文件**: 标准清理会保留 `test_440hz.wav/mp3` 用于基本测试
4. **版本控制**: 所有清理脚本都会忽略 `.git` 目录

## 自定义清理

如果您需要自定义清理规则，可以修改清理脚本：

### Windows (cleanup_project.bat)
```batch
REM 添加要删除的文件模式
del /q *.ext 2>nul
REM 添加要保留的文件
REM if not exist "file.ext" echo "File already removed"
```

### Unix (cleanup_project.sh)
```bash
# 添加要删除的文件模式
find . -name "*.ext" -delete 2>/dev/null || true
# 添加要保留的文件
# cp path/to/file .temp_cleanup/
```

## 建议的清理策略

1. **开发期间**: 使用标准清理，保留所有源代码和测试文件
2. **发布前**: 运行标准清理，确保没有构建产物
3. **归档时**: 考虑激进清理，只保留源代码和文档
4. **备份存储**: 使用 Git 标签或发布功能而不是手动备份

## 相关文档

- [BUILD.md](BUILD.md) - 构建指南
- [DEPENDENCIES.md](DEPENDENCIES.md) - 依赖管理
- [BUILD_USAGE.md](BUILD_USAGE.md) - 构建使用说明

## 故障排除

### 清理脚本无法执行
```bash
# Linux/macOS: 添加执行权限
chmod +x cleanup_project.sh
```

### 文件被锁定无法删除
- 确保没有程序正在运行项目文件
- 关闭 IDE 和编译器
- 重启后再次尝试

### Git 状态异常
```bash
# 查看被删除的文件
git status

# 提交清理更改
git add -A
git commit -m "Project cleanup"
```