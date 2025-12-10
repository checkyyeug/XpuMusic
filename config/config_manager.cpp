/**
 * @file config_manager.cpp
 * @brief JSON配置文件管理器实现
 * @date 2025-12-10
 */

#include "config_manager.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <pwd.h>
#include <unistd.h>

namespace xpumusic {

// 静态成员初始化
std::unique_ptr<ConfigManager> ConfigManagerSingleton::instance_;
bool ConfigManagerSingleton::initialized_ = false;

ConfigManager::ConfigManager() : is_loaded_(false) {
}

ConfigManager::~ConfigManager() {
    if (is_loaded_) {
        save_config();
    }
}

bool ConfigManager::initialize(const std::string& config_file) {
    if (!config_file.empty()) {
        config_file_path_ = expand_path(config_file);
    } else {
        config_file_path_ = get_default_config_path();
    }

    // 确保配置目录存在
    std::filesystem::path config_dir = std::filesystem::path(config_file_path_).parent_path();
    if (!std::filesystem::exists(config_dir)) {
        try {
            std::filesystem::create_directories(config_dir);
            std::cout << "Created config directory: " << config_dir << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to create config directory: " << e.what() << std::endl;
            return false;
        }
    }

    // 尝试加载现有配置
    if (!load_config()) {
        std::cout << "Config file not found or invalid, creating default config..." << std::endl;
        reset_to_defaults();
        save_config();
    }

    // 应用环境变量覆盖
    apply_environment_overrides();

    is_loaded_ = true;
    return true;
}

bool ConfigManager::load_config() {
    nlohmann::json json;
    if (!load_json_from_file(config_file_path_, json)) {
        return false;
    }

    try {
        json_to_config(json);

        // 验证配置
        if (!validate_config(config_)) {
            std::cerr << "Invalid configuration detected, using defaults" << std::endl;
            reset_to_defaults();
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::save_config() {
    if (config_file_path_.empty()) {
        return false;
    }

    try {
        nlohmann::json json = config_to_json();
        return save_json_to_file(config_file_path_, json);
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::reload_config() {
    std::string old_config_file = config_file_path_;
    is_loaded_ = false;

    bool success = initialize(old_config_file);

    if (success) {
        notify_change();
    }

    return success;
}

void ConfigManager::add_change_callback(std::function<void(const AppConfig&)> callback) {
    change_callbacks_.push_back(callback);
}

void ConfigManager::notify_change() {
    for (const auto& callback : change_callbacks_) {
        try {
            callback(config_);
        } catch (const std::exception& e) {
            std::cerr << "Error in config change callback: " << e.what() << std::endl;
        }
    }
}

bool ConfigManager::validate_config(AppConfig& config) const {
    // 验证音频配置
    if (config.audio.sample_rate <= 0 || config.audio.sample_rate > 768000) {
        std::cerr << "Invalid sample rate: " << config.audio.sample_rate << std::endl;
        return false;
    }
    if (config.audio.channels < 1 || config.audio.channels > 8) {
        std::cerr << "Invalid channel count: " << config.audio.channels << std::endl;
        return false;
    }
    if (config.audio.bits_per_sample != 16 &&
        config.audio.bits_per_sample != 24 &&
        config.audio.bits_per_sample != 32) {
        std::cerr << "Invalid bits per sample: " << config.audio.bits_per_sample << std::endl;
        return false;
    }

    // 验证缓冲区配置
    if (config.audio.buffer_size <= 0 || config.audio.buffer_size > 65536) {
        std::cerr << "Invalid buffer size: " << config.audio.buffer_size << std::endl;
        return false;
    }

    // 验证音量
    if (config.audio.volume < 0.0 || config.audio.volume > 2.0) {
        std::cerr << "Invalid volume: " << config.audio.volume << std::endl;
        return false;
    }

    // 验证插件配置
    for (const auto& dir : config.plugins.plugin_directories) {
        std::string expanded_dir = expand_path(dir);
        if (!std::filesystem::exists(expanded_dir)) {
            // 尝试创建目录
            try {
                std::filesystem::create_directories(expanded_dir);
            } catch (...) {
                // 忽略错误，可能只是权限问题
            }
        }
    }

    // 验证重采样配置
    const std::vector<std::string> valid_qualities = {
        "fast", "good", "high", "best", "adaptive"
    };
    if (std::find(valid_qualities.begin(), valid_qualities.end(),
                 config.resampler.quality) == valid_qualities.end()) {
        std::cerr << "Invalid resampler quality: " << config.resampler.quality << std::endl;
        return false;
    }

    // 验证浮点精度
    if (config.resampler.floating_precision != 32 && config.resampler.floating_precision != 64) {
        std::cerr << "Invalid floating precision: " << config.resampler.floating_precision
                  << ". Must be 32 or 64." << std::endl;
        return false;
    }

    // 验证CPU阈值
    if (config.resampler.cpu_threshold < 0.0 || config.resampler.cpu_threshold > 1.0) {
        std::cerr << "Invalid CPU threshold: " << config.resampler.cpu_threshold
                  << ". Must be between 0.0 and 1.0." << std::endl;
        return false;
    }

    // 验证滤波器抽头数
    if (config.resampler.filter_taps < 3 || config.resampler.filter_taps > 999) {
        std::cerr << "Invalid filter taps: " << config.resampler.filter_taps
                  << ". Must be between 3 and 999." << std::endl;
        return false;
    }

    // 验证截止频率比
    if (config.resampler.cutoff_ratio < 0.1 || config.resampler.cutoff_ratio > 0.99) {
        std::cerr << "Invalid cutoff ratio: " << config.resampler.cutoff_ratio
                  << ". Must be between 0.1 and 0.99." << std::endl;
        return false;
    }

    return true;
}

void ConfigManager::reset_to_defaults() {
    config_ = AppConfig();

    // 设置一些合理的默认值
    config_.audio.output_device = "default";
    config_.audio.sample_rate = 44100;
    config_.audio.channels = 2;
    config_.audio.bits_per_sample = 32;
    config_.audio.use_float = true;
    config_.audio.buffer_size = 4096;
    config_.audio.volume = 0.8;  // 默认80%音量

    config_.resampler.quality = "adaptive";
    config_.resampler.enable_adaptive = true;
    config_.resampler.cpu_threshold = 0.8;

    config_.player.repeat = false;
    config_.player.shuffle = false;
    config_.player.crossfade = false;
    config_.player.show_console_output = true;
    config_.player.show_progress_bar = true;

    config_.logging.level = "info";
    config_.logging.console_output = true;
    config_.logging.file_output = false;

    // 获取用户信息
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        config_.user_name = pw->pw_name;
        if (pw->pw_dir) {
            config_.default_music_directory = std::string(pw->pw_dir) + "/Music";
            config_.config_file_path = std::string(pw->pw_dir) + "/.xpumusic/config.json";
        }
    }
}

bool ConfigManager::merge_config(const std::string& other_config_file) {
    nlohmann::json other_json;
    if (!load_json_from_file(other_config_file, other_json)) {
        return false;
    }

    try {
        AppConfig other_config;
        json_to_config(other_json);

        // 合并配置（other_config覆盖当前配置）
        if (!validate_config(other_config)) {
            return false;
        }

        config_ = other_config;
        save_config();
        notify_change();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error merging config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::export_config(const std::string& file_path) const {
    try {
        nlohmann::json json = config_to_json();
        json["exported_at"] = std::time(nullptr);
        json["exported_by"] = "XpuMusic v" + config_.version;

        std::string expanded_path = expand_path(file_path);
        return save_json_to_file(expanded_path, json);
    } catch (const std::exception& e) {
        std::cerr << "Error exporting config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::import_config(const std::string& file_path) {
    return merge_config(file_path);
}

void ConfigManager::load_from_environment() {
    // 音频配置
    if (const char* env = std::getenv("XPUMUSIC_AUDIO_OUTPUT_DEVICE")) {
        config_.audio.output_device = env;
    }
    if (const char* env = std::getenv("XPUMUSIC_SAMPLE_RATE")) {
        config_.audio.sample_rate = std::stoi(env);
    }
    if (const char* env = std::getenv("XPUMUSIC_VOLUME")) {
        config_.audio.volume = std::stod(env);
    }

    // 插件配置
    if (const char* env = std::getenv("XPUMUSIC_PLUGIN_DIR")) {
        config_.plugins.plugin_directories.clear();
        config_.plugins.plugin_directories.push_back(env);
    }

    // 播放器配置
    if (const char* env = std::getenv("XPUMUSIC_MUSIC_DIR")) {
        config_.player.default_music_directory = env;
    }

    // 日志配置
    if (const char* env = std::getenv("XPUMUSIC_LOG_LEVEL")) {
        config_.logging.level = env;
    }
}

void ConfigManager::apply_environment_overrides() {
    load_from_environment();

    // 命令行参数覆盖可以在这里添加
    // 例如：parse_command_line_args(argc, argv);
}

std::string ConfigManager::expand_path(const std::string& path) {
    if (path.empty()) return path;

    std::string result = path;

    // 展开~
    if (result[0] == '~') {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            result.replace(0, 1, pw->pw_dir);
        }
    }

    // 展开$HOME
    if (result[0] == '$' && result.substr(0, 5) == "$HOME") {
        if (const char* home = std::getenv("HOME")) {
            result.replace(0, 5, home);
        }
    }

    return result;
}

nlohmann::json ConfigManager::config_to_json() const {
    nlohmann::json j;

    j["version"] = config_.version;
    j["config_version"] = config_.config_version;

    // 音频配置
    j["audio"] = {
        {"output_device", config_.audio.output_device},
        {"sample_rate", config_.audio.sample_rate},
        {"channels", config_.audio.channels},
        {"bits_per_sample", config_.audio.bits_per_sample},
        {"use_float", config_.audio.use_float},
        {"buffer_size", config_.audio.buffer_size},
        {"buffer_count", config_.audio.buffer_count},
        {"volume", config_.audio.volume},
        {"mute", config_.audio.mute},
        {"equalizer_preset", config_.audio.equalizer_preset}
    };

    // 插件配置
    j["plugins"] = {
        {"plugin_directories", config_.plugins.plugin_directories},
        {"auto_load_plugins", config_.plugins.auto_load_plugins},
        {"plugin_scan_interval", config_.plugins.plugin_scan_interval},
        {"plugin_timeout", config_.plugins.plugin_timeout}
    };

    // 重采样配置
    j["resampler"] = {
        {"quality", config_.resampler.quality},
        {"floating_precision", config_.resampler.floating_precision},
        {"enable_adaptive", config_.resampler.enable_adaptive},
        {"cpu_threshold", config_.resampler.cpu_threshold},
        {"use_anti_aliasing", config_.resampler.use_anti_aliasing},
        {"cutoff_ratio", config_.resampler.cutoff_ratio},
        {"filter_taps", config_.resampler.filter_taps},
        {"format_quality", config_.resampler.format_quality}
    };

    // 播放器配置
    j["player"] = {
        {"repeat", config_.player.repeat},
        {"shuffle", config_.player.shuffle},
        {"crossfade", config_.player.crossfade},
        {"crossfade_duration", config_.player.crossfade_duration},
        {"preferred_backend", config_.player.preferred_backend},
        {"show_console_output", config_.player.show_console_output},
        {"show_progress_bar", config_.player.show_progress_bar},
        {"show_plugin_info", config_.player.show_plugin_info},
        {"key_bindings", config_.player.key_bindings},
        {"max_history", config_.player.max_history},
        {"save_history", config_.player.save_history},
        {"history_file", config_.player.history_file}
    };

    // 日志配置
    j["logging"] = {
        {"level", config_.logging.level},
        {"console_output", config_.logging.console_output},
        {"file_output", config_.logging.file_output},
        {"log_file", config_.logging.log_file},
        {"enable_rotation", config_.logging.enable_rotation},
        {"max_file_size", config_.logging.max_file_size},
        {"max_files", config_.logging.max_files},
        {"include_timestamp", config_.logging.include_timestamp},
        {"include_thread_id", config_.logging.include_thread_id},
        {"include_function_name", config_.logging.include_function_name}
    };

    // UI配置
    j["ui"] = {
        {"theme", config_.ui.theme},
        {"language", config_.ui.language},
        {"font_family", config_.ui.font_family},
        {"font_size", config_.ui.font_size},
        {"save_window_size", config_.ui.save_window_size},
        {"window_width", config_.ui.window_width},
        {"window_height", config_.ui.window_height},
        {"start_maximized", config_.ui.start_maximized}
    };

    // 元数据
    j["metadata"] = {
        {"user_name", config_.user_name},
        {"default_music_directory", config_.default_music_directory},
        {"playlist_directory", config_.playlist_directory}
    };

    // 网络配置
    j["network"] = {
        {"enable_network", config_.enable_network},
        {"check_updates", config_.check_updates},
        {"update_server", config_.update_server}
    };

    return j;
}

void ConfigManager::json_to_config(const nlohmann::json& json) {
    // 基本信息
    if (json.contains("version")) {
        config_.version = json["version"];
    }
    if (json.contains("config_version")) {
        config_.config_version = json["config_version"];
    }

    // 音频配置
    if (json.contains("audio")) {
        const auto& audio = json["audio"];
        if (audio.contains("output_device")) {
            config_.audio.output_device = audio["output_device"];
        }
        if (audio.contains("sample_rate")) {
            config_.audio.sample_rate = audio["sample_rate"];
        }
        if (audio.contains("channels")) {
            config_.audio.channels = audio["channels"];
        }
        if (audio.contains("bits_per_sample")) {
            config_.audio.bits_per_sample = audio["bits_per_sample"];
        }
        if (audio.contains("use_float")) {
            config_.audio.use_float = audio["use_float"];
        }
        if (audio.contains("buffer_size")) {
            config_.audio.buffer_size = audio["buffer_size"];
        }
        if (audio.contains("buffer_count")) {
            config_.audio.buffer_count = audio["buffer_count"];
        }
        if (audio.contains("volume")) {
            config_.audio.volume = audio["volume"];
        }
        if (audio.contains("mute")) {
            config_.audio.mute = audio["mute"];
        }
        if (audio.contains("equalizer_preset")) {
            config_.audio.equalizer_preset = audio["equalizer_preset"];
        }
    }

    // 插件配置
    if (json.contains("plugins")) {
        const auto& plugins = json["plugins"];
        if (plugins.contains("plugin_directories")) {
            config_.plugins.plugin_directories = plugins["plugin_directories"];
        }
        if (plugins.contains("auto_load_plugins")) {
            config_.plugins.auto_load_plugins = plugins["auto_load_plugins"];
        }
        if (plugins.contains("plugin_scan_interval")) {
            config_.plugins.plugin_scan_interval = plugins["plugin_scan_interval"];
        }
        if (plugins.contains("plugin_timeout")) {
            config_.plugins.plugin_timeout = plugins["plugin_timeout"];
        }
    }

    // 重采样配置
    if (json.contains("resampler")) {
        const auto& resampler = json["resampler"];
        if (resampler.contains("quality")) {
            config_.resampler.quality = resampler["quality"];
        }
        if (resampler.contains("floating_precision")) {
            config_.resampler.floating_precision = resampler["floating_precision"];
        }
        if (resampler.contains("enable_adaptive")) {
            config_.resampler.enable_adaptive = resampler["enable_adaptive"];
        }
        if (resampler.contains("cpu_threshold")) {
            config_.resampler.cpu_threshold = resampler["cpu_threshold"];
        }
        if (resampler.contains("use_anti_aliasing")) {
            config_.resampler.use_anti_aliasing = resampler["use_anti_aliasing"];
        }
        if (resampler.contains("cutoff_ratio")) {
            config_.resampler.cutoff_ratio = resampler["cutoff_ratio"];
        }
        if (resampler.contains("filter_taps")) {
            config_.resampler.filter_taps = resampler["filter_taps"];
        }
        if (resampler.contains("format_quality")) {
            config_.resampler.format_quality = resampler["format_quality"];
        }
    }

    // 播放器配置
    if (json.contains("player")) {
        const auto& player = json["player"];
        if (player.contains("repeat")) {
            config_.player.repeat = player["repeat"];
        }
        if (player.contains("shuffle")) {
            config_.player.shuffle = player["shuffle"];
        }
        if (player.contains("crossfade")) {
            config_.player.crossfade = player["crossfade"];
        }
        if (player.contains("crossfade_duration")) {
            config_.player.crossfade_duration = player["crossfade_duration"];
        }
        if (player.contains("preferred_backend")) {
            config_.player.preferred_backend = player["preferred_backend"];
        }
        if (player.contains("show_console_output")) {
            config_.player.show_console_output = player["show_console_output"];
        }
        if (player.contains("show_progress_bar")) {
            config_.player.show_progress_bar = player["show_progress_bar"];
        }
        if (player.contains("show_plugin_info")) {
            config_.player.show_plugin_info = player["show_plugin_info"];
        }
        if (player.contains("key_bindings")) {
            config_.player.key_bindings = player["key_bindings"];
        }
        if (player.contains("max_history")) {
            config_.player.max_history = player["max_history"];
        }
        if (player.contains("save_history")) {
            config_.player.save_history = player["save_history"];
        }
        if (player.contains("history_file")) {
            config_.player.history_file = player["history_file"];
        }
    }

    // 日志配置
    if (json.contains("logging")) {
        const auto& logging = json["logging"];
        if (logging.contains("level")) {
            config_.logging.level = logging["level"];
        }
        if (logging.contains("console_output")) {
            config_.logging.console_output = logging["console_output"];
        }
        if (logging.contains("file_output")) {
            config_.logging.file_output = logging["file_output"];
        }
        if (logging.contains("log_file")) {
            config_.logging.log_file = logging["log_file"];
        }
        if (logging.contains("enable_rotation")) {
            config_.logging.enable_rotation = logging["enable_rotation"];
        }
        if (logging.contains("max_file_size")) {
            config_.logging.max_file_size = logging["max_file_size"];
        }
        if (logging.contains("max_files")) {
            config_.logging.max_files = logging["max_files"];
        }
        if (logging.contains("include_timestamp")) {
            config_.logging.include_timestamp = logging["include_timestamp"];
        }
        if (logging.contains("include_thread_id")) {
            config_.logging.include_thread_id = logging["include_thread_id"];
        }
        if (logging.contains("include_function_name")) {
            config_.logging.include_function_name = logging["include_function_name"];
        }
    }

    // UI配置
    if (json.contains("ui")) {
        const auto& ui = json["ui"];
        if (ui.contains("theme")) {
            config_.ui.theme = ui["theme"];
        }
        if (ui.contains("language")) {
            config_.ui.language = ui["language"];
        }
        if (ui.contains("font_family")) {
            config_.ui.font_family = ui["font_family"];
        }
        if (ui.contains("font_size")) {
            config_.ui.font_size = ui["font_size"];
        }
        if (ui.contains("save_window_size")) {
            config_.ui.save_window_size = ui["save_window_size"];
        }
        if (ui.contains("window_width")) {
            config_.ui.window_width = ui["window_width"];
        }
        if (ui.contains("window_height")) {
            config_.ui.window_height = ui["window_height"];
        }
        if (ui.contains("start_maximized")) {
            config_.ui.start_maximized = ui["start_maximized"];
        }
    }

    // 元数据
    if (json.contains("metadata")) {
        const auto& metadata = json["metadata"];
        if (metadata.contains("user_name")) {
            config_.user_name = metadata["user_name"];
        }
        if (metadata.contains("default_music_directory")) {
            config_.default_music_directory = metadata["default_music_directory"];
        }
        if (metadata.contains("playlist_directory")) {
            config_.playlist_directory = metadata["playlist_directory"];
        }
    }

    // 网络配置
    if (json.contains("network")) {
        const auto& network = json["network"];
        if (network.contains("enable_network")) {
            config_.enable_network = network["enable_network"];
        }
        if (network.contains("check_updates")) {
            config_.check_updates = network["check_updates"];
        }
        if (network.contains("update_server")) {
            config_.update_server = network["update_server"];
        }
    }
}

bool ConfigManager::load_json_from_file(const std::string& path, nlohmann::json& json) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        file >> json;
        return !file.fail() && !json.empty();
    } catch (const std::exception&) {
        return false;
    }
}

bool ConfigManager::save_json_to_file(const std::string& path, const nlohmann::json& json) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << path << std::endl;
        return false;
    }

    try {
        file << json.dump(4);  // 缩进4个空格，提高可读性
        return file.good();
    } catch (const std::exception& e) {
        std::cerr << "Error writing JSON: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::get_default_config_path() const {
    struct passwd *pw = getpwuid(getuid());
    if (!pw || !pw->pw_dir) {
        return "./config.json";
    }

    return std::string(pw->pw_dir) + "/.xpumusic/config.json";
}

std::string ConfigManager::clean_path(const std::string& path) const {
    // 移除多余的斜杠
    std::string result = path;
    size_t pos = 0;
    while ((pos = result.find("//", pos)) != std::string::npos) {
        result.replace(pos, 2, "/");
    }
    return result;
}

// ConfigManagerSingleton 实现
ConfigManager& ConfigManagerSingleton::get_instance() {
    if (!instance_) {
        instance_ = std::make_unique<ConfigManager>();
    }
    return *instance_;
}

bool ConfigManagerSingleton::initialize(const std::string& config_file) {
    if (!instance_) {
        instance_ = std::make_unique<ConfigManager>();
    }

    bool success = instance_->initialize(config_file);
    initialized_ = success;
    return success;
}

void ConfigManagerSingleton::shutdown() {
    instance_.reset();
    initialized_ = false;
}

} // namespace xpumusic