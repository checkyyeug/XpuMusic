#include "audio_format_detector.h"
#include "audio_decoder_registry.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace xpumusic::core {

AudioFormatDetector& AudioFormatDetector::get_instance() {
    static AudioFormatDetector instance;
    return instance;
}

AudioFormatDetector::AudioFormatDetector() {
    init_builtin_formats();
}

void AudioFormatDetector::init_builtin_formats() {
    // WAV format
    AudioFormatInfo wav_info;
    wav_info.format = "WAVE";
    wav_info.extension = "wav";
    wav_info.mime_type = "audio/wav";
    wav_info.lossless = true;
    wav_info.codec = "PCM";
    wav_info.container = "WAV";
    wav_info.supported = true;
    register_format_detector("wav", wav_info);
    magic_numbers_[{0x52, 0x49, 0x46, 0x46}] = "wav";  // "RIFF"

    // MP3 format
    AudioFormatInfo mp3_info;
    mp3_info.format = "MP3";
    mp3_info.extension = "mp3";
    mp3_info.mime_type = "audio/mpeg";
    mp3_info.lossless = false;
    mp3_info.codec = "MPEG-1 Audio Layer III";
    mp3_info.container = "MP3";
    mp3_info.supported = true;
    mp3_info.possible_decoders = {"MP3 Decoder"};
    register_format_detector("mp3", mp3_info);
    magic_numbers_[{0x49, 0x44, 0x33}] = "mp3";  // ID3v2 tag
    magic_numbers_[{0xFF, 0xFB}] = "mp3";        // MPEG-1 Layer 3
    magic_numbers_[{0xFF, 0xF3}] = "mp3";        // MPEG-2 Layer 3
    magic_numbers_[{0xFF, 0xF2}] = "mp3";        // MPEG-2.5 Layer 3

    // MP2 format
    AudioFormatInfo mp2_info;
    mp2_info.format = "MP2";
    mp2_info.extension = "mp2";
    mp2_info.mime_type = "audio/mpeg";
    mp2_info.lossless = false;
    mp2_info.codec = "MPEG-1 Audio Layer II";
    mp2_info.container = "MP2";
    mp2_info.supported = true;
    mp2_info.possible_decoders = {"MP3 Decoder"};
    register_format_detector("mp2", mp2_info);

    // MP1 format
    AudioFormatInfo mp1_info;
    mp1_info.format = "MP1";
    mp1_info.extension = "mp1";
    mp1_info.mime_type = "audio/mpeg";
    mp1_info.lossless = false;
    mp1_info.codec = "MPEG-1 Audio Layer I";
    mp1_info.container = "MP1";
    mp1_info.supported = true;
    mp1_info.possible_decoders = {"MP3 Decoder"};
    register_format_detector("mp1", mp1_info);

    // FLAC format
    AudioFormatInfo flac_info;
    flac_info.format = "FLAC";
    flac_info.extension = "flac";
    flac_info.mime_type = "audio/flac";
    flac_info.lossless = true;
    flac_info.codec = "FLAC";
    flac_info.container = "FLAC";
    flac_info.supported = true;
    flac_info.possible_decoders = {"FLAC Decoder"};
    register_format_detector("flac", flac_info);
    magic_numbers_[{0x66, 0x4C, 0x61, 0x43}] = "flac";  // "fLaC"

    // OGG Vorbis format
    AudioFormatInfo ogg_info;
    ogg_info.format = "OGG Vorbis";
    ogg_info.extension = "ogg";
    ogg_info.mime_type = "audio/ogg";
    ogg_info.lossless = false;
    ogg_info.codec = "Vorbis";
    ogg_info.container = "OGG";
    ogg_info.supported = true;
    ogg_info.possible_decoders = {"OGG/Vorbis Decoder"};
    register_format_detector("ogg", ogg_info);
    magic_numbers_[{0x4F, 0x67, 0x67, 0x53}] = "ogg";  // "OggS"

    // OGA format (OGG Audio)
    AudioFormatInfo oga_info;
    oga_info.format = "OGG Audio";
    oga_info.extension = "oga";
    oga_info.mime_type = "audio/ogg";
    oga_info.lossless = false;
    oga_info.codec = "Vorbis/FLAC";
    oga_info.container = "OGG";
    oga_info.supported = true;
    oga_info.possible_decoders = {"OGG/Vorbis Decoder", "FLAC Decoder"};
    register_format_detector("oga", oga_info);

    // M4A/AAC format
    AudioFormatInfo m4a_info;
    m4a_info.format = "MPEG-4 Audio";
    m4a_info.extension = "m4a";
    m4a_info.mime_type = "audio/mp4";
    m4a_info.lossless = false;
    m4a_info.codec = "AAC";
    m4a_info.container = "MPEG-4";
    m4a_info.supported = false;
    register_format_detector("m4a", m4a_info);

    // AAC format
    AudioFormatInfo aac_info;
    aac_info.format = "Advanced Audio Coding";
    aac_info.extension = "aac";
    aac_info.mime_type = "audio/aac";
    aac_info.lossless = false;
    aac_info.codec = "AAC";
    aac_info.container = "AAC";
    aac_info.supported = false;
    register_format_detector("aac", aac_info);

    // WMA format
    AudioFormatInfo wma_info;
    wma_info.format = "Windows Media Audio";
    wma_info.extension = "wma";
    wma_info.mime_type = "audio/x-ms-wma";
    wma_info.lossless = false;
    wma_info.codec = "Windows Media Audio";
    wma_info.container = "ASF";
    wma_info.supported = false;
    register_format_detector("wma", wma_info);

    // APE format
    AudioFormatInfo ape_info;
    ape_info.format = "Monkey's Audio";
    ape_info.extension = "ape";
    ape_info.mime_type = "audio/x-ape";
    ape_info.lossless = true;
    ape_info.codec = "Monkey's Audio";
    ape_info.container = "APE";
    ape_info.supported = false;
    register_format_detector("ape", ape_info);

    // DSD format (DSDIFF)
    AudioFormatInfo dsd_info;
    dsd_info.format = "DSDIFF";
    dsd_info.extension = "dsf";
    dsd_info.mime_type = "audio/x-dsd";
    dsd_info.lossless = true;
    dsd_info.codec = "DSD";
    dsd_info.container = "DSDIFF";
    dsd_info.supported = false;
    register_format_detector("dsf", dsd_info);
    register_format_detector("dsd", dsd_info);

    // AIFF format
    AudioFormatInfo aiff_info;
    aiff_info.format = "Audio Interchange File Format";
    aiff_info.extension = "aiff";
    aiff_info.mime_type = "audio/aiff";
    aiff_info.lossless = true;
    aiff_info.codec = "PCM";
    aiff_info.container = "AIFF";
    aiff_info.supported = false;
    register_format_detector("aiff", aiff_info);
    register_format_detector("aif", aiff_info);
    magic_numbers_[{0x46, 0x4F, 0x52, 0x4D}] = "aiff";  // "FORM"

    // WavPack format
    AudioFormatInfo wv_info;
    wv_info.format = "WavPack";
    wv_info.extension = "wv";
    wv_info.mime_type = "audio/x-wavpack";
    wv_info.lossless = true;
    wv_info.codec = "WavPack";
    wv_info.container = "WV";
    wv_info.supported = false;
    register_format_detector("wv", wv_info);
}

