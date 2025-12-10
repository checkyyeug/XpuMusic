/**
 * @file test_resampler_precision.cpp
 * @brief 测试32位和64位重采样器的质量和性能
 * @date 2025-12-10
 */

#include "../src/audio/configured_resampler.h"
#include "../src/audio/sample_rate_converter_64.h"
#include "../config/config_manager.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <random>

using namespace xpumusic;
using namespace audio;

// 生成测试信号
void generate_test_signal(float* buffer, int frames, int channels, double freq, double sample_rate) {
    for (int i = 0; i < frames; ++i) {
        double t = i / sample_rate;
        float sample = 0.5f * sinf(2.0f * M_PI * freq * t);

        for (int ch = 0; ch < channels; ++ch) {
            buffer[i * channels + ch] = sample;
        }
    }
}

void generate_test_signal(double* buffer, int frames, int channels, double freq, double sample_rate) {
    for (int i = 0; i < frames; ++i) {
        double t = i / sample_rate;
        double sample = 0.5 * sin(2.0 * M_PI * freq * t);

        for (int ch = 0; ch < channels; ++ch) {
            buffer[i * channels + ch] = sample;
        }
    }
}

// 计算信噪比
double calculate_snr(const float* signal, const float* noise, int length) {
    double signal_power = 0.0;
    double noise_power = 0.0;

    for (int i = 0; i < length; ++i) {
        signal_power += signal[i] * signal[i];
        noise_power += noise[i] * noise[i];
    }

    if (noise_power < 1e-20) noise_power = 1e-20;

    return 10.0 * log10(signal_power / noise_power);
}

double calculate_snr(const double* signal, const double* noise, int length) {
    double signal_power = 0.0;
    double noise_power = 0.0;

    for (int i = 0; i < length; ++i) {
        signal_power += signal[i] * signal[i];
        noise_power += noise[i] * noise[i];
    }

    if (noise_power < 1e-40) noise_power = 1e-40;

    return 10.0 * log10(signal_power / noise_power);
}

// 测试重采样质量
template<typename T>
void test_resampler_quality(ISampleRateConverter* resampler, ISampleRateConverter64* resampler64,
                          const std::string& name, int input_rate, int output_rate) {
    const int test_frames = 8192;
    const int channels = 2;
    const double test_freq = 1000.0; // 1kHz test tone

    std::cout << "\n=== 测试 " << name << " (" << input_rate << " -> " << output_rate << ") ===\n";

    // 32位测试
    if (resampler) {
        std::vector<float> input_32(test_frames * channels);
        std::vector<float> output_32(test_frames * channels * 2);
        std::vector<float> reference_32(test_frames * channels * 2);

        generate_test_signal(input_32.data(), test_frames, channels, test_freq, input_rate);

        // 配置和重采样
        if (resampler->configure(input_rate, output_rate, channels)) {
            auto start = std::chrono::high_resolution_clock::now();
            int out_frames = resampler->process(input_32.data(), output_32.data(), test_frames);
            auto end = std::chrono::high_resolution_clock::now();

            // 生成理想输出（用于比较）
            generate_test_signal(reference_32.data(), out_frames, channels, test_freq, output_rate);

            // 计算SNR
            double snr = calculate_snr(reference_32.data(), output_32.data(), out_frames * channels);
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            std::cout << "32-bit 结果:\n";
            std::cout << "  输出帧数: " << out_frames << "\n";
            std::cout << "  SNR: " << snr << " dB\n";
            std::cout << "  处理时间: " << duration.count() << " μs\n";
        }
    }

    // 64位测试
    if (resampler64) {
        std::vector<double> input_64(test_frames * channels);
        std::vector<double> output_64(test_frames * channels * 2);
        std::vector<double> reference_64(test_frames * channels * 2);

        generate_test_signal(input_64.data(), test_frames, channels, test_freq, input_rate);

        // 配置和重采样
        if (resampler64->configure(input_rate, output_rate, channels)) {
            auto start = std::chrono::high_resolution_clock::now();
            int out_frames = resampler64->process(input_64.data(), output_64.data(), test_frames);
            auto end = std::chrono::high_resolution_clock::now();

            // 生成理想输出（用于比较）
            generate_test_signal(reference_64.data(), out_frames, channels, test_freq, output_rate);

            // 计算SNR
            double snr = calculate_snr(reference_64.data(), output_64.data(), out_frames * channels);
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            std::cout << "64-bit 结果:\n";
            std::cout << "  输出帧数: " << out_frames << "\n";
            std::cout << "  SNR: " << snr << " dB\n";
            std::cout << "  处理时间: " << duration.count() << " μs\n";
        }
    }
}

