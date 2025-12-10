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
 * @brief OGG/Vorbis解码器插件
 *
 * 使用libvorbis库解码OGG/Vorbis文件，支持：
 * - 有损音频解码
 * - 多种采样率和位深度
 * - 完整的元数据支持
 * - 可变比特率(VBR)
 * - 流式解码
 */
class OggVorbisDecoder : public IAudioDecoder {
private:
    // OGG/Vorbis解码器
    OggVorbis_File vf_;
    vorbis_info* vi_;
    vorbis_comment* vc_;

    // 文件信息
    std::string file_path_;
    bool is_open_;

    // 音频格式
    AudioFormat format_;

    // 解码状态
    std::vector<float> output_buffer_;
    size_t output_buffer_size_;
    size_t output_buffer_pos_;

    // 流信息
    ogg_int64_t total_samples_;
    double duration_;
    ogg_int64_t current_sample_;

    // 元数据
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

    // 错误信息
    std::string last_error_;

    // 文件操作回调
    static size_t read_func(void* ptr, size_t size, size_t nmemb, void* datasource);
    static int seek_func(void* datasource, ogg_int64_t offset, int whence);
    static long tell_func(void* datasource);
    static int close_func(void* datasource);

public:
    OggVorbisDecoder();
    ~OggVorbisDecoder() override;

    // IPlugin 接口
    bool initialize() override;
    void shutdown() override;
    PluginState get_state() const override;
    void set_state(PluginState state) override;
    PluginInfo get_info() const override;
    std::string get_last_error() const override;

    // IAudioDecoder 接口
    bool can_decode(const std::string& file_path) override;
    std::vector<std::string> get_supported_extensions() override;
    bool open(const std::string& file_path) override;
    int decode(AudioBuffer& buffer, int max_frames) override;
    void close() override;
    AudioFormat get_format() const override;

    // OGG/Vorbis特定功能
    bool seek(double seconds);
    double get_duration() const;
    VorbisComment get_comments() const;

    // 元数据
    nlohmann::json get_metadata() const override;

private:
    // 内部方法
    void cleanup();
    void set_error(const std::string& error);
    void calculate_duration();
    void parse_comments();
    std::string parse_comment_value(const std::string& comment, const std::string& field);
};

// OGG/Vorbis解码器工厂
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

// 导出插件
QODER_EXPORT_AUDIO_PLUGIN(xpumusic::plugins::OggVorbisDecoder)

// 自动注册解码器
QODER_AUTO_REGISTER_DECODER(xpumusic::plugins::OggVorbisDecoder, {"ogg", "oga", "vorbis"})