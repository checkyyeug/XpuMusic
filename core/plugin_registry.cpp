/**
 * @file plugin_registry.cpp
 * @brief 插件注册表实现
 * @date 2025-12-10
 */

#include "plugin_registry.h"
#include <algorithm>
#include <iostream>

namespace xpumusic {

bool PluginRegistry::register_factory(const std::string& key,
                                     std::unique_ptr<IPluginFactory> factory) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!factory) {
        return false;
    }

    // 检查兼容性
    if (!factory->is_compatible(host_api_version_)) {
        std::cerr << "Plugin API version mismatch: " << key << std::endl;
        return false;
    }

    // 检查是否已存在
    if (factories_.find(key) != factories_.end()) {
        std::cerr << "Plugin already registered: " << key << std::endl;
        return false;
    }

    // 获取插件信息
    PluginInfo info = factory->get_info();

    // 注册工厂
    factories_[key] = std::move(factory);

    // 更新类型映射
    type_map_[info.type].push_back(key);

    // 如果是解码器，更新扩展名映射
    if (info.type == PluginType::AudioDecoder) {
        for (const auto& ext : info.supported_formats) {
            std::string lower_ext = ext;
            std::transform(lower_ext.begin(), lower_ext.end(),
                          lower_ext.begin(), ::tolower);
            extension_map_[lower_ext].push_back(key);
        }
    }

    return true;
}

bool PluginRegistry::unregister_factory(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = factories_.find(key);
    if (it == factories_.end()) {
        return false;
    }

    // 获取插件信息
    PluginInfo info = it->second->get_info();

    // 从类型映射中移除
    auto& type_list = type_map_[info.type];
    type_list.erase(std::remove(type_list.begin(), type_list.end(), key),
                   type_list.end());
    if (type_list.empty()) {
        type_map_.erase(info.type);
    }

    // 从扩展名映射中移除
    if (info.type == PluginType::AudioDecoder) {
        for (const auto& ext : info.supported_formats) {
            std::string lower_ext = ext;
            std::transform(lower_ext.begin(), lower_ext.end(),
                          lower_ext.begin(), ::tolower);
            auto& ext_list = extension_map_[lower_ext];
            ext_list.erase(std::remove(ext_list.begin(), ext_list.end(), key),
                          ext_list.end());
            if (ext_list.empty()) {
                extension_map_.erase(lower_ext);
            }
        }
    }

    // 移除工厂
    factories_.erase(it);

    return true;
}

IPluginFactory* PluginRegistry::get_factory(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = factories_.find(key);
    return (it != factories_.end()) ? it->second.get() : nullptr;
}

std::vector<IPluginFactory*> PluginRegistry::get_all_factories() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<IPluginFactory*> result;
    result.reserve(factories_.size());

    for (const auto& [key, factory] : factories_) {
        result.push_back(factory.get());
    }

    return result;
}

std::vector<IPluginFactory*> PluginRegistry::get_factories_by_type(
    PluginType type) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<IPluginFactory*> result;

    auto it = type_map_.find(type);
    if (it != type_map_.end()) {
        result.reserve(it->second.size());
        for (const auto& key : it->second) {
            auto factory_it = factories_.find(key);
            if (factory_it != factories_.end()) {
                result.push_back(factory_it->second.get());
            }
        }
    }

    return result;
}

std::vector<IPluginFactory*> PluginRegistry::get_decoder_factories_by_extension(
    const std::string& extension) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<IPluginFactory*> result;

    std::string lower_ext = extension;
    std::transform(lower_ext.begin(), lower_ext.end(),
                  lower_ext.begin(), ::tolower);

    auto it = extension_map_.find(lower_ext);
    if (it != extension_map_.end()) {
        result.reserve(it->second.size());
        for (const auto& key : it->second) {
            auto factory_it = factories_.find(key);
            if (factory_it != factories_.end()) {
                result.push_back(factory_it->second.get());
            }
        }
    }

    return result;
}

bool PluginRegistry::is_registered(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return factories_.find(key) != factories_.end();
}

std::vector<std::string> PluginRegistry::get_supported_extensions() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> extensions;
    extensions.reserve(extension_map_.size());

    for (const auto& [ext, factories] : extension_map_) {
        if (!factories.empty()) {
            extensions.push_back(ext);
        }
    }

    return extensions;
}

void PluginRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    factories_.clear();
    extension_map_.clear();
    type_map_.clear();
}

PluginRegistry::Stats PluginRegistry::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats stats = {};
    stats.total_factories = factories_.size();
    stats.decoder_factories = type_map_.count(PluginType::AudioDecoder);
    stats.dsp_factories = type_map_.count(PluginType::DSPEffect);
    stats.output_factories = type_map_.count(PluginType::AudioOutput);
    stats.visualization_factories = type_map_.count(PluginType::Visualization);

    return stats;
}

} // namespace xpumusic