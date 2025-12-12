/**
 * @file configured_resampler.cpp
 * @brief 閰嶇疆椹卞姩鐨勯噸閲囨牱鍣?
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

    // 浠庨厤缃鐞嗗櫒鑾峰彇璁剧疆
    const auto& config = ConfigManagerSingleton::get_instance().get_config().resampler;

    // 纭畾绮惧害
    precision_ = config.floating_precision;
    use_64bit_ = (precision_ == 64);

    std::cout << "Initializing resampler with " << precision_ << "-bit floating point precision" << std::endl;

    // 鏍规嵁閰嶇疆鍒涘缓杞崲鍣?
    std::string quality = config.quality;

    if (use_64bit_) {
        // 鍒涘缓64浣嶈浆鎹㈠櫒
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

            // 閰嶇疆鑷€傚簲鍙傛暟
            if (config.cpu_threshold > 0.0 && config.cpu_threshold <= 1.0) {
                adaptive->set_cpu_threshold(config.cpu_threshold);
                std::cout << "Using 64-bit Adaptive resampler with CPU threshold: "
                          << (config.cpu_threshold * 100) << "%" << std::endl;
            }

            converter64_ = std::move(adaptive);
        }
        else {
            // 榛樿浣跨敤64浣嶈嚜閫傚簲
            converter64_ = std::make_unique<AdaptiveSampleRateConverter64>();
            std::cout << "Using default 64-bit Adaptive resampler" << std::endl;
        }
    }
    else {
        // 鍒涘缓32浣嶈浆鎹㈠櫒
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

            // 閰嶇疆鑷€傚簲鍙傛暟
            if (config.cpu_threshold > 0.0 && config.cpu_threshold <= 1.0) {
                adaptive->set_cpu_threshold(config.cpu_threshold);
                std::cout << "Using 32-bit Adaptive resampler with CPU threshold: "
                          << (config.cpu_threshold * 100) << "%" << std::endl;
            }

            converter_ = std::move(adaptive);
        }
        else {
            // 榛樿浣跨敤32浣嶈嚜閫傚簲
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

// 宸ュ巶鍑芥暟
std::unique_ptr<ISampleRateConverter> create_configured_sample_rate_converter() {
    return std::make_unique<ConfiguredSampleRateConverter>();
}

// 鏍规嵁鏍煎紡鍒涘缓閲嶉噰鏍峰櫒
std::unique_ptr<ISampleRateConverter> create_sample_rate_converter_for_format(const std::string& format) {
    const auto& config = ConfigManagerSingleton::get_instance().get_config().resampler;

    // 鏌ユ壘鏍煎紡鐗瑰畾鐨勮川閲忚缃?
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
        // 鑷€傚簲鎴栭粯璁?
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