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
 * @brief FLAC瑙ｇ爜鍣ㄦ彃浠?
 *
 * 浣跨敤libFLAC搴撹В鐮丗LAC鏂囦欢锛屾敮鎸侊細
 * - 鏃犳崯闊抽瑙ｇ爜
 * - 澶氱閲囨牱鐜囧拰浣嶆繁搴?
 * - 瀹屾暣鐨勫厓鏁版嵁鏀寔
 * - 蹇€熷畾浣?
 * - 娴佸紡瑙ｇ爜
 */
class FLACDecoder : public IAudioDecoder {
private:
    // FLAC瑙ｇ爜鍣?
    FLAC__StreamDecoder* decoder_;

    // 鏂囦欢淇℃伅
    std::string file_path_;
    FLAC__bool is_open_;

    // 闊抽鏍煎紡
    AudioFormat format_;

    // 瑙ｇ爜鐘舵€?
    std::vector<float> output_buffer_;
    size_t output_buffer_size_;
    size_t output_buffer_pos_;
    FLAC__bool end_of_stream_;

    // 娴佷俊鎭?
    FLAC__uint64 total_samples_;
    unsigned sample_rate_;
    unsigned channels_;
    unsigned bits_per_sample_;
    double duration_;

    // 鍏冩暟鎹?
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

    // 閿欒淇℃伅
    std::string last_error_;

    // 褰撳墠浣嶇疆
    FLAC__uint64 current_sample_;

public:
    FLACDecoder();
    ~FLACDecoder() override;

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

    // FLAC鐗瑰畾鍔熻兘
    bool seek(double seconds);
    double get_duration() const;
    FLACMetadata get_metadata() const;

    // 鍏冩暟鎹?
    nlohmann::json get_metadata() const override;

private:
    // FLAC鍥炶皟鍑芥暟
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

    // 鍐呴儴鏂规硶
    bool initialize_decoder();
    void cleanup();
    void set_error(const std::string& error);
    void calculate_duration();
    void process_vorbis_comment(const FLAC__StreamMetadata* metadata);
    void process_picture(const FLAC__StreamMetadata* metadata);
};

// FLAC瑙ｇ爜鍣ㄥ伐鍘?
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

// 瀵煎嚭鎻掍欢
QODER_EXPORT_AUDIO_PLUGIN(xpumusic::plugins::FLACDecoder)

// 鑷姩娉ㄥ唽瑙ｇ爜鍣?
QODER_AUTO_REGISTER_DECODER(xpumusic::plugins::FLACDecoder, {"flac", "oga"})