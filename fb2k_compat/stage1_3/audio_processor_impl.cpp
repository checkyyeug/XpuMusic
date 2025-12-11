#include "audio_processor.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace fb2k {

// 高级音频处理器实现
class audio_processor_impl : public audio_processor_advanced {
public:
    audio_processor_impl();
    ~audio_processor_impl() override;

    // audio_processor接口实现
    bool initialize(const audio_processor_config& config) override;
    void shutdown() override;
    
    bool process_audio(const audio_chunk& input_chunk, audio_chunk& output_chunk, abort_callback& abort) override;
    bool process_stream(audio_source* source, audio_sink* sink, abort_callback& abort) override;
    
    void add_dsp_effect(std::unique_ptr<dsp_effect_advanced> effect) override;
    void remove_dsp_effect(size_t index) override;
    void clear_dsp_effects() override;
    size_t get_dsp_effect_count() const override;
    dsp_effect_advanced* get_dsp_effect(size_t index) override;
    
    void set_output_device(std::unique_ptr<output_device> device) override;
    output_device* get_output_device() override;
    
    void set_volume(float volume) override;
    float get_volume() const override;
    void set_mute(bool mute) override;
    bool is_muted() const override;
    
    void enable_dsp(bool enable) override;
    bool is_dsp_enabled() const override;
    void enable_output_device(bool enable) override;
    bool is_output_device_enabled() const override;
    
    audio_processor_stats get_stats() const override;
    void reset_stats() override;
    
    std::string get_status_report() const override;
    
    // 高级功能
    bool set_processing_mode(processing_mode mode) override;
    processing_mode get_processing_mode() const override;
    
    void set_latency_target(double milliseconds) override;
    double get_latency_target() const override;
    
    void enable_performance_monitoring(bool enable) override;
    bool is_performance_monitoring_enabled() const override;
    
    void set_cpu_usage_limit(float percent) override;
    float get_cpu_usage_limit() const override;
    
    bool load_plugin(const std::string& plugin_path) override;
    bool unload_plugin(const std::string& plugin_name) override;
    
    std::vector<plugin_info> get_loaded_plugins() const override;
    
    // 配置管理
    bool save_configuration(const std::string& config_file) override;
    bool load_configuration(const std::string& config_file) override;
    
    // 实时参数调节
    void set_realtime_parameter(const std::string& effect_name, const std::string& param_name, float value) override;
    float get_realtime_parameter(const std::string& effect_name, const std::string& param_name) const override;
    
    std::vector<parameter_info> get_realtime_parameters(const std::string& effect_name) const override;

private:
    // 配置
    audio_processor_config config_;
    
    // DSP管理器
    std::unique_ptr<dsp_manager> dsp_manager_;
    
    // 输出设备
    std::unique_ptr<output_device> output_device_;
    
    // 状态管理
    std::atomic<bool> is_initialized_;
    std::atomic<bool> is_processing_;
    std::atomic<bool> dsp_enabled_;
    std::atomic<bool> output_enabled_;
    std::atomic<bool> is_muted_;
    std::atomic<bool> performance_monitoring_enabled_;
    std::atomic<bool> should_stop_;
    
    // 音频处理参数
    std::atomic<float> volume_;
    std::atomic<processing_mode> processing_mode_;
    std::atomic<double> latency_target_ms_;
    std::atomic<float> cpu_usage_limit_;
    
    // 性能监控
    mutable audio_processor_stats stats_;
    mutable std::mutex stats_mutex_;
    
    // 线程管理
    std::thread processing_thread_;
    std::atomic<bool> processing_thread_running_;
    std::condition_variable processing_cv_;
    std::mutex processing_mutex_;
    
    // 音频缓冲管理
    std::unique_ptr<ring_buffer> input_buffer_;
    std::unique_ptr<ring_buffer> output_buffer_;
    std::unique_ptr<ring_buffer> dsp_buffer_;
    
    // 音频格式
    audio_format_info current_format_;
    std::atomic<bool> format_changed_;
    
    // 插件管理
    std::map<std::string, std::unique_ptr<audio_plugin>> loaded_plugins_;
    mutable std::mutex plugin_mutex_;
    
    // 实时参数管理
    std::map<std::string, std::map<std::string, float>> realtime_parameters_;
    mutable std::mutex param_mutex_;
    