// 测试动态范围
void test_dynamic_range() {
    std::cout << "\n=== 测试动态范围 ===\n";

    std::vector<double> amplitudes = {1.0, 0.1, 0.01, 0.001, 0.0001};
    const int test_frames = 4096;
    const int channels = 2;

    for (double amp : amplitudes) {
        std::cout << "\n测试幅度: " << amp << "\n";

        // 32位测试
        auto converter32 = std::make_unique<SincSampleRateConverter>(16);
        std::vector<float> input_32(test_frames * channels);
        std::vector<float> output_32(test_frames * channels * 2);

        // 生成低幅度信号
        for (int i = 0; i < test_frames; ++i) {
            double t = i / 44100.0;
            float sample = amp * sinf(2.0f * M_PI * 440.0f * t);
            input_32[i * 2] = sample;
            input_32[i * 2 + 1] = sample;
        }

        converter32->configure(44100, 48000, channels);
        int out_frames = converter32->process(input_32.data(), output_32.data(), test_frames);

        // 计算输出幅度
        double max_amp_32 = 0.0;
        for (int i = 0; i < out_frames * channels; ++i) {
            max_amp_32 = std::max(max_amp_32, std::abs(output_32[i]));
        }
        std::cout << "  32-bit 输出最大幅度: " << max_amp_32 << "\n";

        // 64位测试
        auto converter64 = std::make_unique<SincSampleRateConverter64>(16);
        std::vector<double> input_64(test_frames * channels);
        std::vector<double> output_64(test_frames * channels * 2);

        for (int i = 0; i < test_frames; ++i) {
            double t = i / 44100.0;
            double sample = amp * sin(2.0 * M_PI * 440.0 * t);
            input_64[i * 2] = sample;
            input_64[i * 2 + 1] = sample;
        }

        converter64->configure(44100, 48000, channels);
        out_frames = converter64->process(input_64.data(), output_64.data(), test_frames);

        double max_amp_64 = 0.0;
        for (int i = 0; i < out_frames * channels; ++i) {
            max_amp_64 = std::max(max_amp_64, std::abs(output_64[i]));
        }
        std::cout << "  64-bit 输出最大幅度: " << max_amp_64 << "\n";
    }
}

