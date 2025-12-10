/**
 * @file configured_resampler.h
 * @brief 配置驱动的重采样器头文件
 * @date 2025-12-10
 */

#pragma once

#include "sample_rate_converter.h"
#include "adaptive_resampler.h"
#include "sample_rate_converter_64.h"
#include "../config/config_manager.h"
#include <memory>
#include <string>

namespace audio {

/**
 * @brief 配置驱动的重采样器
 *
 * 根据配置文件自动选择和配置合适的重采样算法
 */
class ConfiguredSampleRateConverter : public ISampleRateConverter {
private:
    std::unique_ptr<ISampleRateConverter> converter_;
    std::unique_ptr<ISampleRateConverter64> converter64_;
    int precision_;
    bool use_64bit_;

public:
    /**
     * @brief 构造函数
     *
     * 从配置管理器读取配置并创建相应的重采样器
     */
    ConfiguredSampleRateConverter();

    /**
     * @brief 析构函数
     */
    ~ConfiguredSampleRateConverter() override;

    /**
     * @brief 配置重采样器
     * @param input_rate 输入采样率
     * @param output_rate 输出采样率
     * @param channels 声道数
     * @return 是否成功
     */
    bool configure(int input_rate, int output_rate, int channels) override;

    /**
     * @brief 处理音频数据（32位）
     * @param input 输入缓冲区
     * @param output 输出缓冲区
     * @param input_frames 输入帧数
     * @return 输出帧数
     */
    int process(const float* input, float* output, int input_frames) override;

    /**
     * @brief 处理音频数据（64位）
     * @param input 输入缓冲区
     * @param output 输出缓冲区
     * @param input_frames 输入帧数
     * @return 输出帧数
     */
    int process_64(const double* input, double* output, int input_frames);

    /**
     * @brief 获取输出延迟
     * @param input_frames 输入帧数
     * @return 输出延迟（帧数）
     */
    int get_output_latency(int input_frames) const override;

    /**
     * @brief 重置重采样器状态
     */
    void reset() override;

    // 获取内部转换器（用于高级配置）
    ISampleRateConverter* get_internal_converter() const {
        return converter_.get();
    }

    ISampleRateConverter64* get_internal_converter_64() const {
        return converter64_.get();
    }

    // 获取当前精度
    int get_precision() const { return precision_; }
    bool is_using_64bit() const { return use_64bit_; }
};

/**
 * @brief 创建配置驱动的重采样器
 * @return 重采样器实例
 */
std::unique_ptr<ISampleRateConverter> create_configured_sample_rate_converter();

/**
 * @brief 根据音频格式创建合适的重采样器
 * @param format 音频格式（如 "mp3", "flac" 等）
 * @return 重采样器实例
 */
std::unique_ptr<ISampleRateConverter> create_sample_rate_converter_for_format(const std::string& format);

} // namespace audio