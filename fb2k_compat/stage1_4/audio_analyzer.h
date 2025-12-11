#pragma once

#include "../stage1_3/audio_processor.h"
#include <vector>
#include <complex>
#include <memory>
#include <functional>
#include <map>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <cmath>

namespace fb2k {

// 音频分析常量
constexpr double ANALYZER_MIN_FREQUENCY = 20.0;
constexpr double ANALYZER_MAX_FREQUENCY = 20000.0;
constexpr int ANALYZER_DEFAULT_FFT_SIZE = 4096;
constexpr int ANALYZER_DEFAULT_SAMPLE_RATE = 44100;

// 频率带定义
enum class frequency_band {
    sub_bass = 0,      // 20-60 Hz
    bass,             // 60-250 Hz
    low_midrange,     // 250-500 Hz
    midrange,         // 500-2000 Hz
    upper_midrange,   // 2000-4000 Hz
    presence,         // 4000-6000 Hz
    brilliance,       // 6000-20000 Hz
    count
};

struct frequency_band_info {
    const char* name;
    double min_freq;
    double max_freq;
    const char* description;
};

static const frequency_band_info frequency_bands[static_cast<int>(frequency_band::count)] = {
    {"Sub Bass", 20.0, 60.0, "最低频率，感觉而非听到"},
    {"Bass", 60.0, 250.0, "低频基础，节奏感"},
    {"Low Midrange", 250.0, 500.0, "中低频，温暖感"},
    {"Midrange", 500.0, 2000.0, "中频，人声主要区域"},
    {"Upper Midrange", 2000.0, 4000.0, "高中频，清晰度和存在感"},
    {"Presence", 4000.0, 6000.0, "临场感，细节和清晰度"},
    {"Brilliance", 6000.0, 20000.0, "高频亮度，空气感和空间感"}
};

// 音频特征结构
struct audio_features {
    double rms_level;           // RMS电平 (dB)
    double peak_level;          // 峰值电平 (dB)
    double crest_factor;        // 波峰因子
    double dynamic_range;       // 动态范围 (dB)
    double loudness;            // 响度 (LUFS)
    double loudness_range;      // 响度范围 (LU)
    double true_peak;           // 真实峰值 (dBTP)
    double dc_offset;           // DC偏移
    double stereo_correlation;  // 立体声相关性
    double phase_correlation;   // 相位相关性
    
    // 频谱特征
    std::vector<double> spectral_centroid;    // 频谱重心
    std::vector<double> spectral_bandwidth;   // 频谱带宽
    std::vector<double> spectral_rolloff;     // 频谱衰减点
    std::vector<double> spectral_flux;        // 频谱通量
    std::vector<double> zero_crossing_rate;   // 过零率
    
    // 时域特征
    std::vector<double> zero_crossing_rate_time; // 时域过零率
    std::vector<double> energy_envelope;         // 能量包络
    std::vector<double> attack_time;             // 起音时间
    std::vector<double> release_time;            // 释放时间
    
    // 统计信息
    double mean_level;
    double variance_level;
    double skewness_level;
    double kurtosis_level;
};

// 频谱分析数据
struct spectrum_data {
    std::vector<double> frequencies;     // 频率值 (Hz)
    std::vector<double> magnitudes;      // 幅度值 (dB)
    std::vector<double> phases;          // 相位值 (radians)
    std::vector<double> power_density;   // 功率密度
    double sample_rate;
    int fft_size;
    int hop_size;
    int window_type;
};

// 实时分析数据
struct real_time_analysis {
    double current_rms;
    double current_peak;
    double current_loudness;
    double current_frequency;
    double current_tempo;
    double current_key;
    
    std::vector<double> spectrum_values;
    std::vector<double> band_levels;
    std::vector<double> harmonic_levels;
    std::vector<double> phase_values;
    
    double time_stamp;
    bool is_valid;
};

// 音频分析器基础接口
class audio_analyzer : public IFB2KService {
public:
    static const GUID iid;
    static const char* interface_name;
    
    virtual ~audio_analyzer() = default;
    
