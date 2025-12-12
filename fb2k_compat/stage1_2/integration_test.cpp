#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

// 鍖呭惈鎵€鏈夐樁娈?.2鐨勫ご鏂囦欢
#include "audio_chunk.h"
#include "dsp_interfaces.h"
#include "output_interfaces.h"
#include "../stage1_1/real_minihost.h"

using namespace fb2k;
using namespace std::chrono;

// 闆嗘垚娴嬭瘯绫?
class IntegrationTest {
private:
    std::unique_ptr<RealMiniHost> host_;
    std::unique_ptr<dsp_chain> dsp_chain_;
    std::unique_ptr<audio_chunk> test_chunk_;
    
public:
    IntegrationTest() {
        host_ = std::make_unique<RealMiniHost>();
        dsp_chain_ = std::make_unique<dsp_chain>();
    }
    
    bool initialize() {
        std::cout << "=== 闃舵1.2闆嗘垚娴嬭瘯寮€濮?===" << std::endl;
        
        if(!host_->Initialize()) {
            std::cout << "鉂?涓绘満鍒濆鍖栧け璐? << std::endl;
            return false;
        }
        
        std::cout << "鉁?涓绘満鍒濆鍖栨垚鍔? << std::endl;
        
        // 鍒濆鍖朌SP绯荤粺
        if(!dsp_system_initializer::initialize_dsp_system()) {
            std::cout << "鉂?DSP绯荤粺鍒濆鍖栧け璐? << std::endl;
            return false;
        }
        
        std::cout << "鉁?DSP绯荤粺鍒濆鍖栨垚鍔? << std::endl;
        
        return true;
    }
    
    void shutdown() {
        dsp_system_initializer::shutdown_dsp_system();
        
        if(host_) {
            host_->Shutdown();
        }
        
        std::cout << "\n=== 闆嗘垚娴嬭瘯瀹屾垚 ===" << std::endl;
    }
    
    // 娴嬭瘯1锛氶煶棰戝潡鍩烘湰鍔熻兘
    bool test_audio_chunk_basic() {
        std::cout << "\n1. 闊抽鍧楀熀鏈姛鑳芥祴璇?.." << std::endl;
        
        // 鍒涘缓闊抽鍧?
        auto chunk = audio_chunk_utils::create_chunk(1024, 2, 44100);
        if(!chunk) {
            std::cout << "鉂?闊抽鍧楀垱寤哄け璐? << std::endl;
            return false;
        }
        
        std::cout << "鉁?闊抽鍧楀垱寤烘垚鍔? << std::endl;
        
        // 楠岃瘉鍩烘湰灞炴€?
        if(chunk->get_sample_count() != 1024) {
            std::cout << "鉂?閲囨牱鏁颁笉鍖归厤" << std::endl;
            return false;
        }
        
        if(chunk->get_channels() != 2) {
            std::cout << "鉂?澹伴亾鏁颁笉鍖归厤" << std::endl;
            return false;
        }
        
        if(chunk->get_sample_rate() != 44100) {
            std::cout << "鉂?閲囨牱鐜囦笉鍖归厤" << std::endl;
            return false;
        }
        
        std::cout << "鉁?闊抽鍧楀睘鎬ч獙璇侀€氳繃" << std::endl;
        
        // 楠岃瘉鏁版嵁鏈夋晥鎬?
        if(!audio_chunk_validation::validate_audio_chunk_basic(*chunk)) {
            std::cout << "鉂?闊抽鍧楀熀鏈獙璇佸け璐? << std::endl;
            return false;
        }
        
        if(!audio_chunk_validation::validate_audio_chunk_format(*chunk)) {
            std::cout << "鉂?闊抽鍧楁牸寮忛獙璇佸け璐? << std::endl;
            return false;
        }
        
        std::cout << "鉁?闊抽鍧楅獙璇侀€氳繃" << std::endl;
        
        // 鏄剧ず闊抽鍧椾俊鎭?
        audio_chunk_validation::log_audio_chunk_info(*chunk, "  ");
        
        return true;
    }
    
