/**
 * @file plugin_registry.h
 * @brief 插件注册表 - 管理所有已注册的插件
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
    // 插件工厂注册表
    std::unordered_map<std::string, std::unique_ptr<IPluginFactory>> factories_;

    // 文件扩展名到插件工厂的映射
    std::unordered_map<std::string, std::vector<std::string>> extension_map_;

    // 插件类型到工厂的映射
    std::unordered_map<PluginType, std::vector<std::string>> type_map_;

    // 线程安全
    mutable std::mutex mutex_;

    // API版本
    uint32_t host_api_version_;

public:
    explicit PluginRegistry(uint32_t api_version = xpumusic::XPUMUSIC_PLUGIN_API_VERSION)
        : host_api_version_(api_version) {}

    // 注册插件工厂
    bool register_factory(const std::string& key,
                         std::unique_ptr<IPluginFactory> factory);

    // 注销插件
    bool unregister_factory(const std::string& key);

    // 获取插件工厂
    IPluginFactory* get_factory(const std::string& key) const;

    // 获取所有工厂
    std::vector<IPluginFactory*> get_all_factories() const;

    // 按类型获取工厂
    std::vector<IPluginFactory*> get_factories_by_type(PluginType type) const;

    // 按扩展名获取解码器工厂
    std::vector<IPluginFactory*> get_decoder_factories_by_extension(
        const std::string& extension) const;

    // 检查插件是否已注册
    bool is_registered(const std::string& key) const;

    // 获取支持的扩展名
    std::vector<std::string> get_supported_extensions() const;

    // 清空注册表
    void clear();

    // 获取统计信息
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