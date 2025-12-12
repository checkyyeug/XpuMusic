/**
 * @file dsp_resampler.cpp
 * @brief XpuMusic 鐮佺巼杞崲DSP鎻掍欢
 * @date 2025-12-10
 */

#include "../../sdk/xpumusic_plugin_sdk.h"
#include "../../src/audio/sample_rate_converter.h"
#include "../../src/audio/adaptive_resampler.h"
#include <cstring>

using namespace xpumusic;

enum class ResamplerType {
    Linear,
    Cubic,
    Sinc8,
    Sinc16,
    Adaptive
};

class ResamplerDSPPlugin : public IDSPProcessor {
private:
    std::unique_ptr<ISampleRateConverter> resampler_;
    AudioFormat input_format_;
    AudioFormat output_format_;
    ResamplerType type_;
    int target_rate_;
    bool configured_;

    // 鍐呴儴缂撳啿鍖?
    std::vector<float> input_buffer_;
    std::vector<float> output_buffer_;

public:
    ResamplerDSPPlugin() : type_(ResamplerType::Adaptive), target_rate_(44100), configured_(false) {
        input_format_ = {};
        output_format_ = {};
    }

    bool initialize() override {
        set_state(PluginState::Initialized);
        return true;
    }

    void finalize() override {
        resampler_.reset();
        input_buffer_.clear();
        output_buffer_.clear();
        configured_ = false;
        set_state(PluginState::Uninitialized);
    }

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "XpuMusic Sample Rate Converter DSP";
        info.version = "1.0.0";
        info.author = "XpuMusic Team";
        info.description = "High-quality sample rate conversion with multiple algorithms";
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

        // 鍒涘缓鍚堥€傜殑杞崲鍣?
        switch (type_) {
            case ResamplerType::Linear:
                resampler_ = std::make_unique<LinearSampleRateConverter>();
                break;
            case ResamplerType::Cubic:
                resampler_ = std::make_unique<CubicSampleRateConverter>();
                break;
            case ResamplerType::Sinc8:
                resampler_ = std::make_unique<SincSampleRateConverter>(8);
                break;
            case ResamplerType::Sinc16:
                resampler_ = std::make_unique<SincSampleRateConverter>(16);
                break;
            case ResamplerType::Adaptive:
            default:
                resampler_ = std::make_unique<AdaptiveSampleRateConverter>();
                break;
        }

        if (!resampler_) {
            last_error_ = "Failed to create resampler";
            set_state(PluginState::Error);
            return false;
        }

        // 鍒濆鍖栬浆鎹㈠櫒
        if (!resampler_->initialize(input_format_.sample_rate,
                                   target_rate_,
                                   input_format_.channels)) {
            last_error_ = "Failed to initialize resampler";
            set_state(PluginState::Error);
            return false;
        }

        // 鍒嗛厤缂撳啿鍖?
        int max_output_frames = static_cast<int>(
            input_format_.sample_rate * 2 * // 2绉掕緭鍏?
            (static_cast<double>(target_rate_) / input_format_.sample_rate) * 1.5 // 浼扮畻杈撳嚭
        );

        input_buffer_.resize(input_format_.channels * max_output_frames);
        output_buffer_.resize(input_format_.channels * max_output_frames);

