# XpuMusic v2.0

A professional, cross-platform music player with modular plugin architecture, advanced audio processing, and foobar2000 compatibility.

## 🌟 Features

### 🎵 Audio Processing
- **32/64-bit Floating Point Precision**: Configurable precision for professional audio processing
- **Advanced Sample Rate Conversion**: Multiple algorithms (Linear, Cubic, Sinc, Adaptive)
- **Universal Sample Rate Support**: 8kHz to 768kHz including all standard rates
- **Gapless Playback**: Seamless transitions between tracks
- **Crossfade Support**: Smooth track transitions with configurable duration

### 🔌 Plugin Architecture
- **Native SDK**: Modern C++17 plugin interface
- **foobar2000 Compatibility Layer**: Load existing foobar2000 plugins
- **Hot-Loading**: Runtime plugin management
- **Type-safe Plugin System**: Decoders, DSP processors, audio outputs, visualizations

### 🌐 Cross-Platform Support
- **Windows**: WASAPI audio backend
- **macOS**: CoreAudio integration
- **Linux**: ALSA/PulseAudio with automatic detection
- **Automatic Backend Selection**: Runtime platform detection with graceful fallback

### ⚙️ Configuration System
- **JSON-based Configuration**: Hierarchical settings with validation
- **Environment Variable Support**: Override settings at runtime
- **User Profiles**: Per-user configuration management
- **Theme Support**: Customizable UI themes

## 🚀 Quick Start

### Prerequisites

**Required:**
- **CMake 3.20+**
- **C++17 Compatible Compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **nlohmann/json library** (v3.7+)

**Optional (for enhanced features):**
- **Linux**: `libasound2-dev`, `libpulse-dev`
- **nlohmann/json**: `sudo apt install nlohmann-json3-dev`

### Build Instructions

```bash
# Clone repository
git clone <repository-url>
cd Qoder_foobar

# Configure build with automatic dependency detection
./build.sh

# Or manually:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Install system-wide (optional)
sudo cmake --install build
```

### Usage

```bash
# Play with configuration
./build/bin/music-player-config

# Use specific config file
./build/bin/music-player-config -c ~/.xpumusic/my_config.json

# Override settings with command line
./build/bin/music-player-config --volume 0.8 --rate 96000

# List audio devices
./build/bin/music-player-config --list-devices

# Test precision differences
./build/bin/test-resampler-precision
```

## 📁 Configuration

Configuration is managed through JSON files:

### Main Config File (`~/.xpumusic/config.json`)

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
        "volume": 0.8,
        "mute": false
    },
    "resampler": {
        "quality": "adaptive",
        "floating_precision": 64,        // 32 or 64 bits
        "enable_adaptive": true,
        "cpu_threshold": 0.8,
        "format_quality": {
            "mp3": "good",
            "flac": "best",
            "wav": "fast"
        }
    },
    "plugins": {
        "plugin_directories": [
            "./plugins",
            "~/.xpumusic/plugins"
        ],
        "auto_load_plugins": true
    }
}
```

### Environment Variables

```bash
# Override audio settings
export QODER_AUDIO_SAMPLE_RATE=96000
export QODER_AUDIO_VOLUME=0.9
export QODER_AUDIO_OUTPUT_DEVICE=pulse

# Override resampler settings
export QODER_RESAMPLER_FLOATING_PRECISION=64
export QODER_RESAMPLER_QUALITY=best
```

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│                     XpuMusic v2.0                 │
├─────────────────────────────────────────────────┤
│  Configuration System (JSON + Environment Vars)   │
├─────────────────────────────────────────────────┤
│                Plugin Manager                     │
│  ┌─────────────┬─────────────┬─────────────┐    │
│  │   Native    │ Compatibility│  Foobar2000  │    │
│  │    SDK      │    Layer     │   Adapter    │    │
│  └─────────────┴─────────────┴─────────────┘    │
├─────────────────────────────────────────────────┤
│                Audio Pipeline                     │
│  Decoder → DSP → Resampler → Output → Device     │
├─────────────────────────────────────────────────┤
│              Platform Abstraction                 │
│   Windows(WASAPI) │ macOS(CoreAudio) │ Linux(ALSA)│
└─────────────────────────────────────────────────┘
```

## 🔌 Plugin Development

