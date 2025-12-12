#pragma once

#include "../stage1_2/audio_output.h"
#include "../stage1_3/dsp_effect.h"
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace fb2k {

// ASIO椹卞姩淇℃伅缁撴瀯
struct asio_driver_info {
    std::string name;
    std::string id;
    std::string description;
    std::string version;
    std::string clsid;
    bool is_active;
    int input_channels;
    int output_channels;
    std::vector<double> supported_sample_rates;
    int buffer_size_min;
    int buffer_size_max;
    int buffer_size_preferred;
    int buffer_size_granularity;
};

// ASIO缂撳啿鍖轰俊鎭?
struct asio_buffer_info {
    long buffer_index;
    long channel_index;
    void* buffer[2];  // 鍙岀紦鍐?
    long data_size;
    long sample_type;
    bool is_active;
};

// ASIO鏃堕棿淇℃伅
struct asio_time_info {
    double sample_position;
    double system_time;
    double sample_rate;
    unsigned long flags;
    char future[64];
};

// ASIO鍥炶皟鍑芥暟绫诲瀷
typedef long (*asio_callback)(long buffer_index);
typedef void (*asio_sample_rate_callback)(double sample_rate);
typedef long (*asio_message_callback)(long selector, long value, void* message, double* opt);
typedef void (*asio_buffer_switch_callback)(long buffer_index, long direct_process);

// ASIO鎺ュ彛瀹氫箟
class asio_driver_interface {
public:
    virtual ~asio_driver_interface() = default;
    
    // 鍩烘湰鎿嶄綔
    virtual long init(void* sys_handle) = 0;
    virtual long get_driver_name(char* name) = 0;
    virtual long get_driver_version() = 0;
    virtual long get_error_message(char* string) = 0;
    
    // 鍚姩鍜屽仠姝?
    virtual long start() = 0;
    virtual long stop() = 0;
    
    // 缂撳啿鍖虹鐞?
    virtual long get_channels(long* num_input_channels, long* num_output_channels) = 0;
    virtual long get_latencies(long* input_latency, long* output_latency) = 0;
    virtual long get_buffer_size(long* min_size, long* max_size, long* preferred_size, long* granularity) = 0;
    virtual long can_sample_rate(double sample_rate) = 0;
    virtual long get_sample_rate(double* sample_rate) = 0;
    virtual long set_sample_rate(double sample_rate) = 0;
    virtual long get_clock_sources(long* clocks, long* num_sources) = 0;
    virtual long set_clock_source(long reference) = 0;
    
    // 缂撳啿鍖烘搷浣?
    virtual long get_sample_position(long long* sample_pos, long long* time_stamp) = 0;
    virtual long get_channel_info(long channel, long is_input, long* sample_type, char* name, long* name_len, long* channel_group) = 0;
    virtual long create_buffers(asio_buffer_info* buffer_infos, long num_channels, long buffer_size, asio_callback callbacks) = 0;
    virtual long dispose_buffers() = 0;
    virtual long control_panel() = 0;
    virtual long future(long selector, void* opt) = 0;
    virtual long output_ready() = 0;
};

// ASIO鏃堕棿浠ｇ爜
enum asio_time_code_flags {
    kSystemTimeValid = 1,
    kSamplePositionValid = 2,
    kSampleRateValid = 4,
    kSpeedValid = 8,
    kSampleRateChanged = 16,
    kClockSourceChanged = 32
};

// ASIO鏍锋湰绫诲瀷
enum asio_sample_type {
    ASIOST_16LSB = 0,
    ASIOST_24LSB = 1,
    ASIOST_32LSB = 2,
    ASIOST_16MSB = 3,
    ASIOST_24MSB = 4,
    ASIOST_32MSB = 5,
    ASIOST_FLOAT32LSB = 6,
    ASIOST_FLOAT32MSB = 7,
    ASIOST_FLOAT64LSB = 8,
    ASIOST_FLOAT64MSB = 9,
    ASIOST_DWORD = 10,
    ASIOST_LAST = 11
};

// ASIO閿欒浠ｇ爜
enum asio_error {
    ASE_OK = 0,
    ASE_SUCCESS = 0x3f4847a0,
    ASE_NotPresent = -1000,
    ASE_HWMalfunction = -999,
    ASE_InvalidParameter = -998,
    ASE_InvalidMode = -997,
    ASE_SPNotAdvancing = -996,
    ASE_NoClock = -995,
    ASE_NoMemory = -994
};

// ASIO杈撳嚭璁惧鎺ュ彛
class output_asio : public output_device {
public:
    // ASIO鐗瑰畾鎺ュ彛
    virtual bool enum_drivers(std::vector<asio_driver_info>& drivers) = 0;
    virtual bool load_driver(const std::string& driver_id) = 0;
    virtual void unload_driver() = 0;
    virtual bool is_driver_loaded() const = 0;
    virtual std::string get_current_driver_name() const = 0;
    
    // ASIO閰嶇疆
    virtual void set_buffer_size(long size) = 0;
    virtual long get_buffer_size() const = 0;
    virtual void set_sample_rate(double rate) = 0;
    virtual double get_sample_rate() const = 0;
    virtual std::vector<double> get_available_sample_rates() const = 0;
    
    // 鎬ц兘淇℃伅
    virtual long get_input_latency() const = 0;
    virtual long get_output_latency() const = 0;
    virtual double get_cpu_load() const = 0;
    
    // 楂樼骇鍔熻兘
    virtual bool supports_time_code() const = 0;
    virtual bool supports_input_monitoring() const = 0;
    virtual bool supports_variable_buffer_size() const = 0;
    virtual void show_control_panel() = 0;
    
    // 缂撳啿鍖轰俊鎭?
    virtual long get_buffer_size_min() const = 0;
    virtual long get_buffer_size_max() const = 0;
    virtual long get_buffer_size_preferred() const = 0;
    virtual long get_buffer_size_granularity() const = 0;
    
    // 鏃堕挓婧?
    virtual std::vector<std::string> get_clock_sources() const = 0;
    virtual void set_clock_source(long index) = 0;
    virtual long get_current_clock_source() const = 0;
};

// ASIO椹卞姩鍔犺浇鍣?
class asio_driver_loader {
public:
    static std::unique_ptr<asio_driver_interface> load_driver(const std::string& clsid);
    static void unload_driver(asio_driver_interface* driver);
    static std::vector<asio_driver_info> enumerate_drivers();
    static bool is_driver_available(const std::string& clsid);
};

// ASIO缂撳啿鍖虹鐞嗗櫒
class asio_buffer_manager {
public:
    asio_buffer_manager();
    ~asio_buffer_manager();
    
    bool initialize(long num_channels, long buffer_size, asio_sample_type sample_type);
    void cleanup();
    
    asio_buffer_info* get_buffer_info(long channel);
    void switch_buffers();
    
    float* get_input_buffer(long channel, long buffer_index);
    float* get_output_buffer(long channel, long buffer_index);
    
    long get_current_buffer_index() const { return current_buffer_index_; }
    long get_buffer_size() const { return buffer_size_; }
    
private:
    std::vector<asio_buffer_info> buffer_infos_;
    long num_channels_;
    long buffer_size_;
    long current_buffer_index_;
    asio_sample_type sample_type_;
    
    bool allocate_buffers();
    void free_buffers();
    long get_sample_size(asio_sample_type type) const;
};

// ASIO鏃堕棿绠＄悊鍣?
class asio_time_manager {
public:
    asio_time_manager();
    ~asio_time_manager();
    
    void update_time_info(const asio_time_info& info);
    asio_time_info get_current_time_info() const;
    
    double get_sample_position() const;
    double get_system_time() const;
    double get_sample_rate() const;
    
    bool is_sample_rate_changed() const;
    bool is_clock_source_changed() const;
    
    void clear_flags();
    
private:
    mutable asio_time_info time_info_;
    mutable std::mutex time_mutex_;
};

// ASIO鍥炶皟澶勭悊鍣?
class asio_callback_handler {
public:
    asio_callback_handler();
    ~asio_callback_handler();
    
    // 璁剧疆鍥炶皟鍑芥暟
    void set_buffer_switch_callback(asio_buffer_switch_callback callback);
    void set_sample_rate_callback(asio_sample_rate_callback callback);
    void set_message_callback(asio_message_callback callback);
    
    // 瑙﹀彂鍥炶皟
    void on_buffer_switch(long buffer_index, long direct_process);
    void on_sample_rate_changed(double sample_rate);
    long on_message(long selector, long value, void* message, double* opt);
    
    // 缂撳啿鍖哄鐞?
    void process_input_buffers(long buffer_index);
    void process_output_buffers(long buffer_index);
    
    // 鏁版嵁澶勭悊鎺ュ彛
    void set_audio_processor(std::function<void(float**, float**, long, long)> processor);
    void set_input_data(float** input_data, long num_channels, long buffer_size);
    void get_output_data(float** output_data, long num_channels, long buffer_size);
    
private:
    asio_buffer_switch_callback buffer_switch_callback_;
    asio_sample_rate_callback sample_rate_callback_;
    asio_message_callback message_callback_;
    
    std::function<void(float**, float**, long, long)> audio_processor_;
    
    // 杈撳叆杈撳嚭缂撳啿鍖?
    std::vector<std::vector<float>> input_buffers_;
    std::vector<std::vector<float>> output_buffers_;
    std::vector<std::vector<float>> process_buffers_;
    
    long num_channels_;
    long buffer_size_;
    
    std::mutex callback_mutex_;
};

// ASIO閿欒澶勭悊
class asio_error_handler {
public:
    static std::string get_error_string(long error_code);
    static void handle_error(const std::string& operation, long error_code);
    static bool is_recoverable_error(long error_code);
    static void log_asio_error(const std::string& context, long error_code);
};

// ASIO閰嶇疆缁撴瀯
struct asio_config {
    bool enable_exclusive_mode = true;
    bool enable_input_monitoring = false;
    bool enable_time_code = false;
    bool enable_hardware_buffer = true;
    
    long preferred_buffer_size = 512;
    long minimum_buffer_size = 64;
    long maximum_buffer_size = 4096;
    
    double preferred_sample_rate = 44100.0;
    asio_sample_type sample_type = ASIOST_FLOAT32LSB;
    
    long num_input_channels = 0;
    long num_output_channels = 2;
    
    bool enable_cpu_optimization = true;
    bool enable_thread_priority_boost = true;
    int thread_priority = THREAD_PRIORITY_TIME_CRITICAL;
};

// 涓昏鐨凙SIO杈撳嚭瀹炵幇绫?
class output_asio_impl : public output_asio {
public:
    output_asio_impl();
    ~output_asio_impl() override;
    
    // output_device鎺ュ彛瀹炵幇
    void open(t_uint32 sample_rate, t_uint32 channels, t_uint32 flags, abort_callback& p_abort) override;
    void close(abort_callback& p_abort) override;
    t_uint32 get_latency() override;
    void write(const void* buffer, t_size bytes, abort_callback& p_abort) override;
    void pause(bool state) override;
    void flush(abort_callback& p_abort) override;
    void volume_set(float volume) override;
    bool is_playing() override;
    bool can_write() override;
    bool requires_spec_ex() override;
    t_uint32 get_latency_ex() override;
    void get_device_name(pfc::string_base& out) override;
    void get_device_desc(pfc::string_base& out) override;
    t_uint32 get_device_id() override;
    void estimate_latency(double& latency_seconds, t_uint32 sample_rate, t_uint32 channels) override;
    void update_device_list() override;
    bool is_realtime() override;
    void on_idle() override;
    void process_samples(const audio_chunk* p_chunk, t_uint32 p_samples_written, t_uint32 p_samples_total, abort_callback& p_abort) override;
    void pause_ex(bool p_state, t_uint32 p_samples_written) override;
    void set_volume_ex(float p_volume, t_uint32 p_samples_written) override;
    void get_latency_ex2(t_uint32& p_samples, t_uint32& p_samples_total) override;
    void get_latency_ex3(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer) override;
    void get_latency_ex4(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer, t_uint32& p_samples_in_device_buffer) override;
    
    // output_asio鎺ュ彛瀹炵幇
    bool enum_drivers(std::vector<asio_driver_info>& drivers) override;
    bool load_driver(const std::string& driver_id) override;
    void unload_driver() override;
    bool is_driver_loaded() const override;
    std::string get_current_driver_name() const override;
    void set_buffer_size(long size) override;
    long get_buffer_size() const override;
    void set_sample_rate(double rate) override;
    double get_sample_rate() const override;
    std::vector<double> get_available_sample_rates() const override;
    long get_input_latency() const override;
    long get_output_latency() const override;
    double get_cpu_load() const override;
    bool supports_time_code() const override;
    bool supports_input_monitoring() const override;
    bool supports_variable_buffer_size() const override;
    void show_control_panel() override;
    long get_buffer_size_min() const override;
    long get_buffer_size_max() const override;
    long get_buffer_size_preferred() const override;
    long get_buffer_size_granularity() const override;
    std::vector<std::string> get_clock_sources() const override;
    void set_clock_source(long index) override;
    long get_current_clock_source() const override;
    
private:
    // ASIO缁勪欢
    std::unique_ptr<asio_driver_interface> driver_;
    std::unique_ptr<asio_buffer_manager> buffer_manager_;
    std::unique_ptr<asio_time_manager> time_manager_;
    std::unique_ptr<asio_callback_handler> callback_handler_;
    
    // 閰嶇疆
    asio_config config_;
    std::string current_driver_name_;
    std::string current_driver_id_;
    
    // 鐘舵€?
    std::atomic<bool> is_initialized_;
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<float> volume_;
    
    // 闊抽鏍煎紡
    t_uint32 sample_rate_;
    t_uint32 channels_;
    t_uint32 bits_per_sample_;
    
    // 缂撳啿鍖轰俊鎭?
    long buffer_size_;
    long input_latency_;
    long output_latency_;
    
    // 绾跨▼绠＄悊
    std::thread asio_thread_;
    std::atomic<bool> asio_thread_running_;
    std::mutex asio_mutex_;
    std::condition_variable asio_cv_;
    
    // 闊抽鏁版嵁
    std::vector<uint8_t> input_buffer_;
    std::vector<uint8_t> output_buffer_;
    std::atomic<t_uint64> samples_written_;
    std::atomic<t_uint64> samples_played_;
    
    // 鎬ц兘鐩戞帶
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<double> cpu_load_;
    
    // 绉佹湁鏂规硶
    bool initialize_asio();
    bool create_asio_buffers();
    bool start_asio_streaming();
    bool stop_asio_streaming();
    void cleanup_asio();
    
    void asio_thread_func();
    static void buffer_switch_callback(long buffer_index, long direct_process);
    static void sample_rate_callback(double sample_rate);
    static long message_callback(long selector, long value, void* message, double* opt);
    
    void process_audio_data(float** input_channels, float** output_channels, long num_channels, long buffer_size);
    void update_performance_stats();
    
    bool convert_to_asio_format(const WAVEFORMATEX& windows_format, long& asio_sample_type);
    bool validate_asio_format(long asio_sample_type, long num_channels, double sample_rate);
    
    std::string get_asio_sample_type_name(long sample_type);
    void log_asio_info();
    void handle_asio_error(const std::string& operation, long error_code);
};

// 鍒涘缓ASIO杈撳嚭璁惧
std::unique_ptr<output_asio> create_asio_output();

// ASIO宸ュ叿鍑芥暟
namespace asio_utils {
    std::vector<asio_driver_info> enumerate_asio_drivers();
    bool is_asio_available();
    std::string get_asio_version();
    long get_optimal_buffer_size(double sample_rate, int channels);
    bool validate_asio_config(const asio_config& config);
    asio_driver_info get_driver_info(const std::string& driver_id);
}

} // namespace fb2k