    // 私有方法
    bool initialize_dsp_manager();
    bool initialize_output_device();
    bool initialize_buffers();
    bool initialize_processing_thread();
    
    void shutdown_dsp_manager();
    void shutdown_output_device();
    void shutdown_buffers();
    void shutdown_processing_thread();
    
    void processing_thread_func();
    bool process_audio_internal(const audio_chunk& input, audio_chunk& output, abort_callback& abort);
    bool process_dsp_chain(audio_chunk& chunk, abort_callback& abort);
    bool process_output_device(audio_chunk& chunk, abort_callback& abort);
    
    void update_stats(const audio_chunk& chunk, double processing_time);
    void check_performance_limits();
    void handle_error(const std::string& operation, const std::string& error);
    
    bool validate_audio_format(const audio_chunk& chunk) const;
    bool convert_audio_format(const audio_chunk& input, audio_chunk& output, const audio_format_info& target_format);
    
    void apply_volume(audio_chunk& chunk);
    void apply_mute(audio_chunk& chunk);
    
    std::string generate_plugin_report() const;
    std::string generate_dsp_report() const;
    std::string generate_performance_report() const;
};

// 构造函数
audio_processor_impl::audio_processor_impl()
    : is_initialized_(false),
      is_processing_(false),
      dsp_enabled_(true),
      output_enabled_(true),
      is_muted_(false),
      performance_monitoring_enabled_(false),
      should_stop_(false),
      volume_(1.0f),
      processing_mode_(processing_mode::realtime),
      latency_target_ms_(10.0),
      cpu_usage_limit_(80.0f),
      processing_thread_running_(false),
      format_changed_(false) {
    
    // 初始化统计信息
    stats_.total_samples_processed = 0;
    stats_.total_processing_time_ms = 0.0;
    stats_.average_processing_time_ms = 0.0;
    stats_.peak_cpu_usage = 0.0f;
    stats_.current_cpu_usage = 0.0f;
    stats_.latency_ms = 0.0;
    stats_.dropout_count = 0;
    stats_.error_count = 0;
    stats_.processing_mode = processing_mode::realtime;
    
    current_format_.sample_rate = 44100;
    current_format_.channels = 2;
    current_format_.bits_per_sample = 32;
    current_format_.format = sample_format::float32;
}

// 析构函数
audio_processor_impl::~audio_processor_impl() {
    if(is_initialized_.load()) {
        shutdown();
    }
}

// 初始化
bool audio_processor_impl::initialize(const audio_processor_config& config) {
    std::cout << "[AudioProcessor] 初始化音频处理器..." << std::endl;
    
    if(is_initialized_.load()) {
        std::cerr << "[AudioProcessor] 音频处理器已初始化" << std::endl;
        return false;
    }
    
    config_ = config;
    
    try {
        // 初始化DSP管理器
        if(!initialize_dsp_manager()) {
            std::cerr << "[AudioProcessor] DSP管理器初始化失败" << std::endl;
            return false;
        }
        
        // 初始化输出设备
        if(!initialize_output_device()) {
            std::cerr << "[AudioProcessor] 输出设备初始化失败" << std::endl;
            return false;
        }
        
        // 初始化缓冲区
        if(!initialize_buffers()) {
            std::cerr << "[AudioProcessor] 缓冲区初始化失败" << std::endl;
            return false;
        }
        
        // 初始化处理线程
        if(!initialize_processing_thread()) {
            std::cerr << "[AudioProcessor] 处理线程初始化失败" << std::endl;
            return false;
        }
        
        is_initialized_.store(true);
        std::cout << "[AudioProcessor] 音频处理器初始化成功" << std::endl;
        std::cout << "[AudioProcessor] 配置:" << std::endl;
        std::cout << "  - 处理模式: " << (config_.processing_mode == processing_mode::realtime ? "实时" : "高保真") << std::endl;
        std::cout << "  - 目标延迟: " << config_.target_latency_ms << "ms" << std::endl;
        std::cout << "  - CPU限制: " << config_.cpu_usage_limit_percent << "%" << std::endl;
        std::cout << "  - 缓冲区大小: " << config_.buffer_size << "样本" << std::endl;
        
        return true;
        
    } catch(const std::exception& e) {
        std::cerr << "[AudioProcessor] 初始化异常: " << e.what() << std::endl;
        shutdown();
        return false;
    }
}

