#pragma once

// 闃舵1.3锛欴SP绠＄悊鍣?
// 楂樼骇DSP鏁堟灉鍣ㄧ鐞嗗拰璋冨害绯荤粺

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

// DSP鏁堟灉鍣ㄧ被鍨嬫灇涓?
enum class dsp_effect_type {
    unknown,
    equalizer,      // 鍧囪　鍣?
    compressor,     // 鍘嬬缉鍣?
    limiter,        // 闄愬埗鍣?
    reverb,         // 娣峰搷
    echo,           // 鍥炲０
    chorus,         // 鍚堝敱
    flanger,        // 闀惰竟
    phaser,         // 绉荤浉
    distortion,     // 澶辩湡
    gate,           // 鍣０闂?
    volume,         // 闊抽噺鎺у埗
    crossfeed,      // 浜ゅ弶棣堥€?
    resampler,      // 閲嶉噰鏍?
    convolver       // 鍗风Н鍣?
};

// DSP鏁堟灉鍣ㄥ弬鏁扮粨鏋?
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

// DSP鏁堟灉鍣ㄥ熀绫伙紙楂樼骇鐗堬級
class dsp_effect_advanced : public dsp {
protected:
    dsp_effect_params params_;
    std::atomic<bool> is_enabled_;
    std::atomic<bool> is_bypassed_;
    std::atomic<float> cpu_usage_;
    
public:
    dsp_effect_advanced(const dsp_effect_params& params)
        : params_(params), is_enabled_(true), is_bypassed_(false), cpu_usage_(0.0f) {}
    
    // 鍩虹鎺ュ彛
    virtual bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                            uint32_t channels) override = 0;
    virtual void run(audio_chunk& chunk, abort_callback& abort) override = 0;
    virtual void reset() override = 0;
    
    // 楂樼骇鎺ュ彛
    virtual bool is_enabled() const { return is_enabled_.load(); }
    virtual void set_enabled(bool enabled) { is_enabled_ = enabled; }
    
    virtual bool is_bypassed() const { return is_bypassed_.load(); }
    virtual void set_bypassed(bool bypassed) { is_bypassed_ = bypassed; }
    
    virtual float get_cpu_usage() const { return cpu_usage_.load(); }
    virtual void set_cpu_usage(float usage) { cpu_usage_ = usage; }
    
    virtual const dsp_effect_params& get_params() const { return params_; }
    virtual void update_params(const dsp_effect_params& params) { params_ = params; }
    
    // 瀹炴椂鍙傛暟璋冭妭
    virtual bool set_realtime_param(const std::string& name, float value);
    virtual float get_realtime_param(const std::string& name) const;
    
    // 鎬ц兘鐩戞帶
    virtual void start_performance_monitoring();
    virtual void stop_performance_monitoring();
    virtual dsp_performance_stats get_performance_stats() const;
    
protected:
    // 鍙椾繚鎶ょ殑鎺ュ彛
    virtual void process_chunk_internal(audio_chunk& chunk, abort_callback& abort) = 0;
    virtual void update_cpu_usage(float usage);
};

// DSP鎬ц兘缁熻
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

// DSP鏁堟灉鍣ㄧ鐞嗗櫒
class dsp_manager {
private:
    std::vector<std::unique_ptr<dsp_effect_advanced>> effects_;
    std::unique_ptr<dsp_preset_manager> preset_manager_;
    std::unique_ptr<dsp_performance_monitor> performance_monitor_;
    
    // 澶氱嚎绋嬫敮鎸?
    std::unique_ptr<multithreaded_dsp_processor> thread_pool_;
    std::atomic<bool> use_multithreading_;
    
    // 閰嶇疆
    struct dsp_config {
        bool enable_multithreading = false;
        size_t max_threads = 4;
        bool enable_performance_monitoring = true;
        size_t memory_pool_size = 16 * 1024 * 1024; // 16MB
    } config_;
    
public:
    dsp_manager();
    ~dsp_manager();
    
    // 鍒濆鍖栧拰娓呯悊
    bool initialize(const dsp_config& config = {});
    void shutdown();
    
    // 鏁堟灉鍣ㄧ鐞?
    bool add_effect(std::unique_ptr<dsp_effect_advanced> effect);
    bool remove_effect(size_t index);
    void clear_effects();
    
    size_t get_effect_count() const;
    dsp_effect_advanced* get_effect(size_t index);
    const dsp_effect_advanced* get_effect(size_t index) const;
    
    // DSP閾惧鐞?
    bool process_chain(audio_chunk& chunk, abort_callback& abort);
    bool process_chain_multithread(audio_chunk& chunk, abort_callback& abort);
    
    // 鏍囧噯鏁堟灉鍣ㄥ伐鍘?
    std::unique_ptr<dsp_effect_advanced> create_equalizer_10band();
    std::unique_ptr<dsp_effect_advanced> create_reverb();
    std::unique_ptr<dsp_effect_advanced> create_compressor();
    std::unique_ptr<dsp_effect_advanced> create_limiter();
    std::unique_ptr<dsp_effect_advanced> create_volume_control();
    
    // 楂樼骇鏁堟灉鍣?
    std::unique_ptr<dsp_effect_advanced> create_convolver();
    std::unique_ptr<dsp_effect_advanced> create_crossfeed();
    std::unique_ptr<dsp_effect_advanced> create_resampler(uint32_t target_rate);
    
