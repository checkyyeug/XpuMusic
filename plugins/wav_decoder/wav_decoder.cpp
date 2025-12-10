/**
 * @file wav_decoder.cpp
 * @brief XpuMusic WAV解码器插件
 * @date 2025-12-10
 */

#include "../../sdk/xpumusic_plugin_sdk.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <endian.h>

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IAudioDecoder;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;

// WAV文件头结构
#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits;
};
#pragma pack(pop)

class WAVDecoderPlugin : public IAudioDecoder {
private:
    std::ifstream file_;
    WAVHeader header_;
    AudioFormat format_;
    int64_t data_start_;
    int64_t data_size_;
    int64_t current_position_;
    bool is_open_;

public:
    WAVDecoderPlugin() : data_start_(0), data_size_(0), current_position_(0), is_open_(false) {
        memset(&header_, 0, sizeof(header_));
    }

    bool initialize() override {
        set_state(PluginState::Initialized);
        return true;
    }

    void finalize() override {
        close();
        set_state(PluginState::Uninitialized);
    }

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "XpuMusic WAV Decoder";
        info.version = "1.0.0";
        info.author = "XpuMusic Team";
        info.description = "WAV audio format decoder plugin";
        info.type = PluginType::AudioDecoder;
        info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
        info.supported_formats = {"wav", "wave"};
        return info;
    }

    PluginState get_state() const override {
        return state_;
    }

    void set_state(PluginState state) override {
        state_ = state;
    }

    std::string get_last_error() const override {
        return last_error_;
    }

    bool can_decode(const std::string& file_path) override {
        // 检查扩展名
        size_t pos = file_path.find_last_of('.');
        if (pos == std::string::npos) return false;

        std::string ext = file_path.substr(pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        return ext == "wav" || ext == "wave";
    }

    std::vector<std::string> get_supported_extensions() override {
        return {"wav", "wave"};
    }

    bool open(const std::string& file_path) override {
        if (is_open_) {
            close();
        }

        file_.open(file_path, std::ios::binary);
        if (!file_.is_open()) {
            last_error_ = "Failed to open file: " + file_path;
            set_state(PluginState::Error);
            return false;
        }

        // 读取WAV头
        file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        if (file_.gcount() != sizeof(header_)) {
            last_error_ = "Invalid WAV file: cannot read header";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        // 验证WAV格式
        if (strncmp(header_.riff, "RIFF", 4) != 0 ||
            strncmp(header_.wave, "WAVE", 4) != 0) {
            last_error_ = "Invalid WAV file: wrong signature";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        // 检查格式是否为PCM
        if (le16toh(header_.format) != 1) {
            last_error_ = "Only PCM WAV format is supported";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        // 设置音频格式
        format_.sample_rate = le32toh(header_.sample_rate);
        format_.channels = le16toh(header_.channels);
        format_.bits_per_sample = le16toh(header_.bits);
        format_.is_float = false;

        if (format_.bits_per_sample != 8 &&
            format_.bits_per_sample != 16 &&
            format_.bits_per_sample != 24 &&
            format_.bits_per_sample != 32) {
            last_error_ = "Unsupported bits per sample: " +
                         std::to_string(format_.bits_per_sample);
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        // 查找data块
        if (!find_data_chunk()) {
            last_error_ = "Data chunk not found";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        current_position_ = 0;
        is_open_ = true;
        set_state(PluginState::Active);
        return true;
    }

    int decode(AudioBuffer& buffer, int max_frames) override {
        if (!is_open_ || !buffer.data) {
            last_error_ = "Decoder not open or invalid buffer";
            return -1;
        }

        // 计算可读取的帧数
        int64_t remaining_frames = (data_size_ / (format_.bits_per_sample / 8) / format_.channels) - current_position_;
        int frames_to_read = std::min(static_cast<int64_t>(max_frames), remaining_frames);

        if (frames_to_read <= 0) {
            return 0; // EOF
        }

        // 读取原始数据
        int bytes_to_read = frames_to_read * format_.channels * (format_.bits_per_sample / 8);
        std::vector<char> raw_data(bytes_to_read);
        file_.read(raw_data.data(), bytes_to_read);
        int bytes_read = file_.gcount();

        if (bytes_read == 0) {
            return 0; // EOF
        }

        int frames_read = bytes_read / (format_.channels * (format_.bits_per_sample / 8));

        // 转换为浮点数
        convert_to_float(raw_data.data(), frames_read, buffer.data);

        current_position_ += frames_read;
        return frames_read;
    }

    bool seek(int64_t sample_pos) override {
        if (!is_open_) {
            last_error_ = "Decoder not open";
            return false;
        }

        int64_t total_frames = data_size_ / (format_.bits_per_sample / 8) / format_.channels;
        if (sample_pos < 0 || sample_pos > total_frames) {
            last_error_ = "Invalid seek position";
            return false;
        }

        int64_t byte_pos = data_start_ +
                          sample_pos * format_.channels * (format_.bits_per_sample / 8);

        file_.seekg(byte_pos, std::ios::beg);
        if (file_.fail()) {
            last_error_ = "Failed to seek";
            return false;
        }

        current_position_ = sample_pos;
        return true;
    }

    void close() override {
        if (is_open_) {
            file_.close();
            is_open_ = false;
        }
        memset(&header_, 0, sizeof(header_));
        format_ = {};
        data_start_ = 0;
        data_size_ = 0;
        current_position_ = 0;
        set_state(PluginState::Initialized);
    }

    AudioFormat get_format() const override {
        return format_;
    }

    int64_t get_length() const override {
        if (format_.bits_per_sample == 0 || format_.channels == 0) return 0;
        return data_size_ / (format_.bits_per_sample / 8) / format_.channels;
    }

    double get_duration() const override {
        int64_t length = get_length();
        if (format_.sample_rate == 0) return 0.0;
        return static_cast<double>(length) / format_.sample_rate;
    }

    std::vector<MetadataItem> get_metadata() override {
        std::vector<MetadataItem> metadata;
        metadata.emplace_back("codec", "PCM");
        metadata.emplace_back("bits_per_sample", std::to_string(format_.bits_per_sample));
        metadata.emplace_back("sample_rate", std::to_string(format_.sample_rate));
        metadata.emplace_back("channels", std::to_string(format_.channels));
        return metadata;
    }

    std::string get_metadata_value(const std::string& key) override {
        if (key == "codec") return "PCM";
        if (key == "bits_per_sample") return std::to_string(format_.bits_per_sample);
        if (key == "sample_rate") return std::to_string(format_.sample_rate);
        if (key == "channels") return std::to_string(format_.channels);
        return "";
    }

    int64_t get_position() const override {
        return current_position_;
    }

    bool is_eof() const override {
        if (!is_open_) return true;
        return current_position_ >= get_length();
    }

private:
    bool find_data_chunk() {
        // 从当前位置开始查找data块
        file_.seekg(sizeof(WAVHeader), std::ios::beg);

        char chunk_id[5] = {0};
        uint32_t chunk_size;

        while (file_.read(chunk_id, 4)) {
            if (strncmp(chunk_id, "data", 4) == 0) {
                file_.read(reinterpret_cast<char*>(&chunk_size), 4);
                data_size_ = le32toh(chunk_size);
                data_start_ = file_.tellg();
                return true;
            }

            // 跳过其他块
            file_.read(reinterpret_cast<char*>(&chunk_size), 4);
            chunk_size = le32toh(chunk_size);
            file_.seekg(chunk_size, std::ios::cur);
        }

        return false;
    }

    void convert_to_float(const char* src, int frames, float* dst) {
        int samples = frames * format_.channels;
        int bytes_per_sample = format_.bits_per_sample / 8;

        for (int i = 0; i < samples; i++) {
            int32_t sample = 0;

            switch (bytes_per_sample) {
                case 1: // 8位
                    sample = static_cast<int8_t>(src[i]) - 128;
                    sample = sample << 24; // 转换为32位
                    break;

                case 2: // 16位
                    sample = static_cast<int16_t>(
                        (static_cast<uint8_t>(src[i*2+1]) << 8) |
                        static_cast<uint8_t>(src[i*2])
                    );
                    sample = sample << 16;
                    break;

                case 3: // 24位
                    sample = (static_cast<int8_t>(src[i*3+2]) << 24) |
                            (static_cast<uint8_t>(src[i*3+1]) << 16) |
                            (static_cast<uint8_t>(src[i*3]) << 8);
                    break;

                case 4: // 32位
                    sample = (static_cast<int8_t>(src[i*4+3]) << 24) |
                            (static_cast<uint8_t>(src[i*4+2]) << 16) |
                            (static_cast<uint8_t>(src[i*4+1]) << 8) |
                            static_cast<uint8_t>(src[i*4]);
                    break;
            }

            // 转换为浮点数 [-1.0, 1.0]
            dst[i] = static_cast<float>(sample) / 2147483648.0f;
        }
    }
};

// 插件工厂
class WAVDecoderFactory : public ITypedPluginFactory<IAudioDecoder> {
public:
    std::unique_ptr<IAudioDecoder> create_typed() override {
        return std::make_unique<WAVDecoderPlugin>();
    }

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "WAV Decoder Factory";
        info.version = "1.0.0";
        info.author = "XpuMusic Team";
        info.description = "Factory for WAV audio decoder plugin";
        info.type = PluginType::AudioDecoder;
        info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
        return info;
    }
};

// 导出插件
XPUMUSIC_EXPORT_AUDIO_PLUGIN(WAVDecoderPlugin)