    // 基本分析功能
    virtual HRESULT analyze_chunk(const audio_chunk& chunk, audio_features& features) = 0;
    virtual HRESULT analyze_spectrum(const audio_chunk& chunk, spectrum_data& spectrum) = 0;
    virtual HRESULT get_real_time_analysis(real_time_analysis& analysis) = 0;
    
    // 配置管理
    virtual HRESULT set_fft_size(int size) = 0;
    virtual HRESULT get_fft_size(int& size) const = 0;
    virtual HRESULT set_window_type(int type) = 0;
    virtual HRESULT get_window_type(int& type) const = 0;
    virtual HRESULT set_overlap_factor(double factor) = 0;
    virtual HRESULT get_overlap_factor(double& factor) const = 0;
    
    // 分析模式
    virtual HRESULT set_analysis_mode(int mode) = 0; // 0=实时, 1=高精度, 2=快速
    virtual HRESULT get_analysis_mode(int& mode) const = 0;
    virtual HRESULT enable_feature(int feature, bool enable) = 0;
    virtual HRESULT is_feature_enabled(int feature, bool& enabled) const = 0;
    
    // 频率分析
    virtual HRESULT get_frequency_band_level(frequency_band band, double& level) const = 0;
    virtual HRESULT get_frequency_response(const std::vector<double>& frequencies, std::vector<double>& magnitudes) const = 0;
    virtual HRESULT detect_peaks(std::vector<std::pair<double, double>>& peaks, double threshold) const = 0;
    
    // 时域分析
    virtual HRESULT detect_onsets(std::vector<double>& onset_times, double threshold) const = 0;
    virtual HRESULT detect_beats(std::vector<double>& beat_times, double& tempo) const = 0;
    virtual HRESULT detect_key(int& key, double& confidence) const = 0;
    
    // 统计和报告
    virtual HRESULT get_analysis_statistics(std::map<std::string, double>& statistics) const = 0;
    virtual HRESULT reset_statistics() = 0;
    virtual HRESULT generate_report(std::string& report) const = 0;
};

// 快速傅里叶变换(FFT)处理器
class fft_processor {
public:
    fft_processor(int size = ANALYZER_DEFAULT_FFT_SIZE);
    ~fft_processor();
    
    // FFT处理
    bool process(const std::vector<float>& input, std::vector<std::complex<float>>& output);
    bool process_real(const std::vector<float>& input, std::vector<float>& magnitudes, std::vector<float>& phases);
    bool process_magnitude(const std::vector<float>& input, std::vector<float>& magnitudes);
    
    // 配置
    void set_size(int size);
    int get_size() const { return fft_size_; }
    void set_window_type(int type);
    int get_window_type() const { return window_type_; }
    
    // 窗口函数
    void apply_window(std::vector<float>& data);
    static std::vector<float> create_window(int type, int size);
    
    // 频率计算
    std::vector<double> get_frequency_bins(double sample_rate) const;
    double get_frequency_resolution(double sample_rate) const;
    
private:
    int fft_size_;
    int window_type_;
    std::vector<float> window_function_;
    
    // FFT工作缓冲区
    std::vector<std::complex<float>> fft_buffer_;
    std::vector<float> real_buffer_;
    std::vector<float> imag_buffer_;
    
    // FFT实现（使用Kiss FFT或类似库）
    void* fft_config_;
    void* fft_inverse_config_;
    
    void initialize_fft();
    void cleanup_fft();
    void apply_window_function(std::vector<float>& data);
};

// 频谱分析仪
class spectrum_analyzer : public audio_analyzer {
public:
    spectrum_analyzer();
    ~spectrum_analyzer() override;
    
    // audio_analyzer接口实现
    HRESULT analyze_chunk(const audio_chunk& chunk, audio_features& features) override;
    HRESULT analyze_spectrum(const audio_chunk& chunk, spectrum_data& spectrum) override;
    HRESULT get_real_time_analysis(real_time_analysis& analysis) override;
    
