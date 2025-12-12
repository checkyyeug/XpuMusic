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

// 楂樼骇闊抽澶勭悊鍣ㄥ疄鐜?
class audio_processor_impl : public audio_processor_advanced {
public:
    audio_processor_impl();
    ~audio_processor_impl() override;

    // audio_processor鎺ュ彛瀹炵幇
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
    
    // 楂樼骇鍔熻兘
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
    
    // 閰嶇疆绠＄悊
    bool save_configuration(const std::string& config_file) override;
    bool load_configuration(const std::string& config_file) override;
    
    // 瀹炴椂鍙傛暟璋冭妭
    void set_realtime_parameter(const std::string& effect_name, const std::string& param_name, float value) override;
    float get_realtime_parameter(const std::string& effect_name, const std::string& param_name) const override;
    
    std::vector<parameter_info> get_realtime_parameters(const std::string& effect_name) const override;

private:
    // 閰嶇疆
    audio_processor_config config_;
    
    // DSP绠＄悊鍣?
    std::unique_ptr<dsp_manager> dsp_manager_;
    
    // 杈撳嚭璁惧
    std::unique_ptr<output_device> output_device_;
    
    // 鐘舵€佺鐞?
    std::atomic<bool> is_initialized_;
    std::atomic<bool> is_processing_;
    std::atomic<bool> dsp_enabled_;
    std::atomic<bool> output_enabled_;
    std::atomic<bool> is_muted_;
    std::atomic<bool> performance_monitoring_enabled_;
    std::atomic<bool> should_stop_;
    
    // 闊抽澶勭悊鍙傛暟
    std::atomic<float> volume_;
    std::atomic<processing_mode> processing_mode_;
    std::atomic<double> latency_target_ms_;
    std::atomic<float> cpu_usage_limit_;
    
    // 鎬ц兘鐩戞帶
    mutable audio_processor_stats stats_;
    mutable std::mutex stats_mutex_;
    
    // 绾跨▼绠＄悊
    std::thread processing_thread_;
    std::atomic<bool> processing_thread_running_;
    std::condition_variable processing_cv_;
    std::mutex processing_mutex_;
    
    // 闊抽缂撳啿绠＄悊
    std::unique_ptr<ring_buffer> input_buffer_;
    std::unique_ptr<ring_buffer> output_buffer_;
    std::unique_ptr<ring_buffer> dsp_buffer_;
    
    // 闊抽鏍煎紡
    audio_format_info current_format_;
    std::atomic<bool> format_changed_;
    
    // 鎻掍欢绠＄悊
    std::map<std::string, std::unique_ptr<audio_plugin>> loaded_plugins_;
    mutable std::mutex plugin_mutex_;
    
    // 瀹炴椂鍙傛暟绠＄悊
    std::map<std::string, std::map<std::string, float>> realtime_parameters_;
    mutable std::mutex param_mutex_;
    
    // 绉佹湁鏂规硶
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

// 鏋勯€犲嚱鏁?
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
    
