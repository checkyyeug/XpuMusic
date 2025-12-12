#include "audio_decoder_manager.h"
#include <iostream>

namespace xpumusic::core {

AudioDecoderManager& AudioDecoderManager::get_instance() {
    static AudioDecoderManager instance;
    return instance;
}

void AudioDecoderManager::initialize() {
    if (initialized_) {
        return;
    }

    // 鍒濆鍖栨牸寮忔娴嬪櫒锛堜細鑷姩娉ㄥ唽鍐呯疆鏍煎紡锛?
    AudioFormatDetector::get_instance();

    // 娉ㄥ唽鍐呯疆瑙ｇ爜鍣ㄤ細閫氳繃 QODER_AUTO_REGISTER_DECODER 瀹忚嚜鍔ㄨ繘琛?
    // 杩欓噷鍙渶瑕佺‘淇濆凡鍒濆鍖?

    initialized_ = true;
}

std::unique_ptr<IAudioDecoder> AudioDecoderManager::get_decoder_for_file(const std::string& file_path) {
    if (!initialized_) {
        initialize();
    }

    auto& registry = AudioDecoderRegistry::get_instance();
    return registry.get_decoder(file_path);
}

AudioFormatInfo AudioDecoderManager::detect_format(const std::string& file_path) {
    if (!initialized_) {
        initialize();
    }

    auto& detector = AudioFormatDetector::get_instance();
    return detector.detect_format(file_path);
}

std::unique_ptr<IAudioDecoder> AudioDecoderManager::open_audio_file(const std::string& file_path) {
    auto decoder = get_decoder_for_file(file_path);
    if (decoder && decoder->open(file_path)) {
        return decoder;
    }
    return nullptr;
}

bool AudioDecoderManager::supports_file(const std::string& file_path) {
    if (!initialized_) {
        initialize();
    }

    auto& detector = AudioFormatDetector::get_instance();
    AudioFormatInfo info = detector.detect_format(file_path);
    return info.supported;
}

std::vector<std::string> AudioDecoderManager::get_supported_formats() {
    if (!initialized_) {
        initialize();
    }

    auto& detector = AudioFormatDetector::get_instance();
    return detector.get_supported_formats();
}

std::vector<PluginInfo> AudioDecoderManager::get_available_decoders() {
    if (!initialized_) {
        initialize();
    }

    auto& registry = AudioDecoderRegistry::get_instance();
    return registry.get_registered_decoders();
}

void AudioDecoderManager::set_default_decoder(const std::string& format, const std::string& decoder_name) {
    if (!initialized_) {
        initialize();
    }

    auto& registry = AudioDecoderRegistry::get_instance();
    registry.set_default_decoder(format, decoder_name);
}

json_map AudioDecoderManager::get_metadata(const std::string& file_path) {
    auto decoder = open_audio_file(file_path);
    if (decoder) {
        auto metadata_items = decoder->get_metadata();
        json_map metadata;
        for (const auto& item : metadata_items) {
            metadata[item.key] = item.value;
        }
        return metadata;
    }
    return json_map{};
}

double AudioDecoderManager::get_duration(const std::string& file_path) {
    // 鍏堝皾璇曢€氳繃瑙ｇ爜鍣ㄨ幏鍙?
    auto decoder = open_audio_file(file_path);
    if (decoder) {
        // Use the standard interface get_duration
        return decoder->get_duration();
    }
    return -1.0;
}

} // namespace xpumusic::core