#pragma once

#include "../../sdk/xpumusic_plugin_sdk.h"
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <memory>
#include <vector>
#include <string>

namespace xpumusic::plugins {

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IAudioDecoder;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;

/**
 * @brief OGG/Vorbis瑙ｇ爜鍣ㄦ彃浠?
 *
 * 浣跨敤libvorbis搴撹В鐮丱GG/Vorbis鏂囦欢锛屾敮鎸侊細
 * - 鏈夋崯闊抽瑙ｇ爜
 * - 澶氱閲囨牱鐜囧拰浣嶆繁搴?
 * - 瀹屾暣鐨勫厓鏁版嵁鏀寔
 * - 鍙彉姣旂壒鐜?VBR)
 * - 娴佸紡瑙ｇ爜
 */
class OggVorbisDecoder : public IAudioDecoder {
private:
    // OGG/Vorbis瑙ｇ爜鍣?
    OggVorbis_File vf_;
    vorbis_info* vi_;
    vorbis_comment* vc_;

    // 鏂囦欢淇℃伅
    std::string file_path_;
    bool is_open_;

    // 闊抽鏍煎紡
    AudioFormat format_;

    // 瑙ｇ爜鐘舵€?
    std::vector<float> output_buffer_;
    size_t output_buffer_size_;
    size_t output_buffer_pos_;

    // 娴佷俊鎭?
    ogg_int64_t total_samples_;
    double duration_;
    ogg_int64_t current_sample_;

    // 鍏冩暟鎹?
    struct VorbisComment {
        std::string title;
        std::string artist;
        std::string album;
        std::string date;
        std::string comment;
        std::string genre;
        std::string track;
        std::string albumartist;
        std::string composer;
        std::string performer;
        std::string copyright;
        std::string license;
        std::string location;
        std::string contact;
        std::string isrc;
    } comments_;

    // 閿欒淇℃伅
    std::string last_error_;

    // 鏂囦欢鎿嶄綔鍥炶皟
    static size_t read_func(void* ptr, size_t size, size_t nmemb, void* datasource);
    static int seek_func(void* datasource, ogg_int64_t offset, int whence);
    static long tell_func(void* datasource);
    static int close_func(void* datasource);

public:
    OggVorbisDecoder();
    ~OggVorbisDecoder() override;

    // IPlugin 鎺ュ彛
    bool initialize() override;
    void shutdown() override;
    PluginState get_state() const override;
    void set_state(PluginState state) override;
    PluginInfo get_info() const override;
    std::string get_last_error() const override;

    // IAudioDecoder 鎺ュ彛
    bool can_decode(const std::string& file_path) override;
    std::vector<std::string> get_supported_extensions() override;
    bool open(const std::string& file_path) override;
    int decode(AudioBuffer& buffer, int max_frames) override;
    void close() override;
    AudioFormat get_format() const override;

    // OGG/Vorbis鐗瑰畾鍔熻兘
    bool seek(double seconds);
    double get_duration() const;
    VorbisComment get_comments() const;

    // 鍏冩暟鎹?
    nlohmann::json get_metadata() const override;

private:
    // 鍐呴儴鏂规硶
    void cleanup();
    void set_error(const std::string& error);
    void calculate_duration();
    void parse_comments();
    std::string parse_comment_value(const std::string& comment, const std::string& field);
};

// OGG/Vorbis瑙ｇ爜鍣ㄥ伐鍘?
class OggVorbisDecoderFactory : public ITypedPluginFactory<IAudioDecoder> {
public:
    std::unique_ptr<IAudioDecoder> create_typed() override {
        return std::make_unique<OggVorbisDecoder>();
    }

    PluginInfo get_info() const override {
        OggVorbisDecoder decoder;
        return decoder.get_info();
    }
};

} // namespace xpumusic::plugins

// 瀵煎嚭鎻掍欢
QODER_EXPORT_AUDIO_PLUGIN(xpumusic::plugins::OggVorbisDecoder)

// 鑷姩娉ㄥ唽瑙ｇ爜鍣?
QODER_AUTO_REGISTER_DECODER(xpumusic::plugins::OggVorbisDecoder, {"ogg", "oga", "vorbis"})