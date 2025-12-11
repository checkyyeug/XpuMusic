#pragma once

// 阶段1.3：DSP管理器
// 高级DSP效果器管理和调度系统

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "../stage1_2/dsp_interfaces.h"
#include "../stage1_2/audio_chunk.h"
#include "../stage1_1/real_minihost.h"

namespace fb2k {

// DSP效果器类型枚举
enum class dsp_effect_type {
    unknown,
    equalizer,      // 均衡器
    compressor,     // 压缩器
    limiter,        // 限制器
    reverb,         // 混响
    echo,           // 回声
    chorus,         // 合唱
    flanger,        // 镶边
    phaser,         // 移相
    distortion,     // 失真
    gate,           // 噪声门
    volume,         // 音量控制
    crossfeed,      // 交叉馈送
    resampler,      // 重采样
    convolver       // 卷积器
};

// DSP效果器参数结构
struct dsp_effect_params {
    dsp_effect_type type;
    std::string name;
    std::string description;
    bool is_enabled;
    bool is_bypassed;
    float cpu_usage_estimate;
    double latency_ms;
    std::vector<dsp_config_param> config_params;
};

// DSP效果器基类（高级版）
class dsp_effect_advanced : public dsp {
protected:
    dsp_effect_params params_;
    std::atomic<bool> is_enabled_;
    std::atomic<bool> is_bypassed_;
    std::atomic<float> cpu_usage_;
    
public:
    dsp_effect_advanced(const dsp_effect_params& params)
        : params_(params), is_enabled_(true), is_bypassed_(false), cpu_usage_(0.0f) {}
    
    // 基础接口
    virtual bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                            uint32_t channels) override = 0;
    virtual void run(audio_chunk& chunk, abort_callback& abort) override = 0;
    virtual void reset() override = 0;
    
    // 高级接口
    virtual bool is_enabled() const { return is_enabled_.load(); }
    virtual void set_enabled(bool enabled) { is_enabled_ = enabled; }
    
    virtual bool is_bypassed() const { return is_bypassed_.load(); }
    virtual void set_bypassed(bool bypassed) { is_bypassed_ = bypassed; }
    
    virtual float get_cpu_usage() const { return cpu_usage_.load(); }
    virtual void set_cpu_usage(float usage) { cpu_usage_ = usage; }
    
    virtual const dsp_effect_params& get_params() const { return params_; }
    virtual void update_params(const dsp_effect_params& params) { params_ = params; }
    
    // 实时参数调节
    virtual bool set_realtime_param(const std::string& name, float value);
    virtual float get_realtime_param(const std::string& name) const;
    
    // 性能监控
    virtual void start_performance_monitoring();
    virtual void stop_performance_monitoring();
    virtual dsp_performance_stats get_performance_stats() const;
    
protected:
    // 受保护的接口
    virtual void process_chunk_internal(audio_chunk& chunk, abort_callback& abort) = 0;
    virtual void update_cpu_usage(float usage);
};

// DSP性能统计
struct dsp_performance_stats {
    uint64_t total_samples_processed;
    uint64_t total_calls;
    double total_time_ms;
    double average_time_ms;
    double min_time_ms;
    double max_time_ms;
    double cpu_usage_percent;
    uint64_t error_count;
    
    dsp_performance_stats() : 
        total_samples_processed(0), total_calls(0), total_time_ms(0.0),
        average_time_ms(0.0), min_time_ms(0.0), max_time_ms(0.0),
        cpu_usage_percent(0.0), error_count(0) {}
};

// DSP效果器管理器
class dsp_manager {
private:
    std::vector<std::unique_ptr<dsp_effect_advanced>> effects_;
    std::unique_ptr<dsp_preset_manager> preset_manager_;
    std::unique_ptr<dsp_performance_monitor> performance_monitor_;
    
    // 多线程支持
    std::unique_ptr<multithreaded_dsp_processor> thread_pool_;
    std::atomic<bool> use_multithreading_;
    
    // 配置
    struct dsp_config {
        bool enable_multithreading = false;
        size_t max_threads = 4;
        bool enable_performance_monitoring = true;
        size_t memory_pool_size = 16 * 1024 * 1024; // 16MB
    } config_;
    
public:
    dsp_manager();
    ~dsp_manager();
    
    // 初始化和清理
    bool initialize(const dsp_config& config = {});
    void shutdown();
    