AudioFormatInfo AudioFormatDetector::detect_format(const std::string& file_path) {
    // 首先尝试通过文件头魔数检测（最准确）
    AudioFormatInfo info = detect_by_magic_number(file_path);
    if (!info.format.empty() && info.format != "Unknown") {
        // 检查解码器注册表以获取支持信息
        auto& registry = AudioDecoderRegistry::get_instance();
        info.supported = registry.supports_format(file_path);
        if (info.supported) {
            info.possible_decoders = registry.get_decoders_for_format(info.extension);
        }
        return info;
    }

    // 如果魔数检测失败，尝试通过扩展名检测
    info = detect_by_extension(file_path);
    auto& registry = AudioDecoderRegistry::get_instance();
    info.supported = registry.supports_format(file_path);
    if (info.supported) {
        info.possible_decoders = registry.get_decoders_for_format(info.extension);
    }

    return info;
}

AudioFormatInfo AudioFormatDetector::detect_by_extension(const std::string& file_path) {
    std::string ext = extract_extension(file_path);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    auto it = formats_.find(ext);
    if (it != formats_.end()) {
        AudioFormatInfo info = it->second.info;
        // 如果有自定义检测器，使用它来获取更详细的信息
        if (it->second.custom_detector) {
            try {
                info = it->second.custom_detector(file_path);
            } catch (...) {
                // 如果自定义检测器失败，使用基本信息
            }
        }
        return info;
    }

    // 未知格式
    AudioFormatInfo unknown;
    unknown.format = "Unknown";
    unknown.extension = ext;
    unknown.lossless = false;
    unknown.supported = false;
    return unknown;
}

