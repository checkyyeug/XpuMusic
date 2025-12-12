#pragma once

#include "../stage1_2/audio_output.h"
#include "../stage1_4/output_asio.h"
#include "../stage1_3/output_wasapi.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace fb2k {

// 骞冲彴绫诲瀷鏋氫妇
enum class platform_type {
    unknown = 0,
    windows,
    macos,
    linux,
    ios,
    android
};

// 闊抽鍚庣绫诲瀷
enum class audio_backend {
    unknown = 0,
    wasapi,      // Windows Audio Session API
    asio,        // Audio Stream Input/Output
    coreaudio,   // macOS Core Audio
    alsa,        // Linux Advanced Linux Sound Architecture
    pulseaudio,  // Linux PulseAudio
    jack,        // JACK Audio Connection Kit
    opensl,      // Android OpenSL ES
    aaudio,      // Android AAudio (API 26+)
    audiounit,   // iOS Audio Unit
    custom       // 鑷畾涔夊悗绔?
};

// 璺ㄥ钩鍙拌澶囦俊鎭?
struct cross_platform_device_info {
    std::string id;
    std::string name;
    std::string description;
    audio_backend backend;
    platform_type platform;
    bool is_default;
    bool is_active;
    std::vector<double> supported_sample_rates;
    std::vector<int> supported_channels;
    std::vector<int> supported_buffer_sizes;
    double current_sample_rate;
    int current_channels;
    int current_buffer_size;
    double estimated_latency_ms;
    std::map<std::string, std::string> properties;
};

// 璺ㄥ钩鍙伴煶棰戦厤缃?
struct cross_platform_config {
    audio_backend preferred_backend = audio_backend::unknown;
    platform_type target_platform = platform_type::unknown;
    double preferred_sample_rate = 44100.0;
    int preferred_channels = 2;
    int preferred_buffer_size = 512;
    bool enable_exclusive_mode = false;
    bool enable_hardware_mixing = true;
    bool enable_low_latency_mode = true;
    bool enable_multi_device = false;
    bool enable_device_switching = true;
    int max_retry_count = 3;
    int retry_delay_ms = 100;
    double target_latency_ms = 10.0;
    double max_acceptable_latency_ms = 50.0;
};

// 骞冲彴鎶借薄鎺ュ彛
class platform_audio_backend {
public:
    virtual ~platform_audio_backend() = default;
    
    // 鍩烘湰淇℃伅
    virtual audio_backend get_backend_type() const = 0;
    virtual platform_type get_platform_type() const = 0;
    virtual std::string get_backend_name() const = 0;
    virtual std::string get_backend_version() const = 0;
    
    // 璁惧绠＄悊
    virtual std::vector<cross_platform_device_info> enumerate_devices() = 0;
    virtual bool select_device(const std::string& device_id) = 0;
    virtual cross_platform_device_info get_current_device_info() const = 0;
    virtual bool is_device_available(const std::string& device_id) const = 0;
    
    // 鏍煎紡鏀寔
    virtual std::vector<double> get_supported_sample_rates() const = 0;
    virtual std::vector<int> get_supported_channels() const = 0;
    virtual std::vector<int> get_supported_buffer_sizes() const = 0;
    virtual bool is_format_supported(double sample_rate, int channels, int bits_per_sample) const = 0;
    
    // 闊抽I/O
    virtual bool open(double sample_rate, int channels, int format, int buffer_size) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;
    virtual bool is_paused() const = 0;
    virtual void pause(bool pause) = 0;
    
    // 鏁版嵁浼犺緭
    virtual int write(const void* buffer, int frames) = 0;
    virtual int read(void* buffer, int frames) = 0;
    virtual int get_available_write_frames() const = 0;
    virtual int get_available_read_frames() const = 0;
    virtual double get_latency() const = 0;
    virtual double get_cpu_load() const = 0;
    
    // 閰嶇疆鍜屾帶鍒?
    virtual void set_volume(float volume) = 0;
    virtual float get_volume() const = 0;
    virtual void set_mute(bool mute) = 0;
    virtual bool get_mute() const = 0;
    virtual void set_buffer_size(int size) = 0;
    virtual int get_buffer_size() const = 0;
    
    // 楂樼骇鍔熻兘
    virtual bool supports_exclusive_mode() const { return false; }
    virtual bool supports_hardware_mixing() const { return false; }
    virtual bool supports_low_latency_mode() const { return false; }
    virtual bool supports_multi_device() const { return false; }
    virtual bool supports_device_switching() const { return false; }
    virtual bool enter_exclusive_mode() { return false; }
    virtual void exit_exclusive_mode() {}
    
