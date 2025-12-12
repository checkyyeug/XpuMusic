#include "audio_decoder_registry.h"
#include <algorithm>
#include <filesystem>

namespace xpumusic::core {

AudioDecoderRegistry& AudioDecoderRegistry::get_instance() {
    static AudioDecoderRegistry instance;
    return instance;
}

void AudioDecoderRegistry::register_decoder(const std::string& name,
                                          const std::vector<std::string>& supported_formats,
                                          DecoderFactory factory) {
    DecoderInfo info;
    info.factory = factory;
    info.supported_formats = supported_formats;

    // 杞崲涓哄皬鍐?
    for (auto& format : info.supported_formats) {
        std::transform(format.begin(), format.end(), format.begin(), ::tolower);
    }

    decoders_[name] = std::move(info);
}

void AudioDecoderRegistry::register_decoder(const std::string& name,
                                          std::unique_ptr<ITypedPluginFactory<IAudioDecoder>> factory) {
    if (!factory) return;

    DecoderInfo info;
    info.supported_formats = factory->get_info().supported_formats;
    info.plugin_factory = std::move(factory);

    // 杞崲涓哄皬鍐?
    for (auto& format : info.supported_formats) {
        std::transform(format.begin(), format.end(), format.begin(), ::tolower);
    }

    decoders_[name] = std::move(info);
}

std::unique_ptr<IAudioDecoder> AudioDecoderRegistry::get_decoder(const std::string& file_path) {
    std::string extension = extract_extension(file_path);

    // 妫€鏌ユ槸鍚︽湁璇ユ牸寮忕殑榛樿瑙ｇ爜鍣?
    auto default_it = format_defaults_.find(extension);
    if (default_it != format_defaults_.end()) {
        auto decoder_it = decoders_.find(default_it->second);
        if (decoder_it != decoders_.end() && decoder_it->second.enabled) {
            if (decoder_it->second.factory) {
                return decoder_it->second.factory();
            } else if (decoder_it->second.plugin_factory) {
                return decoder_it->second.plugin_factory->create_typed();
            }
        }
    }

    // 鏌ユ壘鏀寔璇ユ牸寮忕殑瑙ｇ爜鍣?
    for (const auto& [name, info] : decoders_) {
        if (!info.enabled) continue;

        if (std::find(info.supported_formats.begin(), info.supported_formats.end(), extension)
            != info.supported_formats.end()) {
            if (info.factory) {
                return info.factory();
            } else if (info.plugin_factory) {
                return info.plugin_factory->create_typed();
            }
        }
    }

    return nullptr;
}

std::unique_ptr<IAudioDecoder> AudioDecoderRegistry::get_decoder_by_name(const std::string& decoder_name) {
    auto it = decoders_.find(decoder_name);
    if (it != decoders_.end() && it->second.enabled) {
        if (it->second.factory) {
            return it->second.factory();
        } else if (it->second.plugin_factory) {
            return it->second.plugin_factory->create_typed();
        }
    }
    return nullptr;
}

std::vector<PluginInfo> AudioDecoderRegistry::get_registered_decoders() const {
    std::vector<PluginInfo> infos;

    for (const auto& [name, info] : decoders_) {
        if (info.plugin_factory) {
            infos.push_back(info.plugin_factory->get_info());
        } else {
            // 鍒涘缓涓存椂瑙ｇ爜鍣ㄥ疄渚嬩互鑾峰彇淇℃伅
            auto decoder = info.factory();
            if (decoder) {
                infos.push_back(decoder->get_info());
            }
        }
    }

    return infos;
}

std::vector<std::string> AudioDecoderRegistry::get_decoders_for_format(const std::string& format) {
    std::string lower_format = format;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);

    std::vector<std::string> result;
    for (const auto& [name, info] : decoders_) {
        if (std::find(info.supported_formats.begin(), info.supported_formats.end(), lower_format)
            != info.supported_formats.end()) {
            result.push_back(name);
        }
    }

    return result;
}

bool AudioDecoderRegistry::supports_format(const std::string& file_path) {
    std::string extension = extract_extension(file_path);

    for (const auto& [name, info] : decoders_) {
        if (!info.enabled) continue;

        if (std::find(info.supported_formats.begin(), info.supported_formats.end(), extension)
            != info.supported_formats.end()) {
            return true;
        }
    }

    return false;
}

void AudioDecoderRegistry::set_default_decoder(const std::string& format, const std::string& decoder_name) {
    std::string lower_format = format;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);
    format_defaults_[lower_format] = decoder_name;
}

std::string AudioDecoderRegistry::get_default_decoder(const std::string& format) {
    std::string lower_format = format;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);

    auto it = format_defaults_.find(lower_format);
    return (it != format_defaults_.end()) ? it->second : "";
}

void AudioDecoderRegistry::set_decoder_enabled(const std::string& decoder_name, bool enabled) {
    auto it = decoders_.find(decoder_name);
    if (it != decoders_.end()) {
        it->second.enabled = enabled;
    }
}

bool AudioDecoderRegistry::is_decoder_enabled(const std::string& decoder_name) const {
    auto it = decoders_.find(decoder_name);
    return (it != decoders_.end()) ? it->second.enabled : false;
}

void AudioDecoderRegistry::unregister_decoder(const std::string& decoder_name) {
    decoders_.erase(decoder_name);

    // 娓呯悊榛樿瑙ｇ爜鍣ㄨ缃?
    for (auto it = format_defaults_.begin(); it != format_defaults_.end();) {
        if (it->second == decoder_name) {
            it = format_defaults_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string AudioDecoderRegistry::extract_extension(const std::string& file_path) const {
    std::filesystem::path path(file_path);
    std::string ext = path.extension().string();

    // 鍘绘帀鍓嶉潰鐨勭偣骞惰浆鎹负灏忓啓
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

} // namespace xpumusic::core