# XpuMusic 配置系统

## 配置文件位置

配置文件按优先级加载（高优先级覆盖低优先级）：

1. 命令行指定的配置文件 (`-c` 参数)
2. `~/.xpumusic/config.json`
3. `/etc/xpumusic/config.json`
4. `<安装目录>/share/xpumusic/config/config.json`
5. `<源码目录>/config/default_config.json`

## 主配置文件示例

```json
{
    "version": "2.0.0",
    "audio": {
        "output_device": "default",
        "sample_rate": 44100,
        "channels": 2,
        "bits_per_sample": 32,
        "use_float": true,
        "buffer_size": 4096,
        "buffer_count": 4,
        "volume": 0.8,
        "mute": false,
        "equalizer_preset": "flat"
    },
    "resampler": {
        "quality": "adaptive",
        "floating_precision": 64,
        "enable_adaptive": true,
        "cpu_threshold": 0.8,
        "use_anti_aliasing": true,
        "cutoff_ratio": 0.95,
        "filter_taps": 101,
        "format_quality": {
            "mp3": "good",
            "flac": "best",
            "wav": "fast",
            "ogg": "good",
            "aac": "good",
            "m4a": "good",
            "wma": "fast",
            "ape": "high"
        }
    },
    "plugins": {
        "plugin_directories": [
            "./plugins",
            "~/.xpumusic/plugins",
            "/usr/lib/xpumusic/plugins"
        ],
        "auto_load_plugins": true,
        "plugin_scan_interval": 0,
        "plugin_timeout": 5000
    },
    "player": {
        "repeat": false,
        "shuffle": false,
        "crossfade": false,
        "crossfade_duration": 2.0,
        "preferred_backend": "auto",
        "show_console_output": true,
        "show_progress_bar": true,
        "key_bindings": {
            "play": "space",
            "pause": "p",
            "stop": "s",
            "next": "n",
            "previous": "b",
            "quit": "q"
        }
    },
    "logging": {
        "level": "info",
        "console_output": true,
        "file_output": false,
        "log_file": "~/.xpumusic/xpumusic.log",
        "enable_rotation": true,
        "max_file_size": 10485760,
        "max_files": 5,
        "include_timestamp": true,
        "include_thread_id": false,
        "include_function_name": true
    },
    "ui": {
        "theme": "default",
        "language": "en",
        "font_family": "default",
        "font_size": 12.0,
        "save_window_size": true,
        "window_width": 1024,
        "window_height": 768,
        "start_maximized": false
    }
}
```

## 质量模式说明

- **fast**: 线性插值，最快速度，适合实时应用
- **good**: 三次插值，速度与质量平衡
- **high**: 8抽头Sinc插值，高质量处理
- **best**: 16抽头Sinc插值，最高质量，适合母带处理
- **adaptive**: 自适应选择，根据CPU负载自动调整

## 浮点精度选择

**32位浮点 (默认)**
- 24位尾数精度，约150dB动态范围
- 较低的CPU和内存使用
- 适合一般播放场景

**64位浮点**
- 53位尾数精度，约320dB动态范围
- 更高的累积精度
- 适合专业音频制作和多级处理

## 环境变量

可以使用环境变量覆盖配置项：

```bash
# 设置输出设备
export QODER_AUDIO_OUTPUT_DEVICE=pulse

# 设置采样率
export QODER_AUDIO_SAMPLE_RATE=48000

# 设置音量
export QODER_AUDIO_VOLUME=0.5

# 设置日志级别
export QODER_LOGGING_LEVEL=debug

# 设置重采样精度
export QODER_RESAMPLER_FLOATING_PRECISION=64
```

## 命令行选项

```bash
# 指定配置文件
music-player-config -c /path/to/config.json

# 列出可用设备
music-player-config --list-devices

# 指定输出设备
music-player-config --device pulse

# 设置采样率
music-player-config --rate 48000

# 设置声道数
music-player-config --channels 4

# 设置音量
music-player-config --volume 0.7
```

## 配置验证

配置加载时会自动验证：
- 采样率：8000 - 768000 Hz
- 声道数：1 - 8
- 位深度：16, 24, 32
- 音量：0.0 - 1.0
- 重采样质量：fast, good, high, best, adaptive
- 浮点精度：32, 64
- CPU阈值：0.1 - 1.0
- 日志级别：trace, debug, info, warn, error, fatal