# Audio Visualization Architecture

## Overview

This document describes the architecture of the audio visualization system for XpuMusic music player. The system provides real-time audio analysis and visual effects including spectrum analysis, waveform display, and various visualization effects.

## System Architecture

### Core Components

```
+-------------------------------------------------------------+
|                    Audio Visualization System              |
+-------------------------------------------------------------+
|  Visualization Engine (core/visualization_engine.cpp)      |
|  +-----------+-----------+-----------+-----------+         |
|  | Spectrum  | Waveform  |  VU Meter |  Effects  |         |
|  | Analyzer  | Renderer  |  Display  | Processor |         |
|  +-----------+-----------+-----------+-----------+         |
+-------------------------------------------------------------+
|                    Audio Analysis Layer                     |
|  +-------+--------+----------+-------------+---------------+
|  |  FFT  | Filters | Frequency |   Peak      |               |
|  | Processor | Smoothing |   Bands   | Detection |               |
|  +-------+--------+----------+-------------+---------------+
+-------------------------------------------------------------+
|                    Data Pipeline                            |
|  Audio Samples -> Buffer -> Analysis -> Visualization Data |
+-------------------------------------------------------------+
```

## Detailed Design

### 1. VisualizationEngine

The visualization engine is the core of the system, coordinating all visualization components.

```cpp
class VisualizationEngine {
public:
    // Initialize engine
    bool initialize(int sample_rate, int buffer_size);

    // Process audio data
    void process_audio(const float* samples, int frame_count);

    // Register visualization components
    void register_visualizer(std::shared_ptr<IVisualizer> visualizer);

    // Get visualization data
    const VisualizationData& get_data() const;

    // Set update frequency
    void set_update_rate(int fps);

private:
    std::vector<std::shared_ptr<IVisualizer>> visualizers_;
    std::unique_ptr<FFTProcessor> fft_processor_;
    std::unique_ptr<SpectrumAnalyzer> spectrum_analyzer_;
    VisualizationData data_;
    std::thread update_thread_;
};
```

### 2. FFT Processor

Responsible for converting time-domain audio signals to frequency domain.

```cpp
class FFTProcessor {
public:
    FFTProcessor(int fft_size = 2048);

    // Perform FFT transform
    void process(const float* input, std::vector<std::complex<float>>& output);

    // Get frequency bins
    const std::vector<float>& get_magnitudes() const;

    // Set window function
    void set_window_function(WindowType type);

private:
    int fft_size_;
    std::vector<float> window_;
    std::vector<std::complex<float>> fft_buffer_;
    std::vector<float> magnitudes_;
};
```

**Supported Window Functions:**
- Rectangle
- Hann
- Hamming
- Blackman

### 3. Spectrum Analyzer

Provides detailed frequency domain analysis.

```cpp
class SpectrumAnalyzer {
public:
    SpectrumAnalyzer(int fft_size, int sample_rate);

    // Analyze spectrum
    void analyze(const std::vector<float>& fft_data);

    // Get band data
    const std::vector<float>& get_bands() const;

    // Set band count
    void set_band_count(int count);

    // Get peak frequency
    float get_peak_frequency() const;

private:
    std::vector<float> bands_;
    std::vector<float> band_energies_;
    float peak_frequency_;
    int band_count_;
};
```

### 4. Waveform Renderer

Displays audio time-domain waveform.

```cpp
class WaveformRenderer {
public:
    WaveformRenderer(int buffer_size = 1024);

    // Update waveform data
    void update(const float* samples, int frame_count);

    // Render waveform
    void render(RenderTarget& target);

    // Set display mode
    void set_mode(WaveformMode mode);

private:
    std::vector<float> wave_buffer_;
    std::vector<float> peak_buffer_;
    WaveformMode mode_;
};
```

**Waveform Display Modes:**
- Oscilloscope
- RMS
- Peak

### 5. VU Meter

Monitors audio levels.

```cpp
class VUMeter {
public:
    VUMeter(int channel_count = 2);

    // Update levels
    void update_levels(const float* samples, int frame_count);

    // Get current level
    float get_level(int channel) const;

    // Get peak hold
    float get_peak_hold(int channel) const;

private:
    std::vector<float> current_levels_;
    std::vector<float> peak_holds_;
    std::vector<float> decay_rates_;
};
```

## Visualization Effects

### 1. Spectrum Bars

```cpp
class SpectrumBars : public IVisualizer {
public:
    void render(RenderTarget& target) override;
    void update(const VisualizationData& data) override;

    // Custom settings
    void set_bar_count(int count);
    void set_color_scheme(const ColorScheme& scheme);
    void set_gradient_mode(bool enabled);
};
```