    HRESULT set_fft_size(int size) override;
    HRESULT get_fft_size(int& size) const override;
    HRESULT set_window_type(int type) override;
    HRESULT get_window_type(int& type) const override;
    HRESULT set_overlap_factor(double factor) override;
    HRESULT get_overlap_factor(double& factor) const override;
    
    HRESULT set_analysis_mode(int mode) override;
    HRESULT get_analysis_mode(int& mode) const override;
    HRESULT enable_feature(int feature, bool enable) override;
    HRESULT is_feature_enabled(int feature, bool& enabled) const override;
    
    HRESULT get_frequency_band_level(frequency_band band, double& level) const override;
    HRESULT get_frequency_response(const std::vector<double>& frequencies, std::vector<double>& magnitudes) const override;
    HRESULT detect_peaks(std::vector<std::pair<double, double>>& peaks, double threshold) const override;
    
    HRESULT detect_onsets(std::vector<double>& onset_times, double threshold) const override;
    HRESULT detect_beats(std::vector<double>& beat_times, double& tempo) const override;
    HRESULT detect_key(int& key, double& confidence) const override;
    
    HRESULT get_analysis_statistics(std::map<std::string, double>& statistics) const override;
    HRESULT reset_statistics() override;
    HRESULT generate_report(std::string& report) const override;
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
private:
    std::unique_ptr<fft_processor> fft_proc_;
    
    // 分析配置
    int analysis_mode_;
    int fft_size_;
    int window_type_;
    double overlap_factor_;
    
    // 特征标志
    std::atomic<bool> enable_rms_;
    std::atomic<bool> enable_peak_;
    std::atomic<bool> enable_spectrum_;
    std::atomic<bool> enable_loudness_;
    std::atomic<bool> enable_tempo_;
    std::atomic<bool> enable_key_;
    
    // 实时分析数据
    mutable real_time_analysis current_analysis_;
    mutable std::mutex analysis_mutex_;
    
    // 历史数据（用于统计）
    std::vector<audio_features> feature_history_;
    std::vector<spectrum_data> spectrum_history_;
    std::vector<real_time_analysis> analysis_history_;
    std::mutex history_mutex_;
    
    // 统计信息
    mutable std::map<std::string, double> statistics_;
    mutable std::mutex stats_mutex_;
    
    // 核心分析函数
    double calculate_rms_level(const audio_chunk& chunk) const;
    double calculate_peak_level(const audio_chunk& chunk) const;
    double calculate_loudness(const audio_chunk& chunk) const;
    double calculate_dynamic_range(const audio_chunk& chunk) const;
    double calculate_dc_offset(const audio_chunk& chunk) const;
    double calculate_stereo_correlation(const audio_chunk& chunk) const;
    
    void extract_spectral_features(const spectrum_data& spectrum, audio_features& features) const;
    void extract_temporal_features(const audio_chunk& chunk, audio_features& features) const;
    
    double calculate_spectral_centroid(const std::vector<double>& magnitudes, double sample_rate) const;
    double calculate_spectral_bandwidth(const std::vector<double>& magnitudes, double centroid, double sample_rate) const;
    double calculate_spectral_rolloff(const std::vector<double>& magnitudes, double sample_rate) const;
    double calculate_spectral_flux(const std::vector<double>& prev_magnitudes, const std::vector<double>& curr_magnitudes) const;
    double calculate_zero_crossing_rate(const std::vector<float>& samples) const;
    
    void update_real_time_analysis(const audio_features& features, const spectrum_data& spectrum);
    void update_statistics(const audio_features& features);
    
    // 频率分析
    double get_band_energy(const spectrum_data& spectrum, double min_freq, double max_freq) const;
    std::vector<double> detect_spectral_peaks(const spectrum_data& spectrum, double threshold) const;
    
    // 节奏分析
    double estimate_tempo_from_onsets(const std::vector<double>& onset_times) const;
    std::vector<double> calculate_onset_detection_function(const audio_chunk& chunk) const;
    double calculate_tempo_confidence(const std::vector<double>& beat_intervals) const;
    