    // 娴嬭瘯2锛欴SP棰勮鍔熻兘
    bool test_dsp_preset() {
        std::cout << "\n2. DSP棰勮鍔熻兘娴嬭瘯..." << std::endl;
        
        // 鍒涘缓DSP棰勮
        auto preset = dsp_config_helper::create_basic_preset("TestPreset");
        if(!preset) {
            std::cout << "鉂?DSP棰勮鍒涘缓澶辫触" << std::endl;
            return false;
        }
        
        std::cout << "鉁?DSP棰勮鍒涘缓鎴愬姛" << std::endl;
        
        // 璁剧疆棰勮鍙傛暟
        preset->set_name("Equalizer");
        preset->set_parameter_float("gain", 0.8f);
        preset->set_parameter_float("bass", 1.2f);
        preset->set_parameter_float("treble", 0.9f);
        preset->set_parameter_string("mode", "rock");
        
        // 楠岃瘉鍙傛暟
        if(std::string(preset->get_name()) != "Equalizer") {
            std::cout << "鉂?棰勮鍚嶇О璁剧疆澶辫触" << std::endl;
            return false;
        }
        
        if(preset->get_parameter_float("gain") != 0.8f) {
            std::cout << "鉂?娴偣鍙傛暟璁剧疆澶辫触" << std::endl;
            return false;
        }
        
        if(std::string(preset->get_parameter_string("mode")) != "rock") {
            std::cout << "鉂?瀛楃涓插弬鏁拌缃け璐? << std::endl;
            return false;
        }
        
        std::cout << "鉁?DSP棰勮鍙傛暟璁剧疆鎴愬姛" << std::endl;
        
        // 搴忓垪鍖栧拰鍙嶅簭鍒楀寲娴嬭瘯
        std::vector<uint8_t> serialized_data;
        preset->serialize(serialized_data);
        
        auto new_preset = std::make_unique<simple_dsp_preset>();
        new_preset->deserialize(serialized_data.data(), serialized_data.size());
        
        if(std::string(new_preset->get_name()) != std::string(preset->get_name())) {
            std::cout << "鉂?棰勮搴忓垪鍖?鍙嶅簭鍒楀寲澶辫触" << std::endl;
            return false;
        }
        
        std::cout << "鉁?DSP棰勮搴忓垪鍖栨祴璇曢€氳繃" << std::endl;
        
        return true;
    }
    
