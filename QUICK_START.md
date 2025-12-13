# XpuMusic 快速入门指南

> **XpuMusic是一个高效、跨平台的音频播放系统，支持多种音频格式和插件扩展。**

## 🚀 5分钟快速开始

### 前置要求

1. **Windows 10/11** (主要支持平台)
2. **Visual Studio 2019/2022** 或 **Visual Studio Build Tools**
   - 安装时选择"使用C++的桌面开发"工作负载
3. **CMake 3.16 或更高版本**
   - 下载地址: https://cmake.org/download/

### 一键构建

```bash
# 克隆项目
git clone <repository-url> XpuMusic
cd XpuMusic

# 快速构建（自动配置和编译）
.\quick_build.bat
```

构建成功后，你将看到：
```
🎉 BUILD SUCCESSFUL!
Generated executables:
  music-player.exe
  final_wav_player.exe
  test_decoders.exe
  ...
```

### 运行你的第一个音频

```bash
# 进入可执行文件目录
cd build\bin\Debug

# 播放音频文件
final_wav_player.exe test_1khz.wav
```

## 📚 详细指南

### 项目结构

```
XpuMusic/
├── src/                  # 核心源代码
│   ├── audio/           # 音频处理引擎
│   ├── core/            # 核心功能实现
│   └── platform/        # 平台抽象层
├── compat/              # 兼容性层（foobar2000插件支持）
├── plugins/             # 音频插件实现
├── quality/             # 质量保障系统
├── scripts/             # 构建和工具脚本
├── build/               # 构建输出目录（自动生成）
├── CMakeLists.txt       # CMake配置文件
├── build_all.bat        # 完整构建脚本
└── quick_build.bat      # 快速构建脚本
```

### 构建选项

#### 选项1：快速构建（推荐）
```bash
.\quick_build.bat           # 增量构建
.\quick_build.bat force     # 清理后重新构建
```

#### 选项2：完整构建
```bash
.\build_all.bat             # 构建所有组件
```

#### 选项3：使用PowerShell脚本（高级）
```powershell
.\scripts\build_system.ps1  # 功能丰富的构建系统
```

### 构建模式

- **Debug模式**：包含调试信息，适合开发测试
- **Release模式**：优化构建，适合发布使用

```bash
# 在build目录下执行
cmake --build . --config Debug    # 调试版本
cmake --build . --config Release  # 发布版本
```

## 🎵 使用示例

### 基本音频播放

```cpp
#include "src/audio/audio_player.h"

int main() {
    audio_player player;
    player.load("music.wav");
    player.play();
    return 0;
}
```

### 插件使用示例

```cpp
#include "compat/plugin_loader/plugin_loader.h"

int main() {
    XpuMusicPluginLoader loader;

    // 加载插件
    loader.load_plugin("plugins/flac_decoder.dll");

    // 使用插件解码
    auto decoder = loader.create_decoder("music.flac");
    decoder->decode();

    return 0;
}
```

## 🧪 测试和验证

### 运行测试套件

```bash
cd build\bin\Debug

# 基础功能测试
test_decoders.exe        # 解码器测试
test_audio_direct.exe    # 音频系统测试
test_simple_playback.exe # 播放功能测试
```

### 性能测试

```bash
# 性能基准测试
simple_performance_test.exe

# 优化集成示例
optimization_integration_example.exe
```

## 🔧 开发指南

### 添加新功能

1. **创建新模块**
   ```bash
   # 在src目录下创建新文件夹
   mkdir src\my_feature
   ```

2. **更新CMakeLists.txt**
   ```cmake
   add_subdirectory(src/my_feature)
   add_library(my_feature my_source.cpp)
   ```

3. **构建和测试**
   ```bash
   .\quick_build.bat force
   ```

### 调试技巧

1. **使用Visual Studio调试**
   - 打开 `build\XpuMusic.sln`
   - 设置启动项目
   - 按 F5 开始调试

2. **命令行调试**
   ```bash
   # 使用Debug版本
   .\build\bin\Debug\music-player.exe --verbose
   ```

## 📊 性能特性

XpuMusic内置多项优化：

- **SIMD加速**：SSE2/AVX支持
- **多线程处理**：并行音频处理
- **内存优化**：智能缓存管理
- **零拷贝**：高效数据传输

## 🆘 常见问题

### Q: 构建失败，提示找不到编译器
**A:** 确保从Visual Studio开发者命令提示符运行，或添加VS工具到PATH。

### Q: CMake配置失败
**A:** 检查CMake版本是否>=3.16，确保Windows SDK已安装。

### Q: 音频播放没有声音
**A:** 检查系统音频设备，确保音频文件格式受支持。

### Q: 插件加载失败
**A:** 确认插件DLL在正确路径，检查依赖项是否完整。

## 📖 更多资源

- **完整文档**：`docs/README.md`
- **API参考**：`docs/API.md`
- **插件开发指南**：`docs/PLUGIN_DEVELOPMENT.md`
- **性能优化指南**：`docs/PERFORMANCE.md`

## 🤝 贡献指南

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- foobar2000 社区提供的优秀音频处理参考
- 所有贡献者的努力和支持

---

**开始你的音频之旅吧！** 🎶

如有问题，请查看 [常见问题](#-常见问题) 或提交 Issue。