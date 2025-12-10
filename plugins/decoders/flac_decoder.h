#pragma once

#include "../../sdk/xpumusic_plugin_sdk.h"
#include <FLAC/stream_decoder.h>
#include <FLAC/metadata.h>
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
 * @brief FLAC解码器插件
 *
 * 使用libFLAC库解码FLAC文件，支持：
 * - 无损音频解码
 * - 多种采样率和位深度
 * - 完整的元数据支持
 * - 快速定位
 * - 流式解码
 */
class FLACDecoder : public IAudioDecoder {
private:
    // FLAC解码器
    FLAC__StreamDecoder* decoder_;

    // 文件信息
    std::string file_path_;
    FLAC__bool is_open_;

    // 音频格式
    AudioFormat format_;

    // 解码状态
    std::vector<float> output_buffer_;
    size_t output_buffer_size_;
    size_t output_buffer_pos_;
    FLAC__bool end_of_stream_;

    // 流信息
    FLAC__uint64 total_samples_;
    unsigned sample_rate_;
    unsigned channels_;
    unsigned bits_per_sample_;
    double duration_;

    // 元数据
    struct FLACMetadata {
        std::string title;
        std::string artist;
        std::string album;
        std::string date;
        std::string comment;
        std::string genre;
        unsigned track_number;
        unsigned total_tracks;
    } metadata_;

    // 错误信息
    std::string last_error_;

    // 当前位置
    FLAC__uint64 current_sample_;

public:
    FLACDecoder();
    ~FLACDecoder() override;

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

    // FLAC特定功能
    bool seek(double seconds);
    double get_duration() const;
    FLACMetadata get_metadata() const;

    // 元数据
    nlohmann::json get_metadata() const override;

private:
    // FLAC回调函数
    static FLAC__StreamDecoderWriteStatus write_callback(
        const FLAC__StreamDecoder* decoder,
        const FLAC__int32* const* buffer,
        unsigned samples,
        unsigned bytes_per_sample,
        unsigned frame_size,
        void* client_data);

    static FLAC__StreamDecoderSeekStatus seek_callback(
        const FLAC__StreamDecoder* decoder,
        FLAC__uint64 absolute_byte_offset,
        void* client_data);

    static FLAC__StreamDecoderTellStatus tell_callback(
        const FLAC__StreamDecoder* decoder,
        FLAC__uint64* absolute_byte_offset,
        void* client_data);

    static FLAC__StreamDecoderLengthStatus length_callback(
        const FLAC__StreamDecoder* decoder,
        FLAC__uint64* stream_length,
        void* client_data);

    static FLAC__bool eof_callback(
        const FLAC__StreamDecoder* decoder,
        void* client_data);

    static void metadata_callback(
        const FLAC__StreamDecoder* decoder,
        const FLAC__StreamMetadata* metadata,
        void* client_data);

    static void error_callback(
        const FLAC__StreamDecoder* decoder,
        FLAC__StreamDecoderErrorStatus status,
        void* client_data);

    // 内部方法
    bool initialize_decoder();
    void cleanup();
    void set_error(const std::string& error);
    void calculate_duration();
    void process_vorbis_comment(const FLAC__StreamMetadata* metadata);
    void process_picture(const FLAC__StreamMetadata* metadata);
};

// FLAC解码器工厂
class FLACDecoderFactory : public ITypedPluginFactory<IAudioDecoder> {
public:
    std::unique_ptr<IAudioDecoder> create_typed() override {
        return std::make_unique<FLACDecoder>();
    }

    PluginInfo get_info() const override {
        FLACDecoder decoder;
        return decoder.get_info();
    }
};

} // namespace xpumusic::plugins

// 导出插件
QODER_EXPORT_AUDIO_PLUGIN(xpumusic::plugins::FLACDecoder)

// 自动注册解码器
QODER_AUTO_REGISTER_DECODER(xpumusic::plugins::FLACDecoder, {"flac", "oga"})