AudioFormatInfo AudioFormatDetector::detect_by_magic_number(const std::string& file_path) {
    std::vector<uint8_t> header = read_file_header(file_path, 16);
    if (header.empty()) {
        return AudioFormatInfo();
    }

    // 检查已知的魔数
    for (const auto& [magic, ext] : magic_numbers_) {
        if (header.size() >= magic.size() &&
            std::equal(magic.begin(), magic.end(), header.begin())) {
            AudioFormatInfo info = detect_by_extension(file_path);
            if (info.format != "Unknown") {
                // 对于OGG格式，需要进一步检测内容以确定编解码器
                if (ext == "ogg") {
                    return detect_by_content(file_path);
                }
                return info;
            }
        }
    }

    // 对于MP3，需要检测更多的魔数模式
    if (header.size() >= 3) {
        // ID3v2 tag
        if (header[0] == 0x49 && header[1] == 0x44 && header[2] == 0x33) {
            return detect_by_extension(file_path);
        }
        // MPEG sync word
        if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0) {
            return detect_by_extension(file_path);
        }
    }

    // 对于WAV，检查RIFF头和WAVE标识
    if (header.size() >= 12) {
        if (header[0] == 0x52 && header[1] == 0x49 && header[2] == 0x46 && header[3] == 0x46 &&
            header[8] == 0x57 && header[9] == 0x41 && header[10] == 0x56 && header[11] == 0x45) {
            return detect_by_extension(file_path);
        }
    }

    return AudioFormatInfo();
}

AudioFormatInfo AudioFormatDetector::detect_by_content(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return AudioFormatInfo();
    }

    // 读取更多内容用于深度检测
    std::vector<uint8_t> buffer(1024);
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    size_t bytes_read = file.gcount();
    buffer.resize(bytes_read);

    // 对于OGG容器，检测内容中的编解码器
    if (bytes_read >= 4 && buffer[0] == 0x4F && buffer[1] == 0x67 && buffer[2] == 0x67 && buffer[3] == 0x53) {
        // OggS header
        size_t pos = 28; // Skip OggS header
        if (pos + 7 < bytes_read) {
            // Check for Vorbis header
            if (buffer[pos] == 0x01 &&
                std::string(reinterpret_cast<char*>(&buffer[pos+1]), 6) == "vorbis") {
                AudioFormatInfo info = detect_by_extension(file_path);
                if (info.extension == "oga") {
                    info.format = "OGG Vorbis";
                    info.codec = "Vorbis";
                    info.possible_decoders = {"OGG/Vorbis Decoder"};
                }
                return info;
            }
            // Check for FLAC header
            else if (buffer[pos] == 0x7F &&
                     std::string(reinterpret_cast<char*>(&buffer[pos+1]), 4) == "FLAC") {
                AudioFormatInfo info = detect_by_extension(file_path);
                if (info.extension == "oga") {
                    info.format = "OGG FLAC";
                    info.codec = "FLAC";
                    info.lossless = true;
                    info.possible_decoders = {"FLAC Decoder"};
                }
                return info;
            }
        }
    }

    // 对于MP3，检测具体的Layer
    if (bytes_read >= 4) {
        // Look for MPEG frame header
        for (size_t i = 0; i < bytes_read - 3; ++i) {
            if (buffer[i] == 0xFF && (buffer[i+1] & 0xE0) == 0xE0) {
                uint16_t header = (buffer[i] << 8) | buffer[i+1];
                int layer = (header >> 1) & 0x03;

                AudioFormatInfo info = detect_by_extension(file_path);
                if (layer == 3) info.format = "MP3 Layer I";
                else if (layer == 2) info.format = "MP3 Layer II";
                else if (layer == 1) info.format = "MP3 Layer III";

                return info;
            }
        }
    }

    // 默认返回扩展名检测结果
    return detect_by_extension(file_path);
}

std::vector<std::string> AudioFormatDetector::get_supported_formats() const {
    std::vector<std::string> formats;
    auto& registry = AudioDecoderRegistry::get_instance();

    for (const auto& [ext, info] : formats_) {
        if (registry.supports_format("test." + ext)) {
            formats.push_back(ext);
        }
    }

    return formats;
}

void AudioFormatDetector::register_format_detector(const std::string& extension,
                                                   const AudioFormatInfo& format_info,
                                                   std::function<AudioFormatInfo(const std::string&)> detector) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    FormatDetector fd;
    fd.info = format_info;
    fd.custom_detector = std::move(detector);
    formats_[ext] = std::move(fd);
}

std::string AudioFormatDetector::extract_extension(const std::string& file_path) const {
    std::filesystem::path path(file_path);
    std::string ext = path.extension().string();

    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }

    return ext;
}

std::vector<uint8_t> AudioFormatDetector::read_file_header(const std::string& file_path, size_t bytes) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    std::vector<uint8_t> header(bytes);
    file.read(reinterpret_cast<char*>(header.data()), bytes);
    header.resize(file.gcount());

    return header;
}

std::string AudioFormatDetector::bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

} // namespace xpumusic::core