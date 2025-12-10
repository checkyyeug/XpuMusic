/**
 * @file config_manager.h
 * @brief JSON配置文件管理器
 * @date 2025-12-10
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>
#include <functional>

// Use simple map instead of nlohmann/json
using json_map = std::map<std::string, std::string>;

namespace xpumusic {

/**
 * @brief 音频配置
 */
struct AudioConfig {
    // 输出设备
    std::string output_device = "default";
    int sample_rate = 44100;
    int channels = 2;
    int bits_per_sample = 32;
    bool use_float = true;

    // 缓冲区配置
    int buffer_size = 4096;
    int buffer_count = 4;

    // 音量控制
    double volume = 1.0;
    bool mute = false;

    // 均衡器预设
    std::string equalizer_preset = "flat";
};

/**
 * @brief 插件配置
 */
struct PluginConfig {
    // 插件目录
    std::vector<std::string> plugin_directories = {
        "./plugins",
        "~/.xpumusic/plugins",
        "/usr/lib/xpumusic/plugins",
        "/usr/local/lib/xpumusic/plugins"
    };

    // 自动加载插件
    bool auto_load_plugins = true;

    // 插件扫描间隔（秒，0表示禁用）
    int plugin_scan_interval = 0;

    // 插件超时（毫秒）
    int plugin_timeout = 5000;
};

/**
 * @brief 重采样配置
 */
struct ResamplerConfig {
    // 质量模式：fast, good, high, best, adaptive
    std::string quality = "adaptive";

    // 浮点精度：32, 64
    int floating_precision = 32;

    // 自适应模式配置
    bool enable_adaptive = true;
    double cpu_threshold = 0.8;

    // 高质量模式配置
    bool use_anti_aliasing = true;
    double cutoff_ratio = 0.95;
    int filter_taps = 101;

    // 默认转换质量
    std::map<std::string, std::string> format_quality = {
        {"mp3", "good"},
        {"flac", "best"},
        {"wav", "fast"},
        {"ogg", "good"}
    };
};

/**
 * @brief 播放器配置
 */
struct PlayerConfig {
    // 播放模式
    bool repeat = false;
    bool shuffle = false;
    bool crossfade = false;
    double crossfade_duration = 2.0;

    // 跨平台音频后端选择
    std::string preferred_backend = "auto";  // auto, alsa, pulse, wasapi, coreaudio

    // 显示设置
    bool show_console_output = true;
    bool show_progress_bar = true;
    bool show_plugin_info = false;

    // 键盘快捷键
    std::unordered_map<std::string, std::string> key_bindings = {
        {"play", "space"},
        {"pause", "p"},
        {"stop", "s"},
        {"next", "n"},
        {"previous", "b"},
        {"quit", "q"}
    };

    // 播放历史
    int max_history = 1000;
    bool save_history = true;
    std::string history_file = "~/.xpumusic/history.json";
};

/**
 * @brief 日志配置
 */
struct LogConfig {
    // 日志级别：trace, debug, info, warn, error, fatal
    std::string level = "info";

    // 日志输出
    bool console_output = true;
    bool file_output = false;
    std::string log_file = "~/.xpumusic/xpumusic.log";

    // 日志轮转
    bool enable_rotation = true;
    size_t max_file_size = 10 * 1024 * 1024;  // 10MB
    int max_files = 5;

    // 日志格式
    bool include_timestamp = true;
    bool include_thread_id = false;
    bool include_function_name = true;
};

/**
 * @brief 界面配置
 */
struct UIConfig {
    // 主题
    std::string theme = "default";  // default, dark, light

    // 界面语言
    std::string language = "en";

    // 字体设置
    std::string font_family = "default";
    double font_size = 12.0;

    // 窗口设置
    bool save_window_size = true;
    int window_width = 1024;
    int window_height = 768;
    bool start_maximized = false;
};

/**
 * @brief 完整的应用程序配置
 */
struct AppConfig {
    // 基本信息
    std::string version = "2.0.0";
    std::string config_version = "1.0";

    // 各模块配置
    AudioConfig audio;
    PluginConfig plugins;
    ResamplerConfig resampler;
    PlayerConfig player;
    LogConfig logging;
    UIConfig ui;

    // 元数据
    std::string user_name;
    std::string default_music_directory = "~/Music";
    std::string playlist_directory = "~/.xpumusic/playlists";

    // 网络配置
    bool enable_network = false;
    bool check_updates = true;
    std::string update_server = "https://api.xpumusic.com";
};

/**
 * @brief 配置管理器
 *
 * 负责加载、保存和管理JSON配置文件
 */
class ConfigManager {
private:
    AppConfig config_;
    std::string config_file_path_;
    bool is_loaded_;

    // 配置变更回调
    std::vector<std::function<void(const AppConfig&)>> change_callbacks_;

public:
    ConfigManager();
    ~ConfigManager();

    // 初始化
    bool initialize(const std::string& config_file = "");

    // 加载配置
    bool load_config();

    // 保存配置
    bool save_config();

    // 重新加载配置
    bool reload_config();

    // 配置访问
    const AppConfig& get_config() const { return config_; }

    // 模块配置访问
    AudioConfig& audio() { return config_.audio; }
    PluginConfig& plugins() { return config_.plugins; }
    ResamplerConfig& resampler() { return config_.resampler; }
    PlayerConfig& player() { return config_.player; }
    LogConfig& logging() { return config_.logging; }
    UIConfig& ui() { return config_.ui; }

    // 便捷方法
    std::string get_config_file_path() const { return config_file_path_; }
    bool is_loaded() const { return is_loaded_; }

    // 配置变更通知
    void add_change_callback(std::function<void(const AppConfig&)> callback);
    void notify_change();

    // 配置验证
    bool validate_config(AppConfig& config) const;

    // 配置重置
    void reset_to_defaults();

    // 配置合并
    bool merge_config(const std::string& other_config_file);

    // 配置导入/导出
    bool export_config(const std::string& file_path) const;
    bool import_config(const std::string& file_path);

    // 环境变量支持
    void load_from_environment();
    void apply_environment_overrides();

    // 路径解析（支持~展开）
    static std::string expand_path(const std::string& path);

    // 配置项设置/获取（模板方法）
    template<typename T>
    void set_config_value(const std::string& path, const T& value);

    template<typename T>
    T get_config_value(const std::string& path, const T& default_value = T{}) const;

private:
    // 内部辅助方法
    nlohmann::json config_to_json() const;
    void json_to_config(const nlohmann::json& json);
    bool load_json_from_file(const std::string& path, nlohmann::json& json);
    bool save_json_to_file(const std::string& path, const nlohmann::json& json);

    // 配置文件路径获取
    std::string get_default_config_path() const;

    // 路径清理
    std::string clean_path(const std::string& path) const;
};

// 全局配置管理器实例
class ConfigManagerSingleton {
private:
    static std::unique_ptr<ConfigManager> instance_;
    static bool initialized_;

public:
    static ConfigManager& get_instance();
    static bool initialize(const std::string& config_file = "");
    static void shutdown();
};

// 便利宏
#define CONFIG ConfigManagerSingleton::get_instance()

} // namespace xpumusic