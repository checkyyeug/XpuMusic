#pragma once

#include "../sdk/xpumusic_plugin_sdk.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

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
 * @brief 闊抽瑙ｇ爜鍣ㄦ敞鍐岃〃
 *
 * 璐熻矗绠＄悊鎵€鏈夐煶棰戣В鐮佸櫒鐨勬敞鍐屽拰鑾峰彇
 * 鏀寔鍔ㄦ€佹敞鍐岃В鐮佸櫒锛屽苟鏍规嵁鏂囦欢鎵╁睍鍚嶈嚜鍔ㄩ€夋嫨鍚堥€傜殑瑙ｇ爜鍣?
 */
class AudioDecoderRegistry {
public:
    using DecoderFactory = std::function<std::unique_ptr<IAudioDecoder>()>;

    /**
     * @brief 鑾峰彇鍏ㄥ眬娉ㄥ唽琛ㄥ疄渚?
     */
    static AudioDecoderRegistry& get_instance();

    /**
     * @brief 娉ㄥ唽瑙ｇ爜鍣?
     * @param name 瑙ｇ爜鍣ㄥ悕绉?
     * @param supported_formats 鏀寔鐨勬枃浠舵牸寮忓垪琛?
     * @param factory 瑙ｇ爜鍣ㄥ伐鍘傚嚱鏁?
     */
    void register_decoder(const std::string& name,
                         const std::vector<std::string>& supported_formats,
                         DecoderFactory factory);

    /**
     * @brief 娉ㄥ唽瑙ｇ爜鍣紙浣跨敤鎻掍欢宸ュ巶锛?
     * @param name 瑙ｇ爜鍣ㄥ悕绉?
     * @param factory 鎻掍欢宸ュ巶瀹炰緥
     */
    void register_decoder(const std::string& name,
                         std::unique_ptr<ITypedPluginFactory<IAudioDecoder>> factory);

    /**
     * @brief 鏍规嵁鏂囦欢璺緞鑾峰彇鍚堥€傜殑瑙ｇ爜鍣?
     * @param file_path 鏂囦欢璺緞
     * @return 瑙ｇ爜鍣ㄥ疄渚嬶紝濡傛灉娌℃湁鎵惧埌鍚堥€傜殑瑙ｇ爜鍣ㄥ垯杩斿洖nullptr
     */
    std::unique_ptr<IAudioDecoder> get_decoder(const std::string& file_path);

    /**
     * @brief 鏍规嵁鍚嶇О鑾峰彇瑙ｇ爜鍣?
     * @param decoder_name 瑙ｇ爜鍣ㄥ悕绉?
     * @return 瑙ｇ爜鍣ㄥ疄渚嬶紝濡傛灉娌℃湁鎵惧埌鍒欒繑鍥瀗ullptr
     */
    std::unique_ptr<IAudioDecoder> get_decoder_by_name(const std::string& decoder_name);

    /**
     * @brief 鑾峰彇鎵€鏈夋敞鍐岀殑瑙ｇ爜鍣ㄤ俊鎭?
     */
    std::vector<PluginInfo> get_registered_decoders() const;

    /**
     * @brief 鑾峰彇鏀寔鎸囧畾鏍煎紡鐨勮В鐮佸櫒鍒楄〃
     * @param format 鏂囦欢鏍煎紡
     * @return 鏀寔璇ユ牸寮忕殑瑙ｇ爜鍣ㄥ悕绉板垪琛?
     */
    std::vector<std::string> get_decoders_for_format(const std::string& format);

    /**
     * @brief 妫€鏌ユ槸鍚︽敮鎸佹煇涓枃浠舵牸寮?
     * @param file_path 鏂囦欢璺緞
     * @return 鏄惁鏀寔
     */
    bool supports_format(const std::string& file_path);

    /**
     * @brief 璁剧疆榛樿瑙ｇ爜鍣紙鐢ㄤ簬鏌愮鏍煎紡锛?
     * @param format 鏂囦欢鏍煎紡
     * @param decoder_name 瑙ｇ爜鍣ㄥ悕绉?
     */
    void set_default_decoder(const std::string& format, const std::string& decoder_name);

    /**
     * @brief 鑾峰彇鏍煎紡鐨勯粯璁よВ鐮佸櫒
     * @param format 鏂囦欢鏍煎紡
     * @return 瑙ｇ爜鍣ㄥ悕绉帮紝濡傛灉娌℃湁璁剧疆榛樿瑙ｇ爜鍣ㄥ垯杩斿洖绌哄瓧绗︿覆
     */
    std::string get_default_decoder(const std::string& format);

    /**
     * @brief 鍚敤/绂佺敤瑙ｇ爜鍣?
     * @param decoder_name 瑙ｇ爜鍣ㄥ悕绉?
     * @param enabled 鏄惁鍚敤
     */
    void set_decoder_enabled(const std::string& decoder_name, bool enabled);

    /**
     * @brief 妫€鏌ヨВ鐮佸櫒鏄惁鍚敤
     * @param decoder_name 瑙ｇ爜鍣ㄥ悕绉?
     * @return 鏄惁鍚敤
     */
    bool is_decoder_enabled(const std::string& decoder_name) const;

    /**
     * @brief 娉ㄩ攢瑙ｇ爜鍣?
     * @param decoder_name 瑙ｇ爜鍣ㄥ悕绉?
     */
    void unregister_decoder(const std::string& decoder_name);

private:
    AudioDecoderRegistry() = default;
    ~AudioDecoderRegistry() = default;
    AudioDecoderRegistry(const AudioDecoderRegistry&) = delete;
    AudioDecoderRegistry& operator=(const AudioDecoderRegistry&) = delete;

    struct DecoderInfo {
        DecoderFactory factory;
        std::unique_ptr<ITypedPluginFactory<IAudioDecoder>> plugin_factory;
        std::vector<std::string> supported_formats;
        bool enabled = true;
    };

    std::map<std::string, DecoderInfo> decoders_;
    std::map<std::string, std::string> format_defaults_;

    std::string extract_extension(const std::string& file_path) const;
};

/**
 * @brief 瑙ｇ爜鍣ㄨ嚜鍔ㄦ敞鍐屽櫒
 *
 * 浣跨敤RAII妯″紡鍦ㄧ▼搴忓惎鍔ㄦ椂鑷姩娉ㄥ唽瑙ｇ爜鍣?
 */
template<typename DecoderType>
class DecoderAutoRegister {
public:
    DecoderAutoRegister(const std::vector<std::string>& formats) {
        auto factory = []() -> std::unique_ptr<IAudioDecoder> {
            return std::make_unique<DecoderType>();
        };

        AudioDecoderRegistry::get_instance().register_decoder(
            DecoderType().get_info().name,
            formats,
            factory
        );
    }
};

/**
 * @brief 瀹忓畾涔夛紝鐢ㄤ簬鑷姩娉ㄥ唽瑙ｇ爜鍣?
 */
#define QODER_AUTO_REGISTER_DECODER(decoder_class, formats) \
    static DecoderAutoRegister<decoder_class> _auto_register_##decoder_class(formats);

} // namespace xpumusic::core