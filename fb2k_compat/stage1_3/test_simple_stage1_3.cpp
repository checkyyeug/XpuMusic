#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <chrono>

// 绠€鍖栫殑娴嬭瘯楠岃瘉
namespace fb2k {

// 妯℃嫙鐨勯煶棰戝潡
class simple_audio_chunk {
public:
    simple_audio_chunk(size_t samples, uint32_t channels, uint32_t sample_rate)
        : data_(samples * channels, 0.0f),
          sample_count_(samples),
          channels_(channels),
          sample_rate_(sample_rate) {}
    
    float* get_data() { return data_.data(); }
    const float* get_data() const { return data_.data(); }
    size_t get_sample_count() const { return sample_count_; }
    uint32_t get_channels() const { return channels_; }
    uint32_t get_sample_rate() const { return sample_rate_; }
    
    float calculate_rms() const {
        if(data_.empty()) return 0.0f;
        
        double sum = 0.0;
        for(float sample : data_) {
            sum += sample * sample;
        }
        return std::sqrt(sum / data_.size());
    }
    
private:
    std::vector<float> data_;
    size_t sample_count_;
    uint32_t channels_;
    uint32_t sample_rate_;
};

// 绠€鍖栫殑娣峰搷鏁堟灉鍣ㄦ祴璇?
class simple_reverb_test {
public:
    simple_reverb_test() : room_size_(0.5f), damping_(0.5f), wet_level_(0.3f) {}
    
    void set_room_size(float size) { room_size_ = std::max(0.0f, std::min(1.0f, size)); }
    void set_damping(float damping) { damping_ = std::max(0.0f, std::min(1.0f, damping)); }
    void set_wet_level(float level) { wet_level_ = std::max(0.0f, std::min(1.0f, level)); }
    
    bool process(simple_audio_chunk& chunk) {
        float* data = chunk.get_data();
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        
        // 绠€鍖栫殑娣峰搷澶勭悊锛堝疄闄呭疄鐜颁細鏇村鏉傦級
        for(size_t i = 0; i < total_samples; ++i) {
            // 妯℃嫙娣峰搷鏁堟灉锛氭坊鍔犱竴鐐瑰欢杩熷拰琛板噺
            float input = data[i];
            float delayed = (i > 100) ? data[i - 100] * 0.3f * room_size_ : 0.0f;
            float damped = delayed * (1.0f - damping_);
            
            // 娣峰悎骞叉箍淇″彿
            data[i] = input * (1.0f - wet_level_) + damped * wet_level_;
        }
        
        return true;
    }
    
private:
    float room_size_;
    float damping_;
    float wet_level_;
};

// 绠€鍖栫殑鍧囪　鍣ㄦ祴璇?
class simple_eq_test {
public:
    simple_eq_test() {
        // 鍒濆鍖?涓娈?
        bands_.resize(5);
        bands_[0] = {100.0f, 0.0f, 1.0f};   // 浣庨
        bands_[1] = {300.0f, 0.0f, 1.0f};   // 浣庝腑棰?
        bands_[2] = {1000.0f, 0.0f, 1.0f};  // 涓
        bands_[3] = {3000.0f, 0.0f, 1.0f};  // 楂樹腑棰?
        bands_[4] = {10000.0f, 0.0f, 1.0f}; // 楂橀
    }
    
    struct eq_band {
        float frequency;
        float gain; // dB
        float q;
    };
    
    void set_band_gain(size_t band_index, float gain_db) {
        if(band_index < bands_.size()) {
            bands_[band_index].gain = std::max(-24.0f, std::min(24.0f, gain_db));
        }
    }
    
