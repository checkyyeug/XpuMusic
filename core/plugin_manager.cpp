/**
 * @file plugin_manager.cpp
 * @brief 插件管理器实现
 * @date 2025-12-10
 */

#include "plugin_manager.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <dlfcn.h>

namespace xpumusic {

PluginManager::PluginManager(uint32_t api_version)
    : host_api_version_(api_version) {
    registry_ = std::make_unique<PluginRegistry>(api_version);
}

PluginManager::~PluginManager() {
    unload_all_plugins();
}

void PluginManager::initialize(const PluginConfig& config) {
    // 清除现有目录
    plugin_directories_.clear();

    // 添加配置中的目录
    for (const auto& dir : config.plugin_directories) {
        add_plugin_directory(dir);
    }

    // 如果启用自动加载，立即加载所有插件
    if (config.auto_load_plugins) {
        load_all_plugins();
    }
}

void PluginManager::initialize_from_config_manager(const ConfigManager& config_manager) {
    initialize(config_manager.get_config().plugins);
}

void PluginManager::add_plugin_directory(const std::string& directory) {
    if (std::filesystem::exists(directory)) {
        plugin_directories_.push_back(directory);
    }
}

void PluginManager::remove_plugin_directory(const std::string& directory) {
    auto it = std::find(plugin_directories_.begin(),
                        plugin_directories_.end(),
                        directory);
    if (it != plugin_directories_.end()) {
        plugin_directories_.erase(it);
    }
}

const std::vector<std::string>& PluginManager::get_plugin_directories() const {
    return plugin_directories_;
}

bool PluginManager::load_native_plugin(const std::string& path) {
    // 加载动态库
    void* handle = load_library(path);
    if (!handle) {
        std::cerr << "Failed to load library: " << path << std::endl;
        return false;
    }

    // 获取工厂创建函数
    auto create_factory = reinterpret_cast<IPluginFactory* (*)()>(
        get_library_symbol(handle, "xpumusic_create_plugin_factory"));

    if (!create_factory) {
        std::cerr << "Plugin does not export xpumusic_create_plugin_factory: "
                  << path << std::endl;
        unload_library(handle);
        return false;
    }

    // 创建工厂
    std::unique_ptr<IPluginFactory> factory(create_factory());
    if (!factory) {
        std::cerr << "Failed to create plugin factory: " << path << std::endl;
        unload_library(handle);
        return false;
    }

    // 注册插件
    std::string plugin_key = register_plugin_factory(path, std::move(factory));
    if (plugin_key.empty()) {
        unload_library(handle);
        return false;
    }

    // 保存库句柄
    library_handles_[plugin_key] = handle;

    // 触发回调
    if (on_plugin_loaded_) {
        on_plugin_loaded_(plugin_key);
    }

    std::cout << "Loaded native plugin: " << plugin_key << std::endl;
    return true;
}

bool PluginManager::load_foobar_plugin(const std::string& path) {
    // 使用foobar2000兼容适配器
    auto wrapper = std::make_unique<compat::FoobarPluginWrapper>();
    if (!wrapper->load_plugin(path)) {
        std::cerr << "Failed to load foobar2000 plugin: " << path << std::endl;
        return false;
    }

    // 获取适配后的解码器
    auto decoders = wrapper->get_decoders();
    for (auto& decoder : decoders) {
        // 创建包装工厂
        auto factory = std::make_unique<compat::FoobarDecoderFactory>(
            std::move(decoder));

        PluginInfo info = factory->get_info();
        std::string plugin_key = "foobar:" + info.name + ":" + info.version;

        // 注册到注册表
        if (registry_->register_factory(plugin_key, std::move(factory))) {
            std::cout << "Loaded foobar2000 decoder: " << info.name << std::endl;
        }
    }

    foobar_plugins_.push_back(std::move(wrapper));
    return true;
}

void PluginManager::load_plugins_from_directory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        std::cerr << "Plugin directory does not exist: " << directory << std::endl;
        return;
    }

    std::cout << "Scanning plugin directory: " << directory << std::endl;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();

#ifdef _WIN32
            if (ext == ".dll") {
                if (is_foobar_plugin(path)) {
                    load_foobar_plugin(path);
                } else {
                    load_native_plugin(path);
                }
            }
#else
            if (ext == ".so") {
                load_native_plugin(path);
            }
#endif
        }
    }
}

void PluginManager::load_all_plugins() {
    for (const auto& directory : plugin_directories_) {
        load_plugins_from_directory(directory);
    }
}

bool PluginManager::unload_plugin(const std::string& key) {
    // 卸载插件实例
    instances_.erase(key);

    // 从注册表注销
    if (!registry_->unregister_factory(key)) {
        return false;
    }

    // 卸载动态库
    auto it = library_handles_.find(key);
    if (it != library_handles_.end()) {
        unload_library(it->second);
        library_handles_.erase(it);
    }

    // 触发回调
    if (on_plugin_unloaded_) {
        on_plugin_unloaded_(key);
    }

    std::cout << "Unloaded plugin: " << key << std::endl;
    return true;
}