    // 閿欒澶勭悊
    virtual std::string get_last_error() const = 0;
    virtual void clear_error() = 0;
    virtual bool is_recoverable_error() const = 0;
    virtual bool recover_from_error() { return false; }
    
    // 鎬ц兘鐩戞帶
    virtual std::map<std::string, double> get_performance_metrics() const {
        return {};
    }
    
    // 璁惧浜嬩欢
    virtual void set_device_event_callback(std::function<void(const std::string& event, const std::string& device_id)> callback) {
        device_event_callback_ = callback;
    }
    
protected:
    std::function<void(const std::string& event, const std::string& device_id)> device_event_callback_;
    
    void notify_device_event(const std::string& event, const std::string& device_id) {
        if (device_event_callback_) {
            device_event_callback_(event, device_id);
        }
    }
};

// Windows WASAPI鍚庣
class wasapi_backend : public platform_audio_backend {
public:
    wasapi_backend();
    ~wasapi_backend() override;
    
    audio_backend get_backend_type() const override { return audio_backend::wasapi; }
    platform_type get_platform_type() const override { return platform_type::windows; }
    std::string get_backend_name() const override { return "WASAPI"; }
    std::string get_backend_version() const override { return "1.0"; }
    
    std::vector<cross_platform_device_info> enumerate_devices() override;
    bool select_device(const std::string& device_id) override;
    cross_platform_device_info get_current_device_info() const override;
    bool is_device_available(const std::string& device_id) const override;
    
    std::vector<double> get_supported_sample_rates() const override;
    std::vector<int> get_supported_channels() const override;
    std::vector<int> get_supported_buffer_sizes() const override;
    bool is_format_supported(double sample_rate, int channels, int bits_per_sample) const override;
    
    bool open(double sample_rate, int channels, int format, int buffer_size) override;
    void close() override;
    bool is_open() const override;
    bool start() override;
    void stop() override;
    bool is_running() const override;
    bool is_paused() const override;
    void pause(bool pause) override;
    
    int write(const void* buffer, int frames) override;
    int read(void* buffer, int frames) override;
    int get_available_write_frames() const override;
    int get_available_read_frames() const override;
    double get_latency() const override;
    double get_cpu_load() const override;
    
    void set_volume(float volume) override;
    float get_volume() const override;
    void set_mute(bool mute) override;
    bool get_mute() const override;
    void set_buffer_size(int size) override;
    int get_buffer_size() const override;
    
    bool supports_exclusive_mode() const override { return true; }
    bool supports_low_latency_mode() const override { return true; }
    bool supports_device_switching() const override { return true; }
    bool enter_exclusive_mode() override;
    void exit_exclusive_mode() override;
    
    std::string get_last_error() const override;
    void clear_error() override;
    bool is_recoverable_error() const override;
    bool recover_from_error() override;
    
private:
    std::unique_ptr<output_wasapi> wasapi_output_;
    std::unique_ptr<output_device> wasapi_device_;
    std::string current_device_id_;
    cross_platform_device_info current_device_info_;
    std::string last_error_;
    bool recoverable_error_;
    
    void convert_to_cross_platform_info(const device_info& wasapi_info, cross_platform_device_info& cross_info);
    void update_device_info();
};

// macOS Core Audio鍚庣
class coreaudio_backend : public platform_audio_backend {
public:
    coreaudio_backend();
    ~coreaudio_backend() override;
    
    audio_backend get_backend_type() const override { return audio_backend::coreaudio; }
    platform_type get_platform_type() const override { return platform_type::macos; }
    std::string get_backend_name() const override { return "Core Audio"; }
    std::string get_backend_version() const override { return "1.0"; }
    
    std::vector<cross_platform_device_info> enumerate_devices() override;
    bool select_device(const std::string& device_id) override;
    cross_platform_device_info get_current_device_info() const override;
    bool is_device_available(const std::string& device_id) const override;
    
    std::vector<double> get_supported_sample_rates() const override;
    std::vector<int> get_supported_channels() const override;
    std::vector<int> get_supported_buffer_sizes() const override;
    bool is_format_supported(double sample_rate, int channels, int bits_per_sample) const override;
    
    bool open(double sample_rate, int channels, int format, int buffer_size) override;
    void close() override;
    bool is_open() const override;
    bool start() override;
    void stop() override;
    bool is_running() const override;
    bool is_paused() const override;
    void pause(bool pause) override;
    
    int write(const void* buffer, int frames) override;
    int read(void* buffer, int frames) override;
    int get_available_write_frames() const override;
    int get_available_read_frames() const override;
    double get_latency() const override;
    double get_cpu_load() const override;
    