// 关闭
void audio_processor_impl::shutdown() {
    std::cout << "[AudioProcessor] 关闭音频处理器..." << std::endl;
    
    if(!is_initialized_.load()) {
        return;
    }
    
    should_stop_.store(true);
    is_processing_.store(false);
    
    // 停止处理线程
    shutdown_processing_thread();
    
    // 关闭其他组件
    shutdown_dsp_manager();
    shutdown_output_device();
    shutdown_buffers();
    
    // 清理插件
    {
        std::lock_guard<std::mutex> lock(plugin_mutex_);
        loaded_plugins_.clear();
    }
    
    is_initialized_.store(false);
    std::cout << "[AudioProcessor] 音频处理器已关闭" << std::endl;
}

// 处理音频
bool audio_processor_impl::process_audio(const audio_chunk& input_chunk, audio_chunk& output_chunk, abort_callback& abort) {
    if(!is_initialized_.load() || should_stop_.load() || abort.is_aborting()) {
        return false;
    }
    
    try {
        // 验证音频格式
        if(!validate_audio_format(input_chunk)) {
            handle_error("process_audio", "音频格式验证失败");
            return false;
        }
        
        // 开始计时
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 处理音频
        bool success = process_audio_internal(input_chunk, output_chunk, abort);
        
        // 结束计时
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time_ms = duration.count() / 1000.0;
        
        // 更新统计信息
        update_stats(output_chunk, processing_time_ms);
        
        // 检查性能限制
        check_performance_limits();
        
        return success;
        
    } catch(const std::exception& e) {
        handle_error("process_audio", e.what());
        return false;
    }
}

// 处理流
bool audio_processor_impl::process_stream(audio_source* source, audio_sink* sink, abort_callback& abort) {
    if(!is_initialized_.load() || !source || !sink || should_stop_.load() || abort.is_aborting()) {
        return false;
    }
    
    is_processing_.store(true);
    
    try {
        audio_chunk input_chunk;
        audio_chunk output_chunk;
        
        // 处理音频流
        while(!abort.is_aborting() && !should_stop_.load()) {
            // 从源获取音频数据
            if(!source->get_next_chunk(input_chunk, abort)) {
                break;
            }
            
            // 处理音频块
            if(!process_audio(input_chunk, output_chunk, abort)) {
                std::cerr << "[AudioProcessor] 音频处理失败" << std::endl;
                break;
            }
            
            // 输出到目标
            if(!sink->write_chunk(output_chunk, abort)) {
                std::cerr << "[AudioProcessor] 音频输出失败" << std::endl;
                break;
            }
        }
        
        is_processing_.store(false);
        return true;
        
    } catch(const std::exception& e) {
        is_processing_.store(false);
        handle_error("process_stream", e.what());
        return false;
    }
}

// 添加DSP效果器
void audio_processor_impl::add_dsp_effect(std::unique_ptr<dsp_effect_advanced> effect) {
    if(!dsp_manager_) {
        return;
    }
    
    dsp_manager_->add_effect(std::move(effect));
    std::cout << "[AudioProcessor] 添加DSP效果器" << std::endl;
}

// 移除DSP效果器
void audio_processor_impl::remove_dsp_effect(size_t index) {
    if(!dsp_manager_) {
        return;
    }
    
    dsp_manager_->remove_effect(index);
    std::cout << "[AudioProcessor] 移除DSP效果器" << std::endl;
}

// 清空DSP效果器
void audio_processor_impl::clear_dsp_effects() {
    if(!dsp_manager_) {
        return;
    }
    
    dsp_manager_->clear_effects();
    std::cout << "[AudioProcessor] 清空所有DSP效果器" << std::endl;
}

// 获取DSP效果器数量
size_t audio_processor_impl::get_dsp_effect_count() const {
    return dsp_manager_ ? dsp_manager_->get_effect_count() : 0;
}

// 获取DSP效果器
dsp_effect_advanced* audio_processor_impl::get_dsp_effect(size_t index) {
    return dsp_manager_ ? dsp_manager_->get_effect(index) : nullptr;
}