    // 音调分析
    int estimate_key_from_chroma(const std::vector<double>& chroma_vector) const;
    std::vector<double> calculate_chroma_vector(const spectrum_data& spectrum) const;
    double calculate_key_confidence(const std::vector<double>& chroma_vector, int estimated_key) const;
};

// 音频电平表
class level_meter {
public:
    level_meter();
    ~level_meter();
    
    // 电平检测
    void process_samples(const float* samples, int num_samples, int channels);
    double get_peak_level() const { return peak_level_; }
    double get_rms_level() const { return rms_level_; }
    double get_loudness() const { return loudness_; }
    double get_true_peak() const { return true_peak_; }
    
    // 配置
    void set_attack_time(double time_ms) { attack_time_ = time_ms; }
    void set_release_time(double time_ms) { release_time_ = time_ms; }
    void set_integration_time(double time_ms) { integration_time_ = time_ms; }
    
    // 重置
    void reset();
    
private:
    // 电平计算
    double peak_level_;
    double rms_level_;
    double loudness_;
    double true_peak_;
    
    // 时间常数
    double attack_time_;
    double release_time_;
    double integration_time_;
    
    // 内部状态
    double peak_hold_;
    double rms_sum_;
    int rms_count_;
    std::vector<double> loudness_history_;
    std::vector<double> oversampled_buffer_;
    
    // 过采样滤波器（用于真实峰值检测）
    std::unique_ptr<class oversampling_filter> oversampling_filter_;
    
    // 计算函数
    double calculate_peak(const float* samples, int num_samples) const;
    double calculate_rms(const float* samples, int num_samples) const;
    double calculate_loudness(const float* samples, int num_samples, int channels) const;
    double calculate_true_peak(const float* samples, int num_samples, int channels) const;
    
    // 滤波函数
    void apply_oversampling(const float* input, std::vector<double>& output, int num_samples);
    void apply_loudness_filter(const float* input, std::vector<double>& output, int num_samples, int channels);
};

// 频谱可视化器
class spectrum_visualizer {
public:
    spectrum_visualizer();
    ~spectrum_visualizer();
    
    // 可视化更新
    void update_spectrum(const spectrum_data& spectrum);
    void set_frequency_range(double min_freq, double max_freq);
    void set_magnitude_range(double min_mag, double max_mag);
    void set_display_mode(int mode); // 0=线性, 1=对数, 2=梅尔刻度
    
    // 获取可视化数据
    std::vector<float> get_spectrum_line(int width, int height) const;
    std::vector<std::vector<float>> get_spectrum_bars(int num_bars) const;
    std::vector<float> get_waterfall_line(int width) const;
    
    // 配置
    void set_smoothing_factor(float factor) { smoothing_factor_ = factor; }
    void set_peak_hold_time(float time_ms) { peak_hold_time_ = time_ms; }
    void set_falloff_speed(float speed) { falloff_speed_ = speed; }
    
private:
    // 显示配置
    double min_frequency_;
    double max_frequency_;
    double min_magnitude_;
    double max_magnitude_;
    int display_mode_;
    
    // 平滑和峰值保持
    float smoothing_factor_;
    float peak_hold_time_;
    float falloff_speed_;
    
    // 当前和历史数据
    std::vector<float> current_spectrum_;
    std::vector<float> smoothed_spectrum_;
    std::vector<float> peak_spectrum_;
    std::vector<std::vector<float>> waterfall_data_;
    
    // 时间状态
    std::vector<double> peak_times_;
    double last_update_time_;
    
    // 转换函数
    float frequency_to_x(double frequency, int width) const;
    float magnitude_to_y(double magnitude, int height) const;
    double x_to_frequency(float x, int width) const;
    double y_to_magnitude(float y, int height) const;
    
    // 平滑函数
    void apply_smoothing(std::vector<float>& spectrum);
    void update_peaks(const std::vector<float>& spectrum, double current_time);
    void apply_falloff(std::vector<float>& spectrum, double delta_time);
    
