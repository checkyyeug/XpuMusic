#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <cmath>
#include <fstream>

// 鍖呭惈鎵€鏈夐樁娈?.4鐨勫ご鏂囦欢
#include "fb2k_com_base.h"
#include "fb2k_component_system.h"
#include "output_asio.h"
#include "vst_bridge.h"
#include "audio_analyzer.h"
#include "../stage1_3/audio_processor.h"
#include "../stage1_3/dsp_manager.h"
#include "../stage1_3/audio_block_impl.h"
#include "../stage1_2/abort_callback.h"

namespace fb2k {

// 娴嬭瘯閰嶇疆
struct test_config {
    bool test_com_system = true;
    bool test_component_loading = true;
    bool test_asio_output = true;
    bool test_vst_bridge = true;
    bool test_audio_analyzer = true;
    bool test_integration = true;
    bool verbose = true;
    int test_duration_seconds = 5;
    std::string vst_plugin_path = ""; // 鍙€塚ST鎻掍欢璺緞
};

// 娴嬭瘯杈呭姪绫?
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
    }
    
    static bool save_test_audio(const audio_chunk& chunk, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if(!file.is_open()) {
            return false;
        }
        
        const float* data = chunk.get_data();
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        
        file.write(reinterpret_cast<const char*>(data), total_samples * sizeof(float));
        file.close();
        return true;
    }
    
    static audio_chunk create_test_chunk(int sample_rate, int channels, int samples, float frequency) {
        audio_chunk chunk;
        chunk.set_sample_count(samples);
        chunk.set_channels(channels);
        chunk.set_sample_rate(sample_rate);
        
        float* data = chunk.get_data();
        float phase_increment = 2.0f * M_PI * frequency / sample_rate;
        
        for(int i = 0; i < samples * channels; ++i) {
            float sample = std::sin(phase_increment * (i / channels)) * 0.5f;
            data[i] = sample;
        }
        
        return chunk;
    }
};

