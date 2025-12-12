#pragma once

#include "../../sdk/xpumusic_plugin_sdk.h"
#include "../../sdk/headers/mp_types.h"
#include "minimp3.h"
#include <memory>
#include <vector>
#include <string>
#include <map>

// Use a simple map instead of nlohmann::json for now
using json_map = std::map<std::string, std::string>;

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
using xpumusic::ITypedPluginFactory;

/**
 * @brief MP3瑙ｇ爜鍣ㄦ彃浠?
 *
 * 浣跨敤minimp3搴撹В鐮丮P3鏂囦欢锛屾敮鎸侊細
 * - 鎭掑畾姣旂壒鐜囧拰鍙彉姣旂壒鐜?
 * - 澶氱閲囨牱鐜囧拰澹伴亾閰嶇疆
 * - ID3鏍囩璇诲彇
 * - 蹇€熷畾浣?
 */
class MP3Decoder : public IAudioDecoder {
private:
    // 瑙ｇ爜鐘舵€?
    mp3dec_t mp3d_;
    mp3dec_file_info_t mp3info_;

    // 鏂囦欢淇℃伅
    std::string file_path_;
    FILE* file_ = nullptr;
    bool is_open_ = false;
    bool metadata_loaded_ = false;

    // 闊抽鏍煎紡
    AudioFormat format_;

    // 缂撳啿鍖?
    std::vector<uint8_t> input_buffer_;
    size_t input_buffer_size_;
    size_t input_buffer_pos_;

    // ID3鏍囩
    struct ID3Tag {
        std::string title;
        std::string artist;
        std::string album;
        std::string year;
        std::string comment;
        std::string genre;
        int track = 0;
    } id3_tag_;

    // 閿欒淇℃伅
    std::string last_error_;

    // 缁熻淇℃伅
    uint64_t current_sample_ = 0;
    uint64_t total_samples_ = 0;
    double duration_ = 0.0;

public:
    MP3Decoder();
    ~MP3Decoder() override;

    // IPlugin 鎺ュ彛
    bool initialize() override;
    void finalize() override;
    PluginState get_state() const override;
    void set_state(PluginState state) override;
    PluginInfo get_info() const override;
    std::string get_last_error() const override;

    // IAudioDecoder 鎺ュ彛
    bool can_decode(const std::string& file_path) override;
    std::vector<std::string> get_supported_extensions() override;
    bool open(const std::string& file_path) override;
    int decode(AudioBuffer& buffer, int max_frames) override;
    bool seek(int64_t sample_pos) override;
    void close() override;
    AudioFormat get_format() const override;
    int64_t get_length() const override;
    double get_duration() const override;
    std::vector<MetadataItem> get_metadata() override;
    std::string get_metadata_value(const std::string& key) override;
    int64_t get_position() const override;
    bool is_eof() const override;

    // MP3鐗瑰畾鍔熻兘
    bool seek(double seconds);
    ID3Tag get_id3_tag() const;

    
private:
    // 鍐呴儴鏂规硶
    bool parse_id3v1_tag(FILE* file);
    bool parse_id3v2_tag(FILE* file);
    void calculate_duration();
    bool refill_input_buffer();
    void cleanup();
    void set_error(const std::string& error);

    // ID3鏍囩瑙ｆ瀽
    std::string parse_id3_text(const uint8_t* data, size_t size, bool unicode = false);
    int parse_id3_int(const uint8_t* data, size_t size);
};

// MP3瑙ｇ爜鍣ㄥ伐鍘?
class MP3DecoderFactory : public ITypedPluginFactory<IAudioDecoder> {
public:
    std::unique_ptr<IAudioDecoder> create_typed() override {
        return std::make_unique<MP3Decoder>();
    }

    PluginInfo get_info() const override {
        MP3Decoder decoder;
        return decoder.get_info();
    }
};

} // namespace xpumusic::plugins

