/**
 * @file resampler_plugin.cpp
 * @brief XpuMusic DSP鐮佺巼杞崲鎻掍欢绀轰緥
 * @date 2025-12-10
 */

#include "../../xpumusic_plugin_sdk.h"
#include "../../../src/audio/adaptive_resampler.h"
#include <cstring>

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IDSPProcessor;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;

class ResamplerDSPPlugin : public IDSPProcessor {
private:
    std::unique_ptr<audio::AdaptiveSampleRateConverter> resampler_;
    AudioFormat input_format_;
    AudioFormat output_format_;
    int target_rate_;
    audio::ResampleQuality quality_;

    // 鍐呴儴缂撳啿鍖?
    std::vector<float> internal_buffer_;

public:
    ResamplerDSPPlugin() : target_rate_(44100), quality_(audio::ResampleQuality::Good) {
        input_format_ = {};
        output_format_ = {};
    }

    bool initialize() override {
        set_state(PluginState::Initialized);
        return true;
    }

    void finalize() override {
        resampler_.reset();
        internal_buffer_.clear();
        set_state(PluginState::Uninitialized);
    }

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "XpuMusic Sample Rate Converter";
        info.version = "1.0.0";
        info.author = "XpuMusic Team";
        info.description = "High-quality sample rate conversion with adaptive algorithms";
        info.type = PluginType::DSPEffect;
        info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
        return info;
    }

    PluginState get_state() const override {
        return state_;
    }

    void set_state(PluginState state) override {
        state_ = state;
    }

    std::string get_last_error() const override {
        return last_error_;
    }

    bool configure(const AudioFormat& input_format,
                   const AudioFormat& output_format) override {
        input_format_ = input_format;
        output_format_ = output_format;
        target_rate_ = output_format.sample_rate;

        // 鍒涘缓杞崲鍣?
        resampler_ = std::make_unique<audio::AdaptiveSampleRateConverter>();

        if (!resampler_->initialize(
            input_format.sample_rate,
            target_rate_,
            input_format.channels)) {
            last_error_ = "Failed to initialize resampler";
            set_state(PluginState::Error);
            return false;
        }

        // 鍒嗛厤鍐呴儴缂撳啿鍖?
        internal_buffer_.resize(input_format.channels * 4096);

        set_state(PluginState::Active);
        return true;
    }

    int process(const AudioBuffer& input,
                AudioBuffer& output) override {
        if (!resampler_ || state_ != PluginState::Active) {
            last_error_ = "Resampler not configured";
            return -1;
        }

        // 璁＄畻鎵€闇€杈撳嚭甯ф暟
        double ratio = static_cast<double>(target_rate_) / input_format_.sample_rate;
        int max_output_frames = static_cast<int>(input.frames * ratio * 1.1); // 10% extra

        // 鎵ц杞崲
        int output_frames = resampler_->convert(
            input.data, input.frames,
            output.data, output.frames);

        return output_frames;
    }

    void set_parameter(const std::string& name, double value) override {
        if (name == "quality") {
            // 0=Fast, 1=Good, 2=High, 3=Best, 4=Adaptive
            int quality_int = static_cast<int>(value);
            quality_int = std::max(0, std::min(4, quality_int));

            switch (quality_int) {
                case 0: quality_ = audio::ResampleQuality::Fast; break;
                case 1: quality_ = audio::ResampleQuality::Good; break;
                case 2: quality_ = audio::ResampleQuality::High; break;
                case 3: quality_ = audio::ResampleQuality::Best; break;
                case 4: quality_ = audio::ResampleQuality::Adaptive; break;
            }
        }
        else if (name == "target_rate") {
            target_rate_ = static_cast<int>(value);
        }
    }

    double get_parameter(const std::string& name) override {
        if (name == "quality") {
            switch (quality_) {
                case audio::ResampleQuality::Fast: return 0;
                case audio::ResampleQuality::Good: return 1;
                case audio::ResampleQuality::High: return 2;
                case audio::ResampleQuality::Best: return 3;
                case audio::ResampleQuality::Adaptive: return 4;
            }
        }
        else if (name == "target_rate") {
            return static_cast<double>(target_rate_);
        }
        return 0.0;
    }

    std::vector<std::string> get_parameter_names() override {
        return {"quality", "target_rate"};
    }

    void reset() override {
        if (resampler_) {
            // 閲嶆柊鍒濆鍖栬浆鎹㈠櫒
            resampler_->initialize(
                input_format_.sample_rate,
                target_rate_,
                input_format_.channels);
        }
    }

    int get_latency_samples() const override {
        // 杞崲鍣ㄧ殑寤惰繜
        return 256; // 绀轰緥鍊?
    }
};

// 鎻掍欢宸ュ巶
class ResamplerDSPFactory : public ITypedPluginFactory<IDSPProcessor> {
public:
    std::unique_ptr<IDSPProcessor> create_typed() override {
        return std::make_unique<ResamplerDSPPlugin>();
    }

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "XpuMusic Sample Rate Converter Factory";
        info.version = "1.0.0";
        info.author = "XpuMusic Team";
        info.description = "Factory for sample rate converter plugin";
        info.type = PluginType::DSPEffect;
        info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
        return info;
    }
};

// 瀵煎嚭鎻掍欢
XPUMUSIC_EXPORT_DSP_PLUGIN(ResamplerDSPPlugin)