#pragma once

#include "../sdk/xpumusic_plugin_sdk.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

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
 * @brief 音频解码器注册表
 *
 * 负责管理所有音频解码器的注册和获取
 * 支持动态注册解码器，并根据文件扩展名自动选择合适的解码器
 */
class AudioDecoderRegistry {
public:
    using DecoderFactory = std::function<std::unique_ptr<IAudioDecoder>()>;

    /**
     * @brief 获取全局注册表实例
     */
    static AudioDecoderRegistry& get_instance();

    /**
     * @brief 注册解码器
     * @param name 解码器名称
     * @param supported_formats 支持的文件格式列表
     * @param factory 解码器工厂函数
     */
    void register_decoder(const std::string& name,
                         const std::vector<std::string>& supported_formats,
                         DecoderFactory factory);

    /**
     * @brief 注册解码器（使用插件工厂）
     * @param name 解码器名称
     * @param factory 插件工厂实例
     */
    void register_decoder(const std::string& name,
                         std::unique_ptr<ITypedPluginFactory<IAudioDecoder>> factory);

    /**
     * @brief 根据文件路径获取合适的解码器
     * @param file_path 文件路径
     * @return 解码器实例，如果没有找到合适的解码器则返回nullptr
     */
    std::unique_ptr<IAudioDecoder> get_decoder(const std::string& file_path);

    /**
     * @brief 根据名称获取解码器
     * @param decoder_name 解码器名称
     * @return 解码器实例，如果没有找到则返回nullptr
     */
    std::unique_ptr<IAudioDecoder> get_decoder_by_name(const std::string& decoder_name);

    /**
     * @brief 获取所有注册的解码器信息
     */
    std::vector<PluginInfo> get_registered_decoders() const;

    /**
     * @brief 获取支持指定格式的解码器列表
     * @param format 文件格式
     * @return 支持该格式的解码器名称列表
     */
    std::vector<std::string> get_decoders_for_format(const std::string& format);

    /**
     * @brief 检查是否支持某个文件格式
     * @param file_path 文件路径
     * @return 是否支持
     */
    bool supports_format(const std::string& file_path);

    /**
     * @brief 设置默认解码器（用于某种格式）
     * @param format 文件格式
     * @param decoder_name 解码器名称
     */
    void set_default_decoder(const std::string& format, const std::string& decoder_name);

    /**
     * @brief 获取格式的默认解码器
     * @param format 文件格式
     * @return 解码器名称，如果没有设置默认解码器则返回空字符串
     */
    std::string get_default_decoder(const std::string& format);

    /**
     * @brief 启用/禁用解码器
     * @param decoder_name 解码器名称
     * @param enabled 是否启用
     */
    void set_decoder_enabled(const std::string& decoder_name, bool enabled);

    /**
     * @brief 检查解码器是否启用
     * @param decoder_name 解码器名称
     * @return 是否启用
     */
    bool is_decoder_enabled(const std::string& decoder_name) const;

    /**
     * @brief 注销解码器
     * @param decoder_name 解码器名称
     */
    void unregister_decoder(const std::string& decoder_name);

private:
    AudioDecoderRegistry() = default;
    ~AudioDecoderRegistry() = default;
    AudioDecoderRegistry(const AudioDecoderRegistry&) = delete;
    AudioDecoderRegistry& operator=(const AudioDecoderRegistry&) = delete;

    struct DecoderInfo {
        DecoderFactory factory;
        std::unique_ptr<ITypedPluginFactory<IAudioDecoder>> plugin_factory;
        std::vector<std::string> supported_formats;
        bool enabled = true;
    };

    std::map<std::string, DecoderInfo> decoders_;
    std::map<std::string, std::string> format_defaults_;

    std::string extract_extension(const std::string& file_path) const;
};

/**
 * @brief 解码器自动注册器
 *
 * 使用RAII模式在程序启动时自动注册解码器
 */
template<typename DecoderType>
class DecoderAutoRegister {
public:
    DecoderAutoRegister(const std::vector<std::string>& formats) {
        auto factory = []() -> std::unique_ptr<IAudioDecoder> {
            return std::make_unique<DecoderType>();
        };

        AudioDecoderRegistry::get_instance().register_decoder(
            DecoderType().get_info().name,
            formats,
            factory
        );
    }
};

/**
 * @brief 宏定义，用于自动注册解码器
 */
#define QODER_AUTO_REGISTER_DECODER(decoder_class, formats) \
    static DecoderAutoRegister<decoder_class> _auto_register_##decoder_class(formats);

} // namespace xpumusic::core