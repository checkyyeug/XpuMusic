# 跨平台修改说明

本文档记录了为确保XpuMusic项目在Windows、Linux和macOS上正确编译所做的所有修改。

## 1. M_PI 常量定义

在以下文件中添加了 M_PI 的条件定义：

- `src/audio/cubic_resampler.cpp`
- `src/audio/sinc_resampler.cpp`
- `src/audio/test_resampler.cpp`
- `src/audio/test_universal_converter.cpp`

```cpp
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
```

这确保了在所有平台上都能使用 M_PI 常量，包括 Windows (MSVC)。

## 2. 平台特定音频输出

### Windows (WASAPI)
- `test_cross_platform` 在 Windows 上包含 `platform/windows/audio_output_wasapi.cpp`
- 链接库：`ole32 uuid`

### macOS (CoreAudio)
- `test_cross_platform` 在 macOS 上包含 `src/audio/audio_output_coreaudio.cpp`
- 链接框架：`CoreAudio AudioUnit CoreFoundation`

### Linux (ALSA/Stub)
- `test_cross_platform` 在 Linux 上包含 `platform/linux/audio_output_stub.cpp`
- ALSA 支持需要在系统中安装开发包

## 3. DirectSound 支持 (Windows)

在 `CMakeLists.txt` 中为 Windows 添加了 DirectSound 支持：
```cmake
if(WIN32)
    target_link_libraries(music-player-plugin PRIVATE dsound winmm)
endif()
```

## 4. GUID 转换的跨平台处理

在 `compat/plugin_loader/plugin_loader.cpp` 中添加了平台特定的 GUID 转换：
```cpp
#ifdef _WIN32
memcpy(&fb_guid, &service.guid, sizeof(GUID));
#else
// 非Windows系统，手动复制GUID结构
fb_guid.Data1 = service.guid.Data1;
fb_guid.Data2 = service.guid.Data2;
fb_guid.Data3 = service.guid.Data3;
memcpy(fb_guid.Data4, service.guid.Data4, sizeof(service.guid.Data4));
#endif
```

## 5. Linux 特定代码保护

### ALSA 音频播放器
- `music-player` 目标只在 Linux 上构建 (`if(UNIX AND NOT APPLE)`)
- `music_player_simple.cpp` 添加了编译时检查：
```cpp
#ifdef __linux__
#include <alsa/asoundlib.h>
#else
#error "music_player_simple.cpp is Linux-only and requires ALSA"
#endif
```

## 6. 缺失的头文件

添加了缺失的头文件：
- `core/audio_format_detector.cpp`: 添加了 `#include <sstream>`

## 构建说明

### Windows
```bash
cmake -B build -S . -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Linux
```bash
cmake -B build -S .
cmake --build build
```

### macOS
```bash
cmake -B build -S .
cmake --build build
```

## 平台依赖

### Windows
- Visual Studio 2019/2022
- Windows 10 SDK
- DirectSound (系统自带)
- WASAPI (系统自带)

### Linux
- GCC 或 Clang
- ALSA 开发库 (`libasound2-dev`)
- CMake 3.15+

### macOS
- Xcode 或 Clang
- CoreAudio (系统自带)

## 测试的组件

以下组件已确认在三个平台上都能正确编译：

- 音频解码器核心 (WAV, MP3)
- 采样率转换器 (测试程序)
- 插件系统核心框架
- 跨平台音频输出测试

## 注意事项

1. FLAC 和 OGG/Vorbis 解码器需要安装相应的开发库才能启用
2. foobar2000 插件支持主要针对 Windows，在其他平台可能受限
3. 某些高级功能可能需要平台特定的实现