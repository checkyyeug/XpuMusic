/**
 * @file configured_resampler.cpp
 * @brief 配置驱动的重采样器
 * @date 2025-12-10
 */

#include "configured_resampler.h"
#include "../config/config_manager.h"
#include <iostream>

namespace audio {

ConfiguredSampleRateConverter::ConfiguredSampleRateConverter()
    : converter_()
    , converter64_()
    , precision_(32)
    , use_64bit_(false) {

    // 从配置管理器获取设置
    const auto& config = ConfigManagerSingleton::get_instance().get_config().resampler;

    // 确定精度
    precision_ = config.floating_precision;
    use_64bit_ = (precision_ == 64);

    std::cout << "Initializing resampler with " << precision_ << "-bit floating point precision" << std::endl;

    // 根据配置创建转换器
    std::string quality = config.quality;

    if (use_64bit_) {
        // 创建64位转换器
        if (quality == "fast") {
            converter64_ = std::make_unique<LinearSampleRateConverter64>();
            std::cout << "Using 64-bit Linear resampler (fast quality)" << std::endl;
        }
        else if (quality == "good") {
            converter64_ = std::make_unique<CubicSampleRateConverter64>();
            std::cout << "Using 64-bit Cubic resampler (good quality)" << std::endl;
        }
        else if (quality == "high") {
            converter64_ = std::make_unique<SincSampleRateConverter64>(8);
            std::cout << "Using 64-bit Sinc-8 resampler (high quality)" << std::endl;
        }
        else if (quality == "best") {
            converter64_ = std::make_unique<SincSampleRateConverter64>(16);
            std::cout << "Using 64-bit Sinc-16 resampler (best quality)" << std::endl;
        }
        else if (quality == "adaptive" || config.enable_adaptive) {
            auto adaptive = std::make_unique<AdaptiveSampleRateConverter64>();

            // 配置自适应参数
            if (config.cpu_threshold > 0.0 && config.cpu_threshold <= 1.0) {
                adaptive->set_cpu_threshold(config.cpu_threshold);
                std::cout << "Using 64-bit Adaptive resampler with CPU threshold: "
                          << (config.cpu_threshold * 100) << "%" << std::endl;
            }

            converter64_ = std::move(adaptive);
        }
        else {
            // 默认使用64位自适应
            converter64_ = std::make_unique<AdaptiveSampleRateConverter64>();
            std::cout << "Using default 64-bit Adaptive resampler" << std::endl;
        }
    }
    else {
        // 创建32位转换器
        if (quality == "fast") {
            converter_ = std::make_unique<LinearSampleRateConverter>();
            std::cout << "Using 32-bit Linear resampler (fast quality)" << std::endl;
        }
        else if (quality == "good") {
            converter_ = std::make_unique<CubicSampleRateConverter>();
            std::cout << "Using 32-bit Cubic resampler (good quality)" << std::endl;
        }
        else if (quality == "high") {
            converter_ = std::make_unique<SincSampleRateConverter>(8);
            std::cout << "Using 32-bit Sinc-8 resampler (high quality)" << std::endl;
        }
        else if (quality == "best") {
            converter_ = std::make_unique<SincSampleRateConverter>(16);
            std::cout << "Using 32-bit Sinc-16 resampler (best quality)" << std::endl;
        }
        else if (quality == "adaptive" || config.enable_adaptive) {
            auto adaptive = std::make_unique<AdaptiveSampleRateConverter>();

            // 配置自适应参数
            if (config.cpu_threshold > 0.0 && config.cpu_threshold <= 1.0) {
                adaptive->set_cpu_threshold(config.cpu_threshold);
                std::cout << "Using 32-bit Adaptive resampler with CPU threshold: "
                          << (config.cpu_threshold * 100) << "%" << std::endl;
            }

            converter_ = std::move(adaptive);
        }
        else {
            // 默认使用32位自适应
            converter_ = std::make_unique<AdaptiveSampleRateConverter>();
            std::cout << "Using default 32-bit Adaptive resampler" << std::endl;
        }
    }
}

ConfiguredSampleRateConverter::~ConfiguredSampleRateConverter() = default;

bool ConfiguredSampleRateConverter::configure(int input_rate, int output_rate, int channels) {
    if (use_64bit_) {
        if (!converter64_) {
            return false;
        }
        return converter64_->configure(input_rate, output_rate, channels);
    } else {
        if (!converter_) {
            return false;
        }
        return converter_->configure(input_rate, output_rate, channels);
    }
}

int ConfiguredSampleRateConverter::process(const float* input, float* output, int input_frames) {
    if (!converter_) {
        return 0;
    }

    return converter_->process(input, output, input_frames);
}

int ConfiguredSampleRateConverter::process_64(const double* input, double* output, int input_frames) {
    if (!converter64_) {
        return 0;
    }

    return converter64_->process(input, output, input_frames);
}

int ConfiguredSampleRateConverter::get_output_latency(int input_frames) const {
    if (use_64bit_) {
        if (!converter64_) {
            return 0;
        }
        return converter64_->get_output_latency(input_frames);
    } else {
        if (!converter_) {
            return 0;
        }
        return converter_->get_output_latency(input_frames);
    }
}

void ConfiguredSampleRateConverter::reset() {
    if (converter_) {
        converter_->reset();
    }
    if (converter64_) {
        converter64_->reset();
    }
}

// 工厂函数
std::unique_ptr<ISampleRateConverter> create_configured_sample_rate_converter() {
    return std::make_unique<ConfiguredSampleRateConverter>();
}

// 根据格式创建重采样器
std::unique_ptr<ISampleRateConverter> create_sample_rate_converter_for_format(const std::string& format) {
    const auto& config = ConfigManagerSingleton::get_instance().get_config().resampler;

    // 查找格式特定的质量设置
    auto it = config.format_quality.find(format);
    std::string quality = (it != config.format_quality.end()) ? it->second : config.quality;

    std::cout << "Creating resampler for format '" << format
              << "' with quality '" << quality << "'" << std::endl;

    if (quality == "fast") {
        return std::make_unique<LinearSampleRateConverter>();
    }
    else if (quality == "good") {
        return std::make_unique<CubicSampleRateConverter>();
    }
    else if (quality == "high") {
        return std::make_unique<SincSampleRateConverter>(8);
    }
    else if (quality == "best") {
        return std::make_unique<SincSampleRateConverter>(16);
    }
    else {
        // 自适应或默认
        if (config.enable_adaptive) {
            auto adaptive = std::make_unique<AdaptiveSampleRateConverter>();
            if (config.cpu_threshold > 0.0 && config.cpu_threshold <= 1.0) {
                adaptive->set_cpu_threshold(config.cpu_threshold);
            }
            return std::move(adaptive);
        }
        return std::make_unique<CubicSampleRateConverter>();
    }
}

} // namespace audio