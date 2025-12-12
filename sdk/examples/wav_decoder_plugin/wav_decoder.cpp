/**
 * @file wav_decoder.cpp
 * @brief XpuMusic WAV瑙ｇ爜鍣ㄦ彃浠剁ず渚?
 * @date 2025-12-10
 */

#include "../../xpumusic_plugin_sdk.h"
#include <fstream>
#include <cstring>
#include <algorithm>

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IAudioDecoder;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;

// WAV鏂囦欢澶?
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
    char data[4];
    uint32_t data_size;
};
#pragma pack(pop)

class WAVDecoderPlugin : public IAudioDecoder {
private:
    std::ifstream file_;
    WAVHeader header_;
    AudioFormat format_;
    int64_t current_position_;
    int64_t total_samples_;
    bool is_open_;

public:
    WAVDecoderPlugin() : current_position_(0), total_samples_(0), is_open_(false) {
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
        info.name = "WAV Decoder";
        info.version = "1.0.0";
        info.author = "XpuMusic Team";
        info.description = "WAV audio format decoder";
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
        // 妫€鏌ユ墿灞曞悕
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

        // 璇诲彇WAV澶?
        file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        if (file_.gcount() != sizeof(header_)) {
            last_error_ = "Invalid WAV file: cannot read header";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        // 楠岃瘉WAV鏍煎紡
        if (strncmp(header_.riff, "RIFF", 4) != 0 ||
            strncmp(header_.wave, "WAVE", 4) != 0) {
            last_error_ = "Invalid WAV file: wrong signature";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        // 璁剧疆闊抽鏍煎紡
        format_.sample_rate = header_.sample_rate;
        format_.channels = header_.channels;
        format_.bits_per_sample = header_.bits;
        format_.is_float = false;

        // 璁＄畻鎬绘牱鏈暟
        if (format_.bits_per_sample == 0) {
            last_error_ = "Invalid bits per sample";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        total_samples_ = header_.data_size / (format_.bits_per_sample / 8) / format_.channels;
        current_position_ = 0;

        // 瀹氫綅鍒版暟鎹紑濮?
        file_.seekg(sizeof(header_), std::ios::beg);
        if (file_.fail()) {
            last_error_ = "Failed to seek to data position";
            file_.close();
            set_state(PluginState::Error);
            return false;
        }

        is_open_ = true;
        set_state(PluginState::Active);
        return true;
    }

    int decode(AudioBuffer& buffer, int max_frames) override {
        if (!is_open_ || !buffer.data) {
            last_error_ = "Decoder not open or invalid buffer";
            return -1;
        }

        // 璁＄畻鍙鍙栫殑甯ф暟
        int64_t remaining_frames = total_samples_ - current_position_;
        int frames_to_read = std::min(static_cast<int64_t>(max_frames), remaining_frames);

        if (frames_to_read <= 0) {
            return 0; // EOF
        }

        // 璇诲彇鍘熷鏁版嵁
        std::vector<char> raw_data(frames_to_read * format_.channels * (format_.bits_per_sample / 8));
        file_.read(raw_data.data(), raw_data.size());
        int bytes_read = file_.gcount();

        if (bytes_read == 0) {
            return 0; // EOF
        }

        int frames_read = bytes_read / (format_.channels * (format_.bits_per_sample / 8));

        // 杞崲涓烘诞鐐规暟
        convert_to_float(raw_data.data(), frames_read, buffer.data);

        current_position_ += frames_read;
        return frames_read;
    }

    bool seek(int64_t sample_pos) override {
        if (!is_open_) {
            last_error_ = "Decoder not open";
            return false;
        }

        if (sample_pos < 0 || sample_pos > total_samples_) {
            last_error_ = "Invalid seek position";
            return false;
        }

        int64_t byte_pos = sizeof(header_) +
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
        current_position_ = 0;
        total_samples_ = 0;
        set_state(PluginState::Initialized);
    }

    AudioFormat get_format() const override {
        return format_;
    }

    int64_t get_length() const override {
        return total_samples_;
    }

    double get_duration() const override {
        if (format_.sample_rate == 0) return 0.0;
        return static_cast<double>(total_samples_) / format_.sample_rate;
    }

    std::vector<MetadataItem> get_metadata() override {
        std::vector<MetadataItem> metadata;
        // WAV鏂囦欢閫氬父涓嶅寘鍚厓鏁版嵁锛屼絾鍙互娣诲姞涓€浜涘熀鏈俊鎭?
        metadata.emplace_back("codec", "PCM");
        metadata.emplace_back("bits_per_sample", std::to_string(format_.bits_per_sample));
        return metadata;
    }

    std::string get_metadata_value(const std::string& key) override {
        if (key == "codec") return "PCM";
        if (key == "bits_per_sample") return std::to_string(format_.bits_per_sample);
        return "";
    }

    int64_t get_position() const override {
        return current_position_;
    }

    bool is_eof() const override {
        return current_position_ >= total_samples_;
    }

private:
    void convert_to_float(const char* src, int frames, float* dst) {
        int samples = frames * format_.channels;

        if (format_.bits_per_sample == 16) {
            const int16_t* samples16 = reinterpret_cast<const int16_t*>(src);
            for (int i = 0; i < samples; i++) {
                dst[i] = static_cast<float>(samples16[i]) / 32768.0f;
            }
        }
        else if (format_.bits_per_sample == 24) {
            // 24浣嶆暣鏁?
            for (int i = 0; i < samples; i++) {
                int32_t value = (static_cast<uint8_t>(src[2]) << 16) |
                               (static_cast<uint8_t>(src[1]) << 8) |
                                static_cast<uint8_t>(src[0]);
                // 绗﹀彿鎵╁睍
                if (value & 0x800000) {
                    value |= 0xFF000000;
                }
                dst[i] = static_cast<float>(value) / 8388608.0f;
                src += 3;
            }
        }
        else if (format_.bits_per_sample == 32) {
            const int32_t* samples32 = reinterpret_cast<const int32_t*>(src);
            for (int i = 0; i < samples; i++) {
                dst[i] = static_cast<float>(samples32[i]) / 2147483648.0f;
            }
        }
    }
};

// 鎻掍欢宸ュ巶
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
        info.description = "Factory for WAV decoder plugin";
        info.type = PluginType::AudioDecoder;
        info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
        return info;
    }
};

// 瀵煎嚭鎻掍欢
XPUMUSIC_EXPORT_AUDIO_PLUGIN(WAVDecoderPlugin)