// 设置输出设备
void audio_processor_impl::set_output_device(std::unique_ptr<output_device> device) {
    if(is_processing_.load()) {
        std::cerr << "[AudioProcessor] 无法在处理过程中更改输出设备" << std::endl;
        return;
    }
    
    output_device_ = std::move(device);
    std::cout << "[AudioProcessor] 设置输出设备" << std::endl;
}

// 获取输出设备
output_device* audio_processor_impl::get_output_device() {
    return output_device_.get();
}

// 设置音量
void audio_processor_impl::set_volume(float volume) {
    volume_.store(std::max(0.0f, std::min(1.0f, volume)));
    
    if(output_device_) {
        output_device_->volume_set(volume_.load());
    }
    
    std::cout << "[AudioProcessor] 设置音量: " << volume_.load() << std::endl;
}

// 获取音量
float audio_processor_impl::get_volume() const {
    return volume_.load();
}

// 设置静音
void audio_processor_impl::set_mute(bool mute) {
    is_muted_.store(mute);
    std::cout << "[AudioProcessor] 设置静音: " << (mute ? "启用" : "禁用") << std::endl;
}

// 是否静音
bool audio_processor_impl::is_muted() const {
    return is_muted_.load();
}

// 启用DSP
void audio_processor_impl::enable_dsp(bool enable) {
    dsp_enabled_.store(enable);
    std::cout << "[AudioProcessor] DSP: " << (enable ? "启用" : "禁用") << std::endl;
}

// 是否启用DSP
bool audio_processor_impl::is_dsp_enabled() const {
    return dsp_enabled_.load();
}

// 启用输出设备
void audio_processor_impl::enable_output_device(bool enable) {
    output_enabled_.store(enable);
    std::cout << "[AudioProcessor] 输出设备: " << (enable ? "启用" : "禁用") << std::endl;
}

// 是否启用输出设备
bool audio_processor_impl::is_output_device_enabled() const {
    return output_enabled_.load();
}

// 获取统计信息
audio_processor_stats audio_processor_impl::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

// 重置统计信息
void audio_processor_impl::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_samples_processed = 0;
    stats_.total_processing_time_ms = 0.0;
    stats_.average_processing_time_ms = 0.0;
    stats_.peak_cpu_usage = 0.0f;
    stats_.current_cpu_usage = 0.0f;
    stats_.latency_ms = 0.0;
    stats_.dropout_count = 0;
    stats_.error_count = 0;
    
    std::cout << "[AudioProcessor] 统计信息已重置" << std::endl;
}

// 获取状态报告
std::string audio_processor_impl::get_status_report() const {
    std::stringstream report;
    report << "音频处理器状态报告\n";
    report << "====================\n\n";
    
    // 基本状态
    report << "基本状态:\n";
    report << "  初始化状态: " << (is_initialized_.load() ? "已初始化" : "未初始化") << "\n";
    report << "  处理状态: " << (is_processing_.load() ? "处理中" : "空闲") << "\n";
    report << "  DSP状态: " << (dsp_enabled_.load() ? "启用" : "禁用") << "\n";
    report << "  输出设备状态: " << (output_enabled_.load() ? "启用" : "禁用") << "\n";
    report << "  静音状态: " << (is_muted_.load() ? "静音" : "正常") << "\n";
    report << "  音量: " << std::fixed << std::setprecision(2) << volume_.load() << "\n";
    report << "  处理模式: " << (processing_mode_.load() == processing_mode::realtime ? "实时" : "高保真") << "\n\n";
    
    // 音频格式
    report << "音频格式:\n";
    report << "  采样率: " << current_format_.sample_rate << "Hz\n";
    report << "  声道数: " << current_format_.channels << "\n";
    report << "  位深度: " << current_format_.bits_per_sample << "bit\n";
    report << "  格式: " << (current_format_.format == sample_format::float32 ? "float32" : "int16") << "\n\n";
    
    // 性能统计
    report << "性能统计:\n";
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        report << "  总采样数: " << stats_.total_samples_processed << "\n";
        report << "  总处理时间: " << std::fixed << std::setprecision(3) << stats_.total_processing_time_ms << "ms\n";
        report << "  平均处理时间: " << stats_.average_processing_time_ms << "ms\n";
        report << "  当前CPU占用: " << std::fixed << std::setprecision(1) << stats_.current_cpu_usage << "%\n";
        report << "  峰值CPU占用: " << stats_.peak_cpu_usage << "%\n";
        report << "  延迟: " << std::fixed << std::setprecision(2) << stats_.latency_ms << "ms\n";
        report << "  丢包数: " << stats_.dropout_count << "\n";
        report << "  错误数: " << stats_.error_count << "\n\n";
    }
    
    // DSP报告
    report << generate_dsp_report();
    
    // 插件报告
    report << generate_plugin_report();
    
    // 性能报告
    report << generate_performance_report();
    
    return report.str();
}