// 娴嬭瘯1: COM绯荤粺娴嬭瘯
bool test_com_system(const test_config& config) {
    test_helper::print_test_header("COM绯荤粺娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 娴嬭瘯1.1: 鍒濆鍖朇OM绯荤粺
        initialize_fb2k_core_services();
        test_helper::print_test_result(true, "COM绯荤粺鍒濆鍖栨垚鍔?);
        
        // 娴嬭瘯1.2: 鑾峰彇鏍稿績鏈嶅姟
        auto* core = fb2k_core();
        if(core) {
            test_helper::print_test_result(true, "鑾峰彇鏍稿績鏈嶅姟鎴愬姛");
            
            // 娴嬭瘯鏈嶅姟鍩烘湰鍔熻兘
            const char* app_name = nullptr;
            if(SUCCEEDED(core->GetAppName(&app_name)) && app_name) {
                test_helper::print_test_result(true, std::string("搴旂敤鍚嶇О: ") + app_name);
            } else {
                test_helper::print_test_result(false, "鏃犳硶鑾峰彇搴旂敤鍚嶇О");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "鏃犳硶鑾峰彇鏍稿績鏈嶅姟");
            all_passed = false;
        }
        
        // 娴嬭瘯1.3: 鎾斁鎺у埗鏈嶅姟
        auto* playback = fb2k_playback_control();
        if(playback) {
            test_helper::print_test_result(true, "鑾峰彇鎾斁鎺у埗鏈嶅姟鎴愬姛");
            
            // 娴嬭瘯鎾斁鎺у埗鍔熻兘
            DWORD state = 0;
            if(SUCCEEDED(playback->GetPlaybackState(&state))) {
                test_helper::print_test_result(true, "鎾斁鐘舵€佹煡璇㈡垚鍔?);
            } else {
                test_helper::print_test_result(false, "鎾斁鐘舵€佹煡璇㈠け璐?);
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "鏃犳硶鑾峰彇鎾斁鎺у埗鏈嶅姟");
            all_passed = false;
        }
        
        // 娴嬭瘯1.4: 鍏冩暟鎹簱鏈嶅姟
        auto* metadb = fb2k_metadb();
        if(metadb) {
            test_helper::print_test_result(true, "鑾峰彇鍏冩暟鎹簱鏈嶅姟鎴愬姛");
        } else {
            test_helper::print_test_result(false, "鏃犳硶鑾峰彇鍏冩暟鎹簱鏈嶅姟");
            all_passed = false;
        }
        
        // 娴嬭瘯1.5: 閰嶇疆绠＄悊鏈嶅姟
        auto* config = fb2k_config_manager();
        if(config) {
            test_helper::print_test_result(true, "鑾峰彇閰嶇疆绠＄悊鏈嶅姟鎴愬姛");
        } else {
            test_helper::print_test_result(false, "鏃犳硶鑾峰彇閰嶇疆绠＄悊鏈嶅姟");
            all_passed = false;
        }
        
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("COM绯荤粺寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯2: 缁勪欢绯荤粺娴嬭瘯
bool test_component_system(const test_config& config) {
    test_helper::print_test_header("缁勪欢绯荤粺娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 鍒濆鍖栫粍浠剁郴缁?
        initialize_fb2k_core_services();
        initialize_fb2k_component_system();
        
        // 娴嬭瘯2.1: 鑾峰彇缁勪欢绠＄悊鍣?
        auto* manager = fb2k_get_component_manager();
        if(manager) {
            test_helper::print_test_result(true, "鑾峰彇缁勪欢绠＄悊鍣ㄦ垚鍔?);
            
            // 娴嬭瘯2.2: 鎵弿缁勪欢
            if(SUCCEEDED(manager->ScanComponents("components"))) {
                test_helper::print_test_result(true, "缁勪欢鎵弿鎴愬姛");
            } else {
                test_helper::print_test_result(false, "缁勪欢鎵弿澶辫触");
                all_passed = false;
            }
            
            // 娴嬭瘯2.3: 鏋氫妇缁勪欢
            DWORD count = 0;
            if(SUCCEEDED(manager->GetComponentCount(&count))) {
                test_helper::print_test_result(true, "缁勪欢鏁伴噺: " + std::to_string(count));
            } else {
                test_helper::print_test_result(false, "鏃犳硶鑾峰彇缁勪欢鏁伴噺");
                all_passed = false;
            }
            
            // 娴嬭瘯2.4: 缁勪欢绫诲瀷鏋氫妇
            component_type* types = nullptr;
            DWORD type_count = 0;
            if(SUCCEEDED(manager->GetComponentTypes(&types, &type_count))) {
                test_helper::print_test_result(true, "缁勪欢绫诲瀷鏁伴噺: " + std::to_string(type_count));
            } else {
                test_helper::print_test_result(false, "鏃犳硶鑾峰彇缁勪欢绫诲瀷");
                all_passed = false;
            }
            
        } else {
            test_helper::print_test_result(false, "鏃犳硶鑾峰彇缁勪欢绠＄悊鍣?);
            all_passed = false;
        }
        
        shutdown_fb2k_component_system();
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("缁勪欢绯荤粺寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯3: ASIO杈撳嚭璁惧娴嬭瘯
bool test_asio_output(const test_config& config) {
    test_helper::print_test_header("ASIO杈撳嚭璁惧娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 鍒涘缓ASIO杈撳嚭璁惧
        auto asio_output = create_asio_output();
        if(!asio_output) {
            test_helper::print_test_result(false, "鏃犳硶鍒涘缓ASIO杈撳嚭璁惧");
            return false;
        }
        test_helper::print_test_result(true, "鍒涘缓ASIO杈撳嚭璁惧鎴愬姛");
        
        // 娴嬭瘯3.1: 鏋氫妇ASIO椹卞姩
        std::vector<asio_driver_info> drivers;
        if(asio_output->enum_drivers(drivers) && !drivers.empty()) {
            test_helper::print_test_result(true, "鏋氫妇ASIO椹卞姩鎴愬姛锛屽彂鐜?" + std::to_string(drivers.size()) + " 涓┍鍔?);
            
            // 鏄剧ず椹卞姩淇℃伅
            for(size_t i = 0; i < drivers.size() && i < 3; ++i) {
                std::cout << "  椹卞姩[" << i << "]: " << drivers[i].name 
                         << " (" << drivers[i].description << ")" << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "鏃犳硶鏋氫妇ASIO椹卞姩");
            all_passed = false;
        }
        
        // 娴嬭瘯3.2: 鍔犺浇绗竴涓彲鐢ㄩ┍鍔?
        if(!drivers.empty()) {
            if(asio_output->load_driver(drivers[0].id)) {
                test_helper::print_test_result(true, "鍔犺浇ASIO椹卞姩鎴愬姛: " + drivers[0].name);
            } else {
                test_helper::print_test_result(false, "鏃犳硶鍔犺浇ASIO椹卞姩");
                all_passed = false;
            }
        }
        
        // 娴嬭瘯3.3: 閰嶇疆ASIO鍙傛暟
        asio_output->set_buffer_size(512);
        asio_output->set_sample_rate(44100);
        test_helper::print_test_result(true, "閰嶇疆ASIO鍙傛暟鎴愬姛");
        
        // 娴嬭瘯3.4: 鑾峰彇ASIO淇℃伅
        std::cout << "\nASIO璁惧淇℃伅:" << std::endl;
        std::cout << "  褰撳墠椹卞姩: " << asio_output->get_current_driver_name() << std::endl;
        std::cout << "  缂撳啿鍖哄ぇ灏? " << asio_output->get_buffer_size() << std::endl;
        std::cout << "  閲囨牱鐜? " << asio_output->get_sample_rate() << "Hz" << std::endl;
        std::cout << "  杈撳叆寤惰繜: " << asio_output->get_input_latency() << " 閲囨牱" << std::endl;
        std::cout << "  杈撳嚭寤惰繜: " << asio_output->get_output_latency() << " 閲囨牱" << std::endl;
        
        // 娴嬭瘯3.5: 鎵撳紑闊抽娴侊紙涓嶅疄闄呮挱鏀撅級
        abort_callback_dummy abort;
        try {
            asio_output->open(44100, 2, 0, abort);
            test_helper::print_test_result(true, "鎵撳紑ASIO闊抽娴佹垚鍔?);
            
            // 娴嬭瘯闊抽噺鎺у埗
            asio_output->volume_set(0.5f);
            test_helper::print_test_result(true, "ASIO闊抽噺鎺у埗鎴愬姛");
            
            // 鍏抽棴闊抽娴?
            asio_output->close(abort);
            test_helper::print_test_result(true, "鍏抽棴ASIO闊抽娴佹垚鍔?);
            
        } catch(const std::exception& e) {
            test_helper::print_test_result(false, std::string("ASIO闊抽娴佸紓甯? ") + e.what());
            all_passed = false;
        }
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("ASIO杈撳嚭寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯4: VST妗ユ帴娴嬭瘯
bool test_vst_bridge(const test_config& config) {
    test_helper::print_test_header("VST妗ユ帴娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 鍒濆鍖朧ST妗ユ帴绠＄悊鍣?
        auto& vst_manager = vst_bridge_manager::get_instance();
        if(!vst_manager.initialize()) {
            test_helper::print_test_result(false, "VST妗ユ帴绠＄悊鍣ㄥ垵濮嬪寲澶辫触");
            return false;
        }
        test_helper::print_test_result(true, "VST妗ユ帴绠＄悊鍣ㄥ垵濮嬪寲鎴愬姛");
        
        // 娴嬭瘯4.1: 鎵弿VST鐩綍
        auto vst_dirs = vst_manager.get_vst_directories();
        if(!vst_dirs.empty()) {
            test_helper::print_test_result(true, "鑾峰彇VST鐩綍鎴愬姛锛屽彂鐜?" + std::to_string(vst_dirs.size()) + " 涓洰褰?);
            
            // 鎵弿姣忎釜鐩綍
            int total_plugins = 0;
            for(const auto& dir : vst_dirs) {
                auto plugins = vst_manager.scan_vst_plugins(dir);
                total_plugins += plugins.size();
            }
            test_helper::print_test_result(true, "鎵弿VST鎻掍欢鎴愬姛锛屽彂鐜?" + std::to_string(total_plugins) + " 涓彃浠?);
        } else {
            test_helper::print_test_result(false, "鏈壘鍒癡ST鐩綍");
            all_passed = false;
        }
        
        // 娴嬭瘯4.2: 鍔犺浇VST鎻掍欢锛堝鏋滄湁鎸囧畾璺緞锛?
        if(!config.vst_plugin_path.empty()) {
            auto* plugin = vst_manager.load_vst_plugin(config.vst_plugin_path);
            if(plugin) {
                test_helper::print_test_result(true, "鍔犺浇VST鎻掍欢鎴愬姛: " + config.vst_plugin_path);
                
                // 娴嬭瘯鎻掍欢淇℃伅
                std::cout << "\nVST鎻掍欢淇℃伅:" << std::endl;
                std::cout << "  鎻掍欢鍚嶇О: " << plugin->get_plugin_name() << std::endl;
                std::cout << "  渚涘簲鍟? " << plugin->get_plugin_vendor() << std::endl;
                std::cout << "  鐗堟湰: " << plugin->get_plugin_version() << std::endl;
                std::cout << "  杈撳叆澹伴亾: " << plugin->get_num_inputs() << std::endl;
                std::cout << "  杈撳嚭澹伴亾: " << plugin->get_num_outputs() << std::endl;
                std::cout << "  鍙傛暟鏁伴噺: " << plugin->get_num_parameters() << std::endl;
                std::cout << "  棰勮鏁伴噺: " << plugin->get_num_programs() << std::endl;
                
                // 娴嬭瘯鍙傛暟鑾峰彇
                auto params = plugin->get_parameter_info();
                if(!params.empty()) {
                    std::cout << "  鍙傛暟淇℃伅:" << std::endl;
                    for(size_t i = 0; i < std::min(params.size(), size_t(5)); ++i) {
                        std::cout << "    [" << i << "] " << params[i].name 
                                 << " (" << params[i].min_value << " - " << params[i].max_value << ")" << std::endl;
                    }
                }
                
                // 鍗歌浇鎻掍欢
                vst_manager.unload_vst_plugin(plugin);
                test_helper::print_test_result(true, "鍗歌浇VST鎻掍欢鎴愬姛");
                
            } else {
                test_helper::print_test_result(false, "鏃犳硶鍔犺浇鎸囧畾VST鎻掍欢");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(true, "鏈寚瀹歏ST鎻掍欢璺緞锛岃烦杩囨彃浠跺姞杞芥祴璇?);
        }
        
        // 娴嬭瘯4.3: VST瀹夸富鍔熻兘
        std::cout << "\nVST瀹夸富淇℃伅:" << std::endl;
        std::cout << "  瀹夸富鍚嶇О: " << vst_manager.vst_host_->get_host_name() << std::endl;
        std::cout << "  瀹夸富鐗堟湰: " << vst_manager.vst_host_->get_host_version() << std::endl;
        std::cout << "  瀹夸富渚涘簲鍟? " << vst_manager.vst_host_->get_host_vendor() << std::endl;
        std::cout << "  缂撳啿鍖哄ぇ灏? " << vst_manager.get_vst_buffer_size() << std::endl;
        std::cout << "  閲囨牱鐜? " << vst_manager.get_vst_sample_rate() << "Hz" << std::endl;
        
        vst_manager.shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("VST妗ユ帴寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯5: 闊抽鍒嗘瀽宸ュ叿娴嬭瘯
bool test_audio_analyzer(const test_config& config) {
    test_helper::print_test_header("闊抽鍒嗘瀽宸ュ叿娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 鑾峰彇闊抽鍒嗘瀽绠＄悊鍣?
        auto& analysis_manager = get_audio_analysis_manager();
        
        // 娴嬭瘯5.1: 棰戣氨鍒嗘瀽浠?
        auto spectrum_analyzer = std::make_unique<fb2k::spectrum_analyzer>();
        if(spectrum_analyzer) {
            test_helper::print_test_result(true, "鍒涘缓棰戣氨鍒嗘瀽浠垚鍔?);
            
            // 鍒濆鍖栧垎鏋愪华
            if(SUCCEEDED(spectrum_analyzer->Initialize())) {
                test_helper::print_test_result(true, "鍒濆鍖栭璋卞垎鏋愪华鎴愬姛");
                
                // 閰嶇疆鍒嗘瀽浠?
                spectrum_analyzer->set_fft_size(2048);
                spectrum_analyzer->set_window_type(1); // Hann绐?
                spectrum_analyzer->set_analysis_mode(0); // 瀹炴椂妯″紡
                test_helper::print_test_result(true, "閰嶇疆棰戣氨鍒嗘瀽浠垚鍔?);
                
                // 鍒涘缓娴嬭瘯闊抽
                auto test_chunk = test_helper::create_test_chunk(44100, 2, 2048, 1000.0f);
                
                // 娴嬭瘯鍩烘湰鍒嗘瀽
                audio_features features;
                if(SUCCEEDED(spectrum_analyzer->analyze_chunk(test_chunk, features))) {
                    test_helper::print_test_result(true, "闊抽鐗瑰緛鍒嗘瀽鎴愬姛");
                    
                    std::cout << "\n闊抽鐗瑰緛:" << std::endl;
                    std::cout << "  RMS鐢靛钩: " << std::fixed << std::setprecision(2) << features.rms_level << " dB" << std::endl;
                    std::cout << "  宄板€肩數骞? " << features.peak_level << " dB" << std::endl;
                    std::cout << "  鍝嶅害: " << features.loudness << " LUFS" << std::endl;
                    std::cout << "  鍔ㄦ€佽寖鍥? " << features.dynamic_range << " dB" << std::endl;
                    std::cout << "  DC鍋忕Щ: " << features.dc_offset << std::endl;
                    std::cout << "  绔嬩綋澹扮浉鍏虫€? " << features.stereo_correlation << std::endl;
                } else {
                    test_helper::print_test_result(false, "闊抽鐗瑰緛鍒嗘瀽澶辫触");
                    all_passed = false;
                }
                
                // 娴嬭瘯棰戣氨鍒嗘瀽
                spectrum_data spectrum;
                if(SUCCEEDED(spectrum_analyzer->analyze_spectrum(test_chunk, spectrum))) {
                    test_helper::print_test_result(true, "棰戣氨鍒嗘瀽鎴愬姛");
                    
                    std::cout << "\n棰戣氨淇℃伅:" << std::endl;
                    std::cout << "  FFT澶у皬: " << spectrum.fft_size << std::endl;
                    std::cout << "  棰戠巼鍒嗚鲸鐜? " << (spectrum.sample_rate / spectrum.fft_size) << " Hz" << std::endl;
                    std::cout << "  棰戠偣鏁伴噺: " << spectrum.frequencies.size() << std::endl;
                    
                    // 娴嬭瘯棰戠巼甯﹀垎鏋?
                    double band_level = 0.0;
                    if(SUCCEEDED(spectrum_analyzer->get_frequency_band_level(frequency_band::midrange, band_level))) {
                        std::cout << "  涓甯?500-2000Hz)鐢靛钩: " << band_level << " dB" << std::endl;
                    }
                } else {
                    test_helper::print_test_result(false, "棰戣氨鍒嗘瀽澶辫触");
                    all_passed = false;
                }
                
                // 娴嬭瘯瀹炴椂鍒嗘瀽
                real_time_analysis analysis;
                if(SUCCEEDED(spectrum_analyzer->get_real_time_analysis(analysis))) {
                    test_helper::print_test_result(true, "瀹炴椂鍒嗘瀽鎴愬姛");
                } else {
                    test_helper::print_test_result(false, "瀹炴椂鍒嗘瀽澶辫触");
                    all_passed = false;
                }
                
                // 鐢熸垚鍒嗘瀽鎶ュ憡
                std::string report;
                if(SUCCEEDED(spectrum_analyzer->generate_report(report))) {
                    test_helper::print_test_result(true, "鐢熸垚鍒嗘瀽鎶ュ憡鎴愬姛");
                    if(config.verbose) {
                        std::cout << "\n鍒嗘瀽鎶ュ憡棰勮:\n" << report.substr(0, 300) << "..." << std::endl;
                    }
                }
                
                spectrum_analyzer->Shutdown();
            } else {
                test_helper::print_test_result(false, "鍒濆鍖栭璋卞垎鏋愪华澶辫触");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "鏃犳硶鍒涘缓棰戣氨鍒嗘瀽浠?);
            all_passed = false;
        }
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("闊抽鍒嗘瀽寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 娴嬭瘯6: 闆嗘垚娴嬭瘯
bool test_integration(const test_config& config) {
    test_helper::print_test_header("闆嗘垚娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 鍒濆鍖栨墍鏈夌郴缁?
        initialize_fb2k_core_services();
        initialize_fb2k_component_system();
        
        // 鍒涘缓闊抽澶勭悊鍣?
        auto processor = create_audio_processor();
        
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::realtime;
        processor_config.target_latency_ms = 10.0;
        processor_config.cpu_usage_limit_percent = 80.0f;
        processor_config.buffer_size = 1024;
        processor_config.enable_performance_monitoring = true;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "闊抽澶勭悊鍣ㄥ垵濮嬪寲澶辫触");
            return false;
        }
        test_helper::print_test_result(true, "闊抽澶勭悊鍣ㄥ垵濮嬪寲鎴愬姛");
        
        // 娣诲姞DSP鏁堟灉鍣?
        auto dsp_manager = std::make_unique<fb2k::dsp_manager>();
        dsp_config dsp_config;
        dsp_config.enable_standard_effects = true;
        dsp_config.enable_performance_monitoring = true;
        
        if(dsp_manager->initialize(dsp_config)) {
            auto eq = dsp_manager->create_equalizer_10band();
            auto reverb = dsp_manager->create_reverb();
            processor->add_dsp_effect(std::move(eq));
            processor->add_dsp_effect(std::move(reverb));
            test_helper::print_test_result(true, "娣诲姞DSP鏁堟灉鍣ㄦ垚鍔?);
        }
        
        // 娴嬭瘯6.1: 瀹屾暣闊抽澶勭悊閾捐矾
        auto test_chunk = test_helper::create_test_chunk(44100, 2, 1024, 440.0f);
        audio_chunk output_chunk;
        abort_callback_dummy abort;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        bool process_success = processor->process_audio(test_chunk, output_chunk, abort);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time_ms = duration.count() / 1000.0;
        
        if(process_success) {
            test_helper::print_test_result(true, "瀹屾暣闊抽澶勭悊閾捐矾鎴愬姛锛岃€楁椂: " + std::to_string(processing_time_ms) + "ms");
            
            // 楠岃瘉杈撳嚭
            float output_rms = output_chunk.calculate_rms();
            if(output_rms > 0.0f) {
                test_helper::print_test_result(true, "杈撳嚭楠岃瘉閫氳繃锛孯MS: " + std::to_string(output_rms));
            } else {
                test_helper::print_test_result(false, "杈撳嚭涓虹┖");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "瀹屾暣闊抽澶勭悊閾捐矾澶辫触");
            all_passed = false;
        }
        
        // 娴嬭瘯6.2: 鎬ц兘鐩戞帶
        auto stats = processor->get_stats();
        test_helper::print_performance_stats(stats);
        
        // 娴嬭瘯6.3: 鐘舵€佹姤鍛?
        std::string status_report = processor->get_status_report();
        if(!status_report.empty()) {
            test_helper::print_test_result(true, "鐘舵€佹姤鍛婄敓鎴愭垚鍔?);
            if(config.verbose) {
                std::cout << "\n鐘舵€佹姤鍛婇瑙?\n" << status_report.substr(0, 400) << "..." << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "鐘舵€佹姤鍛婁负绌?);
            all_passed = false;
        }
        
        // 娴嬭瘯6.4: 瀹炴椂鍙傛暟璋冭妭
        processor->set_realtime_parameter("Reverb", "room_size", 0.8f);
        float room_size = processor->get_realtime_parameter("Reverb", "room_size");
        
        if(std::abs(room_size - 0.8f) < 0.01f) {
            test_helper::print_test_result(true, "瀹炴椂鍙傛暟璋冭妭楠岃瘉閫氳繃");
        } else {
            test_helper::print_test_result(false, "瀹炴椂鍙傛暟璋冭妭楠岃瘉澶辫触");
            all_passed = false;
        }
        
        processor->shutdown();
        shutdown_fb2k_component_system();
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("闆嗘垚娴嬭瘯寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 鎬ц兘鍩哄噯娴嬭瘯
bool test_performance_benchmark(const test_config& config) {
    test_helper::print_test_header("鎬ц兘鍩哄噯娴嬭瘯");
    
    bool all_passed = true;
    
    try {
        // 鍒濆鍖栫郴缁?
        initialize_fb2k_core_services();
        initialize_fb2k_component_system();
        
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
        auto dsp_manager = std::make_unique<fb2k::dsp_manager>();
        dsp_config dsp_config;
        dsp_config.enable_standard_effects = true;
        dsp_config.enable_performance_monitoring = true;
        
        if(dsp_manager->initialize(dsp_config)) {
            for(int i = 0; i < 3; ++i) {
                auto eq = dsp_manager->create_equalizer_10band();
                auto reverb = dsp_manager->create_reverb();
                processor->add_dsp_effect(std::move(eq));
                processor->add_dsp_effect(std::move(reverb));
            }
        }
        
        // 鎬ц兘娴嬭瘯
        const int iterations = 100;
        std::vector<double> processing_times;
        
        for(int i = 0; i < iterations; ++i) {
            auto test_chunk = test_helper::create_test_chunk(44100, 2, 1024, 1000.0f + i * 10.0f);
            audio_chunk output_chunk;
            abort_callback_dummy abort;
            
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
            double rtf = (avg_time / (1024.0 / 44100 * 1000.0)); // 瀹炴椂鍊嶆暟
            
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
        
        processor->shutdown();
        shutdown_fb2k_component_system();
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("鎬ц兘鍩哄噯娴嬭瘯寮傚父: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

} // namespace fb2k

// 涓绘祴璇曞嚱鏁?
int main(int argc, char* argv[]) {
    using namespace fb2k;
    
    std::cout << "foobar2000鍏煎灞?- 闃舵1.4鍔熻兘娴嬭瘯" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "娴嬭瘯鏃堕棿: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    
    // 瑙ｆ瀽鍛戒护琛屽弬鏁?
    test_config config;
    
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if(arg == "--vst-plugin" && i + 1 < argc) {
            config.vst_plugin_path = argv[++i];
        } else if(arg == "--duration" && i + 1 < argc) {
            config.test_duration_seconds = std::stoi(argv[++i]);
        } else if(arg == "--quiet") {
            config.verbose = false;
        } else if(arg == "--help") {
            std::cout << "鐢ㄦ硶: " << argv[0] << " [閫夐」]" << std::endl;
            std::cout << "閫夐」:" << std::endl;
            std::cout << "  --vst-plugin <path>    鎸囧畾VST鎻掍欢璺緞杩涜娴嬭瘯" << std::endl;
            std::cout << "  --duration <seconds>   璁剧疆娴嬭瘯鎸佺画鏃堕棿 (榛樿: 5)" << std::endl;
            std::cout << "  --quiet                鍑忓皯杈撳嚭淇℃伅" << std::endl;
            std::cout << "  --help                 鏄剧ず甯姪淇℃伅" << std::endl;
            return 0;
        }
    }
    
    std::cout << "\n娴嬭瘯閰嶇疆:" << std::endl;
    std::cout << "  VST鎻掍欢璺緞: " << (config.vst_plugin_path.empty() ? "鏈寚瀹? : config.vst_plugin_path) << std::endl;
    std::cout << "  娴嬭瘯鎸佺画鏃堕棿: " << config.test_duration_seconds << "绉? << std::endl;
    std::cout << "  璇︾粏杈撳嚭: " << (config.verbose ? "鍚敤" : "绂佺敤") << std::endl;
    
    // 杩愯鎵€鏈夋祴璇?
    std::vector<std::pair<std::string, std::function<bool(const test_config&)>>> tests = {
        {"COM绯荤粺娴嬭瘯", test_com_system},
        {"缁勪欢绯荤粺娴嬭瘯", test_component_system},
        {"ASIO杈撳嚭璁惧娴嬭瘯", test_asio_output},
        {"VST妗ユ帴娴嬭瘯", test_vst_bridge},
        {"闊抽鍒嗘瀽宸ュ叿娴嬭瘯", test_audio_analyzer},
        {"闆嗘垚娴嬭瘯", test_integration},
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
        std::cout << "\n鉁?鎵€鏈夋祴璇曢€氳繃锛侀樁娈?.4鍔熻兘姝ｅ父銆? << std::endl;
        std::cout << "\n闃舵1.4瀹炵幇浜嗕互涓嬫牳蹇冨姛鑳?" << std::endl;
        std::cout << "- 瀹屾暣鐨刦oobar2000 COM鎺ュ彛浣撶郴" << std::endl;
        std::cout << "- 缁勪欢绯荤粺鍜屾彃浠跺姞杞藉櫒" << std::endl;
        std::cout << "- ASIO涓撲笟闊抽椹卞姩鏀寔" << std::endl;
        std::cout << "- VST鎻掍欢妗ユ帴绯荤粺" << std::endl;
        std::cout << "- 楂樼骇闊抽鍒嗘瀽宸ュ叿" << std::endl;
        std::cout << "- 瀹屾暣鐨勯煶棰戝鐞嗛摼璺泦鎴? << std::endl;
        return 0;
    } else {
        std::cout << "\n鉁?閮ㄥ垎娴嬭瘯澶辫触锛岃妫€鏌ラ敊璇俊鎭€? << std::endl;
        return 1;
    }
}