// 测试频谱质量
void test_spectral_quality() {
    std::cout << "\n=== 测试频谱质量 ===\n";
    std::cout << "生成复合信号 (440Hz + 880Hz + 1760Hz)\n";

    const int test_frames = 16384;
    const int channels = 1;  // 单通道便于分析

    // 32位测试
    auto converter32 = std::make_unique<SincSampleRateConverter>(16);
    std::vector<float> input_32(test_frames);
    std::vector<float> output_32(test_frames * 2);

    // 生成复合信号
    for (int i = 0; i < test_frames; ++i) {
        double t = i / 44100.0;
        float sample = 0.3f * sinf(2.0f * M_PI * 440.0f * t)  // A4
                     + 0.2f * sinf(2.0f * M_PI * 880.0f * t)  // A5
                     + 0.1f * sinf(2.0f * M_PI * 1760.0f * t); // A6
        input_32[i] = sample;
    }

    converter32->configure(44100, 96000, channels);
    int out_frames = converter32->process(input_32.data(), output_32.data(), test_frames);

    // 64位测试
    auto converter64 = std::make_unique<SincSampleRateConverter64>(16);
    std::vector<double> input_64(test_frames);
    std::vector<double> output_64(test_frames * 2);

    for (int i = 0; i < test_frames; ++i) {
        double t = i / 44100.0;
        double sample = 0.3 * sin(2.0 * M_PI * 440.0 * t)
                      + 0.2 * sin(2.0 * M_PI * 880.0 * t)
                      + 0.1 * sin(2.0 * M_PI * 1760.0 * t);
        input_64[i] = sample;
    }

    converter64->configure(44100, 96000, channels);
    out_frames = converter64->process(input_64.data(), output_64.data(), test_frames);

    std::cout << "32-bit 输出帧数: " << out_frames << "\n";
    std::cout << "64-bit 输出帧数: " << out_frames << "\n";

    // 这里应该进行FFT分析来比较频谱质量，但为了简化，只输出基本统计
    double mean_32 = 0.0, mean_64 = 0.0;
    double rms_32 = 0.0, rms_64 = 0.0;

    for (int i = 0; i < out_frames; ++i) {
        mean_32 += output_32[i];
        mean_64 += output_64[i];
        rms_32 += output_32[i] * output_32[i];
        rms_64 += output_64[i] * output_64[i];
    }

    mean_32 /= out_frames;
    mean_64 /= out_frames;
    rms_32 = sqrt(rms_32 / out_frames);
    rms_64 = sqrt(rms_64 / out_frames);

    std::cout << "32-bit 均值: " << mean_32 << ", RMS: " << rms_32 << "\n";
    std::cout << "64-bit 均值: " << mean_64 << ", RMS: " << rms_64 << "\n";
}

int main() {
    std::cout << "XpuMusic 重采样精度测试\n";
    std::cout << "==============================\n";

    // 初始化配置管理器
    ConfigManagerSingleton::initialize("config/default_config.json");

    // 测试场景
    struct TestScenario {
        int input_rate;
        int output_rate;
        std::string description;
    };

    std::vector<TestScenario> scenarios = {
        {44100, 48000, "44.1k -> 48k (常见)"},
        {48000, 44100, "48k -> 44.1k (常见)"},
        {44100, 96000, "44.1k -> 96k (升采样)"},
        {96000, 44100, "96k -> 44.1k (降采样)"},
        {44100, 192000, "44.1k -> 192k (高倍升采样)"},
        {192000, 44100, "192k -> 44.1k (高倍降采样)"}
    };

    // 测试不同质量级别
    std::vector<std::string> qualities = {"fast", "good", "high", "best"};
    std::vector<std::unique_ptr<ISampleRateConverter>> converters32;
    std::vector<std::unique_ptr<ISampleRateConverter64>> converters64;

    for (const auto& quality : qualities) {
        // 设置配置
        auto& config = ConfigManagerSingleton::get_instance();
        config.resampler().quality = quality;
        config.resampler().floating_precision = 32;
        converters32.push_back(create_configured_sample_rate_converter());

        config.resampler().floating_precision = 64;
        converters64.push_back(std::make_unique<ConfiguredSampleRateConverter>());
    }

    // 运行测试
    for (size_t i = 0; i < qualities.size(); ++i) {
        std::cout << "\n\n####################\n";
        std::cout << "质量级别: " << qualities[i] << "\n";
        std::cout << "####################\n";

        auto configured32 = dynamic_cast<ConfiguredSampleRateConverter*>(converters32[i].get());
        auto configured64 = dynamic_cast<ConfiguredSampleRateConverter*>(converters64[i].get());

        for (const auto& scenario : scenarios) {
            test_resampler_quality<float>(
                configured32 ? configured32->get_internal_converter() : nullptr,
                configured64 ? configured64->get_internal_converter_64() : nullptr,
                qualities[i],
                scenario.input_rate,
                scenario.output_rate
            );
        }
    }

    // 额外测试
    test_dynamic_range();
    test_spectral_quality();

    std::cout << "\n\n=== 测试完成 ===\n";

    // 清理
    ConfigManagerSingleton::shutdown();

    return 0;
}