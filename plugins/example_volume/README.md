# Volume Control Plugin for XpuMusic

A simple volume control plugin demonstrating XpuMusic plugin development.

## Features

- Volume control in decibels (-60 dB to 0 dB)
- Linear volume control (0.0 to 1.0)
- Mute functionality
- Soft clipping to prevent audio distortion

## Parameters

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| volume_db | float | -60 to 0 | Volume in decibels |
| volume_linear | float | 0.0 to 1.0 | Linear volume multiplier |
| mute | float | 0.0 or 1.0 | Mute toggle |

## Building

```bash
cd plugins/example_volume
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

The plugin will be built as `volume_plugin.dll` (Windows) or `libvolume_plugin.so` (Linux).

## Installation

Copy the built plugin to XpuMusic's plugins directory:

```bash
# Windows
copy volume_plugin.dll %XPU_MUSIC_HOME%\plugins\

# Linux
cp libvolume_plugin.so ~/.xpumusic/plugins/
```

## Usage

Once installed, the plugin will be automatically loaded by XpuMusic. You can control it programmatically or through the UI if supported.

## Example Code

```cpp
// Create and configure the effect
auto plugin = create_effect_plugin();
auto effect = plugin->create_effect();
effect->initialize(44100, 2);

// Set volume to -6dB
effect->set_parameter("volume_db", -6.0);

// Process audio
float input[1024];
float output[1024];
effect->process(input, output, 512, 2);

// Mute audio
effect->set_parameter("mute", 1.0);
```

## License

This plugin is released under the MIT License.