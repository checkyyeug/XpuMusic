/**
 * @file plugin_manager_v2.cpp
 * @brief 基于新架构的插件管理器实现
 * @date 2025-12-10
 */

#include "plugin_manager.h"
#include "../sdk/xpumusic_plugin_sdk.h"
#include "../compat/foobar2000/foobar_adapter.h"
#include <filesystem>
#include <iostream>

namespace xpumusic {

PluginManager::PluginManager() : host_api_version_(xpumusic::XPUMUSIC_PLUGIN_API_VERSION) {
}

bool PluginManager::load_native_plugin(const std::string& path) {
    // 平台相关的动态库加载
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        std::cerr << "Failed to load plugin: " << path << std::endl;
        return false;
    }

    // 获取工厂创建函数
    auto create_factory = reinterpret_cast<IPluginFactory* (*)()>(
        GetProcAddress(handle, "xpumusic_create_plugin_factory"));
    if (!create_factory) {
        std::cerr << "Plugin does not export factory function: " << path << std::endl;
        FreeLibrary(handle);
        return false;
    }
#else
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << path << " - " << dlerror() << std::endl;
        return false;
    }

    auto create_factory = reinterpret_cast<IPluginFactory* (*)()>(
        dlsym(handle, "xpumusic_create_plugin_factory"));
    if (!create_factory) {
        std::cerr << "Plugin does not export factory function: " << path << std::endl;
        dlclose(handle);
        return false;
    }
#endif

    // 创建工厂
    std::unique_ptr<IPluginFactory> factory(create_factory());
    if (!factory) {
        std::cerr << "Failed to create plugin factory: " << path << std::endl;
        return false;
    }

    // 检查兼容性
    if (!factory->is_compatible(host_api_version_)) {
        std::cerr << "Plugin API version mismatch: " << path << std::endl;
        return false;
    }

    // 获取插件信息
    PluginInfo info = factory->get_info();
    std::cout << "Loading plugin: " << info.name << " v" << info.version << std::endl;

    // 存储插件
    std::string plugin_key = info.name + ":" + info.version;
    native_plugins_[plugin_key] = std::move(factory);

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
        // 创建一个包装工厂
        auto factory = std::make_unique<compat::FoobarDecoderFactory>(
            std::move(decoder));

        PluginInfo info = factory->get_info();
        std::cout << "Loading foobar2000 plugin: " << info.name << std::endl;

        std::string plugin_key = "foobar:" + info.name;
        native_plugins_[plugin_key] = std::move(factory);
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
                if (path.find("foo_") != std::string::npos) {
                    // 可能是foobar2000插件
                    load_foobar_plugin(path);
                } else {
                    // 原生插件
                    load_native_plugin(path);
                }
            }
#else
            if (ext == ".so") {
                // 在Linux上，需要检查文件名或内容来确定类型
                // 这里简化处理，都尝试作为原生插件
                load_native_plugin(path);
            }
#endif
        }
    }

    std::cout << "Loaded " << native_plugins_.size() << " plugin factories" << std::endl;
}

std::unique_ptr<IAudioDecoder> PluginManager::get_decoder(const std::string& file_path) {
    // 遍历所有解码器工厂，找到能处理该文件的
    for (const auto& [key, factory] : native_plugins_) {
        PluginInfo info = factory->get_info();
        if (info.type != PluginType::AudioDecoder) {
            continue;
        }

        // 创建解码器实例
        auto plugin = factory->create();
        if (!plugin) continue;

        auto decoder = dynamic_cast<IAudioDecoder*>(plugin.get());
        if (!decoder) continue;

        // 检查是否能解码
        if (decoder->can_decode(file_path)) {
            plugin.release(); // 释放所有权
            return std::unique_ptr<IAudioDecoder>(decoder);
        }
    }

    return nullptr;
}

std::vector<std::string> PluginManager::get_supported_formats() {
    std::vector<std::string> formats;
    std::set<std::string> unique_formats;

    for (const auto& [key, factory] : native_plugins_) {
        PluginInfo info = factory->get_info();
        if (info.type == PluginType::AudioDecoder) {
            // 创建临时实例获取支持的格式
            auto plugin = factory->create();
            auto decoder = dynamic_cast<IAudioDecoder*>(plugin.get());
            if (decoder) {
                auto exts = decoder->get_supported_extensions();
                for (const auto& ext : exts) {
                    unique_formats.insert(ext);
                }
            }
        } else {
            // 从插件信息获取
            for (const auto& ext : info.supported_formats) {
                unique_formats.insert(ext);
            }
        }
    }

    formats.assign(unique_formats.begin(), unique_formats.end());
    return formats;
}

std::vector<PluginInfo> PluginManager::get_plugin_list() {
    std::vector<PluginInfo> plugins;

    for (const auto& [key, factory] : native_plugins_) {
        plugins.push_back(factory->get_info());
    }

    return plugins;
}

} // namespace xpumusic