### 2. Spectrum Circle

```cpp
class SpectrumCircle : public IVisualizer {
public:
    void render(RenderTarget& target) override;
    void update(const VisualizationData& data) override;

    // Set parameters
    void set_radius(float inner_radius, float outer_radius);
    void set_rotation_speed(float speed);
    void set_mirror_mode(bool enabled);
};
```

### 3. Waveform Display

```cpp
class WaveformDisplay : public IVisualizer {
public:
    void render(RenderTarget& target) override;
    void update(const VisualizationData& data) override;

    // Display options
    void set_thickness(float thickness);
    void set_color(const Color& color);
    void set_glow_effect(bool enabled);
};
```

## Performance Optimization

### 1. Multi-threading

- **Audio Thread**: Real-time audio capture
- **Analysis Thread**: FFT and spectrum analysis
- **Render Thread**: Update visual effects

```cpp
class ThreadedVisualizationEngine {
private:
    std::thread audio_thread_;
    std::thread analysis_thread_;
    std::thread render_thread_;

    // Thread-safe data buffers
    ThreadSafeQueue<AudioBuffer> audio_queue_;
    ThreadSafeBuffer<VisualizationData> analysis_buffer_;
};
```

### 2. GPU Acceleration

Using OpenGL or Direct3D for hardware-accelerated rendering:

```cpp
class GPUAcceleratedRenderer {
public:
    // Create shaders
    bool load_shaders(const std::string& vertex_shader,
                     const std::string& fragment_shader);

    // Upload spectrum data to GPU
    void upload_spectrum_data(const float* data, int size);

    // Render to texture
    void render_to_texture(GLuint texture_id);

private:
    GLuint shader_program_;
    GLuint vao_, vbo_;
    GLuint spectrum_texture_;
};
```

### 3. Optimized FFT

Using FFTW library or optimized Kiss FFT:

```cpp
class OptimizedFFT {
public:
    OptimizedFFT(int size);

    // Use FFTW for FFT
    void process_fftw(const float* input, float* output);

    // Use SSE optimization
    void process_sse(const float* input, float* output);

private:
    fftwf_plan fft_plan_;
    float* fft_input_;
    float* fft_output_;
};
```

## Configuration System

### Visualization Configuration

```json
{
    "visualization": {
        "enabled": true,
        "update_rate": 60,
        "fft_size": 2048,
        "window_function": "hann",
        "spectrum": {
            "bar_count": 64,
            "min_frequency": 20,
            "max_frequency": 20000,
            "smoothing": 0.8,
            "color_scheme": "rainbow"
        },
        "waveform": {
            "mode": "oscilloscope",
            "thickness": 2.0,
            "color": "#00FF00"
        },
        "vu_meter": {
            "channels": 2,
            "decay_rate": 0.95,
            "peak_hold_time": 1000
        }
    }
}
```

## Extension Interface

### Custom Visualization Plugins

```cpp
class IVisualizationPlugin {
public:
    virtual ~IVisualizationPlugin() = default;

    // Plugin information
    virtual PluginInfo get_info() const = 0;

    // Initialize
    virtual bool initialize(const VisualizationConfig& config) = 0;

    // Process data
    virtual void process(const VisualizationData& data) = 0;

    // Render
    virtual void render(RenderTarget& target) = 0;
};
```

### Plugin Registration

```cpp
REGISTER_VISUALIZATION_PLUGIN("spectrum_circle", SpectrumCirclePlugin);
REGISTER_VISUALIZATION_PLUGIN("particle_system", ParticleSystemPlugin);
```

## Platform Features

### Windows
- Direct2D/Direct3D rendering
- WASAPI low-latency audio capture
- Windows 7+ Aero effects

### Linux
- OpenGL rendering
- PulseAudio/JACK audio support
- X11/Wayland display support

### macOS
- Metal rendering
- CoreAudio integration
- Retina display optimization

## Future Extensions

### 1. 3D Visualization
- WebGL/Three.js integration
- VR/AR support
- Stereo visual effects

### 2. AI-driven Effects
- Beat detection and synchronization
- Emotion recognition and color mapping
- Music style adaptation

### 3. Interactive Features
- Mouse/touch interaction
- Gesture control
- Real-time effect adjustment

## Summary

XpuMusic's audio visualization system uses a modular design to provide powerful real-time audio analysis and visual effects. Through multi-threading optimization and GPU acceleration, the system can smoothly run various complex visualization effects. The plugin system allows users to customize and extend visualization functionality.