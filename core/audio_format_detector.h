#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace xpumusic::core {

/**
 * @brief 闊抽鏂囦欢鏍煎紡妫€娴嬬粨鏋?
 */
struct AudioFormatInfo {
    std::string format;           // 鏍煎紡鍚嶇О锛堝 "MP3", "FLAC", "OGG"锛?
    std::string extension;         // 鏂囦欢鎵╁睍鍚?
    std::string mime_type;         // MIME绫诲瀷
    bool lossless;                 // 鏄惁鏃犳崯
    std::string codec;             // 缂栬В鐮佸櫒鍚嶇О
    std::string container;         // 瀹瑰櫒鏍煎紡
    bool supported;                // 鏄惁鏀寔瑙ｇ爜
    std::vector<std::string> possible_decoders; // 鍙兘鐨勮В鐮佸櫒鍒楄〃
};

/**
 * @brief 闊抽鏍煎紡妫€娴嬪櫒
 *
 * 鏀寔閫氳繃鏂囦欢鎵╁睍鍚嶃€佹枃浠跺ご榄旀暟銆佹枃浠跺唴瀹圭瓑澶氱鏂瑰紡妫€娴嬮煶棰戞牸寮?
 */
class AudioFormatDetector {
public:
    /**
     * @brief 鑾峰彇鍏ㄥ眬妫€娴嬪櫒瀹炰緥
     */
    static AudioFormatDetector& get_instance();

    /**
     * @brief 妫€娴嬮煶棰戞枃浠舵牸寮?
     * @param file_path 鏂囦欢璺緞
     * @return 鏍煎紡淇℃伅
     */
    AudioFormatInfo detect_format(const std::string& file_path);

    /**
     * @brief 閫氳繃鏂囦欢鎵╁睍鍚嶆娴嬫牸寮?
     * @param file_path 鏂囦欢璺緞
     * @return 鏍煎紡淇℃伅锛屽鏋滄棤娉曡瘑鍒垯杩斿洖鏈煡鏍煎紡
     */
    AudioFormatInfo detect_by_extension(const std::string& file_path);

    /**
     * @brief 閫氳繃鏂囦欢澶撮瓟鏁版娴嬫牸寮?
     * @param file_path 鏂囦欢璺緞
     * @return 鏍煎紡淇℃伅锛屽鏋滄棤娉曡瘑鍒垯杩斿洖鏈煡鏍煎紡
     */
    AudioFormatInfo detect_by_magic_number(const std::string& file_path);

    /**
     * @brief 閫氳繃鏂囦欢鍐呭妫€娴嬫牸寮忥紙鏇存繁鍏ョ殑妫€娴嬶級
     * @param file_path 鏂囦欢璺緞
     * @return 鏍煎紡淇℃伅锛屽鏋滄棤娉曡瘑鍒垯杩斿洖鏈煡鏍煎紡
     */
    AudioFormatInfo detect_by_content(const std::string& file_path);

    /**
     * @brief 鑾峰彇鎵€鏈夋敮鎸佺殑鏍煎紡
     * @return 鏀寔鐨勬牸寮忓垪琛?
     */
    std::vector<std::string> get_supported_formats() const;

    /**
     * @brief 娉ㄥ唽鑷畾涔夋牸寮忔娴嬪櫒
     * @param extension 鏂囦欢鎵╁睍鍚?
     * @param format_info 鏍煎紡淇℃伅
     * @param detector 妫€娴嬪嚱鏁帮紙鍙€夛紝鐢ㄤ簬鏇村鏉傜殑妫€娴嬶級
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