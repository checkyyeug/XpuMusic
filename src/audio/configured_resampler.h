/**
 * @file configured_resampler.h
 * @brief 閰嶇疆椹卞姩鐨勯噸閲囨牱鍣ㄥご鏂囦欢
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
 * @brief 閰嶇疆椹卞姩鐨勯噸閲囨牱鍣?
 *
 * 鏍规嵁閰嶇疆鏂囦欢鑷姩閫夋嫨鍜岄厤缃悎閫傜殑閲嶉噰鏍风畻娉?
 */
class ConfiguredSampleRateConverter : public ISampleRateConverter {
private:
    std::unique_ptr<ISampleRateConverter> converter_;
    std::unique_ptr<ISampleRateConverter64> converter64_;
    int precision_;
    bool use_64bit_;

public:
    /**
     * @brief 鏋勯€犲嚱鏁?
     *
     * 浠庨厤缃鐞嗗櫒璇诲彇閰嶇疆骞跺垱寤虹浉搴旂殑閲嶉噰鏍峰櫒
     */
    ConfiguredSampleRateConverter();

    /**
     * @brief 鏋愭瀯鍑芥暟
     */
    ~ConfiguredSampleRateConverter() override;

    /**
     * @brief 閰嶇疆閲嶉噰鏍峰櫒
     * @param input_rate 杈撳叆閲囨牱鐜?
     * @param output_rate 杈撳嚭閲囨牱鐜?
     * @param channels 澹伴亾鏁?
     * @return 鏄惁鎴愬姛
     */
    bool configure(int input_rate, int output_rate, int channels) override;

    /**
     * @brief 澶勭悊闊抽鏁版嵁锛?2浣嶏級
     * @param input 杈撳叆缂撳啿鍖?
     * @param output 杈撳嚭缂撳啿鍖?
     * @param input_frames 杈撳叆甯ф暟
     * @return 杈撳嚭甯ф暟
     */
    int process(const float* input, float* output, int input_frames) override;

    /**
     * @brief 澶勭悊闊抽鏁版嵁锛?4浣嶏級
     * @param input 杈撳叆缂撳啿鍖?
     * @param output 杈撳嚭缂撳啿鍖?
     * @param input_frames 杈撳叆甯ф暟
     * @return 杈撳嚭甯ф暟
     */
    int process_64(const double* input, double* output, int input_frames);

    /**
     * @brief 鑾峰彇杈撳嚭寤惰繜
     * @param input_frames 杈撳叆甯ф暟
     * @return 杈撳嚭寤惰繜锛堝抚鏁帮級
     */
    int get_output_latency(int input_frames) const override;

    /**
     * @brief 閲嶇疆閲嶉噰鏍峰櫒鐘舵€?
     */
    void reset() override;

    // 鑾峰彇鍐呴儴杞崲鍣紙鐢ㄤ簬楂樼骇閰嶇疆锛?
    ISampleRateConverter* get_internal_converter() const {
        return converter_.get();
    }

    ISampleRateConverter64* get_internal_converter_64() const {
        return converter64_.get();
    }

    // 鑾峰彇褰撳墠绮惧害
    int get_precision() const { return precision_; }
    bool is_using_64bit() const { return use_64bit_; }
};

/**
 * @brief 鍒涘缓閰嶇疆椹卞姩鐨勯噸閲囨牱鍣?
 * @return 閲嶉噰鏍峰櫒瀹炰緥
 */
std::unique_ptr<ISampleRateConverter> create_configured_sample_rate_converter();

/**
 * @brief 鏍规嵁闊抽鏍煎紡鍒涘缓鍚堥€傜殑閲嶉噰鏍峰櫒
 * @param format 闊抽鏍煎紡锛堝 "mp3", "flac" 绛夛級
 * @return 閲嶉噰鏍峰櫒瀹炰緥
 */
std::unique_ptr<ISampleRateConverter> create_sample_rate_converter_for_format(const std::string& format);

} // namespace audio