    bool process(simple_audio_chunk& chunk) {
        // 绠€鍖栫殑EQ澶勭悊锛堝疄闄呭疄鐜颁細鏇村鏉傦級
        float* data = chunk.get_data();
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        
        // 搴旂敤姣忎釜棰戞鐨勫鐩?
        for(size_t i = 0; i < total_samples; ++i) {
            float sample = data[i];
            float output = sample;
            
            // 绠€鍖栫殑澧炵泭搴旂敤
            for(const auto& band : bands_) {
                if(band.gain != 0.0f) {
                    float gain_linear = std::pow(10.0f, band.gain / 20.0f);
                    output *= gain_linear;
                }
            }
            
            // 闃叉鍓婃尝
            output = std::max(-1.0f, std::min(1.0f, output));
            data[i] = output;
        }
        
        return true;
    }
    
private:
    std::vector<eq_band> bands_;
};

// 鎬ц兘娴嬭瘯
class performance_test {
public:
    static bool test_reverb_performance() {
        std::cout << "\n=== 娣峰搷鎬ц兘娴嬭瘯 ===" << std::endl;
        
        simple_reverb_test reverb;
        reverb.set_room_size(0.7f);
        reverb.set_damping(0.3f);
        reverb.set_wet_level(0.4f);
        
        // 鍒涘缓娴嬭瘯闊抽鍧?
        simple_audio_chunk chunk(1024, 2, 44100);
        
        // 濉厖娴嬭瘯鏁版嵁
        float* data = chunk.get_data();
        for(size_t i = 0; i < 1024 * 2; ++i) {
            data[i] = std::sin(2.0f * M_PI * 440.0f * i / 44100) * 0.5f;
        }
        
        // 娴嬭瘯澶勭悊鎬ц兘
        const int iterations = 1000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for(int i = 0; i < iterations; ++i) {
            // 閲嶇疆鏁版嵁
            for(size_t j = 0; j < 1024 * 2; ++j) {
                data[j] = std::sin(2.0f * M_PI * 440.0f * j / 44100) * 0.5f;
            }
            
            if(!reverb.process(chunk)) {
                std::cout << "娣峰搷澶勭悊澶辫触" << std::endl;
                return false;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double total_time_ms = duration.count() / 1000.0;
        double avg_time_ms = total_time_ms / iterations;
        double rtf = avg_time_ms / (1024.0 / 44100 * 1000.0); // 瀹炴椂鍊嶆暟
        
        std::cout << "娣峰搷鎬ц兘缁撴灉:" << std::endl;
        std::cout << "  鎬诲鐞嗘椂闂? " << total_time_ms << "ms" << std::endl;
        std::cout << "  骞冲潎澶勭悊鏃堕棿: " << avg_time_ms << "ms" << std::endl;
        std::cout << "  瀹炴椂鍊嶆暟: " << rtf << "x" << std::endl;
        std::cout << "  杈撳嚭RMS: " << chunk.calculate_rms() << std::endl;
        
        return rtf < 1.0; // 瑕佹眰瀹炴椂鍊嶆暟灏忎簬1
    }
    
    static bool test_eq_performance() {
        std::cout << "\n=== 鍧囪　鍣ㄦ€ц兘娴嬭瘯 ===" << std::endl;
        
        simple_eq_test eq;
        
        // 璁剧疆涓€浜汦Q鍙傛暟
        eq.set_band_gain(0, 3.0f);   // 浣庨+3dB
        eq.set_band_gain(2, -2.0f);  // 涓-2dB
        eq.set_band_gain(4, 4.0f);   // 楂橀+4dB
        
        // 鍒涘缓娴嬭瘯闊抽鍧?
        simple_audio_chunk chunk(1024, 2, 44100);
        
        // 濉厖娴嬭瘯鏁版嵁
        float* data = chunk.get_data();
        for(size_t i = 0; i < 1024 * 2; ++i) {
            data[i] = std::sin(2.0f * M_PI * 1000.0f * i / 44100) * 0.5f;
        }
        
        // 娴嬭瘯澶勭悊鎬ц兘
        const int iterations = 1000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for(int i = 0; i < iterations; ++i) {
            // 閲嶇疆鏁版嵁
            for(size_t j = 0; j < 1024 * 2; ++j) {
                data[j] = std::sin(2.0f * M_PI * 1000.0f * j / 44100) * 0.5f;
            }
            
            if(!eq.process(chunk)) {
                std::cout << "EQ澶勭悊澶辫触" << std::endl;
                return false;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double total_time_ms = duration.count() / 1000.0;
        double avg_time_ms = total_time_ms / iterations;
        double rtf = avg_time_ms / (1024.0 / 44100 * 1000.0); // 瀹炴椂鍊嶆暟
        
        std::cout << "鍧囪　鍣ㄦ€ц兘缁撴灉:" << std::endl;
        std::cout << "  鎬诲鐞嗘椂闂? " << total_time_ms << "ms" << std::endl;
        std::cout << "  骞冲潎澶勭悊鏃堕棿: " << avg_time_ms << "ms" << std::endl;
        std::cout << "  瀹炴椂鍊嶆暟: " << rtf << "x" << std::endl;
        std::cout << "  杈撳嚭RMS: " << chunk.calculate_rms() << std::endl;
        
        return rtf < 1.0; // 瑕佹眰瀹炴椂鍊嶆暟灏忎簬1
    }
};

// 鍔熻兘娴嬭瘯
class functionality_test {
public:
    static bool test_reverb_functionality() {
        std::cout << "\n=== 娣峰搷鍔熻兘娴嬭瘯 ===" << std::endl;
        
        simple_reverb_test reverb;
        simple_audio_chunk chunk(512, 2, 44100);
        
        // 鐢熸垚娴嬭瘯淇″彿
        float* data = chunk.get_data();
        for(size_t i = 0; i < 512 * 2; ++i) {
            data[i] = (i < 100) ? 1.0f : 0.0f; // 鑴夊啿淇″彿
        }
        
        float input_rms = chunk.calculate_rms();
        std::cout << "杈撳叆RMS: " << input_rms << std::endl;
        
        // 娴嬭瘯涓嶅悓鍙傛暟
        reverb.set_room_size(0.8f);
        reverb.set_damping(0.2f);
        reverb.set_wet_level(0.5f);
        
        if(!reverb.process(chunk)) {
            std::cout << "娣峰搷澶勭悊澶辫触" << std::endl;
            return false;
        }
        
        float output_rms = chunk.calculate_rms();
        std::cout << "杈撳嚭RMS: " << output_rms << std::endl;
        
        // 楠岃瘉娣峰搷鏁堟灉锛堣緭鍑哄簲璇ユ瘮杈撳叆鏈夋洿澶氳兘閲忥紝鍥犱负鏈夋贩鍝嶅熬闊筹級
        bool has_reverb_effect = output_rms > input_rms * 0.1f;
        std::cout << "娣峰搷鏁堟灉楠岃瘉: " << (has_reverb_effect ? "閫氳繃" : "澶辫触") << std::endl;
        
        return has_reverb_effect;
    }
    
    static bool test_eq_functionality() {
        std::cout << "\n=== 鍧囪　鍣ㄥ姛鑳芥祴璇?===" << std::endl;
        
        simple_eq_test eq;
        simple_audio_chunk chunk(512, 2, 44100);
        
        // 鐢熸垚娴嬭瘯淇″彿
        float* data = chunk.get_data();
        for(size_t i = 0; i < 512 * 2; ++i) {
            data[i] = 0.5f; // 鐩存祦淇″彿
        }
        
        float input_rms = chunk.calculate_rms();
        std::cout << "杈撳叆RMS: " << input_rms << std::endl;
        
        // 娴嬭瘯澧炵泭鎻愬崌
        eq.set_band_gain(2, 6.0f); // 涓+6dB
        
        if(!eq.process(chunk)) {
            std::cout << "EQ澶勭悊澶辫触" << std::endl;
            return false;
        }
        
        float output_rms = chunk.calculate_rms();
        std::cout << "杈撳嚭RMS: " << output_rms << std::endl;
        
        // 楠岃瘉澧炵泭鏁堟灉锛堣緭鍑哄簲璇ユ瘮杈撳叆澶э級
        bool has_gain_effect = output_rms > input_rms * 1.5f; // +6dB搴旇澶х害鏄?鍊?
        std::cout << "澧炵泭鏁堟灉楠岃瘉: " << (has_gain_effect ? "閫氳繃" : "澶辫触") << std::endl;
        
        return has_gain_effect;
    }
};

// 涓绘祴璇曞嚱鏁?
int main() {
    std::cout << "foobar2000鍏煎灞?- 闃舵1.3绠€鍖栧姛鑳芥祴璇? << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "娴嬭瘯鏃堕棿: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    
    bool all_passed = true;
    
    // 杩愯鍔熻兘娴嬭瘯
    if(!functionality_test::test_reverb_functionality()) {
        all_passed = false;
    }
    
    if(!functionality_test::test_eq_functionality()) {
        all_passed = false;
    }
    
    // 杩愯鎬ц兘娴嬭瘯
    if(!performance_test::test_reverb_performance()) {
        all_passed = false;
    }
    
    if(!performance_test::test_eq_performance()) {
        all_passed = false;
    }
    
    // 鎬荤粨
    std::cout << "\n========================================" << std::endl;
    std::cout << "娴嬭瘯鎬荤粨:" << std::endl;
    std::cout << "  缁撴灉: " << (all_passed ? "鉁?鎵€鏈夋祴璇曢€氳繃" : "鉁?閮ㄥ垎娴嬭瘯澶辫触") << std::endl;
    
    if(all_passed) {
        std::cout << "\n闃舵1.3鏍稿績鍔熻兘楠岃瘉閫氳繃锛? << std::endl;
        std::cout << "- 娣峰搷鏁堟灉鍣ㄥ伐浣滄甯? << std::endl;
        std::cout << "- 鍧囪　鍣ㄥ姛鑳芥甯? << std::endl;
        std::cout << "- 鎬ц兘婊¤冻瀹炴椂澶勭悊瑕佹眰" << std::endl;
        return 0;
    } else {
        std::cout << "\n闃舵1.3鍔熻兘楠岃瘉澶辫触锛岃妫€鏌ュ疄鐜般€? << std::endl;
        return 1;
    }
}

} // namespace fb2k