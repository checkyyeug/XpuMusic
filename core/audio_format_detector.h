#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace xpumusic::core {

/**
 * @brief 音频文件格式检测结果
 */
struct AudioFormatInfo {
    std::string format;           // 格式名称（如 "MP3", "FLAC", "OGG"）
    std::string extension;         // 文件扩展名
    std::string mime_type;         // MIME类型
    bool lossless;                 // 是否无损
    std::string codec;             // 编解码器名称
    std::string container;         // 容器格式
    bool supported;                // 是否支持解码
    std::vector<std::string> possible_decoders; // 可能的解码器列表
};

/**
 * @brief 音频格式检测器
 *
 * 支持通过文件扩展名、文件头魔数、文件内容等多种方式检测音频格式
 */
class AudioFormatDetector {
public:
    /**
     * @brief 获取全局检测器实例
     */
    static AudioFormatDetector& get_instance();

    /**
     * @brief 检测音频文件格式
     * @param file_path 文件路径
     * @return 格式信息
     */
    AudioFormatInfo detect_format(const std::string& file_path);

    /**
     * @brief 通过文件扩展名检测格式
     * @param file_path 文件路径
     * @return 格式信息，如果无法识别则返回未知格式
     */
    AudioFormatInfo detect_by_extension(const std::string& file_path);

    /**
     * @brief 通过文件头魔数检测格式
     * @param file_path 文件路径
     * @return 格式信息，如果无法识别则返回未知格式
     */
    AudioFormatInfo detect_by_magic_number(const std::string& file_path);

    /**
     * @brief 通过文件内容检测格式（更深入的检测）
     * @param file_path 文件路径
     * @return 格式信息，如果无法识别则返回未知格式
     */
    AudioFormatInfo detect_by_content(const std::string& file_path);

    /**
     * @brief 获取所有支持的格式
     * @return 支持的格式列表
     */
    std::vector<std::string> get_supported_formats() const;

    /**
     * @brief 注册自定义格式检测器
     * @param extension 文件扩展名
     * @param format_info 格式信息
     * @param detector 检测函数（可选，用于更复杂的检测）
     */
    void register_format_detector(const std::string& extension,
                                  const AudioFormatInfo& format_info,
                                  std::function<AudioFormatInfo(const std::string&)> detector = nullptr);

private:
    AudioFormatDetector();
    ~AudioFormatDetector() = default;
    AudioFormatDetector(const AudioFormatDetector&) = delete;
    AudioFormatDetector& operator=(const AudioFormatDetector&) = delete;

    void init_builtin_formats();

    struct FormatDetector {
        AudioFormatInfo info;
        std::function<AudioFormatInfo(const std::string&)> custom_detector;
    };

    std::map<std::string, FormatDetector> formats_;  // key: extension (lowercase)
    std::map<std::vector<uint8_t>, std::string> magic_numbers_;  // key: magic bytes, value: extension

    std::string extract_extension(const std::string& file_path) const;
    std::vector<uint8_t> read_file_header(const std::string& file_path, size_t bytes = 16);
    std::string bytes_to_hex(const std::vector<uint8_t>& bytes);
};

} // namespace xpumusic::core