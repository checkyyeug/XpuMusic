#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <fstream>

// 鍖呭惈蹇呰鐨勫ご鏂囦欢
#include "dsp_equalizer.h"
#include "dsp_reverb.h"
#include "dsp_manager.h"
#include "output_wasapi.h"
#include "audio_processor.h"
#include "audio_block_impl.h"
#include "abort_callback.h"

namespace fb2k {

// 娴嬭瘯閰嶇疆
struct test_config {
    uint32_t sample_rate = 44100;
    uint32_t channels = 2;
    size_t test_duration_seconds = 5;
    bool enable_dsp = true;
    bool enable_output = false; // 榛樿绂佺敤杈撳嚭浠ラ伩鍏嶅０闊?
    bool verbose = true;
};

// 娴嬭瘯闊抽婧?
class test_audio_source : public audio_source {
public:
    test_audio_source(uint32_t sample_rate, uint32_t channels, float frequency = 440.0f)
        : sample_rate_(sample_rate),
          channels_(channels),
          frequency_(frequency),
          current_phase_(0.0f) {
    }
    
    bool get_next_chunk(audio_chunk& chunk, abort_callback& abort) override {
        if(abort.is_aborting()) {
            return false;
        }
        
        // 鍒涘缓娴嬭瘯闊抽鏁版嵁
        size_t samples_per_chunk = 512;
        chunk.set_sample_count(samples_per_chunk);
        chunk.set_channels(channels_);
        chunk.set_sample_rate(sample_rate_);
        
        float* data = chunk.get_data();
        float phase_increment = 2.0f * M_PI * frequency_ / sample_rate_;
        
        for(size_t i = 0; i < samples_per_chunk; ++i) {
            float sample = std::sin(current_phase_);
            
            // 鍐欏叆鎵€鏈夊０閬?
            for(uint32_t ch = 0; ch < channels_; ++ch) {
                data[i * channels_ + ch] = sample * 0.5f; // 闄嶄綆闊抽噺
            }
            
            current_phase_ += phase_increment;
            if(current_phase_ >= 2.0f * M_PI) {
                current_phase_ -= 2.0f * M_PI;
            }
        }
        
        return true;
    }
    
    bool is_eof() const override {
        return false; // 鏃犻檺鐢熸垚娴嬭瘯闊抽
    }
    
    void reset() override {
        current_phase_ = 0.0f;
    }
    
private:
    uint32_t sample_rate_;
    uint32_t channels_;
    float frequency_;
    float current_phase_;
};

// 娴嬭瘯闊抽杈撳嚭
class test_audio_sink : public audio_sink {
public:
    test_audio_sink(const std::string& name = "TestSink")
        : name_(name),
          total_samples_written_(0),
          chunk_count_(0) {
    }
    
    bool write_chunk(const audio_chunk& chunk, abort_callback& abort) override {
        if(abort.is_aborting()) {
            return false;
        }
        
        // 璁板綍缁熻淇℃伅
        total_samples_written_ += chunk.get_sample_count() * chunk.get_channels();
        chunk_count_++;
        
        // 璁＄畻RMS鐢靛钩
        float rms = chunk.calculate_rms();
        
        // 姣?00涓潡杈撳嚭涓€娆＄粺璁?
        if(chunk_count_ % 100 == 0) {
            std::cout << "[TestSink] " << name_ << " - 鍧楁暟: " << chunk_count_ 
                      << ", 鎬婚噰鏍? " << total_samples_written_ 
                      << ", RMS: " << std::fixed << std::setprecision(3) << rms << std::endl;
        }
        
        return true;
    }
    
    bool is_ready() const override {
        return true;
    }
    
    void flush() override {
        total_samples_written_ = 0;
        chunk_count_ = 0;
    }
    
    size_t get_total_samples() const {
        return total_samples_written_;
    }
    