    // 刻度转换
    std::vector<double> apply_frequency_scaling(const std::vector<double>& frequencies) const;
    double mel_scale(double frequency) const;
    double inverse_mel_scale(double mel) const;
};

// 音调检测器
class pitch_detector {
public:
    pitch_detector();
    ~pitch_detector();
    
    // 音调检测
    bool detect_pitch(const audio_chunk& chunk, double& frequency, double& confidence);
    bool detect_multiple_pitches(const audio_chunk& chunk, std::vector<std::pair<double, double>>& pitches);
    
    // 算法选择
    enum detection_algorithm {
        autocorrelation,
        cepstral,
        harmonic_product_spectrum,
        yin,
        mcleod
    };
    
    void set_algorithm(detection_algorithm algo) { algorithm_ = algo; }
    detection_algorithm get_algorithm() const { return algorithm_; }
    
    // 配置
    void set_min_frequency(double freq) { min_frequency_ = freq; }
    void set_max_frequency(double freq) { max_frequency_ = freq; }
    void set_confidence_threshold(double threshold) { confidence_threshold_ = threshold; }
    
private:
    detection_algorithm algorithm_;
    double min_frequency_;
    double max_frequency_;
    double confidence_threshold_;
    
    // 算法实现
    double detect_pitch_autocorrelation(const std::vector<float>& samples, double sample_rate);
    double detect_pitch_cepstral(const std::vector<float>& samples, double sample_rate);
    double detect_pitch_hps(const std::vector<float>& samples, double sample_rate);
    double detect_pitch_yin(const std::vector<float>& samples, double sample_rate);
    double detect_pitch_mcleod(const std::vector<float>& samples, double sample_rate);
    
    // 辅助函数
    double calculate_confidence(const std::vector<float>& correlation, double period);
    std::vector<double> calculate_harmonic_product_spectrum(const std::vector<std::complex<float>>& spectrum);
    double parabolic_interpolation(const std::vector<double>& values, int peak_index);
};

// 音频分析管理器
class audio_analysis_manager {
public:
    audio_analysis_manager();
    ~audio_analysis_manager();
    
    // 分析器管理
    bool register_analyzer(std::unique_ptr<audio_analyzer> analyzer);
    void unregister_analyzer(const std::string& name);
    audio_analyzer* get_analyzer(const std::string& name);
    std::vector<std::string> get_analyzer_names() const;
    
    // 实时分析
    bool start_real_time_analysis();
    void stop_real_time_analysis();
    bool is_analyzing() const { return analyzing_; }
    
    // 数据分析
    bool analyze_audio(const audio_chunk& chunk, const std::string& analyzer_name);
    bool analyze_audio_all(const audio_chunk& chunk);
    
    // 结果获取
    bool get_analysis_results(const std::string& analyzer_name, audio_features& features);
    bool get_spectrum_results(const std::string& analyzer_name, spectrum_data& spectrum);
    bool get_real_time_results(const std::string& analyzer_name, real_time_analysis& analysis);
    
    // 配置管理
    void set_analysis_enabled(bool enabled) { analysis_enabled_ = enabled; }
    bool is_analysis_enabled() const { return analysis_enabled_; }
    void set_analysis_interval(double interval_ms) { analysis_interval_ = interval_ms; }
    double get_analysis_interval() const { return analysis_interval_; }
    
    // 统计和报告
    void get_all_statistics(std::map<std::string, std::map<std::string, double>>& all_stats);
    void reset_all_statistics();
    std::string generate_comprehensive_report() const;
    
private:
    std::map<std::string, std::unique_ptr<audio_analyzer>> analyzers_;
    std::vector<std::string> analyzer_names_;
    std::mutable std::mutex manager_mutex_;
    
    std::atomic<bool> analyzing_;
    std::atomic<bool> analysis_enabled_;
    double analysis_interval_;
    
    std::thread analysis_thread_;
    std::queue<audio_chunk> analysis_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    void analysis_worker();
    void process_analysis_queue();
};

// 全局音频分析管理器访问
audio_analysis_manager& get_audio_analysis_manager();

} // namespace fb2k