    void set_volume(float volume) override;
    float get_volume() const override;
    void set_mute(bool mute) override;
    bool get_mute() const override;
    void set_buffer_size(int size) override;
    int get_buffer_size() const override;
    
    bool supports_exclusive_mode() const override { return true; }
    bool supports_low_latency_mode() const override { return true; }
    bool supports_device_switching() const override { return true; }
    bool enter_exclusive_mode() override;
    void exit_exclusive_mode() override;
    
    std::string get_last_error() const override;
    void clear_error() override;
    bool is_recoverable_error() const override;
    bool recover_from_error() override;
    
private:
    // macOS Core Audio瀹炵幇
    struct CoreAudioImpl;
    std::unique_ptr<CoreAudioImpl> impl_;
};

// Linux ALSA鍚庣
class alsa_backend : public platform_audio_backend {
public:
    alsa_backend();
    ~alsa_backend() override;
    
    audio_backend get_backend_type() const override { return audio_backend::alsa; }
    platform_type get_platform_type() const override { return platform_type::linux; }
    std::string get_backend_name() const override { return "ALSA"; }
    std::string get_backend_version() const override { return "1.0"; }
    
    std::vector<cross_platform_device_info> enumerate_devices() override;
    bool select_device(const std::string& device_id) override;
    cross_platform_device_info get_current_device_info() const override;
    bool is_device_available(const std::string& device_id) const override;
    
    std::vector<double> get_supported_sample_rates() const override;
    std::vector<int> get_supported_channels() const override;
    std::vector<int> get_supported_buffer_sizes() const override;
    bool is_format_supported(double sample_rate, int channels, int bits_per_sample) const override;
    
    bool open(double sample_rate, int channels, int format, int buffer_size) override;
    void close() override;
    bool is_open() const override;
    bool start() override;
    void stop() override;
    bool is_running() const override;
    bool is_paused() const override;
    void pause(bool pause) override;
    
    int write(const void* buffer, int frames) override;
    int read(void* buffer, int frames) override;
    int get_available_write_frames() const override;
    int get_available_read_frames() const override;
    double get_latency() const override;
    double get_cpu_load() const override;
    
    void set_volume(float volume) override;
    float get_volume() const override;
    void set_mute(bool mute) override;
    bool get_mute() const override;
    void set_buffer_size(int size) override;
    int get_buffer_size() const override;
    
    bool supports_exclusive_mode() const override { return true; }
    bool supports_low_latency_mode() const override { return true; }
    bool supports_device_switching() const override { return true; }
    bool enter_exclusive_mode() override;
    void exit_exclusive_mode() override;
    
    std::string get_last_error() const override;
    void clear_error() override;
    bool is_recoverable_error() const override;
    bool recover_from_error() override;
    
private:
    // ALSA瀹炵幇
    struct ALSAImpl;
    std::unique_ptr<ALSAImpl> impl_;
};

// Linux PulseAudio鍚庣
class pulseaudio_backend : public platform_audio_backend {
public:
    pulseaudio_backend();
    ~pulseaudio_backend() override;
    
    audio_backend get_backend_type() const override { return audio_backend::pulseaudio; }
    platform_type get_platform_type() const override { return platform_type::linux; }
    std::string get_backend_name() const override { return "PulseAudio"; }
    std::string get_backend_version() const override { return "1.0"; }
    
    std::vector<cross_platform_device_info> enumerate_devices() override;
    bool select_device(const std::string& device_id) override;
    cross_platform_device_info get_current_device_info() const override;
    bool is_device_available(const std::string& device_id) const override;
    
    std::vector<double> get_supported_sample_rates() const override;
    std::vector<int> get_supported_channels() const override;
    std::vector<int> get_supported_buffer_sizes() const override;
    bool is_format_supported(double sample_rate, int channels, int bits_per_sample) const override;
    
    bool open(double sample_rate, int channels, int format, int buffer_size) override;
    void close() override;
    bool is_open() const override;
    bool start() override;
    void stop() override;
    bool is_running() const override;
    bool is_paused() const override;
    void pause(bool pause) override;
    
    int write(const void* buffer, int frames) override;
    int read(void* buffer, int frames) override;
    int get_available_write_frames() const override;
    int get_available_read_frames() const override;
    double get_latency() const override;
    double get_cpu_load() const override;
    
    void set_volume(float volume) override;
    float get_volume() const override;
    void set_mute(bool mute) override;
    bool get_mute() const override;
    void set_buffer_size(int size) override;
    int get_buffer_size() const override;
    
    bool supports_hardware_mixing() const override { return true; }
    bool supports_device_switching() const override { return true; }
    
