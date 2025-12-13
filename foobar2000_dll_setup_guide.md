# foobar2000 DLL 安装指南

## 问题：`foobar2k-player.exe` 初始化失败

### 解决方案

#### 方法 1：手动复制 DLL（推荐）

1. **打开文件资源管理器**
2. **导航到**：`C:\Program Files\foobar2000\`
3. **复制以下文件到** `C:\workspace\XpuMusic\build\bin\Debug\`：
   - `shared.dll` ✓ (最重要)
   - `avcodec-fb2k-62.dll`
   - `avformat-fb2k-62.dll`
   - `avutil-fb2k-60.dll`
   - `sqlite3.dll`

4. **可选：复制组件 DLL**
   - 进入 `C:\Program Files\foobar2000\components\`
   - 复制所有 `.dll` 文件到 `C:\workspace\XpuMusic\build\bin\Debug\`

#### 方法 2：使用批处理脚本

```cmd
C:\workspace\XpuMusic>.\setup_all_foobar_dlls.bat
```

#### 方法 3：直接运行命令

```cmd
copy "c:\Program Files\foobar2000\*.dll" "C:\workspace\XpuMusic\build\bin\Debug\"
```

### 验证安装

运行以下命令验证：
```cmd
C:\workspace\XpuMusic\build\bin\Debug>dir *.dll
```

应该看到至少：
- shared.dll
- avcodec-fb2k-62.dll
- avformat-fb2k-62.dll
- avutil-fb2k-60.dll

### 测试播放器

1. **music-player.exe**（推荐）：
   ```cmd
   C:\workspace\XpuMusic\build\bin\Debug>music-player.exe \music\1.wav
   ```

2. **foobar2k-player.exe**（实验性）：
   ```cmd
   C:\workspace\XpuMusic\build\bin\Debug>foobar2k-player.exe \music\1.wav
   ```

### 预期输出

成功加载后应该看到：
```
Initializing foobar2000 compatible player...
[OK] Loaded foobar2000 shared.dll
[OK] Using foobar2000 DLLs with SDK wrapper
[OK] Using foobar2000 SDK services
Loading file: \music\1.wav
```

### 注意事项

1. **新版 foobar2000 使用模块化架构**
   - 没有单一的 `foobar2000.dll`
   - 使用多个组件 DLL

2. **music-player.exe 更稳定**
   - 包含完整的 foobar2000 支持层
   - 推荐日常使用

3. **foobar2k-player.exe 是实验性的**
   - 主要用于开发测试
   - 功能有限

### 故障排除

如果仍然失败：
1. 确认 DLL 文件已正确复制
2. 检查文件权限
3. 使用 `music-player.exe` 替代

### 相关文件

- `config/default_config.json` - foobar2000 兼容性配置
- `src/foobar2000_modern_loader.cpp` - 现代 DLL 加载器
- `src/foobar2k_compatible_player.cpp` - 兼容播放器实现