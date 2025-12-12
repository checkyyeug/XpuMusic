/**
 * @file plugin_registry.h
 * @brief 鎻掍欢娉ㄥ唽琛?- 绠＄悊鎵€鏈夊凡娉ㄥ唽鐨勬彃浠?
 * @date 2025-12-10
 */

#pragma once

#include "../sdk/xpumusic_plugin_sdk.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace xpumusic {

class PluginRegistry {
private:
    // 鎻掍欢宸ュ巶娉ㄥ唽琛?
    std::unordered_map<std::string, std::unique_ptr<IPluginFactory>> factories_;

    // 鏂囦欢鎵╁睍鍚嶅埌鎻掍欢宸ュ巶鐨勬槧灏?
    std::unordered_map<std::string, std::vector<std::string>> extension_map_;

    // 鎻掍欢绫诲瀷鍒板伐鍘傜殑鏄犲皠
    std::unordered_map<PluginType, std::vector<std::string>> type_map_;

    // 绾跨▼瀹夊叏
    mutable std::mutex mutex_;

    // API鐗堟湰
    uint32_t host_api_version_;

public:
    explicit PluginRegistry(uint32_t api_version = xpumusic::XPUMUSIC_PLUGIN_API_VERSION)
        : host_api_version_(api_version) {}

    // 娉ㄥ唽鎻掍欢宸ュ巶
    bool register_factory(const std::string& key,
                         std::unique_ptr<IPluginFactory> factory);

    // 娉ㄩ攢鎻掍欢
    bool unregister_factory(const std::string& key);

    // 鑾峰彇鎻掍欢宸ュ巶
    IPluginFactory* get_factory(const std::string& key) const;

    // 鑾峰彇鎵€鏈夊伐鍘?
    std::vector<IPluginFactory*> get_all_factories() const;

    // 鎸夌被鍨嬭幏鍙栧伐鍘?
    std::vector<IPluginFactory*> get_factories_by_type(PluginType type) const;

    // 鎸夋墿灞曞悕鑾峰彇瑙ｇ爜鍣ㄥ伐鍘?
    std::vector<IPluginFactory*> get_decoder_factories_by_extension(
        const std::string& extension) const;

    // 妫€鏌ユ彃浠舵槸鍚﹀凡娉ㄥ唽
    bool is_registered(const std::string& key) const;

    // 鑾峰彇鏀寔鐨勬墿灞曞悕
    std::vector<std::string> get_supported_extensions() const;

    // 娓呯┖娉ㄥ唽琛?
    void clear();

    // 鑾峰彇缁熻淇℃伅
    struct Stats {
        size_t total_factories;
        size_t decoder_factories;
        size_t dsp_factories;
        size_t output_factories;
        size_t visualization_factories;
    };

    Stats get_stats() const;
};

} // namespace xpumusic