    // 效果器管理
    bool add_effect(std::unique_ptr<dsp_effect_advanced> effect);
    bool remove_effect(size_t index);
    void clear_effects();
    
    size_t get_effect_count() const;
    dsp_effect_advanced* get_effect(size_t index);
    const dsp_effect_advanced* get_effect(size_t index) const;
    
    // DSP链处理
    bool process_chain(audio_chunk& chunk, abort_callback& abort);
    bool process_chain_multithread(audio_chunk& chunk, abort_callback& abort);
    
    // 标准效果器工厂
    std::unique_ptr<dsp_effect_advanced> create_equalizer_10band();
    std::unique_ptr<dsp_effect_advanced> create_reverb();
    std::unique_ptr<dsp_effect_advanced> create_compressor();
    std::unique_ptr<dsp_effect_advanced> create_limiter();
    std::unique_ptr<dsp_effect_advanced> create_volume_control();
    
    // 高级效果器
    std::unique_ptr<dsp_effect_advanced> create_convolver();
    std::unique_ptr<dsp_effect_advanced> create_crossfeed();
    std::unique_ptr<dsp_effect_advanced> create_resampler(uint32_t target_rate);
    
    // 预设管理
    bool save_effect_preset(size_t effect_index, const std::string& preset_name);
    bool load_effect_preset(size_t effect_index, const std::string& preset_name);
    std::vector<std::string> get_available_presets() const;
    
    // 性能监控
    void start_performance_monitoring();
    void stop_performance_monitoring();
    dsp_performance_stats get_overall_stats() const;
    std::vector<dsp_performance_stats> get_all_effect_stats() const;
    
    // 配置管理
    const dsp_config& get_config() const { return config_; }
    void update_config(const dsp_config& config);
    
    // 实用工具
    float estimate_total_cpu_usage() const;
    double estimate_total_latency() const;
    bool validate_dsp_chain() const;
    std::string generate_dsp_report() const;
};

// DSP预设管理器
class dsp_preset_manager {
private:
    std::map<std::string, std::unique_ptr<dsp_preset>> presets_;
    std::string presets_directory_;
    
public:
    dsp_preset_manager(const std::string& directory = "dsp_presets");
    
    bool load_preset(const std::string& name, dsp_preset& preset);
    bool save_preset(const std::string& name, const dsp_preset& preset);
    bool delete_preset(const std::string& name);
    
    std::vector<std::string> get_preset_list() const;
    bool preset_exists(const std::string& name) const;
    
    // 预设导入/导出
    bool import_preset_file(const std::string& file_path);
    bool export_preset_file(const std::string& preset_name, const std::string& file_path);
    
    // 预设验证
    bool validate_preset(const dsp_preset& preset) const;
    std::string get_preset_error(const dsp_preset& preset) const;
    
    // 预设转换
    bool convert_preset_to_native(const dsp_preset& source, dsp_preset& target);
};

// DSP性能监控器
class dsp_performance_monitor {
private:
    std::atomic<bool> is_monitoring_;
    std::chrono::steady_clock::time_point start_time_;
    
    // 统计信息
    std::atomic<uint64_t> total_samples_processed_;
    std::atomic<double> total_processing_time_ms_;
    std::atomic<uint64_t> total_calls_;
    std::atomic<double> min_time_ms_;
    std::atomic<double> max_time_ms_;
    std::atomic<uint64_t> error_count_;
    
public:
    dsp_performance_monitor();
    ~dsp_performance_monitor();
    
    void start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const { return is_monitoring_.load(); }
    
    // 记录性能数据
    void record_processing_start();
    void record_processing_end(size_t samples_processed);
    void record_error(const std::string& error_type);
    
    // 获取统计信息
    dsp_performance_stats get_stats() const;
    double get_current_cpu_usage() const;
    double get_realtime_factor() const;
    
    // 性能分析
    std::string generate_performance_report() const;
    bool is_performance_acceptable() const;
    std::vector<std::string> get_performance_warnings() const;
    
    // 重置统计
    void reset_stats();
};

// 多线程DSP处理器
class multithreaded_dsp_processor {
private:
    std::vector<std::unique_ptr<std::thread>> worker_threads_;
    std::queue<std::unique_ptr<dsp_task>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> should_stop_;
    std::atomic<size_t> active_tasks_;
    
