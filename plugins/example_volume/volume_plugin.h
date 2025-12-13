/**
 * @file volume_plugin.h
 * @brief Example volume control plugin for XpuMusic
 * @date 2025-12-13
 */

#pragma once

#include <memory>
#include <string>

namespace xpumusic {
    // Forward declarations for XpuMusic interfaces
    class IEffect {
    public:
        virtual ~IEffect() = default;
        virtual bool initialize(int sample_rate, int channels) = 0;
        virtual void process(float* input, float* output, size_t frames, int channels) = 0;
        virtual void set_parameter(const char* name, double value) = 0;
        virtual double get_parameter(const char* name) const = 0;
    };

    class IEffectPlugin {
    public:
        virtual ~IEffectPlugin() = default;
        virtual const char* get_name() const = 0;
        virtual const char* get_description() const = 0;
        virtual std::unique_ptr<IEffect> create_effect() = 0;
    };
}

/**
 * @brief Volume control effect implementation
 */
class VolumeControlEffect : public xpumusic::IEffect {
private:
    float volume_linear_ = 1.0f;
    bool is_muted_ = false;
    int sample_rate_ = 44100;
    int channels_ = 2;

public:
    bool initialize(int sample_rate, int channels) override {
        sample_rate_ = sample_rate;
        channels_ = channels;
        return true;
    }

    void process(float* input, float* output, size_t frames, int channels) override {
        const float actual_volume = is_muted_ ? 0.0f : volume_linear_;

        for (size_t i = 0; i < frames * channels; ++i) {
            output[i] = input[i] * actual_volume;

            // Soft clipping to prevent distortion
            if (output[i] > 1.0f) output[i] = 1.0f;
            if (output[i] < -1.0f) output[i] = -1.0f;
        }
    }

    void set_parameter(const char* name, double value) override {
        if (strcmp(name, "volume_db") == 0) {
            // Convert dB to linear: 20 * log10(linear)
            volume_linear_ = static_cast<float>(std::pow(10.0, value / 20.0));
        } else if (strcmp(name, "volume_linear") == 0) {
            volume_linear_ = static_cast<float>(value);
        } else if (strcmp(name, "mute") == 0) {
            is_muted_ = value > 0.5;
        }
    }

    double get_parameter(const char* name) const override {
        if (strcmp(name, "volume_db") == 0) {
            return 20.0 * std::log10(volume_linear_);
        } else if (strcmp(name, "volume_linear") == 0) {
            return volume_linear_;
        } else if (strcmp(name, "mute") == 0) {
            return is_muted_ ? 1.0 : 0.0;
        }
        return 0.0;
    }
};

/**
 * @brief Volume control plugin implementation
 */
class VolumeControlPlugin : public xpumusic::IEffectPlugin {
public:
    const char* get_name() const override {
        return "Volume Control";
    }

    const char* get_description() const override {
        return "Simple volume control with mute support";
    }

    std::unique_ptr<xpumusic::IEffect> create_effect() override {
        return std::make_unique<VolumeControlEffect>();
    }
};