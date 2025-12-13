# Music Player 使用说明

## 概述

`music-player.exe` 是 XpuMusic 的主要音频播放器，支持通过命令行参数控制是否加载 foobar2000 组件。

## 使用方法

### 基本语法
```bash
music-player.exe [--fb2k] <audio_file>
```

### 选项
- `--fb2k` - 启用 foobar2000 组件支持（可选）
- `<audio_file>` - 要播放的音频文件路径

### 使用示例

#### 1. 使用原生解码器（默认）
```bash
# 播放 WAV 文件
music-player.exe \music\1.wav

# 查看帮助
music-player.exe
```

输出：
```
[OK] Using native decoders only
  Use --fb2k flag to enable foobar2000 support
```

#### 2. 启用 foobar2000 支持
```bash
# 播放音频文件并加载 foobar2000 组件
music-player.exe --fb2k \music\1.mp3
music-player.exe --fb2k \music\song.flac
```

输出：
```
[Flag] --fb2k specified, loading foobar2000 components...
[OK] Loaded: c:\Program Files\foobar2000\shared.dll
[OK] Successfully loaded foobar2000 modern components
  Extended format support enabled
```

## 支持的格式

### 原生支持（无需 --fb2k）
- WAV - 所有采样率、位深度、声道配置

### foobar2000 扩展支持（需要 --fb2k）
- MP3
- FLAC
- OGG Vorbis
- M4A/AAC
- WMA
- APE
- TAK
- WV (WavPack)
- TTA
- MPC
- OPUS
- DSD (DSF/DST)

## 前提条件

要使用 foobar2000 支持：
1. 安装 foobar2000 播放器
2. 确保 DLL 文件可访问（自动从 `C:\Program Files\foobar2000\` 加载）

## 性能说明

- **原生模式**：启动更快，资源占用低
- **foobar2000 模式**：支持更多格式，但启动稍慢

## 故障排除

如果 foobar2000 组件加载失败：
1. 检查是否已安装 foobar2000
2. 确保 DLL 文件在正确位置
3. 播放器会自动回退到原生解码器

## 与其他播放器的区别

| 播放器 | 用途 | foobar2000 支持 |
|--------|------|----------------|
| music-player.exe | 主要播放器 | 通过 --fb2k 参数控制 |
| foobar2k-player.exe | 实验性播放器 | 总是尝试加载 |
| final_wav_player.exe | 简单 WAV 播放器 | 不支持 |