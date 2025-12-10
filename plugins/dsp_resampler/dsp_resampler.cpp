/**
 * @file dsp_resampler.cpp
 * @brief XpuMusic 码率转换DSP插件
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

    // 内部缓冲区
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

        // 创建合适的转换器
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

        // 初始化转换器
        if (!resampler_->initialize(input_format_.sample_rate,
                                   target_rate_,
                                   input_format_.channels)) {
            last_error_ = "Failed to initialize resampler";
            set_state(PluginState::Error);
            return false;
        }

        // 分配缓冲区
        int max_output_frames = static_cast<int>(
            input_format_.sample_rate * 2 * // 2秒输入
            (static_cast<double>(target_rate_) / input_format_.sample_rate) * 1.5 // 估算输出
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

        // 计算最大可能的输出帧数
        double ratio = static_cast<double>(target_rate_) / input_format_.sample_rate;
        int max_output_frames = static_cast<int>(input.frames * ratio * 1.2);

        // 确保输出缓冲区足够大
        if (output.channels * max_output_frames > static_cast<int>(output_buffer_.size())) {
            output_buffer_.resize(output.channels * max_output_frames);
        }

        // 执行转换
        int output_frames = resampler_->convert(input.data, input.frames,
                                               output_buffer_.data(), max_output_frames);

        // 如果输出缓冲区太小，需要调整
        if (output.channels * output_frames > output.frames) {
            // 返回错误，让调用者分配更大的缓冲区
            last_error_ = "Output buffer too small";
            return -output_frames; // 负数表示需要的大小
        }

        // 复制到输出缓冲区
        memcpy(output.data, output_buffer_.data(),
               output_frames * output.channels * sizeof(float));

        return output_frames;
    }

    void set_parameter(const std::string& name, double value) override {
        if (name == "type") {
            int type_int = static_cast<int>(value);
            type_int = std::max(0, std::min(4, type_int));
            type_ = static_cast<ResamplerType>(type_int);

            // 如果已配置，需要重新配置
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
            // 设置自适应转换器的质量
            if (auto adaptive = dynamic_cast<AdaptiveSampleRateConverter*>(resampler_.get())) {
                int quality = static_cast<int>(value);
                quality = std::max(0, std::min(3, quality));
                // 这里需要实现质量设置方法
            }
        }
    }

    double get_parameter(const std::string& name) override {
        if (name == "type") return static_cast<double>(type_);
        if (name == "target_rate") return static_cast<double>(target_rate_);
        if (name == "quality") return 2.0; // 默认高质量
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
        // 不同类型的延迟
        switch (type_) {
            case ResamplerType::Linear: return 0;
            case ResamplerType::Cubic: return 4;
            case ResamplerType::Sinc8: return 8;
            case ResamplerType::Sinc16: return 16;
            case ResamplerType::Adaptive: return 64;
            default: return 0;
        }
    }

    // 额外方法：获取类型名称
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

// 插件工厂
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

// 导出插件
XPUMUSIC_EXPORT_DSP_PLUGIN(ResamplerDSPPlugin)