void PluginManager::unload_all_plugins() {
    // 清理所有实例
    instances_.clear();

    // 卸载所有动态库
    for (auto& [key, handle] : library_handles_) {
        unload_library(handle);
    }
    library_handles_.clear();

    // 清理foobar插件
    foobar_plugins_.clear();

    // 清空注册表
    registry_->clear();
}

std::unique_ptr<IAudioDecoder> PluginManager::get_decoder(const std::string& file_path) {
    std::string extension = get_file_extension(file_path);
    if (extension.empty()) {
        return nullptr;
    }

    // 获取支持该扩展名的解码器工厂
    auto factories = registry_->get_decoder_factories_by_extension(extension);
    if (factories.empty()) {
        return nullptr;
    }

    // 尝试每个工厂
    for (auto* factory : factories) {
        auto plugin = factory->create();
        if (!plugin) continue;

        auto decoder = dynamic_cast<IAudioDecoder*>(plugin.get());
        if (!decoder) continue;

        // 检查是否能解码该文件
        if (decoder->can_decode(file_path)) {
            plugin.release(); // 释放所有权
            return std::unique_ptr<IAudioDecoder>(decoder);
        }
    }

    return nullptr;
}

std::unique_ptr<IDSPProcessor> PluginManager::get_dsp_processor(const std::string& name) {
    auto factories = registry_->get_factories_by_type(PluginType::DSPEffect);

    for (auto* factory : factories) {
        PluginInfo info = factory->get_info();
        if (info.name == name) {
            auto plugin = factory->create();
            auto processor = dynamic_cast<IDSPProcessor*>(plugin.get());
            if (processor) {
                plugin.release();
                return std::unique_ptr<IDSPProcessor>(processor);
            }
        }
    }

    return nullptr;
}

std::unique_ptr<IAudioOutput> PluginManager::get_audio_output(const std::string& device_id) {
    auto factories = registry_->get_factories_by_type(PluginType::AudioOutput);

    // 优先获取指定设备
    for (auto* factory : factories) {
        auto plugin = factory->create();
        auto output = dynamic_cast<IAudioOutput*>(plugin.get());
        if (output) {
            plugin.release();
            return std::unique_ptr<IAudioOutput>(output);
        }
    }

    return nullptr;
}

std::unique_ptr<IVisualization> PluginManager::get_visualization(const std::string& name) {
    auto factories = registry_->get_factories_by_type(PluginType::Visualization);

    for (auto* factory : factories) {
        PluginInfo info = factory->get_info();
        if (info.name == name) {
            auto plugin = factory->create();
            auto viz = dynamic_cast<IVisualization*>(plugin.get());
            if (viz) {
                plugin.release();
                return std::unique_ptr<IVisualization>(viz);
            }
        }
    }

    return nullptr;
}

std::vector<PluginInfo> PluginManager::get_plugin_list() const {
    auto factories = registry_->get_all_factories();
    std::vector<PluginInfo> plugins;

    plugins.reserve(factories.size());
    for (auto* factory : factories) {
        plugins.push_back(factory->get_info());
    }

    return plugins;
}

std::vector<PluginInfo> PluginManager::get_plugins_by_type(PluginType type) const {
    auto factories = registry_->get_factories_by_type(type);
    std::vector<PluginInfo> plugins;

    plugins.reserve(factories.size());
    for (auto* factory : factories) {
        plugins.push_back(factory->get_info());
    }

    return plugins;
}

std::vector<std::string> PluginManager::get_supported_extensions() const {
    return registry_->get_supported_extensions();
}

bool PluginManager::can_decode(const std::string& file_path) const {
    std::string extension = get_file_extension(file_path);
    if (extension.empty()) {
        return false;
    }

    auto factories = registry_->get_decoder_factories_by_extension(extension);
    return !factories.empty();
}

bool PluginManager::is_plugin_loaded(const std::string& key) const {
    return registry_->is_registered(key);
}

IPluginFactory* PluginManager::get_factory(const std::string& key) const {
    return registry_->get_factory(key);
}

PluginManager::ManagerStats PluginManager::get_stats() const {
    ManagerStats stats;
    stats.registry_stats = registry_->get_stats();
    stats.loaded_instances = instances_.size();
    stats.foobar_plugins = foobar_plugins_.size();
    return stats;
}

void* PluginManager::load_library(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

void PluginManager::unload_library(void* handle) {
    if (handle) {
        dlclose(handle);
    }
}

void* PluginManager::get_library_symbol(void* handle, const std::string& symbol) {
    return dlsym(handle, symbol.c_str());
}

bool PluginManager::register_plugin_factory(const std::string& path,
                                           std::unique_ptr<IPluginFactory> factory) {
    PluginInfo info = factory->get_info();
    std::string plugin_key = generate_plugin_key(info);

    if (registry_->register_factory(plugin_key, std::move(factory))) {
        return plugin_key;
    }

    return "";
}

std::string PluginManager::generate_plugin_key(const PluginInfo& info) const {
    return info.name + ":" + info.version;
}

std::string PluginManager::get_file_extension(const std::string& path) const {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) {
        return "";
    }

    std::string ext = path.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool PluginManager::is_foobar_plugin(const std::string& path) const {
    // 简单判断：如果文件名包含"foo_"，很可能是foobar2000插件
    return path.find("foo_") != std::string::npos;
}

} // namespace xpumusic