    // 棰勮绠＄悊
    bool save_effect_preset(size_t effect_index, const std::string& preset_name);
    bool load_effect_preset(size_t effect_index, const std::string& preset_name);
    std::vector<std::string> get_available_presets() const;
    
    // 鎬ц兘鐩戞帶
    void start_performance_monitoring();
    void stop_performance_monitoring();
    dsp_performance_stats get_overall_stats() const;
    std::vector<dsp_performance_stats> get_all_effect_stats() const;
    
    // 閰嶇疆绠＄悊
    const dsp_config& get_config() const { return config_; }
    void update_config(const dsp_config& config);
    
    // 瀹炵敤宸ュ叿
    float estimate_total_cpu_usage() const;
    double estimate_total_latency() const;
    bool validate_dsp_chain() const;
    std::string generate_dsp_report() const;
};

// DSP棰勮绠＄悊鍣?
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
    
    // 棰勮瀵煎叆/瀵煎嚭
    bool import_preset_file(const std::string& file_path);
    bool export_preset_file(const std::string& preset_name, const std::string& file_path);
    
    // 棰勮楠岃瘉
    bool validate_preset(const dsp_preset& preset) const;
    std::string get_preset_error(const dsp_preset& preset) const;
    
    // 棰勮杞崲
    bool convert_preset_to_native(const dsp_preset& source, dsp_preset& target);
};

// DSP鎬ц兘鐩戞帶鍣?
class dsp_performance_monitor {
private:
    std::atomic<bool> is_monitoring_;
    std::chrono::steady_clock::time_point start_time_;
    
    // 缁熻淇℃伅
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
    
    // 璁板綍鎬ц兘鏁版嵁
    void record_processing_start();
    void record_processing_end(size_t samples_processed);
    void record_error(const std::string& error_type);
    
    // 鑾峰彇缁熻淇℃伅
    dsp_performance_stats get_stats() const;
    double get_current_cpu_usage() const;
    double get_realtime_factor() const;
    
    // 鎬ц兘鍒嗘瀽
    std::string generate_performance_report() const;
    bool is_performance_acceptable() const;
    std::vector<std::string> get_performance_warnings() const;
    
    // 閲嶇疆缁熻
    void reset_stats();
};

// 澶氱嚎绋婦SP澶勭悊鍣?
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
    
    // 浠诲姟鎻愪氦
    void submit_task(std::unique_ptr<dsp_task> task);
    void submit_tasks(std::vector<std::unique_ptr<dsp_task>> tasks);
    
    // 绛夊緟瀹屾垚
    void wait_for_completion();
    bool wait_for_completion_timeout(std::chrono::milliseconds timeout);
    
    // 鐘舵€佹煡璇?
    size_t get_queue_size() const;
    size_t get_active_tasks() const;
    uint64_t get_total_tasks_processed() const;
    
    // 鎬ц兘鎺у埗
    void set_num_threads(size_t num_threads);
    size_t get_num_threads() const { return num_threads_; }
    
private:
    void worker_thread_func(size_t thread_id);
    void process_task(dsp_task* task);
};

// DSP浠诲姟
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

// DSP瀹炵敤宸ュ叿
namespace dsp_utils {

// 鍒涘缓鏍囧噯DSP閾?
std::unique_ptr<dsp_chain> create_standard_dsp_chain();

// 鍒涘缓甯哥敤DSP缁勫悎
std::unique_ptr<dsp_chain> create_hifi_dsp_chain();
std::unique_ptr<dsp_chain> create_headphone_dsp_chain();
std::unique_ptr<dsp_chain> create_vinyl_dsp_chain();
std::unique_ptr<dsp_chain> create_loudness_dsp_chain();

// DSP鏁堟灉鍣ㄦ帹鑽?
std::vector<dsp_effect_params> recommend_effects_for_genre(const std::string& genre);
std::vector<dsp_effect_params> recommend_effects_for_device(const std::string& device_type);

// DSP鎬ц兘鍩哄噯娴嬭瘯
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

// DSP璋冭瘯宸ュ叿
void dump_dsp_chain_state(const dsp_chain& chain);
void analyze_dsp_performance(const dsp_manager& manager);
std::string generate_dsp_diagnostic_report(const dsp_manager& manager);

} // namespace dsp_utils

// DSP閰嶇疆缁撴瀯
struct dsp_config {
    // 鍩烘湰閰嶇疆
    bool enable_multithreading = true;
    size_t max_threads = std::thread::hardware_concurrency();
    bool enable_performance_monitoring = true;
    
    // 鍐呭瓨閰嶇疆
    size_t memory_pool_size = 32 * 1024 * 1024; // 32MB
    size_t chunk_pool_size = 64;
    size_t max_chunk_samples = 65536;
    
    // 鎬ц兘閰嶇疆
    double target_cpu_usage = 10.0; // 10%
    double max_latency_ms = 20.0;   // 20ms
    size_t max_effects = 16;
    
    // 鏁堟灉鍣ㄩ厤缃?
    bool enable_standard_effects = true;
    bool enable_advanced_effects = false;
    bool enable_experimental_effects = false;
    
    // 杈撳嚭閰嶇疆
    bool enable_exclusive_mode = true;
    bool enable_asio_support = false;
    bool enable_device_fallback = true;
    
    // 璋冭瘯閰嶇疆
    bool enable_debug_logging = false;
    bool enable_performance_warnings = true;
    bool enable_memory_tracking = false;
};

// DSP绯荤粺鍒濆鍖?
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