    size_t num_threads_;
    std::atomic<uint64_t> total_tasks_processed_;
    
public:
    multithreaded_dsp_processor(size_t num_threads = 4);
    ~multithreaded_dsp_processor();
    
    void start();
    void stop();
    bool is_running() const;
    
    // 任务提交
    void submit_task(std::unique_ptr<dsp_task> task);
    void submit_tasks(std::vector<std::unique_ptr<dsp_task>> tasks);
    
    // 等待完成
    void wait_for_completion();
    bool wait_for_completion_timeout(std::chrono::milliseconds timeout);
    
    // 状态查询
    size_t get_queue_size() const;
    size_t get_active_tasks() const;
    uint64_t get_total_tasks_processed() const;
    
    // 性能控制
    void set_num_threads(size_t num_threads);
    size_t get_num_threads() const { return num_threads_; }
    
private:
    void worker_thread_func(size_t thread_id);
    void process_task(dsp_task* task);
};

// DSP任务
class dsp_task {
private:
    audio_chunk* input_chunk_;
    audio_chunk* output_chunk_;
    dsp_chain* chain_;
    abort_callback* abort_;
    
    std::atomic<bool> is_completed_;
    std::atomic<bool> has_error_;
    std::string error_message_;
    
public:
    dsp_task(audio_chunk* input, audio_chunk* output, 
             dsp_chain* chain, abort_callback* abort)
        : input_chunk_(input), output_chunk_(output), 
          chain_(chain), abort_(abort),
          is_completed_(false), has_error_(false) {}
    
    void execute();
    bool is_completed() const { return is_completed_.load(); }
    bool has_error() const { return has_error_.load(); }
    const std::string& get_error_message() const { return error_message_; }
};

// DSP实用工具
namespace dsp_utils {

// 创建标准DSP链
std::unique_ptr<dsp_chain> create_standard_dsp_chain();

// 创建常用DSP组合
std::unique_ptr<dsp_chain> create_hifi_dsp_chain();
std::unique_ptr<dsp_chain> create_headphone_dsp_chain();
std::unique_ptr<dsp_chain> create_vinyl_dsp_chain();
std::unique_ptr<dsp_chain> create_loudness_dsp_chain();

// DSP效果器推荐
std::vector<dsp_effect_params> recommend_effects_for_genre(const std::string& genre);
std::vector<dsp_effect_params> recommend_effects_for_device(const std::string& device_type);

// DSP性能基准测试
struct dsp_benchmark_result {
    double processing_speed_x_realtime;
    double cpu_usage_percent;
    double memory_usage_mb;
    double latency_ms;
    bool is_performance_acceptable;
    std::vector<std::string> bottlenecks;
};

dsp_benchmark_result benchmark_dsp_chain(const dsp_chain& chain, 
                                       size_t test_duration_seconds);

// DSP调试工具
void dump_dsp_chain_state(const dsp_chain& chain);
void analyze_dsp_performance(const dsp_manager& manager);
std::string generate_dsp_diagnostic_report(const dsp_manager& manager);

} // namespace dsp_utils

// DSP配置结构
struct dsp_config {
    // 基本配置
    bool enable_multithreading = true;
    size_t max_threads = std::thread::hardware_concurrency();
    bool enable_performance_monitoring = true;
    
    // 内存配置
    size_t memory_pool_size = 32 * 1024 * 1024; // 32MB
    size_t chunk_pool_size = 64;
    size_t max_chunk_samples = 65536;
    
    // 性能配置
    double target_cpu_usage = 10.0; // 10%
    double max_latency_ms = 20.0;   // 20ms
    size_t max_effects = 16;
    
    // 效果器配置
    bool enable_standard_effects = true;
    bool enable_advanced_effects = false;
    bool enable_experimental_effects = false;
    
    // 输出配置
    bool enable_exclusive_mode = true;
    bool enable_asio_support = false;
    bool enable_device_fallback = true;
    
    // 调试配置
    bool enable_debug_logging = false;
    bool enable_performance_warnings = true;
    bool enable_memory_tracking = false;
};

// DSP系统初始化
class dsp_system_initializer_advanced {
public:
    static bool initialize(const dsp_config& config = {});
    static void shutdown();
    static bool is_initialized();
    static const dsp_config& get_current_config();
    
    static std::string get_system_info();
    static std::string get_build_info();
    static std::vector<std::string> get_available_features();
};

} // namespace fb2k