    // 娴嬭瘯3锛欴SP鏁堟灉鍣ㄥ姛鑳?
    bool test_dsp_effects() {
        std::cout << "\n3. DSP鏁堟灉鍣ㄥ姛鑳芥祴璇?.." << std::endl;
        
        // 鍒涘缓娴嬭瘯DSP鏁堟灉鍣?
        auto effect = dsp_effect_factory::create_test_effect("TestEffect");
        if(!effect) {
            std::cout << "鉂?DSP鏁堟灉鍣ㄥ垱寤哄け璐? << std::endl;
            return false;
        }
        
        std::cout << "鉁?DSP鏁堟灉鍣ㄥ垱寤烘垚鍔? " << effect->get_name() << std::endl;
        
        // 鍒涘缓闊抽鍧?
        auto chunk = audio_chunk_utils::create_chunk(1024, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 瀹炰緥鍖栨晥鏋滃櫒
        if(!effect->instantiate(*chunk, 44100, 2)) {
            std::cout << "鉂?DSP鏁堟灉鍣ㄥ疄渚嬪寲澶辫触" << std::endl;
            return false;
        }
        
        std::cout << "鉁?DSP鏁堟灉鍣ㄥ疄渚嬪寲鎴愬姛" << std::endl;
        
        // 澶勭悊闊抽
        float rms_before = audio_chunk_utils::calculate_rms(*chunk);
        
        effect->run(*chunk, *abort);
        
        float rms_after = audio_chunk_utils::calculate_rms(*chunk);
        
        std::cout << "鉁?DSP鏁堟灉鍣ㄥ鐞嗗畬鎴? << std::endl;
        std::cout << "  澶勭悊鍓峈MS: " << rms_before << std::endl;
        std::cout << "  澶勭悊鍚嶳MS: " << rms_after << std::endl;
        
        // 楠岃瘉鏁堟灉鍣ㄥ弬鏁?
        auto params = effect->get_config_params();
        std::cout << "鉁?DSP鏁堟灉鍣ㄩ厤缃弬鏁?" << std::endl;
        for(const auto& param : params) {
            std::cout << "    - " << param.name << ": " << param.description 
                     << " (" << param.min_value << " - " << param.max_value << ")" << std::endl;
        }
        
        return true;
    }
    
    // 娴嬭瘯4锛欴SP閾惧姛鑳?
    bool test_dsp_chain() {
        std::cout << "\n4. DSP閾惧姛鑳芥祴璇?.." << std::endl;
        
        // 鍒涘缓DSP閾?
        auto chain = std::make_unique<dsp_chain>();
        
        // 娣诲姞澶氫釜鏁堟灉鍣?
        chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_volume_effect(0.8f)));
        chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_passthrough_effect("PassThrough")));
        
        std::cout << "鉁?DSP閾惧垱寤烘垚鍔燂紝鏁堟灉鍣ㄦ暟閲? " << chain->get_effect_count() << std::endl;
        
        // 鏄剧ずDSP閾句俊鎭?
        std::string chain_info = dsp_utils::get_dsp_chain_info(*chain);
        std::cout << chain_info << std::endl;
        
        // 楠岃瘉DSP閾?
        auto validation_result = dsp_chain_validator::validate_chain(*chain);
        if(!validation_result.is_valid) {
            std::cout << "鉂?DSP閾鹃獙璇佸け璐? " << validation_result.error_message << std::endl;
            return false;
        }
        
        if(!validation_result.warnings.empty()) {
            std::cout << "鈿狅笍  DSP閾捐鍛?" << std::endl;
            for(const auto& warning : validation_result.warnings) {
                std::cout << "  - " << warning << std::endl;
            }
        }
        
        std::cout << "鉁?DSP閾鹃獙璇侀€氳繃" << std::endl;
        
        // 娴嬭瘯DSP閾惧鐞?
        auto chunk = audio_chunk_utils::create_chunk(2048, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 瀹炰緥鍖栨墍鏈夋晥鏋滃櫒
        for(size_t i = 0; i < chain->get_effect_count(); ++i) {
            dsp* effect = chain->get_effect(i);
            if(effect) {
                effect->instantiate(*chunk, 44100, 2);
            }
        }
        
        float rms_before = audio_chunk_utils::calculate_rms(*chunk);
        
        chain->run_chain(*chunk, *abort);
        
        float rms_after = audio_chunk_utils::calculate_rms(*chunk);
        
        std::cout << "鉁?DSP閾惧鐞嗗畬鎴? << std::endl;
        std::cout << "  澶勭悊鍓峈MS: " << rms_before << std::endl;
        std::cout << "  澶勭悊鍚嶳MS: " << rms_after << std::endl;
        
        return true;
    }
    
    // 娴嬭瘯5锛氬畬鏁撮煶棰戦摼璺?
    bool test_complete_audio_chain() {
        std::cout << "\n5. 瀹屾暣闊抽閾捐矾娴嬭瘯..." << std::endl;
        
        // 鍒涘缓闊抽鏁版嵁
        auto input_chunk = audio_chunk_utils::create_chunk(4096, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 娣诲姞娴嬭瘯闊抽鏁版嵁锛堟寮︽尝锛?
        float* data = input_chunk->get_data();
        if(data) {
            const float frequency = 440.0f; // A4闊崇
            const float amplitude = 0.5f;
            
            for(size_t i = 0; i < input_chunk->get_sample_count(); ++i) {
                float time = static_cast<float>(i) / input_chunk->get_sample_rate();
                float value = amplitude * sin(2.0f * 3.14159f * frequency * time);
                
                // 绔嬩綋澹?
                data[i * 2] = value;      // 宸﹀０閬?
                data[i * 2 + 1] = value;  // 鍙冲０閬?
            }
        }
        
        std::cout << "鉁?杈撳叆闊抽鏁版嵁鍒涘缓瀹屾垚" << std::endl;
        
        // 鍒涘缓DSP閾?
        auto dsp_chain = std::make_unique<dsp_chain>();
        dsp_chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_volume_effect(0.8f)));
        dsp_chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_passthrough_effect("Clean")));
        
        std::cout << "鉁?DSP閾鹃厤缃畬鎴? << std::endl;
        
        // 澶勭悊闊抽鏁版嵁
        float input_rms = audio_chunk_utils::calculate_rms(*input_chunk);
        
        // 瀹炰緥鍖栨墍鏈塂SP鏁堟灉鍣?
        for(size_t i = 0; i < dsp_chain->get_effect_count(); ++i) {
            dsp* effect = dsp_chain->get_effect(i);
            if(effect) {
                effect->instantiate(*input_chunk, 44100, 2);
            }
        }
        
        // 杩愯DSP澶勭悊
        dsp_chain->run_chain(*input_chunk, *abort);
        
        float output_rms = audio_chunk_utils::calculate_rms(*input_chunk);
        
        std::cout << "鉁?闊抽澶勭悊閾捐矾瀹屾垚" << std::endl;
        std::cout << "  杈撳叆RMS: " << input_rms << std::endl;
        std::cout << "  杈撳嚭RMS: " << output_rms << std::endl;
        std::cout << "  澶勭悊澧炵泭: " << (output_rms / input_rms) << std::endl;
        
        return true;
    }
    
    // 娴嬭瘯6锛氭€ц兘鍩哄噯娴嬭瘯
    bool test_performance_benchmark() {
        std::cout << "\n6. 鎬ц兘鍩哄噯娴嬭瘯..." << std::endl;
        
        const size_t test_samples = 44100 * 10; // 10绉掗煶棰?
        const size_t iterations = 100;
        
        // 鍒涘缓娴嬭瘯闊抽鍧?
        auto chunk = audio_chunk_utils::create_chunk(test_samples, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 鍒涘缓DSP閾?
        auto dsp_chain = std::make_unique<dsp_chain>();
        dsp_chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_passthrough_effect()));
        
        // 瀹炰緥鍖栨墍鏈夋晥鏋滃櫒
        for(size_t i = 0; i < dsp_chain->get_effect_count(); ++i) {
            dsp* effect = dsp_chain->get_effect(i);
            if(effect) {
                effect->instantiate(*chunk, 44100, 2);
            }
        }
        
        // 鎬ц兘娴嬭瘯
        auto start_time = high_resolution_clock::now();
        
        for(size_t i = 0; i < iterations; ++i) {
            dsp_chain->run_chain(*chunk, *abort);
        }
        
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end_time - start_time);
        
        double total_seconds = static_cast<double>(duration.count()) / 1000000.0;
        double samples_per_second = (test_samples * iterations) / total_seconds;
        double realtime_factor = samples_per_second / 44100.0;
        
        std::cout << "鉁?鎬ц兘娴嬭瘯瀹屾垚" << std::endl;
        std::cout << "  鎬诲鐞嗘椂闂? " << total_seconds << " 绉? << std::endl;
        std::cout << "  鎬婚噰鏍锋暟: " << (test_samples * iterations) << std::endl;
        std::cout << "  澶勭悊閫熷害: " << samples_per_second << " 閲囨牱/绉? << std::endl;
        std::cout << "  瀹炴椂鍊嶆暟: " << realtime_factor << "x" << std::endl;
        std::cout << "  CPU鍗犵敤浼扮畻: " << dsp_utils::estimate_cpu_usage(*dsp_chain) << "%" << std::endl;
        
        return true;
    }
    
    // 杩愯鎵€鏈夋祴璇?
    bool run_all_tests() {
        std::cout << "=" << std::string(60, '=') << std::endl;
        std::cout << "闃舵1.2锛氬姛鑳芥墿灞曢泦鎴愭祴璇? << std::endl;
        std::cout << "=" << std::string(60, '=') << std::endl;
        
        if(!initialize()) {
            return false;
        }
        
        int passed_tests = 0;
        int total_tests = 6;
        
        // 杩愯鎵€鏈夋祴璇?
        std::vector<std::pair<std::string, std::function<bool()>>> tests = {
            {"闊抽鍧楀熀鏈姛鑳?, [this]() { return test_audio_chunk_basic(); }},
            {"DSP棰勮鍔熻兘", [this]() { return test_dsp_preset(); }},
            {"DSP鏁堟灉鍣ㄥ姛鑳?, [this]() { return test_dsp_effects(); }},
            {"DSP閾惧姛鑳?, [this]() { return test_dsp_chain(); }},
            {"瀹屾暣闊抽閾捐矾", [this]() { return test_complete_audio_chain(); }},
            {"鎬ц兘鍩哄噯娴嬭瘯", [this]() { return test_performance_benchmark(); }}
        };
        
        for(size_t i = 0; i < tests.size(); ++i) {
            std::cout << "\n[" << (i+1) << "/" << tests.size() << "] " << tests[i].first << std::endl;
            
            try {
                if(tests[i].second()) {
                    passed_tests++;
                    std::cout << "鉁?" << tests[i].first << " - 閫氳繃" << std::endl;
                } else {
                    std::cout << "鉂?" << tests[i].first << " - 澶辫触" << std::endl;
                }
            } catch(const std::exception& e) {
                std::cout << "鉂?" << tests[i].first << " - 寮傚父: " << e.what() << std::endl;
            }
        }
        
        shutdown();
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "娴嬭瘯缁撴灉: " << passed_tests << "/" << total_tests << " 閫氳繃" << std::endl;
        
        if(passed_tests == total_tests) {
            std::cout << "馃帀 鎵€鏈夋祴璇曢€氳繃锛侀樁娈?.2鍔熻兘鎵╁睍瀹屾垚銆? << std::endl;
            std::cout << "\n鏍稿績鎴愬氨:" << std::endl;
            std::cout << "  鉁?闊抽鍧楃郴缁熷畬鏁村疄鐜? << std::endl;
            std::cout << "  鉁?DSP棰勮鍜岄厤缃郴缁? << std::endl;
            std::cout << "  鉁?DSP鏁堟灉鍣ㄦ鏋? << std::endl;
            std::cout << "  鉁?DSP閾剧鐞嗗櫒" << std::endl;
            std::cout << "  鉁?瀹屾暣闊抽澶勭悊閾捐矾" << std::endl;
            std::cout << "  鉁?鎬ц兘鍩哄噯楠岃瘉" << std::endl;
            std::cout << "\n涓嬩竴姝ワ細闃舵1.3 - 楂樼骇鍔熻兘鍜屼紭鍖? << std::endl;
            return true;
        } else {
            std::cout << "鈿狅笍  閮ㄥ垎娴嬭瘯澶辫触锛岄渶瑕佽皟璇? << std::endl;
            return false;
        }
    }
};

// 涓绘祴璇曞嚱鏁?
int main() {
    try {
        IntegrationTest test;
        return test.run_all_tests() ? 0 : 1;
    } catch(const std::exception& e) {
        std::cerr << "娴嬭瘯寮傚父: " << e.what() << std::endl;
        return 1;
    }
}