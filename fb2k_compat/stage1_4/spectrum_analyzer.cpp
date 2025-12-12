#include "audio_analyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>

namespace fb2k {

// FFT澶勭悊鍣ㄥ疄鐜?
class fft_processor_impl {
public:
    fft_processor_impl(int size);
    ~fft_processor_impl();
    
    bool process(const std::vector<float>& input, std::vector<std::complex<float>>& output);
    bool process_real(const std::vector<float>& input, std::vector<float>& magnitudes, std::vector<float>& phases);
    
    void set_window_type(int type);
    std::vector<double> get_frequency_bins(double sample_rate) const;
    
private:
    int size_;
    int window_type_;
    std::vector<float> window_;
    
    // 绠€鍗曠殑FFT瀹炵幇锛堜娇鐢ㄥ鏁拌繍绠楋級
    void perform_fft(const std::vector<std::complex<float>>& input, std::vector<std::complex<float>>& output);
    void apply_window_to_signal(std::vector<float>& signal);
    std::vector<float> create_window_function(int type, int size);
};

fft_processor_impl::fft_processor_impl(int size) 
    : size_(size), window_type_(0) {
    window_ = create_window_function(window_type_, size_);
}

fft_processor_impl::~fft_processor_impl() = default;

bool fft_processor_impl::process(const std::vector<float>& input, std::vector<std::complex<float>>& output) {
    if (input.size() != static_cast<size_t>(size_)) {
        return false;
    }
    
    output.resize(size_);
    
    // 搴旂敤绐楀彛鍑芥暟
    std::vector<float> windowed_input = input;
    apply_window_to_signal(windowed_input);
    
    // 杞崲涓哄鏁?
    std::vector<std::complex<float>> complex_input(size_);
    for (int i = 0; i < size_; ++i) {
        complex_input[i] = std::complex<float>(windowed_input[i], 0.0f);
    }
    
    // 鎵цFFT
    perform_fft(complex_input, output);
    
    return true;
}

bool fft_processor_impl::process_real(const std::vector<float>& input, std::vector<float>& magnitudes, std::vector<float>& phases) {
    std::vector<std::complex<float>> fft_output;
    if (!process(input, fft_output)) {
        return false;
    }
    
    magnitudes.resize(size_ / 2 + 1);
    phases.resize(size_ / 2 + 1);
    
    // 璁＄畻骞呭害鍜岀浉浣嶏紙鍙彇姝ｉ鐜囬儴鍒嗭級
    for (int i = 0; i <= size_ / 2; ++i) {
        magnitudes[i] = std::abs(fft_output[i]);
        phases[i] = std::arg(fft_output[i]);
    }
    
    return true;
}

void fft_processor_impl::set_window_type(int type) {
    window_type_ = type;
    window_ = create_window_function(window_type_, size_);
}

std::vector<double> fft_processor_impl::get_frequency_bins(double sample_rate) const {
    std::vector<double> frequencies(size_ / 2 + 1);
    double freq_resolution = sample_rate / size_;
    
    for (int i = 0; i <= size_ / 2; ++i) {
        frequencies[i] = i * freq_resolution;
    }
    
    return frequencies;
}

void fft_processor_impl::perform_fft(const std::vector<std::complex<float>>& input, std::vector<std::complex<float>>& output) {
    // 绠€鍖栫殑FFT瀹炵幇锛圕ooley-Tukey绠楁硶锛?
    int n = input.size();
    output = input;
    
    // 浣嶅弽杞噸鎺掑垪
    int j = 0;
    for (int i = 1; i < n; ++i) {
        int bit = n >> 1;
        while (j >= bit) {
            j -= bit;
            bit >>= 1;
        }
        j += bit;
        if (i < j) {
            std::swap(output[i], output[j]);
        }
    }
    
    // FFT璁＄畻
    for (int len = 2; len <= n; len <<= 1) {
        double angle = -2.0 * M_PI / len;
        std::complex<float> wlen(std::cos(angle), std::sin(angle));
        
        for (int i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int j = 0; j < len / 2; ++j) {
                std::complex<float> u = output[i + j];
                std::complex<float> v = output[i + j + len / 2] * w;
                output[i + j] = u + v;
                output[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

void fft_processor_impl::apply_window_to_signal(std::vector<float>& signal) {
    if (window_.size() != signal.size()) {
        return;
    }
    
    for (size_t i = 0; i < signal.size(); ++i) {
        signal[i] *= window_[i];
    }
}

std::vector<float> fft_processor_impl::create_window_function(int type, int size) {
    std::vector<float> window(size);
    
    switch (type) {
        case 0: // 鐭╁舰绐?
            std::fill(window.begin(), window.end(), 1.0f);
            break;
            
        case 1: // Hann绐?
            for (int i = 0; i < size; ++i) {
                window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
            }
            break;
            
        case 2: // Hamming绐?
            for (int i = 0; i < size; ++i) {
                window[i] = 0.54f - 0.46f * std::cos(2.0f * M_PI * i / (size - 1));
            }
            break;
            
        case 3: // Blackman绐?
            for (int i = 0; i < size; ++i) {
                window[i] = 0.42f - 0.5f * std::cos(2.0f * M_PI * i / (size - 1)) + 
                           0.08f * std::cos(4.0f * M_PI * i / (size - 1));
            }
            break;
            
        default:
            std::fill(window.begin(), window.end(), 1.0f);
            break;
    }
    
    return window;
}

// 棰戣氨鍒嗘瀽浠疄鐜?
spectrum_analyzer::spectrum_analyzer()
    : analysis_mode_(0)
    , fft_size_(ANALYZER_DEFAULT_FFT_SIZE)
    , window_type_(1) // Hann绐?
    , overlap_factor_(0.5)
    , enable_rms_(true)
    , enable_peak_(true)
    , enable_spectrum_(true)
    , enable_loudness_(true)
    , enable_tempo_(false)
    , enable_key_(false) {
    
    fft_proc_ = std::make_unique<fft_processor_impl>(fft_size_);
    
    // 鍒濆鍖栧垎鏋愭暟鎹?
    current_analysis_.is_valid = false;
    current_analysis_.time_stamp = 0.0;
    
    // 鍒濆鍖栫粺璁′俊鎭?
    reset_statistics();
}

spectrum_analyzer::~spectrum_analyzer() = default;

HRESULT spectrum_analyzer::do_initialize() {
    std::cout << "[Spectrum Analyzer] 鍒濆鍖栭璋卞垎鏋愪华" << std::endl;
    return S_OK;
}

HRESULT spectrum_analyzer::do_shutdown() {
    std::cout << "[Spectrum Analyzer] 鍏抽棴棰戣氨鍒嗘瀽浠? << std::endl;
    return S_OK;
}

HRESULT spectrum_analyzer::analyze_chunk(const audio_chunk& chunk, audio_features& features) {
    if (!enable_rms_ && !enable_peak_ && !enable_loudness_) {
        return S_FALSE;
    }
    
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    
    // 鎻愬彇鍩烘湰鐗瑰緛
    if (enable_rms_) {
        features.rms_level = calculate_rms_level(chunk);
    }
    
    if (enable_peak_) {
        features.peak_level = calculate_peak_level(chunk);
    }
    
    if (enable_loudness_) {
        features.loudness = calculate_loudness(chunk);
    }
    
    // 璁＄畻娲剧敓鐗瑰緛
    if (features.rms_level > 0 && features.peak_level > 0) {
        features.crest_factor = features.peak_level / features.rms_level;
        features.dynamic_range = 20.0 * std::log10(features.peak_level / features.rms_level);
    }
    
    features.dc_offset = calculate_dc_offset(chunk);
    features.stereo_correlation = calculate_stereo_correlation(chunk);
    
    // 鏃跺煙鐗瑰緛
    extract_temporal_features(chunk, features);
    
    // 鏇存柊瀹炴椂鍒嗘瀽
    update_real_time_analysis(features, spectrum_data());
    
    // 鏇存柊缁熻淇℃伅
    update_statistics(features);
    
    // 淇濆瓨鍘嗗彶鏁版嵁
    {
        std::lock_guard<std::mutex> history_lock(history_mutex_);
        feature_history_.push_back(features);
        if (feature_history_.size() > 1000) { // 闄愬埗鍘嗗彶鏁版嵁澶у皬
            feature_history_.erase(feature_history_.begin());
        }
    }
    
    return S_OK;
}

HRESULT spectrum_analyzer::analyze_spectrum(const audio_chunk& chunk, spectrum_data& spectrum) {
    if (!enable_spectrum_) {
        return S_FALSE;
    }
    
    int num_samples = chunk.get_sample_count();
    int channels = chunk.get_channels();
    const float* data = chunk.get_data();
    double sample_rate = chunk.get_sample_rate();
    
    // 鍑嗗FFT杈撳叆鏁版嵁
    std::vector<float> fft_input(fft_size_, 0.0f);
    
    // 娣峰悎澹伴亾鎴栧彇绗竴涓０閬?
    if (channels == 1) {
        int copy_samples = std::min(num_samples, fft_size_);
        std::memcpy(fft_input.data(), data, copy_samples * sizeof(float));
    } else {
        // 娣峰悎澹伴亾
        int copy_samples = std::min(num_samples, fft_size_);
        for (int i = 0; i < copy_samples; ++i) {
            float mixed_sample = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                mixed_sample += data[i * channels + ch];
            }
            fft_input[i] = mixed_sample / channels;
        }
    }
    
    // 鎵цFFT
    std::vector<float> magnitudes;
    std::vector<float> phases;
    
    if (!fft_proc_->process_real(fft_input, magnitudes, phases)) {
        return E_FAIL;
    }
    
    // 濉厖棰戣氨鏁版嵁
    spectrum.sample_rate = sample_rate;
    spectrum.fft_size = fft_size_;
    spectrum.hop_size = fft_size_ / 2; // 50%閲嶅彔
    spectrum.window_type = window_type_;
    
    spectrum.frequencies = fft_proc_->get_frequency_bins(sample_rate);
    spectrum.magnitudes.resize(magnitudes.size());
    spectrum.phases.resize(phases.size());
    spectrum.power_density.resize(magnitudes.size());
    
    // 杞崲涓篸B骞惰绠楀姛鐜囧瘑搴?
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        // 骞呭害杞琩B
        double magnitude_db = 20.0 * std::log10(std::max(magnitudes[i], 1e-10f));
        spectrum.magnitudes[i] = magnitude_db;
        
        spectrum.phases[i] = phases[i];
        
        // 鍔熺巼瀵嗗害 (鍔熺巼/Hz)
        double power = magnitudes[i] * magnitudes[i];
        double bandwidth = (i == 0) ? spectrum.frequencies[1] : 
                          (spectrum.frequencies[i] - spectrum.frequencies[i-1]);
        spectrum.power_density[i] = power / bandwidth;
    }
    
    // 鎻愬彇棰戣氨鐗瑰緛
    audio_features features;
    extract_spectral_features(spectrum, features);
    
    // 淇濆瓨鍘嗗彶鏁版嵁
    {
        std::lock_guard<std::mutex> history_lock(history_mutex_);
        spectrum_history_.push_back(spectrum);
        if (spectrum_history_.size() > 1000) {
            spectrum_history_.erase(spectrum_history_.begin());
        }
    }
    
    return S_OK;
}

HRESULT spectrum_analyzer::get_real_time_analysis(real_time_analysis& analysis) {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    analysis = current_analysis_;
    return S_OK;
}

// 閰嶇疆绠＄悊
HRESULT spectrum_analyzer::set_fft_size(int size) {
    if (size < 128 || size > 65536) return E_INVALIDARG;
    
    fft_size_ = size;
    fft_proc_->set_size(size);
    return S_OK;
}

HRESULT spectrum_analyzer::get_fft_size(int& size) const {
    size = fft_size_;
    return S_OK;
}

HRESULT spectrum_analyzer::set_window_type(int type) {
    window_type_ = type;
    fft_proc_->set_window_type(type);
    return S_OK;
}

HRESULT spectrum_analyzer::get_window_type(int& type) const {
    type = window_type_;
    return S_OK;
}

HRESULT spectrum_analyzer::set_overlap_factor(double factor) {
    if (factor < 0.0 || factor > 0.95) return E_INVALIDARG;
    overlap_factor_ = factor;
    return S_OK;
}

HRESULT spectrum_analyzer::get_overlap_factor(double& factor) const {
    factor = overlap_factor_;
    return S_OK;
}

HRESULT spectrum_analyzer::set_analysis_mode(int mode) {
    if (mode < 0 || mode > 2) return E_INVALIDARG;
    analysis_mode_ = mode;
    return S_OK;
}

HRESULT spectrum_analyzer::get_analysis_mode(int& mode) const {
    mode = analysis_mode_;
    return S_OK;
}

HRESULT spectrum_analyzer::enable_feature(int feature, bool enable) {
    switch (feature) {
        case 0: enable_rms_ = enable; break;
        case 1: enable_peak_ = enable; break;
        case 2: enable_spectrum_ = enable; break;
        case 3: enable_loudness_ = enable; break;
        case 4: enable_tempo_ = enable; break;
        case 5: enable_key_ = enable; break;
        default: return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT spectrum_analyzer::is_feature_enabled(int feature, bool& enabled) const {
    switch (feature) {
        case 0: enabled = enable_rms_; break;
        case 1: enabled = enable_peak_; break;
        case 2: enabled = enable_spectrum_; break;
        case 3: enabled = enable_loudness_; break;
        case 4: enabled = enable_tempo_; break;
        case 5: enabled = enable_key_; break;
        default: return E_INVALIDARG;
    }
    return S_OK;
}

// 棰戠巼鍒嗘瀽
HRESULT spectrum_analyzer::get_frequency_band_level(frequency_band band, double& level) const {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    
    if (!current_analysis_.is_valid) return E_FAIL;
    
    int band_index = static_cast<int>(band);
    if (band_index < 0 || band_index >= static_cast<int>(frequency_band::count)) {
        return E_INVALIDARG;
    }
    
    if (band_index < current_analysis_.band_levels.size()) {
        level = current_analysis_.band_levels[band_index];
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT spectrum_analyzer::get_frequency_response(const std::vector<double>& frequencies, std::vector<double>& magnitudes) const {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    
    if (!current_analysis_.is_valid || current_analysis_.spectrum_values.empty()) {
        return E_FAIL;
    }
    
    magnitudes.resize(frequencies.size());
    
    // 鎻掑€艰绠楅鐜囧搷搴?
    for (size_t i = 0; i < frequencies.size(); ++i) {
        // 杩欓噷搴旇浣跨敤鎻掑€肩畻娉曚粠棰戣氨鏁版嵁璁＄畻鎸囧畾棰戠巼鐨勫搷搴?
        // 绠€鍖栧疄鐜帮細浣跨敤鏈€杩戠殑棰戠巼鐐?
        double target_freq = frequencies[i];
        double closest_mag = -80.0; // 榛樿-80dB
        
        // 鍦ㄥ疄闄呭疄鐜颁腑锛岃繖閲屽簲璇ヤ娇鐢ㄧ嚎鎬ф彃鍊兼垨鏇撮珮闃舵彃鍊?
        magnitudes[i] = closest_mag;
    }
    
    return S_OK;
}

HRESULT spectrum_analyzer::detect_peaks(std::vector<std::pair<double, double>>& peaks, double threshold) const {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    
    if (!current_analysis_.is_valid || current_analysis_.spectrum_values.empty()) {
        return E_FAIL;
    }
    
    peaks.clear();
    
    // 绠€鍖栫殑宄板€兼娴嬬畻娉?
    const auto& spectrum = current_analysis_.spectrum_values;
    if (spectrum.size() < 3) return S_OK;
    
    for (size_t i = 1; i < spectrum.size() - 1; ++i) {
        if (spectrum[i] > threshold && 
            spectrum[i] > spectrum[i-1] && 
            spectrum[i] > spectrum[i+1]) {
            // 璁＄畻绮剧‘鐨勫嘲鍊奸鐜囷紙鎶涚墿绾挎彃鍊硷級
            double freq_bin = static_cast<double>(i);
            double magnitude = spectrum[i];
            peaks.push_back({freq_bin, magnitude});
        }
    }
    
    return S_OK;
}

// 鑺傚鍜岄煶璋冨垎鏋愶紙绠€鍖栧疄鐜帮級
HRESULT spectrum_analyzer::detect_onsets(std::vector<double>& onset_times, double threshold) const {
    // 杩欓噷搴旇瀹炵幇onset妫€娴嬬畻娉?
    // 绠€鍖栧疄鐜帮細杩斿洖绌虹粨鏋?
    onset_times.clear();
    return S_OK;
}

HRESULT spectrum_analyzer::detect_beats(std::vector<double>& beat_times, double& tempo) const {
    // 杩欓噷搴旇瀹炵幇beat妫€娴嬬畻娉?
    // 绠€鍖栧疄鐜帮細杩斿洖榛樿缁撴灉
    beat_times.clear();
    tempo = 120.0; // 榛樿120 BPM
    return S_OK;
}

HRESULT spectrum_analyzer::detect_key(int& key, double& confidence) const {
    // 杩欓噷搴旇瀹炵幇key妫€娴嬬畻娉?
    // 绠€鍖栧疄鐜帮細杩斿洖C澶ц皟
    key = 0; // C major
    confidence = 0.5;
    return S_OK;
}

// 缁熻鍜屾姤鍛?
HRESULT spectrum_analyzer::get_analysis_statistics(std::map<std::string, double>& statistics) const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    statistics = statistics_;
    return S_OK;
}

HRESULT spectrum_analyzer::reset_statistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    statistics_.clear();
    return S_OK;
}

HRESULT spectrum_analyzer::generate_report(std::string& report) const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::stringstream ss;
    ss << "棰戣氨鍒嗘瀽鎶ュ憡\n";
    ss << "================\n\n";
    
    ss << "閰嶇疆淇℃伅:\n";
    ss << "  FFT澶у皬: " << fft_size_ << "\n";
    ss << "  绐楀彛绫诲瀷: " << window_type_ << "\n";
    ss << "  閲嶅彔鍥犲瓙: " << overlap_factor_ << "\n";
    ss << "  鍒嗘瀽妯″紡: " << analysis_mode_ << "\n\n";
    
    ss << "缁熻淇℃伅:\n";
    for (const auto& [key, value] : statistics_) {
        ss << "  " << key << ": " << std::fixed << std::setprecision(3) << value << "\n";
    }
    
    report = ss.str();
    return S_OK;
}

// 鏍稿績鍒嗘瀽鍑芥暟瀹炵幇
double spectrum_analyzer::calculate_rms_level(const audio_chunk& chunk) const {
    const float* data = chunk.get_data();
    int total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    double sum_squares = 0.0;
    for (int i = 0; i < total_samples; ++i) {
        sum_squares += data[i] * data[i];
    }
    
    double rms = std::sqrt(sum_squares / total_samples);
    return 20.0 * std::log10(std::max(rms, 1e-10f));
}

double spectrum_analyzer::calculate_peak_level(const audio_chunk& chunk) const {
    const float* data = chunk.get_data();
    int total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    float peak = 0.0f;
    for (int i = 0; i < total_samples; ++i) {
        peak = std::max(peak, std::abs(data[i]));
    }
    
    return 20.0 * std::log10(std::max(peak, 1e-10f));
}

double spectrum_analyzer::calculate_loudness(const audio_chunk& chunk) const {
    // 绠€鍖栫殑鍝嶅害璁＄畻锛堝簲璇ヤ娇鐢↖TU-R BS.1770鏍囧噯锛?
    return calculate_rms_level(chunk) + 3.0; // 绮楃暐浼拌
}

double spectrum_analyzer::calculate_dc_offset(const audio_chunk& chunk) const {
    const float* data = chunk.get_data();
    int total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    double sum = 0.0;
    for (int i = 0; i < total_samples; ++i) {
        sum += data[i];
    }
    
    return sum / total_samples;
}

double spectrum_analyzer::calculate_stereo_correlation(const audio_chunk& chunk) const {
    if (chunk.get_channels() != 2) return 0.0;
    
    const float* data = chunk.get_data();
    int num_samples = chunk.get_sample_count();
    
    double sum_l = 0.0, sum_r = 0.0, sum_l2 = 0.0, sum_r2 = 0.0, sum_lr = 0.0;
    
    for (int i = 0; i < num_samples; ++i) {
        float left = data[i * 2];
        float right = data[i * 2 + 1];
        
        sum_l += left;
        sum_r += right;
        sum_l2 += left * left;
        sum_r2 += right * right;
        sum_lr += left * right;
    }
    
    double corr = (num_samples * sum_lr - sum_l * sum_r) / 
                  std::sqrt((num_samples * sum_l2 - sum_l * sum_l) * 
                           (num_samples * sum_r2 - sum_r * sum_r));
    
    return corr;
}

void spectrum_analyzer::extract_spectral_features(const spectrum_data& spectrum, audio_features& features) const {
    if (spectrum.magnitudes.empty()) return;
    
    features.spectral_centroid.resize(1);
    features.spectral_bandwidth.resize(1);
    features.spectral_rolloff.resize(1);
    features.spectral_flux.resize(1);
    
    features.spectral_centroid[0] = calculate_spectral_centroid(spectrum.magnitudes, spectrum.sample_rate);
    features.spectral_bandwidth[0] = calculate_spectral_bandwidth(spectrum.magnitudes, features.spectral_centroid[0], spectrum.sample_rate);
    features.spectral_rolloff[0] = calculate_spectral_rolloff(spectrum.magnitudes, spectrum.sample_rate);
    
    // 璁＄畻棰戣氨閫氶噺锛堥渶瑕佸墠涓€涓璋憋級
    // 绠€鍖栧疄鐜帮細浣跨敤褰撳墠棰戣氨
    features.spectral_flux[0] = 0.0;
}

void spectrum_analyzer::extract_temporal_features(const audio_chunk& chunk, audio_features& features) const {
    const float* data = chunk.get_data();
    int num_samples = chunk.get_sample_count() * chunk.get_channels();
    
    features.zero_crossing_rate_time.resize(1);
    features.zero_crossing_rate_time[0] = calculate_zero_crossing_rate(std::vector<float>(data, data + num_samples));
    
    // 鍏朵粬鏃跺煙鐗瑰緛...
    features.energy_envelope.clear();
    features.attack_time.clear();
    features.release_time.clear();
}

double spectrum_analyzer::calculate_spectral_centroid(const std::vector<double>& magnitudes, double sample_rate) const {
    double weighted_sum = 0.0;
    double magnitude_sum = 0.0;
    double freq_resolution = sample_rate / fft_size_;
    
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        double frequency = i * freq_resolution;
        weighted_sum += frequency * magnitudes[i];
        magnitude_sum += magnitudes[i];
    }
    
    return magnitude_sum > 0 ? weighted_sum / magnitude_sum : 0.0;
}

double spectrum_analyzer::calculate_spectral_bandwidth(const std::vector<double>& magnitudes, double centroid, double sample_rate) const {
    double weighted_sum = 0.0;
    double magnitude_sum = 0.0;
    double freq_resolution = sample_rate / fft_size_;
    
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        double frequency = i * freq_resolution;
        double deviation = std::abs(frequency - centroid);
        weighted_sum += deviation * magnitudes[i];
        magnitude_sum += magnitudes[i];
    }
    
    return magnitude_sum > 0 ? weighted_sum / magnitude_sum : 0.0;
}

double spectrum_analyzer::calculate_spectral_rolloff(const std::vector<double>& magnitudes, double sample_rate) const {
    double total_energy = 0.0;
    for (double mag : magnitudes) {
        total_energy += mag * mag;
    }
    
    double cumulative_energy = 0.0;
    double threshold_energy = total_energy * 0.85; // 85%鑳介噺闃堝€?
    double freq_resolution = sample_rate / fft_size_;
    
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        cumulative_energy += magnitudes[i] * magnitudes[i];
        if (cumulative_energy >= threshold_energy) {
            return i * freq_resolution;
        }
    }
    
    return sample_rate / 2.0;
}

double spectrum_analyzer::calculate_zero_crossing_rate(const std::vector<float>& samples) const {
    int zero_crossings = 0;
    
    for (size_t i = 1; i < samples.size(); ++i) {
        if ((samples[i] >= 0) != (samples[i-1] >= 0)) {
            zero_crossings++;
        }
    }
    
    return static_cast<double>(zero_crossings) / samples.size();
}

void spectrum_analyzer::update_real_time_analysis(const audio_features& features, const spectrum_data& spectrum) {
    std::lock_guard<std::mutex> lock(analysis_mutex_);
    
    current_analysis_.current_rms = features.rms_level;
    current_analysis_.current_peak = features.peak_level;
    current_analysis_.current_loudness = features.loudness;
    current_analysis_.time_stamp = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    current_analysis_.is_valid = true;
    
    // 鏇存柊棰戣氨鏁版嵁
    if (!spectrum.magnitudes.empty()) {
        current_analysis_.spectrum_values = spectrum.magnitudes;
    }
    
    // 鏇存柊棰戞鐢靛钩
    current_analysis_.band_levels.resize(static_cast<int>(frequency_band::count));
    for (int i = 0; i < static_cast<int>(frequency_band::count); ++i) {
        frequency_band band = static_cast<frequency_band>(i);
        double level = 0.0;
        get_frequency_band_level(band, level);
        current_analysis_.band_levels[i] = level;
    }
    
    // 淇濆瓨鍘嗗彶鏁版嵁
    {
        std::lock_guard<std::mutex> history_lock(history_mutex_);
        analysis_history_.push_back(current_analysis_);
        if (analysis_history_.size() > 1000) {
            analysis_history_.erase(analysis_history_.begin());
        }
    }
}

void spectrum_analyzer::update_statistics(const audio_features& features) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    statistics_["rms_mean"] = features.rms_level;
    statistics_["peak_mean"] = features.peak_level;
    statistics_["loudness_mean"] = features.loudness;
    statistics_["dynamic_range_mean"] = features.dynamic_range;
    statistics_["spectral_centroid_mean"] = features.spectral_centroid.empty() ? 0.0 : features.spectral_centroid[0];
}

// 鎺ュ彛IID瀹氫箟
template<> const GUID audio_analyzer::iid = 
    { 0x12345678, 0x9abc, 0xdef0, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };

template<> const char* audio_analyzer::interface_name = "IAudioAnalyzer";

// 娉ㄥ唽棰戣氨鍒嗘瀽浠?
FB2K_REGISTER_SERVICE(spectrum_analyzer, audio_analyzer::iid)

} // namespace fb2k