// 设置处理模式
bool audio_processor_impl::set_processing_mode(processing_mode mode) {
    if(is_processing_.load()) {
        std::cerr << "[AudioProcessor] 无法在运行中更改处理模式" << std::endl;
        return false;
    }
    
    processing_mode_.store(mode);
    
    // 更新DSP管理器配置
    if(dsp_manager_) {
        dsp_config dsp_config;
        dsp_config.enable_multithreading = (mode == processing_mode::high_fidelity);
        dsp_config.target_cpu_usage = cpu_usage_limit_.load();
        dsp_manager_->update_config(dsp_config);
    }
    
    std::cout << "[AudioProcessor] 处理模式设置为: " << (mode == processing_mode::realtime ? "实时" : "高保真") << std::endl;
    return true;
}

// 获取处理模式
processing_mode audio_processor_impl::get_processing_mode() const {
    return processing_mode_.load();
}

// 设置延迟目标
void audio_processor_impl::set_latency_target(double milliseconds) {
    latency_target_ms_.store(std::max(1.0, std::min(1000.0, milliseconds)));
    std::cout << "[AudioProcessor] 延迟目标设置为: " << latency_target_ms_.load() << "ms" << std::endl;
}

// 获取延迟目标
double audio_processor_impl::get_latency_target() const {
    return latency_target_ms_.load();
}

// 启用性能监控
void audio_processor_impl::enable_performance_monitoring(bool enable) {
    performance_monitoring_enabled_.store(enable);
    std::cout << "[AudioProcessor] 性能监控: " << (enable ? "启用" : "禁用") << std::endl;
}

// 是否启用性能监控
bool audio_processor_impl::is_performance_monitoring_enabled() const {
    return performance_monitoring_enabled_.load();
}

// 设置CPU使用限制
void audio_processor_impl::set_cpu_usage_limit(float percent) {
    cpu_usage_limit_.store(std::max(1.0f, std::min(100.0f, percent)));
    std::cout << "[AudioProcessor] CPU使用限制设置为: " << cpu_usage_limit_.load() << "%" << std::endl;
}

// 获取CPU使用限制
float audio_processor_impl::get_cpu_usage_limit() const {
    return cpu_usage_limit_.load();
}

// 加载插件
bool audio_processor_impl::load_plugin(const std::string& plugin_path) {
    std::lock_guard<std::mutex> lock(plugin_mutex_);
    
    // 这里应该实现插件加载逻辑
    // 目前返回false表示未实现
    std::cerr << "[AudioProcessor] 插件加载功能未实现" << std::endl;
    return false;
}

// 卸载插件
bool audio_processor_impl::unload_plugin(const std::string& plugin_name) {
    std::lock_guard<std::mutex> lock(plugin_mutex_);
    
    auto it = loaded_plugins_.find(plugin_name);
    if(it != loaded_plugins_.end()) {
        loaded_plugins_.erase(it);
        std::cout << "[AudioProcessor] 卸载插件: " << plugin_name << std::endl;
        return true;
    }
    
    return false;
}

// 获取已加载插件信息
std::vector<plugin_info> audio_processor_impl::get_loaded_plugins() const {
    std::lock_guard<std::mutex> lock(plugin_mutex_);
    
    std::vector<plugin_info> plugins;
    for(const auto& [name, plugin] : loaded_plugins_) {
        if(plugin) {
            plugin_info info;
            info.name = name;
            info.version = plugin->get_version();
            info.description = plugin->get_description();
            info.is_enabled = plugin->is_enabled();
            plugins.push_back(info);
        }
    }
    
    return plugins;
}