        configured_ = true;
        set_state(PluginState::Active);
        return true;
    }

    int process(const AudioBuffer& input, AudioBuffer& output) override {
        if (!configured_ || !resampler_ || state_ != PluginState::Active) {
            last_error_ = "Resampler not configured";
            return -1;
        }

        // 璁＄畻鏈€澶у彲鑳界殑杈撳嚭甯ф暟
        double ratio = static_cast<double>(target_rate_) / input_format_.sample_rate;
        int max_output_frames = static_cast<int>(input.frames * ratio * 1.2);

        // 纭繚杈撳嚭缂撳啿鍖鸿冻澶熷ぇ
        if (output.channels * max_output_frames > static_cast<int>(output_buffer_.size())) {
            output_buffer_.resize(output.channels * max_output_frames);
        }

        // 鎵ц杞崲
        int output_frames = resampler_->convert(input.data, input.frames,
                                               output_buffer_.data(), max_output_frames);

        // 濡傛灉杈撳嚭缂撳啿鍖哄お灏忥紝闇€瑕佽皟鏁?
        if (output.channels * output_frames > output.frames) {
            // 杩斿洖閿欒锛岃璋冪敤鑰呭垎閰嶆洿澶х殑缂撳啿鍖?
            last_error_ = "Output buffer too small";
            return -output_frames; // 璐熸暟琛ㄧず闇€瑕佺殑澶у皬
        }

        // 澶嶅埗鍒拌緭鍑虹紦鍐插尯
        memcpy(output.data, output_buffer_.data(),
               output_frames * output.channels * sizeof(float));

        return output_frames;
    }

    void set_parameter(const std::string& name, double value) override {
        if (name == "type") {
            int type_int = static_cast<int>(value);
            type_int = std::max(0, std::min(4, type_int));
            type_ = static_cast<ResamplerType>(type_int);

            // 濡傛灉宸查厤缃紝闇€瑕侀噸鏂伴厤缃?
            if (configured_) {
                configure(input_format_, output_format_);
            }
        }
        else if (name == "target_rate") {
            target_rate_ = static_cast<int>(value);
            if (configured_) {
                output_format_.sample_rate = target_rate_;
                configure(input_format_, output_format_);
            }
        }
        else if (name == "quality" && resampler_) {
            // 璁剧疆鑷€傚簲杞崲鍣ㄧ殑璐ㄩ噺
            if (auto adaptive = dynamic_cast<AdaptiveSampleRateConverter*>(resampler_.get())) {
                int quality = static_cast<int>(value);
                quality = std::max(0, std::min(3, quality));
                // 杩欓噷闇€瑕佸疄鐜拌川閲忚缃柟娉?
            }
        }
    }

    double get_parameter(const std::string& name) override {
        if (name == "type") return static_cast<double>(type_);
        if (name == "target_rate") return static_cast<double>(target_rate_);
        if (name == "quality") return 2.0; // 榛樿楂樿川閲?
        return 0.0;
    }

    std::vector<std::string> get_parameter_names() override {
        return {"type", "target_rate", "quality"};
    }

    void reset() override {
        if (resampler_ && configured_) {
            resampler_->initialize(input_format_.sample_rate,
                                  target_rate_,
                                  input_format_.channels);
        }
    }

    int get_latency_samples() const override {
        // 涓嶅悓绫诲瀷鐨勫欢杩?
        switch (type_) {
            case ResamplerType::Linear: return 0;
            case ResamplerType::Cubic: return 4;
            case ResamplerType::Sinc8: return 8;
            case ResamplerType::Sinc16: return 16;
            case ResamplerType::Adaptive: return 64;
            default: return 0;
        }
    }

    // 棰濆鏂规硶锛氳幏鍙栫被鍨嬪悕绉?
    const char* get_type_name() const {
        switch (type_) {
            case ResamplerType::Linear: return "Linear";
            case ResamplerType::Cubic: return "Cubic";
            case ResamplerType::Sinc8: return "Sinc 8-tap";
            case ResamplerType::Sinc16: return "Sinc 16-tap";
            case ResamplerType::Adaptive: return "Adaptive";
            default: return "Unknown";
        }
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
        info.name = "XpuMusic Sample Rate Converter DSP Factory";
        info.version = "1.0.0";
        info.author = "XpuMusic Team";
        info.description = "Factory for sample rate converter DSP plugin";
        info.type = PluginType::DSPEffect;
        info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
        return info;
    }
};

// 瀵煎嚭鎻掍欢
XPUMUSIC_EXPORT_DSP_PLUGIN(ResamplerDSPPlugin)