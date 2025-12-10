#pragma once

#include "audio_decoder_registry.h"
#include "audio_format_detector.h"
#include "../sdk/xpumusic_plugin_sdk.h"
#include <memory>
#include <string>
#include <map>
#include <vector>

// Use simple map instead of nlohmann/json
using json_map = std::map<std::string, std::string>;

namespace xpumusic::core {

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IAudioDecoder;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;
using xpumusic::IPluginFactory;
using xpumusic::ITypedPluginFactory;

/**
 * @brief 音频解码器管理器
 *
 * 统一管理音频解码器和格式检测的高级接口
 * 提供简化的API用于音频文件的解码
 */
class AudioDecoderManager {
public:
    /**
     * @brief 获取全局管理器实例
     */
    static AudioDecoderManager& get_instance();

    /**
     * @brief 初始化管理器
     * 加载所有内置解码器
     */
    void initialize();

    /**
     * @brief 获取音频文件的解码器
     * @param file_path 文件路径
     * @return 解码器实例，如果没有找到合适的解码器则返回nullptr
     */
    std::unique_ptr<IAudioDecoder> get_decoder_for_file(const std::string& file_path);

    /**
     * @brief 检测音频文件格式
     * @param file_path 文件路径
     * @return 格式信息
     */
    AudioFormatInfo detect_format(const std::string& file_path);

    /**
     * @brief 打开音频文件并返回解码器
     * @param file_path 文件路径
     * @return 已打开的解码器实例，如果失败则返回nullptr
     */
    std::unique_ptr<IAudioDecoder> open_audio_file(const std::string& file_path);

    /**
     * @brief 检查是否支持某个文件
     * @param file_path 文件路径
     * @return 是否支持
     */
    bool supports_file(const std::string& file_path);

    /**
     * @brief 获取所有支持的格式
     * @return 支持的格式列表
     */
    std::vector<std::string> get_supported_formats();

    /**
     * @brief 获取所有注册的解码器
     * @return 解码器信息列表
     */
    std::vector<PluginInfo> get_available_decoders();

    /**
     * @brief 设置格式的默认解码器
     * @param format 文件格式
     * @param decoder_name 解码器名称
     */
    void set_default_decoder(const std::string& format, const std::string& decoder_name);

    /**
     * @brief 获取解码器的元数据
     * @param file_path 文件路径
     * @return 元数据映射，如果失败则返回空映射
     */
    json_map get_metadata(const std::string& file_path);

    /**
     * @brief 获取解码器的持续时长
     * @param file_path 文件路径
     * @return 持续时长（秒），如果失败返回-1.0
     */
    double get_duration(const std::string& file_path);

    /**
     * @brief 启用/禁用解码器
     * @param decoder_name 解码器名称
     * @param enabled 是否启用
     */
    void set_decoder_enabled(const std::string& decoder_name, bool enabled);

    /**
     * @brief 注册自定义解码器工厂
     * @param name 解码器名称
     * @param factory 工厂函数
     */
    void register_decoder_factory(const std::string& name,
                                 std::function<std::unique_ptr<IAudioDecoder>()> factory);

private:
    AudioDecoderManager() = default;
    ~AudioDecoderManager() = default;
    AudioDecoderManager(const AudioDecoderManager&) = delete;
    AudioDecoderManager& operator=(const AudioDecoderManager&) = delete;

    bool initialized_ = false;
};

} // namespace xpumusic::core