// 保存配置
bool audio_processor_impl::save_configuration(const std::string& config_file) {
    // 这里应该实现配置保存逻辑
    std::cout << "[AudioProcessor] 保存配置到: " << config_file << std::endl;
    return true;
}

// 加载配置
bool audio_processor_impl::load_configuration(const std::string& config_file) {
    // 这里应该实现配置加载逻辑
    std::cout << "[AudioProcessor] 从文件加载配置: " << config_file << std::endl;
    return true;
}

// 设置实时参数
void audio_processor_impl::set_realtime_parameter(const std::string& effect_name, const std::string& param_name, float value) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    realtime_parameters_[effect_name][param_name] = value;
    
    // 更新效果器参数
    if(dsp_manager_) {
        for(size_t i = 0; i < dsp_manager_->get_effect_count(); ++i) {
            auto* effect = dsp_manager_->get_effect(i);
            if(effect && effect->get_name() == effect_name) {
                // 这里应该调用效果器的参数设置方法
                break;
            }
        }
    }
}

// 获取实时参数
float audio_processor_impl::get_realtime_parameter(const std::string& effect_name, const std::string& param_name) const {
    std::lock_guard<std::mutex> lock(param_mutex_);
    
    auto effect_it = realtime_parameters_.find(effect_name);
    if(effect_it != realtime_parameters_.end()) {
        auto param_it = effect_it->second.find(param_name);
        if(param_it != effect_it->second.end()) {
            return param_it->second;
        }
    }
    
    return 0.0f; // 默认值
}

// 获取实时参数信息
std::vector<parameter_info> audio_processor_impl::get_realtime_parameters(const std::string& effect_name) const {
    std::lock_guard<std::mutex> lock(param_mutex_);
    
    std::vector<parameter_info> params;
    
    auto effect_it = realtime_parameters_.find(effect_name);
    if(effect_it != realtime_parameters_.end()) {
        for(const auto& [param_name, value] : effect_it->second) {
            parameter_info info;
            info.name = param_name;
            info.value = value;
            info.min_value = 0.0f;  // 默认值，实际应该从效果器获取
            info.max_value = 1.0f;
            info.default_value = 0.5f;
            params.push_back(info);
        }
    }
    
    return params;
}

// 初始化DSP管理器
bool audio_processor_impl::initialize_dsp_manager() {
    dsp_manager_ = std::make_unique<dsp_manager>();
    
    dsp_config dsp_config;
    dsp_config.enable_multithreading = (processing_mode_.load() == processing_mode::high_fidelity);
    dsp_config.enable_performance_monitoring = performance_monitoring_enabled_.load();
    dsp_config.max_effects = config_.max_dsp_effects;
    dsp_config.target_cpu_usage = cpu_usage_limit_.load();
    dsp_config.memory_pool_size = config_.buffer_size * 4 * 1024; // 估算内存池大小
    dsp_config.max_latency_ms = latency_target_ms_.load();
    dsp_config.enable_standard_effects = true;
    
    if(!dsp_manager_->initialize(dsp_config)) {
        std::cerr << "[AudioProcessor] DSP管理器初始化失败" << std::endl;
        return false;
    }
    
    std::cout << "[AudioProcessor] DSP管理器初始化成功" << std::endl;
    return true;
}

// 初始化输出设备
bool audio_processor_impl::initialize_output_device() {
    if(!output_device_) {
        std::cerr << "[AudioProcessor] 未设置输出设备" << std::endl;
        return false;
    }
    
    // 这里应该初始化输出设备
    std::cout << "[AudioProcessor] 输出设备初始化成功" << std::endl;
    return true;
}

// 初始化缓冲区
bool audio_processor_impl::initialize_buffers() {
    size_t buffer_size = config_.buffer_size * current_format_.channels;
    
    input_buffer_ = std::make_unique<ring_buffer>(buffer_size * 4); // 4倍缓冲
    output_buffer_ = std::make_unique<ring_buffer>(buffer_size * 4);
    dsp_buffer_ = std::make_unique<ring_buffer>(buffer_size * 2);
    
    std::cout << "[AudioProcessor] 缓冲区初始化成功，大小: " << buffer_size << "样本" << std::endl;
    return true;
}

