#pragma once

#include "audio_decoder_registry.h"
#include "audio_format_detector.h"
#include "../sdk/xpumusic_plugin_sdk.h"
#include <memory>
#include <string>
#include <map>
#include <vector>

// Use simple map instead of nlohmann/json
using json_map = std::map<std::string, std::string>;

namespace xpumusic::core {

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IAudioDecoder;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;
using xpumusic::IPluginFactory;
using xpumusic::ITypedPluginFactory;

/**
 * @brief 闊抽瑙ｇ爜鍣ㄧ鐞嗗櫒
 *
 * 缁熶竴绠＄悊闊抽瑙ｇ爜鍣ㄥ拰鏍煎紡妫€娴嬬殑楂樼骇鎺ュ彛
 * 鎻愪緵绠€鍖栫殑API鐢ㄤ簬闊抽鏂囦欢鐨勮В鐮?
 */
class AudioDecoderManager {
public:
    /**
     * @brief 鑾峰彇鍏ㄥ眬绠＄悊鍣ㄥ疄渚?
     */
    static AudioDecoderManager& get_instance();

    /**
     * @brief 鍒濆鍖栫鐞嗗櫒
     * 鍔犺浇鎵€鏈夊唴缃В鐮佸櫒
     */
    void initialize();

    /**
     * @brief 鑾峰彇闊抽鏂囦欢鐨勮В鐮佸櫒
     * @param file_path 鏂囦欢璺緞
     * @return 瑙ｇ爜鍣ㄥ疄渚嬶紝濡傛灉娌℃湁鎵惧埌鍚堥€傜殑瑙ｇ爜鍣ㄥ垯杩斿洖nullptr
     */
    std::unique_ptr<IAudioDecoder> get_decoder_for_file(const std::string& file_path);

    /**
     * @brief 妫€娴嬮煶棰戞枃浠舵牸寮?
     * @param file_path 鏂囦欢璺緞
     * @return 鏍煎紡淇℃伅
     */
    AudioFormatInfo detect_format(const std::string& file_path);

    /**
     * @brief 鎵撳紑闊抽鏂囦欢骞惰繑鍥炶В鐮佸櫒
     * @param file_path 鏂囦欢璺緞
     * @return 宸叉墦寮€鐨勮В鐮佸櫒瀹炰緥锛屽鏋滃け璐ュ垯杩斿洖nullptr
     */
    std::unique_ptr<IAudioDecoder> open_audio_file(const std::string& file_path);

    /**
     * @brief 妫€鏌ユ槸鍚︽敮鎸佹煇涓枃浠?
     * @param file_path 鏂囦欢璺緞
     * @return 鏄惁鏀寔
     */
    bool supports_file(const std::string& file_path);

    /**
     * @brief 鑾峰彇鎵€鏈夋敮鎸佺殑鏍煎紡
     * @return 鏀寔鐨勬牸寮忓垪琛?
     */
    std::vector<std::string> get_supported_formats();

    /**
     * @brief 鑾峰彇鎵€鏈夋敞鍐岀殑瑙ｇ爜鍣?
     * @return 瑙ｇ爜鍣ㄤ俊鎭垪琛?
     */
    std::vector<PluginInfo> get_available_decoders();

    /**
     * @brief 璁剧疆鏍煎紡鐨勯粯璁よВ鐮佸櫒
     * @param format 鏂囦欢鏍煎紡
     * @param decoder_name 瑙ｇ爜鍣ㄥ悕绉?
     */
    void set_default_decoder(const std::string& format, const std::string& decoder_name);

    /**
     * @brief 鑾峰彇瑙ｇ爜鍣ㄧ殑鍏冩暟鎹?
     * @param file_path 鏂囦欢璺緞
     * @return 鍏冩暟鎹槧灏勶紝濡傛灉澶辫触鍒欒繑鍥炵┖鏄犲皠
     */
    json_map get_metadata(const std::string& file_path);

    /**
     * @brief 鑾峰彇瑙ｇ爜鍣ㄧ殑鎸佺画鏃堕暱
     * @param file_path 鏂囦欢璺緞
     * @return 鎸佺画鏃堕暱锛堢锛夛紝濡傛灉澶辫触杩斿洖-1.0
     */
    double get_duration(const std::string& file_path);

    /**
     * @brief 鍚敤/绂佺敤瑙ｇ爜鍣?
     * @param decoder_name 瑙ｇ爜鍣ㄥ悕绉?
     * @param enabled 鏄惁鍚敤
     */
    void set_decoder_enabled(const std::string& decoder_name, bool enabled);

    /**
     * @brief 娉ㄥ唽鑷畾涔夎В鐮佸櫒宸ュ巶
     * @param name 瑙ｇ爜鍣ㄥ悕绉?
     * @param factory 宸ュ巶鍑芥暟
     */
    void register_decoder_factory(const std::string& name,
                                 std::function<std::unique_ptr<IAudioDecoder>()> factory);

private:
    AudioDecoderManager() = default;
    ~AudioDecoderManager() = default;
    AudioDecoderManager(const AudioDecoderManager&) = delete;
    AudioDecoderManager& operator=(const AudioDecoderManager&) = delete;

    bool initialized_ = false;
};

} // namespace xpumusic::core