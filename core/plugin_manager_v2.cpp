/**
 * @file plugin_manager_v2.cpp
 * @brief 鍩轰簬鏂版灦鏋勭殑鎻掍欢绠＄悊鍣ㄥ疄鐜?
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
    // 骞冲彴鐩稿叧鐨勫姩鎬佸簱鍔犺浇
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        std::cerr << "Failed to load plugin: " << path << std::endl;
        return false;
    }

    // 鑾峰彇宸ュ巶鍒涘缓鍑芥暟
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

    // 鍒涘缓宸ュ巶
    std::unique_ptr<IPluginFactory> factory(create_factory());
    if (!factory) {
        std::cerr << "Failed to create plugin factory: " << path << std::endl;
        return false;
    }

    // 妫€鏌ュ吋瀹规€?
    if (!factory->is_compatible(host_api_version_)) {
        std::cerr << "Plugin API version mismatch: " << path << std::endl;
        return false;
    }

    // 鑾峰彇鎻掍欢淇℃伅
    PluginInfo info = factory->get_info();
    std::cout << "Loading plugin: " << info.name << " v" << info.version << std::endl;

    // 瀛樺偍鎻掍欢
    std::string plugin_key = info.name + ":" + info.version;
    native_plugins_[plugin_key] = std::move(factory);

    return true;
}

bool PluginManager::load_foobar_plugin(const std::string& path) {
    // 浣跨敤foobar2000鍏煎閫傞厤鍣?
    auto wrapper = std::make_unique<compat::FoobarPluginWrapper>();
    if (!wrapper->load_plugin(path)) {
        std::cerr << "Failed to load foobar2000 plugin: " << path << std::endl;
        return false;
    }

    // 鑾峰彇閫傞厤鍚庣殑瑙ｇ爜鍣?
    auto decoders = wrapper->get_decoders();
    for (auto& decoder : decoders) {
        // 鍒涘缓涓€涓寘瑁呭伐鍘?
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
                    // 鍙兘鏄痜oobar2000鎻掍欢
                    load_foobar_plugin(path);
                } else {
                    // 鍘熺敓鎻掍欢
                    load_native_plugin(path);
                }
            }
#else
            if (ext == ".so") {
                // 鍦↙inux涓婏紝闇€瑕佹鏌ユ枃浠跺悕鎴栧唴瀹规潵纭畾绫诲瀷
                // 杩欓噷绠€鍖栧鐞嗭紝閮藉皾璇曚綔涓哄師鐢熸彃浠?
                load_native_plugin(path);
            }
#endif
        }
    }

    std::cout << "Loaded " << native_plugins_.size() << " plugin factories" << std::endl;
}

std::unique_ptr<IAudioDecoder> PluginManager::get_decoder(const std::string& file_path) {
    // 閬嶅巻鎵€鏈夎В鐮佸櫒宸ュ巶锛屾壘鍒拌兘澶勭悊璇ユ枃浠剁殑
    for (const auto& [key, factory] : native_plugins_) {
        PluginInfo info = factory->get_info();
        if (info.type != PluginType::AudioDecoder) {
            continue;
        }

        // 鍒涘缓瑙ｇ爜鍣ㄥ疄渚?
        auto plugin = factory->create();
        if (!plugin) continue;

        auto decoder = dynamic_cast<IAudioDecoder*>(plugin.get());
        if (!decoder) continue;

        // 妫€鏌ユ槸鍚﹁兘瑙ｇ爜
        if (decoder->can_decode(file_path)) {
            plugin.release(); // 閲婃斁鎵€鏈夋潈
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
            // 鍒涘缓涓存椂瀹炰緥鑾峰彇鏀寔鐨勬牸寮?
            auto plugin = factory->create();
            auto decoder = dynamic_cast<IAudioDecoder*>(plugin.get());
            if (decoder) {
                auto exts = decoder->get_supported_extensions();
                for (const auto& ext : exts) {
                    unique_formats.insert(ext);
                }
            }
        } else {
            // 浠庢彃浠朵俊鎭幏鍙?
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