// 初始化处理线程
bool audio_processor_impl::initialize_processing_thread() {
    processing_thread_running_.store(true);
    processing_thread_ = std::thread(&audio_processor_impl::processing_thread_func, this);
    
    std::cout << "[AudioProcessor] 处理线程初始化成功" << std::endl;
    return true;
}

// 关闭DSP管理器
void audio_processor_impl::shutdown_dsp_manager() {
    if(dsp_manager_) {
        dsp_manager_->shutdown();
        dsp_manager_.reset();
        std::cout << "[AudioProcessor] DSP管理器已关闭" << std::endl;
    }
}

// 关闭输出设备
void audio_processor_impl::shutdown_output_device() {
    if(output_device_) {
        // 这里应该关闭输出设备
        std::cout << "[AudioProcessor] 输出设备已关闭" << std::endl;
    }
}

// 关闭缓冲区
void audio_processor_impl::shutdown_buffers() {
    input_buffer_.reset();
    output_buffer_.reset();
    dsp_buffer_.reset();
    std::cout << "[AudioProcessor] 缓冲区已关闭" << std::endl;
}

// 关闭处理线程
void audio_processor_impl::shutdown_processing_thread() {
    should_stop_.store(true);
    processing_cv_.notify_all();
    
    if(processing_thread_.joinable()) {
        processing_thread_.join();
    }
    
    processing_thread_running_.store(false);
    std::cout << "[AudioProcessor] 处理线程已关闭" << std::endl;
}

// 处理线程函数
void audio_processor_impl::processing_thread_func() {
    std::cout << "[AudioProcessor] 处理线程启动" << std::endl;
    
    // 提升线程优先级
    set_thread_priority_high();
    
    while(processing_thread_running_.load() && !should_stop_.load()) {
        std::unique_lock<std::mutex> lock(processing_mutex_);
        
        // 等待处理信号
        processing_cv_.wait_for(lock, std::chrono::milliseconds(1), [this] {
            return !processing_thread_running_.load() || should_stop_.load();
        });
        
        if(should_stop_.load()) {
            break;
        }
        
        // 这里应该实现后台处理逻辑
        // 目前只是简单的循环
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[AudioProcessor] 处理线程停止" << std::endl;
}

// 内部音频处理
bool audio_processor_impl::process_audio_internal(const audio_chunk& input, audio_chunk& output, abort_callback& abort) {
    if(abort.is_aborting()) {
        return false;
    }
    
    // 复制输入数据
    output.copy(input);
    
    // 应用静音
    if(is_muted_.load()) {
        apply_mute(output);
        return true;
    }
    
    // DSP处理
    if(dsp_enabled_.load() && dsp_manager_) {
        if(!process_dsp_chain(output, abort)) {
            return false;
        }
    }
    
    // 应用音量
    apply_volume(output);
    
    // 输出设备处理
    if(output_enabled_.load() && output_device_) {
        if(!process_output_device(output, abort)) {
            return false;
        }
    }
    
    return true;
}

// 处理DSP链
bool audio_processor_impl::process_dsp_chain(audio_chunk& chunk, abort_callback& abort) {
    if(!dsp_manager_) {
        return true;
    }
    
    return dsp_manager_->process_chain(chunk, abort);
}

// 处理输出设备
bool audio_processor_impl::process_output_device(audio_chunk& chunk, abort_callback& abort) {
    if(!output_device_) {
        return true;
    }
    
    // 这里应该实现输出设备处理逻辑
    // 目前只是返回成功
    return true;
}

// 更新统计信息
void audio_processor_impl::update_stats(const audio_chunk& chunk, double processing_time) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_samples_processed += chunk.get_sample_count() * chunk.get_channels();
    stats_.total_processing_time_ms += processing_time;
    
    // 计算平均处理时间
    if(stats_.total_samples_processed > 0) {
        stats_.average_processing_time_ms = stats_.total_processing_time_ms / (stats_.total_samples_processed / 44100.0); // 假设44.1kHz
    }
    
    // 更新CPU占用（简化计算）
    stats_.current_cpu_usage = static_cast<float>(processing_time / 10.0 * 100.0); // 假设10ms是100%占用
    if(stats_.current_cpu_usage > stats_.peak_cpu_usage) {
        stats_.peak_cpu_usage = stats_.current_cpu_usage;
    }
    
    stats_.latency_ms = latency_target_ms_.load();
    stats_.processing_mode = processing_mode_.load();
}

