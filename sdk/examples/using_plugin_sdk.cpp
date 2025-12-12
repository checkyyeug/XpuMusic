/**
 * @file using_plugin_sdk.cpp
 * @brief XpuMusic 鎻掍欢SDK浣跨敤绀轰緥
 * @date 2025-12-10
 */

#include "../xpumusic_plugin_sdk.h"
#include "../../core/plugin_manager.h"
#include <iostream>
#include <memory>

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IAudioDecoder;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;

int main() {
    std::cout << "=== XpuMusic Plugin SDK Demo ===\n\n";

    // 1. 鍒涘缓鎻掍欢绠＄悊鍣?
    PluginManager manager;

    // 2. 鍔犺浇鎻掍欢鐩綍涓殑鎵€鏈夋彃浠?
    std::cout << "Loading plugins from ./plugins directory...\n";
    manager.load_plugins_from_directory("./plugins");

    // 3. 鏄剧ず鍔犺浇鐨勬彃浠?
    std::cout << "\nLoaded plugins:\n";
    auto plugins = manager.get_plugin_list();
    for (const auto& plugin : plugins) {
        std::cout << "- " << plugin.name << " v" << plugin.version
                  << " by " << plugin.author << "\n";
    }

    // 4. 鏄剧ず鏀寔鐨勬牸寮?
    std::cout << "\nSupported formats:\n";
    auto formats = manager.get_supported_formats();
    for (const auto& format : formats) {
        std::cout << "- ." << format << "\n";
    }

    // 5. 鎵撳紑闊抽鏂囦欢
    std::string file_path = "test.wav";
    std::cout << "\nOpening file: " << file_path << "\n";

    auto decoder = manager.get_decoder(file_path);
    if (!decoder) {
        std::cerr << "No decoder found for file!\n";
        return 1;
    }

    // 6. 鎵撳紑鏂囦欢骞惰幏鍙栦俊鎭?
    if (!decoder->open(file_path)) {
        std::cerr << "Failed to open file: " << decoder->get_last_error() << "\n";
        return 1;
    }

    AudioFormat format = decoder->get_format();
    std::cout << "\nAudio format:\n";
    std::cout << "- Sample rate: " << format.sample_rate << " Hz\n";
    std::cout << "- Channels: " << format.channels << "\n";
    std::cout << "- Bits per sample: " << format.bits_per_sample << "\n";
    std::cout << "- Duration: " << decoder->get_duration() << " seconds\n";

    // 7. 鏄剧ず鍏冩暟鎹?
    auto metadata = decoder->get_metadata();
    if (!metadata.empty()) {
        std::cout << "\nMetadata:\n";
        for (const auto& item : metadata) {
            std::cout << "- " << item.key << ": " << item.value << "\n";
        }
    }

    // 8. 瑙ｇ爜绀轰緥
    const int buffer_size = 4096;
    std::vector<float> buffer(buffer_size);

    std::cout << "\nDecoding first " << buffer_size << " samples...\n";
    int frames_decoded = decoder->decode(
        {buffer.data(), buffer_size, format.channels},
        buffer_size);

    std::cout << "Decoded " << frames_decoded << " frames\n";

    // 9. 浣跨敤DSP鎻掍欢
    std::cout << "\n--- DSP Plugin Demo ---\n";

    // 鍔犺浇鐮佺巼杞崲鍣ㄦ彃浠?
    if (manager.load_native_plugin("./plugins/resampler_dsp.so")) {
        // 鑾峰彇DSP鎻掍欢宸ュ巶
        auto* factory = manager.get_factory("XpuMusic Sample Rate Converter");
        if (factory) {
            auto dsp_plugin = std::unique_ptr<IDSPProcessor>(
                static_cast<IDSPProcessor*>(factory->create()));

            if (dsp_plugin && dsp_plugin->initialize()) {
                // 閰嶇疆DSP
                AudioFormat output_format = format;
                output_format.sample_rate = 96000; // 杞崲鍒?6kHz

                if (dsp_plugin->configure(format, output_format)) {
                    std::cout << "Configured resampler to convert "
                              << format.sample_rate << "Hz to "
                              << output_format.sample_rate << "Hz\n";

                    // 璁剧疆璐ㄩ噺
                    dsp_plugin->set_parameter("quality", 3.0); // Best quality
                    std::cout << "Set quality to Best\n";

                    // 鍒涘缓杈撳嚭缂撳啿鍖?
                    std::vector<float> output_buffer(buffer_size * 2);

                    // 澶勭悊闊抽
                    AudioBuffer input_buf = {buffer.data(), frames_decoded, format.channels};
                    AudioBuffer output_buf = {output_buffer.data(), buffer_size * 2, format.channels};

                    int output_frames = dsp_plugin->process(input_buf, output_buf);
                    std::cout << "Processed " << output_frames << " output frames\n";
                    std::cout << "DSP latency: " << dsp_plugin->get_latency_samples() << " samples\n";
                }
            }
        }
    }

    // 10. 娓呯悊
    decoder->close();

    std::cout << "\nDemo completed successfully!\n";
    return 0;
}