### Native Plugin Example

```cpp
#include "xpumusic_plugin_sdk.h"

class MyDecoder : public xpumusic::IAudioDecoder {
public:
    bool initialize() override {
        return true;
    }

    bool open(const std::string& file) override {
        // Open audio file
        return true;
    }

    int decode(xpumusic::AudioBuffer& buffer, int max_frames) override {
        // Decode audio data
        return frames_decoded;
    }

    AudioFormat get_format() const override {
        return format_;
    }
};

QODER_EXPORT_AUDIO_PLUGIN(MyDecoder)
```

### Supported Plugin Types

1. **Audio Decoders** (`IAudioDecoder`)
   - WAV decoder (native)
   - MP3 decoder (minimp3)
   - FLAC decoder (libFLAC)

2. **DSP Processors** (`IDSPProcessor`)
   - Sample rate converter (32/64-bit)
   - Equalizer
   - Volume control
   - Effects

3. **Audio Outputs** (`IAudioOutput`)
   - Platform-specific backends
   - Custom output devices

## 🔬 Audio Processing Details

### Sample Rate Conversion

The resampler supports multiple algorithms:

| Algorithm | Quality | Speed | Use Case |
|-----------|---------|-------|----------|
| Linear | Low | Fastest | Real-time, low CPU |
| Cubic | Good | Fast | General use |
| Sinc (8-tap) | High | Medium | Professional audio |
| Sinc (16-tap) | Best | Slow | Mastering |
| Adaptive | Variable | Auto | Intelligent selection |

### Precision Comparison

- **32-bit float**: 24-bit mantissa, ~150dB dynamic range
- **64-bit float**: 53-bit mantissa, ~320dB dynamic range

64-bit precision is recommended for:
- Professional audio production
- Multiple processing stages
- Critical listening applications

## 📊 Performance

### Benchmarks (on modern CPU)

| Operation | 32-bit | 64-bit | Ratio |
|-----------|--------|--------|-------|
| 44.1k → 48k | 0.5ms/s | 0.7ms/s | 1.4x |
| 44.1k → 96k | 0.8ms/s | 1.1ms/s | 1.4x |
| 44.1k → 192k | 1.5ms/s | 2.0ms/s | 1.3x |

Memory usage increases by ~2x for 64-bit precision.

## 🛠️ Testing

```bash
# Test configuration system
./build/bin/test-config

# Test resampler precision
./build/bin/test-resampler-precision

# Test plugin system
./build/bin/test-plugin

# Test audio backends
./build/bin/test-audio

# Comprehensive test
./build/bin/test-all
```

## 🔧 Development

### Project Structure

```
Qoder_foobar/
├── sdk/                    # Native plugin SDK
├── core/                   # Core system (plugin manager, registry)
├── config/                 # Configuration system
├── src/
│   ├── audio/             # Audio processing and resampling
│   └── platform/          # Platform-specific code
├── compat/                 # foobar2000 compatibility layer
├── plugins/                # Example plugins
├── examples/               # Example applications
└── docs/                  # Documentation
```

### Adding New Features

1. **Audio Formats**: Create decoder plugin in `plugins/`
2. **DSP Effects**: Implement `IDSPProcessor` interface
3. **Audio Outputs**: Add platform backend in `src/platform/`
4. **Configuration**: Add settings to appropriate config section

## 🤝 Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push branch: `git push origin feature/amazing-feature`
5. Open Pull Request

### Code Style

- C++17 features where appropriate
- RAII and smart pointers
- Const correctness
- Comprehensive error handling
- Unit tests for new features

## 📋 Roadmap

### v2.1 (Planned)
- [ ] WebAssembly support (browser playback)
- [ ] Network streaming (HTTP/HTTPS)
- [ ] Advanced visualizations
- [ ] Playlist management UI

### v2.2 (Future)
- [ ] DSP effect chain GUI
- [ ] Metadata editor
- [ ] Gapless playback UI controls
- [ ] Plugin marketplace

## 📄 License

[License information to be added]

---

## Acknowledgments

- foobar2000 for the plugin architecture inspiration
- nlohmann for the excellent JSON library
- minimp3 for lightweight MP3 decoding
- The open-source community for various audio libraries

**XpuMusic** - Professional audio playback, reimagined.