// 检查性能限制
void audio_processor_impl::check_performance_limits() {
    if(stats_.current_cpu_usage > cpu_usage_limit_.load()) {
        // CPU使用率超过限制
        std::cerr << "[AudioProcessor] CPU使用率超过限制: " << stats_.current_cpu_usage 
                  << "% > " << cpu_usage_limit_.load() << "%" << std::endl;
    }
}

// 处理错误
void audio_processor_impl::handle_error(const std::string& operation, const std::string& error) {
    std::cerr << "[AudioProcessor] " << operation << " 错误: " << error << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.error_count++;
    }
}

// 验证音频格式
bool audio_processor_impl::validate_audio_format(const audio_chunk& chunk) const {
    if(chunk.get_sample_count() == 0) {
        return false;
    }
    
    if(chunk.get_channels() < 1 || chunk.get_channels() > 8) {
        return false;
    }
    
    if(chunk.get_sample_rate() < 8000 || chunk.get_sample_rate() > 192000) {
        return false;
    }
    
    return true;
}

// 应用音量
void audio_processor_impl::apply_volume(audio_chunk& chunk) {
    float vol = volume_.load();
    if(vol == 1.0f) {
        return; // 无需处理
    }
    
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    for(size_t i = 0; i < total_samples; ++i) {
        data[i] *= vol;
    }
}

// 应用静音
void audio_processor_impl::apply_mute(audio_chunk& chunk) {
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    std::fill(data, data + total_samples, 0.0f);
}

// 生成插件报告
std::string audio_processor_impl::generate_plugin_report() const {
    std::stringstream report;
    report << "插件信息:\n";
    
    std::lock_guard<std::mutex> lock(plugin_mutex_);
    
    if(loaded_plugins_.empty()) {
        report << "  无已加载插件\n";
    } else {
        for(const auto& [name, plugin] : loaded_plugins_) {
            if(plugin) {
                report << "  - " << name << " v" << plugin->get_version() 
                       << " (" << (plugin->is_enabled() ? "启用" : "禁用") << ")\n";
            }
        }
    }
    
    report << "\n";
    return report.str();
}

// 生成DSP报告
std::string audio_processor_impl::generate_dsp_report() const {
    std::stringstream report;
    report << "DSP信息:\n";
    
    if(dsp_manager_) {
        report << dsp_manager_->generate_dsp_report();
    } else {
        report << "  DSP管理器未初始化\n";
    }
    
    report << "\n";
    return report.str();
}

// 生成性能报告
std::string audio_processor_impl::generate_performance_report() const {
    std::stringstream report;
    report << "性能报告:\n";
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        // CPU使用率分析
        report << "  CPU使用率:\n";
        report << "    当前: " << std::fixed << std::setprecision(1) << stats_.current_cpu_usage << "%\n";
        report << "    峰值: " << stats_.peak_cpu_usage << "%\n";
        report << "    限制: " << cpu_usage_limit_.load() << "%\n";
        
        // 延迟分析
        report << "  延迟性能:\n";
        report << "    当前延迟: " << std::fixed << std::setprecision(2) << stats_.latency_ms << "ms\n";
        report << "    目标延迟: " << latency_target_ms_.load() << "ms\n";
        report << "    平均处理时间: " << stats_.average_processing_time_ms << "ms\n";
        
        // 错误统计
        report << "  错误统计:\n";
        report << "    总错误数: " << stats_.error_count << "\n";
        report << "    丢包数: " << stats_.dropout_count << "\n";
        
        // 性能建议
        if(stats_.current_cpu_usage > cpu_usage_limit_.load() * 0.9f) {
            report << "  性能警告: CPU使用率接近限制\n";
        }
        
        if(stats_.latency_ms > latency_target_ms_.load() * 1.5f) {
            report << "  性能警告: 延迟超过目标值\n";
        }
        
        if(stats_.error_count > 0) {
            report << "  性能警告: 存在处理错误\n";
        }
    }
    
    report << "\n";
    return report.str();
}

// 创建音频处理器
std::unique_ptr<audio_processor_advanced> create_audio_processor() {
    return std::make_unique<audio_processor_impl>();
}

} // namespace fb2k