    std::string get_last_error() const override;
    void clear_error() override;
    bool is_recoverable_error() const override;
    bool recover_from_error() override;
    
private:
    // PulseAudio瀹炵幇
    struct PulseAudioImpl;
    std::unique_ptr<PulseAudioImpl> impl_;
};

// JACK闊抽杩炴帴宸ュ叿鍚庣
class jack_backend : public platform_audio_backend {
public:
    jack_backend();
    ~jack_backend() override;
    
    audio_backend get_backend_type() const override { return audio_backend::jack; }
    platform_type get_platform_type() const override { return platform_type::linux; }
    std::string get_backend_name() const override { return "JACK"; }
    std::string get_backend_version() const override { return "1.0"; }
    
    std::vector<cross_platform_device_info> enumerate_devices() override;
    bool select_device(const std::string& device_id) override;
    cross_platform_device_info get_current_device_info() const override;
    bool is_device_available(const std::string& device_id) const override;
    
    std::vector<double> get_supported_sample_rates() const override;
    std::vector<int> get_supported_channels() const override;
    std::vector<int> get_supported_buffer_sizes() const override;
    bool is_format_supported(double sample_rate, int channels, int bits_per_sample) const override;
    
    bool open(double sample_rate, int channels, int format, int buffer_size) override;
    void close() override;
    bool is_open() const override;
    bool start() override;
    void stop() override;
    bool is_running() const override;
    bool is_paused() const override;
    void pause(bool pause) override;
    
    int write(const void* buffer, int frames) override;
    int read(void* buffer, int frames) override;
    int get_available_write_frames() const override;
    int get_available_read_frames() const override;
    double get_latency() const override;
    double get_cpu_load() const override;
    
    void set_volume(float volume) override;
    float get_volume() const override;
    void set_mute(bool mute) override;
    bool get_mute() const override;
    void set_buffer_size(int size) override;
    int get_buffer_size() const override;
    
    bool supports_low_latency_mode() const override { return true; }
    bool supports_multi_device() const override { return true; }
    bool supports_device_switching() const override { return true; }
    
    std::string get_last_error() const override;
    void clear_error() override;
    bool is_recoverable_error() const override;
    bool recover_from_error() override;
    
private:
    // JACK瀹炵幇
    struct JACKImpl;
    std::unique_ptr<JACKImpl> impl_;
};

// 璺ㄥ钩鍙伴煶棰戣緭鍑鸿澶?
class output_cross_platform : public output_device {
public:
    output_cross_platform();
    ~output_cross_platform() override;
    
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
    
    // 璺ㄥ钩鍙扮壒瀹氭帴鍙?
    void set_backend(audio_backend backend);
    audio_backend get_current_backend() const;
    void set_platform(platform_type platform);
    platform_type get_current_platform() const;
    
    bool select_best_backend();
    bool auto_detect_platform();
    
    std::vector<cross_platform_device_info> enumerate_all_devices();
    bool switch_device(const std::string& device_id);
    bool switch_backend(audio_backend new_backend);
    
    void set_config(const cross_platform_config& config);
    cross_platform_config get_config() const;
    
    // 鍚庣绠＄悊
    bool register_backend(std::unique_ptr<platform_audio_backend> backend);
    bool unregister_backend(audio_backend backend);
    platform_audio_backend* get_backend(audio_backend backend) const;
    std::vector<audio_backend> get_available_backends() const;
    
    // 鎬ц兘鐩戞帶
    std::map<std::string, double> get_performance_metrics() const;
    void reset_performance_metrics();
    
    // 璁惧浜嬩欢
    void set_device_change_callback(std::function<void(const std::string& event, const std::string& device_id)> callback) {
        device_change_callback_ = callback;
    }
    
private:
    // 鍚庣绠＄悊
    std::map<audio_backend, std::unique_ptr<platform_audio_backend>> backends_;
    platform_audio_backend* current_backend_;
    audio_backend current_backend_type_;
    platform_type current_platform_;
    
    // 閰嶇疆
    cross_platform_config config_;
    
    // 鐘舵€?
    std::atomic<bool> is_initialized_;
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<float> volume_;
    std::atomic<float> cpu_load_;
    
    // 闊抽鏍煎紡
    t_uint32 sample_rate_;
    t_uint32 channels_;
    t_uint32 bits_per_sample_;
    t_uint32 buffer_size_;
    
    // 绾跨▼鍜岀紦鍐?
    std::thread audio_thread_;
    std::atomic<bool> audio_thread_running_;
    std::mutex audio_mutex_;
    std::condition_variable audio_cv_;
    