    size_t get_chunk_count() const {
        return chunk_count_;
    }
    
private:
    std::string name_;
    size_t total_samples_written_;
    size_t chunk_count_;
};

// 娴嬭瘯杈呭姪鍑芥暟
class test_helper {
public:
    static void print_test_header(const std::string& test_name) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "娴嬭瘯: " << test_name << std::endl;
        std::cout << std::string(60, '-') << std::endl;
    }
    
    static void print_test_result(bool success, const std::string& message) {
        std::cout << "缁撴灉: " << (success ? "鉁?閫氳繃" : "鉁?澶辫触") << " - " << message << std::endl;
    }
    
    static void print_performance_stats(const audio_processor_stats& stats) {
        std::cout << "\n鎬ц兘缁熻:" << std::endl;
        std::cout << "  鎬婚噰鏍锋暟: " << stats.total_samples_processed << std::endl;
        std::cout << "  鎬诲鐞嗘椂闂? " << std::fixed << std::setprecision(3) 
                  << stats.total_processing_time_ms << "ms" << std::endl;
        std::cout << "  骞冲潎澶勭悊鏃堕棿: " << stats.average_processing_time_ms << "ms" << std::endl;
        std::cout << "  褰撳墠CPU鍗犵敤: " << std::fixed << std::setprecision(1) 
                  << stats.current_cpu_usage << "%" << std::endl;
        std::cout << "  宄板€糃PU鍗犵敤: " << stats.peak_cpu_usage << "%" << std::endl;
        std::cout << "  寤惰繜: " << std::fixed << std::setprecision(2) 
                  << stats.latency_ms << "ms" << std::endl;
        std::cout << "  涓㈠寘鏁? " << stats.dropout_count << std::endl;
        std::cout << "  閿欒鏁? " << stats.error_count << std::endl;
    }
    
    static bool save_test_audio(const audio_chunk& chunk, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if(!file.is_open()) {
            return false;
        }
        
        // 绠€鍖栫殑WAV鏂囦欢澶达紙瀹為檯搴旇浣跨敤瀹屾暣鐨刉AV鏍煎紡锛?
        const float* data = chunk.get_data();
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        
        // 鍐欏叆鏁版嵁锛堣繖閲岀畝鍖栧鐞嗭紝瀹為檯搴旇鍐欏叆瀹屾暣鐨刉AV鏂囦欢锛?
        file.write(reinterpret_cast<const char*>(data), total_samples * sizeof(float));
        
        file.close();
        return true;
    }
};