    // 鍒濆鍖栫粺璁′俊鎭?
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

// 鏋愭瀯鍑芥暟
audio_processor_impl::~audio_processor_impl() {
    if(is_initialized_.load()) {
        shutdown();
    }
}

// 鍒濆鍖?
bool audio_processor_impl::initialize(const audio_processor_config& config) {
    std::cout << "[AudioProcessor] 鍒濆鍖栭煶棰戝鐞嗗櫒..." << std::endl;
    
    if(is_initialized_.load()) {
        std::cerr << "[AudioProcessor] 闊抽澶勭悊鍣ㄥ凡鍒濆鍖? << std::endl;
        return false;
    }
    
    config_ = config;
    
    try {
        // 鍒濆鍖朌SP绠＄悊鍣?
        if(!initialize_dsp_manager()) {
            std::cerr << "[AudioProcessor] DSP绠＄悊鍣ㄥ垵濮嬪寲澶辫触" << std::endl;
            return false;
        }
        
        // 鍒濆鍖栬緭鍑鸿澶?
        if(!initialize_output_device()) {
            std::cerr << "[AudioProcessor] 杈撳嚭璁惧鍒濆鍖栧け璐? << std::endl;
            return false;
        }
        
        // 鍒濆鍖栫紦鍐插尯
        if(!initialize_buffers()) {
            std::cerr << "[AudioProcessor] 缂撳啿鍖哄垵濮嬪寲澶辫触" << std::endl;
            return false;
        }
        
        // 鍒濆鍖栧鐞嗙嚎绋?
        if(!initialize_processing_thread()) {
            std::cerr << "[AudioProcessor] 澶勭悊绾跨▼鍒濆鍖栧け璐? << std::endl;
            return false;
        }
        
        is_initialized_.store(true);
        std::cout << "[AudioProcessor] 闊抽澶勭悊鍣ㄥ垵濮嬪寲鎴愬姛" << std::endl;
        std::cout << "[AudioProcessor] 閰嶇疆:" << std::endl;
        std::cout << "  - 澶勭悊妯″紡: " << (config_.processing_mode == processing_mode::realtime ? "瀹炴椂" : "楂樹繚鐪?) << std::endl;
        std::cout << "  - 鐩爣寤惰繜: " << config_.target_latency_ms << "ms" << std::endl;
        std::cout << "  - CPU闄愬埗: " << config_.cpu_usage_limit_percent << "%" << std::endl;
        std::cout << "  - 缂撳啿鍖哄ぇ灏? " << config_.buffer_size << "鏍锋湰" << std::endl;
        
        return true;
        
    } catch(const std::exception& e) {
        std::cerr << "[AudioProcessor] 鍒濆鍖栧紓甯? " << e.what() << std::endl;
        shutdown();
        return false;
    }
}

// 鍏抽棴
void audio_processor_impl::shutdown() {
    std::cout << "[AudioProcessor] 鍏抽棴闊抽澶勭悊鍣?.." << std::endl;
    
    if(!is_initialized_.load()) {
        return;
    }
    
    should_stop_.store(true);
    is_processing_.store(false);
    
    // 鍋滄澶勭悊绾跨▼
    shutdown_processing_thread();
    
    // 鍏抽棴鍏朵粬缁勪欢
    shutdown_dsp_manager();
    shutdown_output_device();
    shutdown_buffers();
    
    // 娓呯悊鎻掍欢
    {
        std::lock_guard<std::mutex> lock(plugin_mutex_);
        loaded_plugins_.clear();
    }
    
    is_initialized_.store(false);
    std::cout << "[AudioProcessor] 闊抽澶勭悊鍣ㄥ凡鍏抽棴" << std::endl;
}

// 澶勭悊闊抽
bool audio_processor_impl::process_audio(const audio_chunk& input_chunk, audio_chunk& output_chunk, abort_callback& abort) {
    if(!is_initialized_.load() || should_stop_.load() || abort.is_aborting()) {
        return false;
    }
    
    try {
        // 楠岃瘉闊抽鏍煎紡
        if(!validate_audio_format(input_chunk)) {
            handle_error("process_audio", "闊抽鏍煎紡楠岃瘉澶辫触");
            return false;
        }
        
        // 寮€濮嬭鏃?
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 澶勭悊闊抽
        bool success = process_audio_internal(input_chunk, output_chunk, abort);
        
        // 缁撴潫璁℃椂
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time_ms = duration.count() / 1000.0;
        
        // 鏇存柊缁熻淇℃伅
        update_stats(output_chunk, processing_time_ms);
        
        // 妫€鏌ユ€ц兘闄愬埗
        check_performance_limits();
        
        return success;
        
    } catch(const std::exception& e) {
        handle_error("process_audio", e.what());
        return false;
    }
}

// 澶勭悊娴?
bool audio_processor_impl::process_stream(audio_source* source, audio_sink* sink, abort_callback& abort) {
    if(!is_initialized_.load() || !source || !sink || should_stop_.load() || abort.is_aborting()) {
        return false;
    }
    
    is_processing_.store(true);
    
    try {
        audio_chunk input_chunk;
        audio_chunk output_chunk;
        
        // 澶勭悊闊抽娴?
        while(!abort.is_aborting() && !should_stop_.load()) {
            // 浠庢簮鑾峰彇闊抽鏁版嵁
            if(!source->get_next_chunk(input_chunk, abort)) {
                break;
            }
            
            // 澶勭悊闊抽鍧?
            if(!process_audio(input_chunk, output_chunk, abort)) {
                std::cerr << "[AudioProcessor] 闊抽澶勭悊澶辫触" << std::endl;
                break;
            }
            
            // 杈撳嚭鍒扮洰鏍?
            if(!sink->write_chunk(output_chunk, abort)) {
                std::cerr << "[AudioProcessor] 闊抽杈撳嚭澶辫触" << std::endl;
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

// 娣诲姞DSP鏁堟灉鍣?
void audio_processor_impl::add_dsp_effect(std::unique_ptr<dsp_effect_advanced> effect) {
    if(!dsp_manager_) {
        return;
    }
    
    dsp_manager_->add_effect(std::move(effect));
    std::cout << "[AudioProcessor] 娣诲姞DSP鏁堟灉鍣? << std::endl;
}

// 绉婚櫎DSP鏁堟灉鍣?
void audio_processor_impl::remove_dsp_effect(size_t index) {
    if(!dsp_manager_) {
        return;
    }
    
    dsp_manager_->remove_effect(index);
    std::cout << "[AudioProcessor] 绉婚櫎DSP鏁堟灉鍣? << std::endl;
}

// 娓呯┖DSP鏁堟灉鍣?
void audio_processor_impl::clear_dsp_effects() {
    if(!dsp_manager_) {
        return;
    }
    
    dsp_manager_->clear_effects();
    std::cout << "[AudioProcessor] 娓呯┖鎵€鏈塂SP鏁堟灉鍣? << std::endl;
}

// 鑾峰彇DSP鏁堟灉鍣ㄦ暟閲?
size_t audio_processor_impl::get_dsp_effect_count() const {
    return dsp_manager_ ? dsp_manager_->get_effect_count() : 0;
}

// 鑾峰彇DSP鏁堟灉鍣?
dsp_effect_advanced* audio_processor_impl::get_dsp_effect(size_t index) {
    return dsp_manager_ ? dsp_manager_->get_effect(index) : nullptr;
}

// 璁剧疆杈撳嚭璁惧
void audio_processor_impl::set_output_device(std::unique_ptr<output_device> device) {
    if(is_processing_.load()) {
        std::cerr << "[AudioProcessor] 鏃犳硶鍦ㄥ鐞嗚繃绋嬩腑鏇存敼杈撳嚭璁惧" << std::endl;
        return;
    }
    
    output_device_ = std::move(device);
    std::cout << "[AudioProcessor] 璁剧疆杈撳嚭璁惧" << std::endl;
}

// 鑾峰彇杈撳嚭璁惧
output_device* audio_processor_impl::get_output_device() {
    return output_device_.get();
}

// 璁剧疆闊抽噺
void audio_processor_impl::set_volume(float volume) {
    volume_.store(std::max(0.0f, std::min(1.0f, volume)));
    
    if(output_device_) {
        output_device_->volume_set(volume_.load());
    }
    
    std::cout << "[AudioProcessor] 璁剧疆闊抽噺: " << volume_.load() << std::endl;
}

// 鑾峰彇闊抽噺
float audio_processor_impl::get_volume() const {
    return volume_.load();
}

// 璁剧疆闈欓煶
void audio_processor_impl::set_mute(bool mute) {
    is_muted_.store(mute);
    std::cout << "[AudioProcessor] 璁剧疆闈欓煶: " << (mute ? "鍚敤" : "绂佺敤") << std::endl;
}

// 鏄惁闈欓煶
bool audio_processor_impl::is_muted() const {
    return is_muted_.load();
}

// 鍚敤DSP
void audio_processor_impl::enable_dsp(bool enable) {
    dsp_enabled_.store(enable);
    std::cout << "[AudioProcessor] DSP: " << (enable ? "鍚敤" : "绂佺敤") << std::endl;
}

// 鏄惁鍚敤DSP
bool audio_processor_impl::is_dsp_enabled() const {
    return dsp_enabled_.load();
}

// 鍚敤杈撳嚭璁惧
void audio_processor_impl::enable_output_device(bool enable) {
    output_enabled_.store(enable);
    std::cout << "[AudioProcessor] 杈撳嚭璁惧: " << (enable ? "鍚敤" : "绂佺敤") << std::endl;
}

// 鏄惁鍚敤杈撳嚭璁惧
bool audio_processor_impl::is_output_device_enabled() const {
    return output_enabled_.load();
}

// 鑾峰彇缁熻淇℃伅
audio_processor_stats audio_processor_impl::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

// 閲嶇疆缁熻淇℃伅
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
    
    std::cout << "[AudioProcessor] 缁熻淇℃伅宸查噸缃? << std::endl;
}

// 鑾峰彇鐘舵€佹姤鍛?
std::string audio_processor_impl::get_status_report() const {
    std::stringstream report;
    report << "闊抽澶勭悊鍣ㄧ姸鎬佹姤鍛奬n";
    report << "====================\n\n";
    
    // 鍩烘湰鐘舵€?
    report << "鍩烘湰鐘舵€?\n";
    report << "  鍒濆鍖栫姸鎬? " << (is_initialized_.load() ? "宸插垵濮嬪寲" : "鏈垵濮嬪寲") << "\n";
    report << "  澶勭悊鐘舵€? " << (is_processing_.load() ? "澶勭悊涓? : "绌洪棽") << "\n";
    report << "  DSP鐘舵€? " << (dsp_enabled_.load() ? "鍚敤" : "绂佺敤") << "\n";
    report << "  杈撳嚭璁惧鐘舵€? " << (output_enabled_.load() ? "鍚敤" : "绂佺敤") << "\n";
    report << "  闈欓煶鐘舵€? " << (is_muted_.load() ? "闈欓煶" : "姝ｅ父") << "\n";
    report << "  闊抽噺: " << std::fixed << std::setprecision(2) << volume_.load() << "\n";
    report << "  澶勭悊妯″紡: " << (processing_mode_.load() == processing_mode::realtime ? "瀹炴椂" : "楂樹繚鐪?) << "\n\n";
    
    // 闊抽鏍煎紡
    report << "闊抽鏍煎紡:\n";
    report << "  閲囨牱鐜? " << current_format_.sample_rate << "Hz\n";
    report << "  澹伴亾鏁? " << current_format_.channels << "\n";
    report << "  浣嶆繁搴? " << current_format_.bits_per_sample << "bit\n";
    report << "  鏍煎紡: " << (current_format_.format == sample_format::float32 ? "float32" : "int16") << "\n\n";
    
    // 鎬ц兘缁熻
    report << "鎬ц兘缁熻:\n";
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        report << "  鎬婚噰鏍锋暟: " << stats_.total_samples_processed << "\n";
        report << "  鎬诲鐞嗘椂闂? " << std::fixed << std::setprecision(3) << stats_.total_processing_time_ms << "ms\n";
        report << "  骞冲潎澶勭悊鏃堕棿: " << stats_.average_processing_time_ms << "ms\n";
        report << "  褰撳墠CPU鍗犵敤: " << std::fixed << std::setprecision(1) << stats_.current_cpu_usage << "%\n";
        report << "  宄板€糃PU鍗犵敤: " << stats_.peak_cpu_usage << "%\n";
        report << "  寤惰繜: " << std::fixed << std::setprecision(2) << stats_.latency_ms << "ms\n";
        report << "  涓㈠寘鏁? " << stats_.dropout_count << "\n";
        report << "  閿欒鏁? " << stats_.error_count << "\n\n";
    }
    
    // DSP鎶ュ憡
    report << generate_dsp_report();
    
    // 鎻掍欢鎶ュ憡
    report << generate_plugin_report();
    
    // 鎬ц兘鎶ュ憡
    report << generate_performance_report();
    
    return report.str();
}

// 璁剧疆澶勭悊妯″紡
bool audio_processor_impl::set_processing_mode(processing_mode mode) {
    if(is_processing_.load()) {
        std::cerr << "[AudioProcessor] 鏃犳硶鍦ㄨ繍琛屼腑鏇存敼澶勭悊妯″紡" << std::endl;
        return false;
    }
    
    processing_mode_.store(mode);
    
    // 鏇存柊DSP绠＄悊鍣ㄩ厤缃?
    if(dsp_manager_) {
        dsp_config dsp_config;
        dsp_config.enable_multithreading = (mode == processing_mode::high_fidelity);
        dsp_config.target_cpu_usage = cpu_usage_limit_.load();
        dsp_manager_->update_config(dsp_config);
    }
    
    std::cout << "[AudioProcessor] 澶勭悊妯″紡璁剧疆涓? " << (mode == processing_mode::realtime ? "瀹炴椂" : "楂樹繚鐪?) << std::endl;
    return true;
}

// 鑾峰彇澶勭悊妯″紡
processing_mode audio_processor_impl::get_processing_mode() const {
    return processing_mode_.load();
}

// 璁剧疆寤惰繜鐩爣
void audio_processor_impl::set_latency_target(double milliseconds) {
    latency_target_ms_.store(std::max(1.0, std::min(1000.0, milliseconds)));
    std::cout << "[AudioProcessor] 寤惰繜鐩爣璁剧疆涓? " << latency_target_ms_.load() << "ms" << std::endl;
}

// 鑾峰彇寤惰繜鐩爣
double audio_processor_impl::get_latency_target() const {
    return latency_target_ms_.load();
}

// 鍚敤鎬ц兘鐩戞帶
void audio_processor_impl::enable_performance_monitoring(bool enable) {
    performance_monitoring_enabled_.store(enable);
    std::cout << "[AudioProcessor] 鎬ц兘鐩戞帶: " << (enable ? "鍚敤" : "绂佺敤") << std::endl;
}

// 鏄惁鍚敤鎬ц兘鐩戞帶
bool audio_processor_impl::is_performance_monitoring_enabled() const {
    return performance_monitoring_enabled_.load();
}

// 璁剧疆CPU浣跨敤闄愬埗
void audio_processor_impl::set_cpu_usage_limit(float percent) {
    cpu_usage_limit_.store(std::max(1.0f, std::min(100.0f, percent)));
    std::cout << "[AudioProcessor] CPU浣跨敤闄愬埗璁剧疆涓? " << cpu_usage_limit_.load() << "%" << std::endl;
}

// 鑾峰彇CPU浣跨敤闄愬埗
float audio_processor_impl::get_cpu_usage_limit() const {
    return cpu_usage_limit_.load();
}

// 鍔犺浇鎻掍欢
bool audio_processor_impl::load_plugin(const std::string& plugin_path) {
    std::lock_guard<std::mutex> lock(plugin_mutex_);
    
    // 杩欓噷搴旇瀹炵幇鎻掍欢鍔犺浇閫昏緫
    // 鐩墠杩斿洖false琛ㄧず鏈疄鐜?
    std::cerr << "[AudioProcessor] 鎻掍欢鍔犺浇鍔熻兘鏈疄鐜? << std::endl;
    return false;
}

// 鍗歌浇鎻掍欢
bool audio_processor_impl::unload_plugin(const std::string& plugin_name) {
    std::lock_guard<std::mutex> lock(plugin_mutex_);
    
    auto it = loaded_plugins_.find(plugin_name);
    if(it != loaded_plugins_.end()) {
        loaded_plugins_.erase(it);
        std::cout << "[AudioProcessor] 鍗歌浇鎻掍欢: " << plugin_name << std::endl;
        return true;
    }
    
    return false;
}

// 鑾峰彇宸插姞杞芥彃浠朵俊鎭?
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

// 淇濆瓨閰嶇疆
bool audio_processor_impl::save_configuration(const std::string& config_file) {
    // 杩欓噷搴旇瀹炵幇閰嶇疆淇濆瓨閫昏緫
    std::cout << "[AudioProcessor] 淇濆瓨閰嶇疆鍒? " << config_file << std::endl;
    return true;
}

// 鍔犺浇閰嶇疆
bool audio_processor_impl::load_configuration(const std::string& config_file) {
    // 杩欓噷搴旇瀹炵幇閰嶇疆鍔犺浇閫昏緫
    std::cout << "[AudioProcessor] 浠庢枃浠跺姞杞介厤缃? " << config_file << std::endl;
    return true;
}

// 璁剧疆瀹炴椂鍙傛暟
void audio_processor_impl::set_realtime_parameter(const std::string& effect_name, const std::string& param_name, float value) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    realtime_parameters_[effect_name][param_name] = value;
    
    // 鏇存柊鏁堟灉鍣ㄥ弬鏁?
    if(dsp_manager_) {
        for(size_t i = 0; i < dsp_manager_->get_effect_count(); ++i) {
            auto* effect = dsp_manager_->get_effect(i);
            if(effect && effect->get_name() == effect_name) {
                // 杩欓噷搴旇璋冪敤鏁堟灉鍣ㄧ殑鍙傛暟璁剧疆鏂规硶
                break;
            }
        }
    }
}

// 鑾峰彇瀹炴椂鍙傛暟
float audio_processor_impl::get_realtime_parameter(const std::string& effect_name, const std::string& param_name) const {
    std::lock_guard<std::mutex> lock(param_mutex_);
    
    auto effect_it = realtime_parameters_.find(effect_name);
    if(effect_it != realtime_parameters_.end()) {
        auto param_it = effect_it->second.find(param_name);
        if(param_it != effect_it->second.end()) {
            return param_it->second;
        }
    }
    
    return 0.0f; // 榛樿鍊?
}

// 鑾峰彇瀹炴椂鍙傛暟淇℃伅
std::vector<parameter_info> audio_processor_impl::get_realtime_parameters(const std::string& effect_name) const {
    std::lock_guard<std::mutex> lock(param_mutex_);
    
    std::vector<parameter_info> params;
    
    auto effect_it = realtime_parameters_.find(effect_name);
    if(effect_it != realtime_parameters_.end()) {
        for(const auto& [param_name, value] : effect_it->second) {
            parameter_info info;
            info.name = param_name;
            info.value = value;
            info.min_value = 0.0f;  // 榛樿鍊硷紝瀹為檯搴旇浠庢晥鏋滃櫒鑾峰彇
            info.max_value = 1.0f;
            info.default_value = 0.5f;
            params.push_back(info);
        }
    }
    
    return params;
}

// 鍒濆鍖朌SP绠＄悊鍣?
bool audio_processor_impl::initialize_dsp_manager() {
    dsp_manager_ = std::make_unique<dsp_manager>();
    
    dsp_config dsp_config;
    dsp_config.enable_multithreading = (processing_mode_.load() == processing_mode::high_fidelity);
    dsp_config.enable_performance_monitoring = performance_monitoring_enabled_.load();
    dsp_config.max_effects = config_.max_dsp_effects;
    dsp_config.target_cpu_usage = cpu_usage_limit_.load();
    dsp_config.memory_pool_size = config_.buffer_size * 4 * 1024; // 浼扮畻鍐呭瓨姹犲ぇ灏?
    dsp_config.max_latency_ms = latency_target_ms_.load();
    dsp_config.enable_standard_effects = true;
    
    if(!dsp_manager_->initialize(dsp_config)) {
        std::cerr << "[AudioProcessor] DSP绠＄悊鍣ㄥ垵濮嬪寲澶辫触" << std::endl;
        return false;
    }
    
    std::cout << "[AudioProcessor] DSP绠＄悊鍣ㄥ垵濮嬪寲鎴愬姛" << std::endl;
    return true;
}

// 鍒濆鍖栬緭鍑鸿澶?
bool audio_processor_impl::initialize_output_device() {
    if(!output_device_) {
        std::cerr << "[AudioProcessor] 鏈缃緭鍑鸿澶? << std::endl;
        return false;
    }
    
    // 杩欓噷搴旇鍒濆鍖栬緭鍑鸿澶?
    std::cout << "[AudioProcessor] 杈撳嚭璁惧鍒濆鍖栨垚鍔? << std::endl;
    return true;
}

// 鍒濆鍖栫紦鍐插尯
bool audio_processor_impl::initialize_buffers() {
    size_t buffer_size = config_.buffer_size * current_format_.channels;
    
    input_buffer_ = std::make_unique<ring_buffer>(buffer_size * 4); // 4鍊嶇紦鍐?
    output_buffer_ = std::make_unique<ring_buffer>(buffer_size * 4);
    dsp_buffer_ = std::make_unique<ring_buffer>(buffer_size * 2);
    
    std::cout << "[AudioProcessor] 缂撳啿鍖哄垵濮嬪寲鎴愬姛锛屽ぇ灏? " << buffer_size << "鏍锋湰" << std::endl;
    return true;
}

// 鍒濆鍖栧鐞嗙嚎绋?
bool audio_processor_impl::initialize_processing_thread() {
    processing_thread_running_.store(true);
    processing_thread_ = std::thread(&audio_processor_impl::processing_thread_func, this);
    
    std::cout << "[AudioProcessor] 澶勭悊绾跨▼鍒濆鍖栨垚鍔? << std::endl;
    return true;
}

// 鍏抽棴DSP绠＄悊鍣?
void audio_processor_impl::shutdown_dsp_manager() {
    if(dsp_manager_) {
        dsp_manager_->shutdown();
        dsp_manager_.reset();
        std::cout << "[AudioProcessor] DSP绠＄悊鍣ㄥ凡鍏抽棴" << std::endl;
    }
}

// 鍏抽棴杈撳嚭璁惧
void audio_processor_impl::shutdown_output_device() {
    if(output_device_) {
        // 杩欓噷搴旇鍏抽棴杈撳嚭璁惧
        std::cout << "[AudioProcessor] 杈撳嚭璁惧宸插叧闂? << std::endl;
    }
}

// 鍏抽棴缂撳啿鍖?
void audio_processor_impl::shutdown_buffers() {
    input_buffer_.reset();
    output_buffer_.reset();
    dsp_buffer_.reset();
    std::cout << "[AudioProcessor] 缂撳啿鍖哄凡鍏抽棴" << std::endl;
}

// 鍏抽棴澶勭悊绾跨▼
void audio_processor_impl::shutdown_processing_thread() {
    should_stop_.store(true);
    processing_cv_.notify_all();
    
    if(processing_thread_.joinable()) {
        processing_thread_.join();
    }
    
    processing_thread_running_.store(false);
    std::cout << "[AudioProcessor] 澶勭悊绾跨▼宸插叧闂? << std::endl;
}

// 澶勭悊绾跨▼鍑芥暟
void audio_processor_impl::processing_thread_func() {
    std::cout << "[AudioProcessor] 澶勭悊绾跨▼鍚姩" << std::endl;
    
    // 鎻愬崌绾跨▼浼樺厛绾?
    set_thread_priority_high();
    
    while(processing_thread_running_.load() && !should_stop_.load()) {
        std::unique_lock<std::mutex> lock(processing_mutex_);
        
        // 绛夊緟澶勭悊淇″彿
        processing_cv_.wait_for(lock, std::chrono::milliseconds(1), [this] {
            return !processing_thread_running_.load() || should_stop_.load();
        });
        
        if(should_stop_.load()) {
            break;
        }
        
        // 杩欓噷搴旇瀹炵幇鍚庡彴澶勭悊閫昏緫
        // 鐩墠鍙槸绠€鍗曠殑寰幆
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[AudioProcessor] 澶勭悊绾跨▼鍋滄" << std::endl;
}

// 鍐呴儴闊抽澶勭悊
bool audio_processor_impl::process_audio_internal(const audio_chunk& input, audio_chunk& output, abort_callback& abort) {
    if(abort.is_aborting()) {
        return false;
    }
    
    // 澶嶅埗杈撳叆鏁版嵁
    output.copy(input);
    
    // 搴旂敤闈欓煶
    if(is_muted_.load()) {
        apply_mute(output);
        return true;
    }
    
    // DSP澶勭悊
    if(dsp_enabled_.load() && dsp_manager_) {
        if(!process_dsp_chain(output, abort)) {
            return false;
        }
    }
    
    // 搴旂敤闊抽噺
    apply_volume(output);
    
    // 杈撳嚭璁惧澶勭悊
    if(output_enabled_.load() && output_device_) {
        if(!process_output_device(output, abort)) {
            return false;
        }
    }
    
    return true;
}

// 澶勭悊DSP閾?
bool audio_processor_impl::process_dsp_chain(audio_chunk& chunk, abort_callback& abort) {
    if(!dsp_manager_) {
        return true;
    }
    
    return dsp_manager_->process_chain(chunk, abort);
}

// 澶勭悊杈撳嚭璁惧
bool audio_processor_impl::process_output_device(audio_chunk& chunk, abort_callback& abort) {
    if(!output_device_) {
        return true;
    }
    
    // 杩欓噷搴旇瀹炵幇杈撳嚭璁惧澶勭悊閫昏緫
    // 鐩墠鍙槸杩斿洖鎴愬姛
    return true;
}

// 鏇存柊缁熻淇℃伅
void audio_processor_impl::update_stats(const audio_chunk& chunk, double processing_time) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_samples_processed += chunk.get_sample_count() * chunk.get_channels();
    stats_.total_processing_time_ms += processing_time;
    
    // 璁＄畻骞冲潎澶勭悊鏃堕棿
    if(stats_.total_samples_processed > 0) {
        stats_.average_processing_time_ms = stats_.total_processing_time_ms / (stats_.total_samples_processed / 44100.0); // 鍋囪44.1kHz
    }
    
    // 鏇存柊CPU鍗犵敤锛堢畝鍖栬绠楋級
    stats_.current_cpu_usage = static_cast<float>(processing_time / 10.0 * 100.0); // 鍋囪10ms鏄?00%鍗犵敤
    if(stats_.current_cpu_usage > stats_.peak_cpu_usage) {
        stats_.peak_cpu_usage = stats_.current_cpu_usage;
    }
    
    stats_.latency_ms = latency_target_ms_.load();
    stats_.processing_mode = processing_mode_.load();
}

// 妫€鏌ユ€ц兘闄愬埗
void audio_processor_impl::check_performance_limits() {
    if(stats_.current_cpu_usage > cpu_usage_limit_.load()) {
        // CPU浣跨敤鐜囪秴杩囬檺鍒?
        std::cerr << "[AudioProcessor] CPU浣跨敤鐜囪秴杩囬檺鍒? " << stats_.current_cpu_usage 
                  << "% > " << cpu_usage_limit_.load() << "%" << std::endl;
    }
}

// 澶勭悊閿欒
void audio_processor_impl::handle_error(const std::string& operation, const std::string& error) {
    std::cerr << "[AudioProcessor] " << operation << " 閿欒: " << error << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.error_count++;
    }
}

// 楠岃瘉闊抽鏍煎紡
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

// 搴旂敤闊抽噺
void audio_processor_impl::apply_volume(audio_chunk& chunk) {
    float vol = volume_.load();
    if(vol == 1.0f) {
        return; // 鏃犻渶澶勭悊
    }
    
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    for(size_t i = 0; i < total_samples; ++i) {
        data[i] *= vol;
    }
}

// 搴旂敤闈欓煶
void audio_processor_impl::apply_mute(audio_chunk& chunk) {
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    std::fill(data, data + total_samples, 0.0f);
}

// 鐢熸垚鎻掍欢鎶ュ憡
std::string audio_processor_impl::generate_plugin_report() const {
    std::stringstream report;
    report << "鎻掍欢淇℃伅:\n";
    
    std::lock_guard<std::mutex> lock(plugin_mutex_);
    
    if(loaded_plugins_.empty()) {
        report << "  鏃犲凡鍔犺浇鎻掍欢\n";
    } else {
        for(const auto& [name, plugin] : loaded_plugins_) {
            if(plugin) {
                report << "  - " << name << " v" << plugin->get_version() 
                       << " (" << (plugin->is_enabled() ? "鍚敤" : "绂佺敤") << ")\n";
            }
        }
    }
    
    report << "\n";
    return report.str();
}

// 鐢熸垚DSP鎶ュ憡
std::string audio_processor_impl::generate_dsp_report() const {
    std::stringstream report;
    report << "DSP淇℃伅:\n";
    
    if(dsp_manager_) {
        report << dsp_manager_->generate_dsp_report();
    } else {
        report << "  DSP绠＄悊鍣ㄦ湭鍒濆鍖朶n";
    }
    
    report << "\n";
    return report.str();
}

// 鐢熸垚鎬ц兘鎶ュ憡
std::string audio_processor_impl::generate_performance_report() const {
    std::stringstream report;
    report << "鎬ц兘鎶ュ憡:\n";
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        // CPU浣跨敤鐜囧垎鏋?
        report << "  CPU浣跨敤鐜?\n";
        report << "    褰撳墠: " << std::fixed << std::setprecision(1) << stats_.current_cpu_usage << "%\n";
        report << "    宄板€? " << stats_.peak_cpu_usage << "%\n";
        report << "    闄愬埗: " << cpu_usage_limit_.load() << "%\n";
        
        // 寤惰繜鍒嗘瀽
        report << "  寤惰繜鎬ц兘:\n";
        report << "    褰撳墠寤惰繜: " << std::fixed << std::setprecision(2) << stats_.latency_ms << "ms\n";
        report << "    鐩爣寤惰繜: " << latency_target_ms_.load() << "ms\n";
        report << "    骞冲潎澶勭悊鏃堕棿: " << stats_.average_processing_time_ms << "ms\n";
        
        // 閿欒缁熻
        report << "  閿欒缁熻:\n";
        report << "    鎬婚敊璇暟: " << stats_.error_count << "\n";
        report << "    涓㈠寘鏁? " << stats_.dropout_count << "\n";
        
        // 鎬ц兘寤鸿
        if(stats_.current_cpu_usage > cpu_usage_limit_.load() * 0.9f) {
            report << "  鎬ц兘璀﹀憡: CPU浣跨敤鐜囨帴杩戦檺鍒禱n";
        }
        
        if(stats_.latency_ms > latency_target_ms_.load() * 1.5f) {
            report << "  鎬ц兘璀﹀憡: 寤惰繜瓒呰繃鐩爣鍊糪n";
        }
        
        if(stats_.error_count > 0) {
            report << "  鎬ц兘璀﹀憡: 瀛樺湪澶勭悊閿欒\n";
        }
    }
    
    report << "\n";
    return report.str();
}

// 鍒涘缓闊抽澶勭悊鍣?
std::unique_ptr<audio_processor_advanced> create_audio_processor() {
    return std::make_unique<audio_processor_impl>();
}

} // namespace fb2k