    // 鐜舰缂撳啿鍖?
    std::vector<float> audio_buffer_;
    std::atomic<size_t> write_position_;
    std::atomic<size_t> read_position_;
    std::atomic<size_t> available_frames_;
    
    // 鍥炶皟鍜屼簨浠?
    std::function<void(const std::string& event, const std::string& device_id)> device_change_callback_;
    
    // 鎬ц兘缁熻
    std::map<std::string, double> performance_metrics_;
    mutable std::mutex metrics_mutex_;
    std::chrono::steady_clock::time_point start_time_;
    
    // 绉佹湁鏂规硶
    bool initialize_backend();
    void shutdown_backend();
    void audio_thread_func();
    void process_audio_data();
    void update_performance_metrics();
    platform_audio_backend* create_backend_for_platform(platform_type platform);
    platform_audio_backend* create_backend_for_current_platform();
    bool detect_best_backend_for_platform(platform_type platform);
    void handle_backend_error(const std::string& operation);
    void notify_device_change(const std::string& event, const std::string& device_id);
};

// 鍚庣宸ュ巶
class audio_backend_factory {
public:
    static std::unique_ptr<platform_audio_backend> create_backend(audio_backend type);
    static std::unique_ptr<platform_audio_backend> create_backend_for_platform(platform_type platform);
    static std::vector<audio_backend> get_available_backends_for_platform(platform_type platform);
    static platform_type detect_current_platform();
    static bool is_backend_available_on_platform(audio_backend backend, platform_type platform);
    
private:
    static std::map<platform_type, std::vector<audio_backend>> platform_backends_;
    static void initialize_platform_backends();
};

// 璁惧绠＄悊鍣?
class cross_platform_device_manager {
public:
    static cross_platform_device_manager& get_instance();
    
    // 璁惧鏋氫妇
    std::vector<cross_platform_device_info> enumerate_all_devices();
    std::vector<cross_platform_device_info> enumerate_devices_by_backend(audio_backend backend);
    std::vector<cross_platform_device_info> enumerate_devices_by_platform(platform_type platform);
    
    // 璁惧閫夋嫨
    bool select_device(const std::string& device_id);
    bool select_best_device(audio_backend backend);
    bool select_default_device(audio_backend backend);
    
    // 璁惧淇℃伅
    cross_platform_device_info get_device_info(const std::string& device_id) const;
    cross_platform_device_info get_current_device() const;
    bool is_device_available(const std::string& device_id) const;
    
    // 璁惧鐩戞帶
    void start_device_monitoring();
    void stop_device_monitoring();
    bool is_monitoring() const { return monitoring_; }
    
    // 鏈€浣宠澶囨帹鑽?
    cross_platform_device_info recommend_best_device(
        audio_backend preferred_backend = audio_backend::unknown,
        double target_latency_ms = 10.0,
        bool prefer_exclusive_mode = false
    ) const;
    
    // 璁惧璇勫垎
    double score_device(const cross_platform_device_info& device, 
                       audio_backend preferred_backend = audio_backend::unknown,
                       double target_latency_ms = 10.0,
                       bool prefer_exclusive_mode = false) const;
    
private:
    cross_platform_device_manager();
    ~cross_platform_device_manager();
    cross_platform_device_manager(const cross_platform_device_manager&) = delete;
    cross_platform_device_manager& operator=(const cross_platform_device_manager&) = delete;
    
    std::vector<cross_platform_device_info> all_devices_;
    cross_platform_device_info current_device_;
    std::atomic<bool> monitoring_;
    std::thread monitor_thread_;
    
    void device_monitor_thread();
    void update_device_list();
    void detect_device_changes();
    
    // 璁惧缂撳瓨
    std::map<std::string, cross_platform_device_info> device_cache_;
    mutable std::mutex device_mutex_;
};

// 璺ㄥ钩鍙伴煶棰戣緭鍑哄伐鍘?
std::unique_ptr<output_cross_platform> create_cross_platform_output();
std::unique_ptr<output_cross_platform> create_cross_platform_output_for_platform(platform_type platform);

// 骞冲彴妫€娴嬪伐鍏?
namespace platform_utils {
    platform_type get_current_platform();
    std::string platform_type_to_string(platform_type platform);
    std::string audio_backend_to_string(audio_backend backend);
    bool is_platform_supported(platform_type platform);
    std::vector<platform_type> get_supported_platforms();
    
    // 骞冲彴鐗瑰畾鍔熻兘妫€娴?
    bool is_wasapi_available();
    bool is_asio_available();
    bool is_coreaudio_available();
    bool is_alsa_available();
    bool is_pulseaudio_available();
    bool is_jack_available();
}

} // namespace fb2k