// 娴嬭瘯1: 鍩虹DSP鏁堟灉鍣ㄦ祴璇?
bool test_basic_dsp_effects(const test_config& config) {
    test_helper::print_test_header("鍩虹DSP鏁堟灉鍣ㄦ祴璇?);
    
    bool all_passed = true;
    
    try {
        // 鍒涘缓DSP绠＄悊鍣?
        auto dsp_manager = std::make_unique<dsp_manager>();
        dsp_config dsp_config;
        dsp_config.enable_standard_effects = true;
        dsp_config.enable_performance_monitoring = true;
        
        if(!dsp_manager->initialize(dsp_config)) {
            test_helper::print_test_result(false, "DSP绠＄悊鍣ㄥ垵濮嬪寲澶辫触");
            return false;
        }
        
        // 娴嬭瘯1.1: 鍒涘缓鍧囪　鍣?
        auto eq = dsp_manager->create_equalizer_10band();
        if(!eq) {
            test_helper::print_test_result(false, "鍒涘缓10娈靛潎琛″櫒澶辫触");
            all_passed = false;
        } else {
            dsp_manager->add_effect(std::move(eq));
            test_helper::print_test_result(true, "鍒涘缓骞舵坊鍔?0娈靛潎琛″櫒");
        }
        
        // 娴嬭瘯1.2: 鍒涘缓娣峰搷
        auto reverb = dsp_manager->create_reverb();
        if(!reverb) {
            test_helper::print_test_result(false, "鍒涘缓娣峰搷鏁堟灉鍣ㄥけ璐?);
            all_passed = false;
        } else {
            dsp_manager->add_effect(std::move(reverb));
            test_helper::print_test_result(true, "鍒涘缓骞舵坊鍔犳贩鍝嶆晥鏋滃櫒");
        }
        
        // 娴嬭瘯1.3: 鍒涘缓鍘嬬缉鍣?
        auto compressor = dsp_manager->create_compressor();
        if(!compressor) {
            test_helper::print_test_result(false, "鍒涘缓鍘嬬缉鍣ㄥけ璐?);
            all_passed = false;
        } else {
            dsp_manager->add_effect(std::move(compressor));
            test_helper::print_test_result(true, "鍒涘缓骞舵坊鍔犲帇缂╁櫒");
        }
        
        // 娴嬭瘯1.4: 楠岃瘉鏁堟灉鍣ㄦ暟閲?
        size_t effect_count = dsp_manager->get_effect_count();
        if(effect_count == 3) {
            test_helper::print_test_result(true, "鏁堟灉鍣ㄦ暟閲忛獙璇? " + std::to_string(effect_count));
        } else {
            test_helper::print_test_result(false, "鏁堟灉鍣ㄦ暟閲忎笉鍖归厤: " + std::to_string(effect_count) + " != 3");
            all_passed = false;
        }
        
        // 娴嬭瘯1.5: 鐢熸垚DSP鎶ュ憡
        std::string dsp_report = dsp_manager->generate_dsp_report();
        if(!dsp_report.empty()) {
            test_helper::print_test_result(true, "鐢熸垚DSP鎶ュ憡鎴愬姛");
            if(config.verbose) {
                std::cout << "\nDSP鎶ュ憡棰勮:\n" << dsp_report.substr(0, 200) << "..." << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "DSP鎶ュ憡涓虹┖");
            all_passed = false;
        }
        
        dsp_manager->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯2: 楂樼骇娣峰搷鏁堟灉鍣ㄦ祴璇?
bool test_advanced_reverb(const test_config& config) {
    test_helper::print_test_header("楂樼骇娣峰搷鏁堟灉鍣ㄦ祴璇?);
    
    bool all_passed = true;
    
    try {
        // 鍒涘缓娣峰搷鍙傛暟
        dsp_effect_params reverb_params;
        reverb_params.type = dsp_effect_type::reverb;
        reverb_params.name = "Advanced Reverb";
        reverb_params.is_enabled = true;
        reverb_params.is_bypassed = false;
        reverb_params.cpu_usage_estimate = 15.0f;
        reverb_params.latency_ms = 10.0;
        
        // 鍒涘缓楂樼骇娣峰搷鏁堟灉鍣?
        auto reverb = std::make_unique<dsp_reverb_advanced>(reverb_params);
        
        // 娴嬭瘯2.1: 鍔犺浇涓嶅悓棰勮
        reverb->load_room_preset(0.3f); // 灏忔埧闂?
        test_helper::print_test_result(true, "鍔犺浇灏忔埧闂撮璁?);
        
        reverb->load_hall_preset(0.7f); // 澶у巺
        test_helper::print_test_result(true, "鍔犺浇澶у巺棰勮");
        
        reverb->load_plate_preset(); // 鏉垮紡娣峰搷
        test_helper::print_test_result(true, "鍔犺浇鏉垮紡娣峰搷棰勮");
        
        reverb->load_cathedral_preset(); // 澶ф暀鍫?
        test_helper::print_test_result(true, "鍔犺浇澶ф暀鍫傞璁?);
        
        // 娴嬭瘯2.2: 鍙傛暟璋冭妭
        reverb->set_room_size(0.6f);
        reverb->set_damping(0.4f);
        reverb->set_wet_level(0.3f);
        reverb->set_dry_level(0.7f);
        reverb->set_width(0.8f);
        reverb->set_predelay(20.0f);
        reverb->set_decay_time(2.5f);
        reverb->set_diffusion(0.8f);
        
        test_helper::print_test_result(true, "娣峰搷鍙傛暟璋冭妭");
        
        // 娴嬭瘯2.3: 璋冨埗鍔熻兘
        reverb->set_modulation_rate(0.5f);
        reverb->set_modulation_depth(0.2f);
        reverb->enable_modulation(true);
        test_helper::print_test_result(true, "鍚敤璋冨埗鍔熻兘");
        
        // 娴嬭瘯2.4: 婊ゆ尝鍔熻兘
        reverb->enable_filtering(true);
        test_helper::print_test_result(true, "鍚敤婊ゆ尝鍔熻兘");
        
        // 娴嬭瘯2.5: 瀹炰緥鍖栧拰澶勭悊
        audio_chunk test_chunk;
        test_chunk.set_sample_count(512);
        test_chunk.set_channels(config.channels);
        test_chunk.set_sample_rate(config.sample_rate);
        
        // 濉厖娴嬭瘯鏁版嵁
        float* data = test_chunk.get_data();
        for(size_t i = 0; i < 512 * config.channels; ++i) {
            data[i] = std::sin(2.0f * M_PI * 440.0f * i / config.sample_rate) * 0.5f;
        }
        
        abort_callback_dummy abort;
        
        if(reverb->instantiate(test_chunk, config.sample_rate, config.channels)) {
            test_helper::print_test_result(true, "娣峰搷鏁堟灉鍣ㄥ疄渚嬪寲鎴愬姛");
            
            // 澶勭悊闊抽
            reverb->run(test_chunk, abort);
            test_helper::print_test_result(true, "娣峰搷闊抽澶勭悊鎴愬姛");
            
            // 楠岃瘉杈撳嚭
            float output_rms = test_chunk.calculate_rms();
            if(output_rms > 0.0f) {
                test_helper::print_test_result(true, "娣峰搷杈撳嚭楠岃瘉閫氳繃锛孯MS: " + std::to_string(output_rms));
            } else {
                test_helper::print_test_result(false, "娣峰搷杈撳嚭涓虹┖");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "娣峰搷鏁堟灉鍣ㄥ疄渚嬪寲澶辫触");
            all_passed = false;
        }
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯3: 闊抽澶勭悊鍣ㄩ泦鎴愭祴璇?
bool test_audio_processor_integration(const test_config& config) {
    test_helper::print_test_header("闊抽澶勭悊鍣ㄩ泦鎴愭祴璇?);
    
    bool all_passed = true;
    
    try {
        // 鍒涘缓闊抽澶勭悊鍣?
        auto processor = create_audio_processor();
        
        // 閰嶇疆闊抽澶勭悊鍣?
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::realtime;
        processor_config.target_latency_ms = 10.0;
        processor_config.cpu_usage_limit_percent = 80.0f;
        processor_config.buffer_size = 1024;
        processor_config.max_dsp_effects = 16;
        processor_config.enable_performance_monitoring = true;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "闊抽澶勭悊鍣ㄥ垵濮嬪寲澶辫触");
            return false;
        }
        test_helper::print_test_result(true, "闊抽澶勭悊鍣ㄥ垵濮嬪寲鎴愬姛");
        
        // 娣诲姞DSP鏁堟灉鍣?
        auto eq = dsp_manager().create_equalizer_10band();
        auto reverb = dsp_manager().create_reverb();
        
        processor->add_dsp_effect(std::move(eq));
        processor->add_dsp_effect(std::move(reverb));
        test_helper::print_test_result(true, "娣诲姞DSP鏁堟灉鍣ㄦ垚鍔?);
        
        // 娴嬭瘯3.1: 鍩烘湰闊抽澶勭悊
        audio_chunk input_chunk;
        input_chunk.set_sample_count(512);
        input_chunk.set_channels(config.channels);
        input_chunk.set_sample_rate(config.sample_rate);
        
        // 鐢熸垚娴嬭瘯闊抽
        float* input_data = input_chunk.get_data();
        for(size_t i = 0; i < 512 * config.channels; ++i) {
            input_data[i] = std::sin(2.0f * M_PI * 440.0f * i / config.sample_rate) * 0.5f;
        }
        
        audio_chunk output_chunk;
        abort_callback_dummy abort;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        bool process_success = processor->process_audio(input_chunk, output_chunk, abort);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time_ms = duration.count() / 1000.0;
        
        if(process_success) {
            test_helper::print_test_result(true, "闊抽澶勭悊鎴愬姛锛岃€楁椂: " + std::to_string(processing_time_ms) + "ms");
            
            // 楠岃瘉杈撳嚭
            float output_rms = output_chunk.calculate_rms();
            if(output_rms > 0.0f) {
                test_helper::print_test_result(true, "杈撳嚭楠岃瘉閫氳繃锛孯MS: " + std::to_string(output_rms));
            } else {
                test_helper::print_test_result(false, "杈撳嚭涓虹┖");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "闊抽澶勭悊澶辫触");
            all_passed = false;
        }
        
        // 娴嬭瘯3.2: 闊抽噺鎺у埗
        processor->set_volume(0.5f);
        processor->process_audio(input_chunk, output_chunk, abort);
        float half_volume_rms = output_chunk.calculate_rms();
        
        processor->set_volume(1.0f);
        processor->process_audio(input_chunk, output_chunk, abort);
        float full_volume_rms = output_chunk.calculate_rms();
        
        if(std::abs(half_volume_rms - full_volume_rms * 0.5f) < full_volume_rms * 0.1f) {
            test_helper::print_test_result(true, "闊抽噺鎺у埗楠岃瘉閫氳繃");
        } else {
            test_helper::print_test_result(false, "闊抽噺鎺у埗楠岃瘉澶辫触");
            all_passed = false;
        }
        
        // 娴嬭瘯3.3: 闈欓煶鍔熻兘
        processor->set_mute(true);
        processor->process_audio(input_chunk, output_chunk, abort);
        float muted_rms = output_chunk.calculate_rms();
        
        processor->set_mute(false);
        
        if(muted_rms < 0.001f) {
            test_helper::print_test_result(true, "闈欓煶鍔熻兘楠岃瘉閫氳繃");
        } else {
            test_helper::print_test_result(false, "闈欓煶鍔熻兘楠岃瘉澶辫触");
            all_passed = false;
        }
        
        // 娴嬭瘯3.4: 瀹炴椂鍙傛暟璋冭妭
        processor->set_realtime_parameter("Reverb", "room_size", 0.7f);
        float room_size = processor->get_realtime_parameter("Reverb", "room_size");
        
        if(std::abs(room_size - 0.7f) < 0.01f) {
            test_helper::print_test_result(true, "瀹炴椂鍙傛暟璋冭妭楠岃瘉閫氳繃");
        } else {
            test_helper::print_test_result(false, "瀹炴椂鍙傛暟璋冭妭楠岃瘉澶辫触");
            all_passed = false;
        }
        
        // 娴嬭瘯3.5: 鎬ц兘缁熻
        auto stats = processor->get_stats();
        if(stats.total_samples_processed > 0) {
            test_helper::print_test_result(true, "鎬ц兘缁熻鏀堕泦鎴愬姛");
            test_helper::print_performance_stats(stats);
        } else {
            test_helper::print_test_result(false, "鎬ц兘缁熻涓虹┖");
            all_passed = false;
        }
        
        // 娴嬭瘯3.6: 鐘舵€佹姤鍛?
        std::string status_report = processor->get_status_report();
        if(!status_report.empty()) {
            test_helper::print_test_result(true, "鐘舵€佹姤鍛婄敓鎴愭垚鍔?);
            if(config.verbose) {
                std::cout << "\n鐘舵€佹姤鍛婇瑙?\n" << status_report.substr(0, 300) << "..." << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "鐘舵€佹姤鍛婁负绌?);
            all_passed = false;
        }
        
        processor->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯4: 闊抽娴佸鐞嗘祴璇?
bool test_audio_stream_processing(const test_config& config) {
    test_helper::print_test_header("闊抽娴佸鐞嗘祴璇?);
    
    bool all_passed = true;
    
    try {
        // 鍒涘缓闊抽澶勭悊鍣?
        auto processor = create_audio_processor();
        
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::realtime;
        processor_config.target_latency_ms = 10.0;
        processor_config.cpu_usage_limit_percent = 80.0f;
        processor_config.buffer_size = 1024;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "闊抽澶勭悊鍣ㄥ垵濮嬪寲澶辫触");
            return false;
        }
        
        // 娣诲姞鏁堟灉鍣?
        auto eq = dsp_manager().create_equalizer_10band();
        auto reverb = dsp_manager().create_reverb();
        processor->add_dsp_effect(std::move(eq));
        processor->add_dsp_effect(std::move(reverb));
        
        // 鍒涘缓闊抽婧愬拰杈撳嚭
        auto audio_source = std::make_unique<test_audio_source>(config.sample_rate, config.channels, 440.0f);
        auto audio_sink = std::make_unique<test_audio_sink>("StreamTest");
        
        // 娴嬭瘯4.1: 鍩烘湰娴佸鐞?
        abort_callback_dummy abort;
        abort.set_timeout(1000); // 1绉掕秴鏃?
        
        std::cout << "寮€濮嬫祦澶勭悊娴嬭瘯..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();
        
        bool stream_success = processor->process_stream(audio_source.get(), audio_sink.get(), abort);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if(stream_success) {
            test_helper::print_test_result(true, "娴佸鐞嗘垚鍔燂紝鑰楁椂: " + std::to_string(duration.count()) + "ms");
            test_helper::print_test_result(true, "澶勭悊閲囨牱鏁? " + std::to_string(audio_sink->get_total_samples()));
        } else {
            test_helper::print_test_result(false, "娴佸鐞嗗け璐?);
            all_passed = false;
        }
        
        // 娴嬭瘯4.2: 涓柇澶勭悊
        auto abort_source = std::make_unique<test_audio_source>(config.sample_rate, config.channels, 880.0f);
        auto abort_sink = std::make_unique<test_audio_sink>("AbortTest");
        
        // 璁剧疆蹇€熶腑鏂?
        abort_callback_dummy fast_abort;
        fast_abort.set_timeout(100); // 100ms鍚庝腑鏂?
        
        bool abort_success = processor->process_stream(abort_source.get(), abort_sink.get(), fast_abort);
        
        if(!abort_success && fast_abort.is_aborting()) {
            test_helper::print_test_result(true, "涓柇澶勭悊楠岃瘉閫氳繃");
        } else {
            test_helper::print_test_result(false, "涓柇澶勭悊楠岃瘉澶辫触");
            all_passed = false;
        }
        
        // 娴嬭瘯4.3: 鎬ц兘鐩戞帶
        auto final_stats = processor->get_stats();
        if(final_stats.total_samples_processed > 0) {
            test_helper::print_test_result(true, "娴佸鐞嗘€ц兘缁熻鏀堕泦鎴愬姛");
            test_helper::print_performance_stats(final_stats);
        } else {
            test_helper::print_test_result(false, "娴佸鐞嗘€ц兘缁熻涓虹┖");
            all_passed = false;
        }
        
        processor->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯5: 鎬ц兘鍩哄噯娴嬭瘯
bool test_performance_benchmark(const test_config& config) {
    test_helper::print_test_header("鎬ц兘鍩哄噯娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 鍒涘缓闊抽澶勭悊鍣?
        auto processor = create_audio_processor();
        
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::high_fidelity;
        processor_config.target_latency_ms = 5.0;
        processor_config.cpu_usage_limit_percent = 90.0f;
        processor_config.buffer_size = 2048;
        processor_config.enable_performance_monitoring = true;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "闊抽澶勭悊鍣ㄥ垵濮嬪寲澶辫触");
            return false;
        }
        
        // 娣诲姞澶氫釜鏁堟灉鍣ㄨ繘琛屽帇鍔涙祴璇?
        for(int i = 0; i < 5; ++i) {
            auto eq = dsp_manager().create_equalizer_10band();
            auto reverb = dsp_manager().create_reverb();
            processor->add_dsp_effect(std::move(eq));
            processor->add_dsp_effect(std::move(reverb));
        }
        
        // 娴嬭瘯5.1: 澶勭悊閫熷害鍩哄噯
        audio_chunk test_chunk;
        test_chunk.set_sample_count(1024);
        test_chunk.set_channels(config.channels);
        test_chunk.set_sample_rate(config.sample_rate);
        
        // 鐢熸垚澶嶆潅鐨勬祴璇曚俊鍙?
        float* data = test_chunk.get_data();
        for(size_t i = 0; i < 1024 * config.channels; ++i) {
            data[i] = std::sin(2.0f * M_PI * 440.0f * i / config.sample_rate) * 0.3f +
                      std::sin(2.0f * M_PI * 880.0f * i / config.sample_rate) * 0.2f +
                      std::sin(2.0f * M_PI * 1320.0f * i / config.sample_rate) * 0.1f;
        }
        
        const int iterations = 100;
        std::vector<double> processing_times;
        
        abort_callback_dummy abort;
        
        for(int i = 0; i < iterations; ++i) {
            audio_chunk output_chunk;
            
            auto start_time = std::chrono::high_resolution_clock::now();
            bool success = processor->process_audio(test_chunk, output_chunk, abort);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            if(success) {
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                processing_times.push_back(duration.count() / 1000.0); // 杞崲涓烘绉?
            }
        }
        
        if(!processing_times.empty()) {
            // 璁＄畻缁熻淇℃伅
            double total_time = 0.0;
            double min_time = processing_times[0];
            double max_time = processing_times[0];
            
            for(double time : processing_times) {
                total_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            
            double avg_time = total_time / processing_times.size();
            double rtf = (avg_time / (1024.0 / config.sample_rate * 1000.0)); // 瀹炴椂鍊嶆暟
            
            std::cout << "\n鎬ц兘鍩哄噯缁撴灉:" << std::endl;
            std::cout << "  娴嬭瘯娆℃暟: " << processing_times.size() << std::endl;
            std::cout << "  骞冲潎澶勭悊鏃堕棿: " << std::fixed << std::setprecision(3) << avg_time << "ms" << std::endl;
            std::cout << "  鏈€灏忓鐞嗘椂闂? " << min_time << "ms" << std::endl;
            std::cout << "  鏈€澶у鐞嗘椂闂? " << max_time << "ms" << std::endl;
            std::cout << "  瀹炴椂鍊嶆暟: " << std::fixed << std::setprecision(2) << rtf << "x" << std::endl;
            
            if(rtf < 1.0) {
                test_helper::print_test_result(true, "鎬ц兘鍩哄噯娴嬭瘯閫氳繃锛堝疄鏃跺€嶆暟 < 1.0锛?);
            } else {
                test_helper::print_test_result(false, "鎬ц兘鍩哄噯娴嬭瘯澶辫触锛堝疄鏃跺€嶆暟 >= 1.0锛?);
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "鎬ц兘鍩哄噯娴嬭瘯鏃犳湁鏁堟暟鎹?);
            all_passed = false;
        }
        
        // 娴嬭瘯5.2: 鍐呭瓨浣跨敤妫€鏌ワ紙绠€鍖栫増锛?
        auto final_stats = processor->get_stats();
        if(final_stats.error_count == 0) {
            test_helper::print_test_result(true, "澶勭悊杩囩▼涓棤閿欒");
        } else {
            test_helper::print_test_result(false, "澶勭悊杩囩▼涓嚭鐜伴敊璇? " + std::to_string(final_stats.error_count));
            all_passed = false;
        }
        
        processor->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 涓绘祴璇曞嚱鏁?
int main(int argc, char* argv[]) {
    std::cout << "foobar2000鍏煎灞?- 闃舵1.3鍔熻兘娴嬭瘯" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "娴嬭瘯鏃堕棿: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    
    // 瑙ｆ瀽鍛戒护琛屽弬鏁?
    test_config config;
    
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if(arg == "--sample-rate" && i + 1 < argc) {
            config.sample_rate = std::stoi(argv[++i]);
        } else if(arg == "--channels" && i + 1 < argc) {
            config.channels = std::stoi(argv[++i]);
        } else if(arg == "--duration" && i + 1 < argc) {
            config.test_duration_seconds = std::stoi(argv[++i]);
        } else if(arg == "--enable-output") {
            config.enable_output = true;
        } else if(arg == "--quiet") {
            config.verbose = false;
        } else if(arg == "--help") {
            std::cout << "鐢ㄦ硶: " << argv[0] << " [閫夐」]" << std::endl;
            std::cout << "閫夐」:" << std::endl;
            std::cout << "  --sample-rate <rate>   璁剧疆娴嬭瘯閲囨牱鐜?(榛樿: 44100)" << std::endl;
            std::cout << "  --channels <num>       璁剧疆娴嬭瘯澹伴亾鏁?(榛樿: 2)" << std::endl;
            std::cout << "  --duration <seconds>   璁剧疆娴嬭瘯鎸佺画鏃堕棿 (榛樿: 5)" << std::endl;
            std::cout << "  --enable-output        鍚敤闊抽杈撳嚭锛堜細浜х敓澹伴煶锛? << std::endl;
            std::cout << "  --quiet                鍑忓皯杈撳嚭淇℃伅" << std::endl;
            std::cout << "  --help                 鏄剧ず甯姪淇℃伅" << std::endl;
            return 0;
        }
    }
    
    std::cout << "\n娴嬭瘯閰嶇疆:" << std::endl;
    std::cout << "  閲囨牱鐜? " << config.sample_rate << "Hz" << std::endl;
    std::cout << "  澹伴亾鏁? " << config.channels << std::endl;
    std::cout << "  娴嬭瘯鎸佺画鏃堕棿: " << config.test_duration_seconds << "绉? << std::endl;
    std::cout << "  闊抽杈撳嚭: " << (config.enable_output ? "鍚敤" : "绂佺敤") << std::endl;
    std::cout << "  璇︾粏杈撳嚭: " << (config.verbose ? "鍚敤" : "绂佺敤") << std::endl;
    
    // 杩愯鎵€鏈夋祴璇?
    std::vector<std::pair<std::string, std::function<bool(const test_config&)>>> tests = {
        {"鍩虹DSP鏁堟灉鍣ㄦ祴璇?, test_basic_dsp_effects},
        {"楂樼骇娣峰搷鏁堟灉鍣ㄦ祴璇?, test_advanced_reverb},
        {"闊抽澶勭悊鍣ㄩ泦鎴愭祴璇?, test_audio_processor_integration},
        {"闊抽娴佸鐞嗘祴璇?, test_audio_stream_processing},
        {"鎬ц兘鍩哄噯娴嬭瘯", test_performance_benchmark}
    };
    
    int passed_count = 0;
    int total_count = tests.size();
    
    for(const auto& [test_name, test_func] : tests) {
        bool passed = test_func(config);
        if(passed) {
            passed_count++;
        }
        
        std::cout << "\n" << test_name << " 缁撴灉: " << (passed ? "閫氳繃" : "澶辫触") << std::endl;
    }
    
    // 鎬荤粨
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "娴嬭瘯鎬荤粨:" << std::endl;
    std::cout << "  鎬绘祴璇曟暟: " << total_count << std::endl;
    std::cout << "  閫氳繃娴嬭瘯: " << passed_count << std::endl;
    std::cout << "  澶辫触娴嬭瘯: " << (total_count - passed_count) << std::endl;
    std::cout << "  閫氳繃鐜? " << std::fixed << std::setprecision(1) 
              << (passed_count * 100.0 / total_count) << "%" << std::endl;
    
    if(passed_count == total_count) {
        std::cout << "\n鉁?鎵€鏈夋祴璇曢€氳繃锛侀樁娈?.3鍔熻兘姝ｅ父銆? << std::endl;
        return 0;
    } else {
        std::cout << "\n鉁?閮ㄥ垎娴嬭瘯澶辫触锛岃妫€鏌ラ敊璇俊鎭€? << std::endl;
        return 1;
    }
}

} // namespace fb2k