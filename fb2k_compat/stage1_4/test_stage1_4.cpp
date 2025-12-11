#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <cmath>
#include <fstream>

// 包含所有阶段1.4的头文件
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

// 测试配置
struct test_config {
    bool test_com_system = true;
    bool test_component_loading = true;
    bool test_asio_output = true;
    bool test_vst_bridge = true;
    bool test_audio_analyzer = true;
    bool test_integration = true;
    bool verbose = true;
    int test_duration_seconds = 5;
    std::string vst_plugin_path = ""; // 可选VST插件路径
};

// 测试辅助类
class test_helper {
public:
    static void print_test_header(const std::string& test_name) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "测试: " << test_name << std::endl;
        std::cout << std::string(60, '-') << std::endl;
    }
    
    static void print_test_result(bool success, const std::string& message) {
        std::cout << "结果: " << (success ? "✓ 通过" : "✗ 失败") << " - " << message << std::endl;
    }
    
    static void print_performance_stats(const audio_processor_stats& stats) {
        std::cout << "\n性能统计:" << std::endl;
        std::cout << "  总采样数: " << stats.total_samples_processed << std::endl;
        std::cout << "  总处理时间: " << std::fixed << std::setprecision(3) 
                  << stats.total_processing_time_ms << "ms" << std::endl;
        std::cout << "  平均处理时间: " << stats.average_processing_time_ms << "ms" << std::endl;
        std::cout << "  当前CPU占用: " << std::fixed << std::setprecision(1) 
                  << stats.current_cpu_usage << "%" << std::endl;
        std::cout << "  峰值CPU占用: " << stats.peak_cpu_usage << "%" << std::endl;
        std::cout << "  延迟: " << std::fixed << std::setprecision(2) 
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

// 测试1: COM系统测试
bool test_com_system(const test_config& config) {
    test_helper::print_test_header("COM系统测试");
    
    bool all_passed = true;
    
    try {
        // 测试1.1: 初始化COM系统
        initialize_fb2k_core_services();
        test_helper::print_test_result(true, "COM系统初始化成功");
        
        // 测试1.2: 获取核心服务
        auto* core = fb2k_core();
        if(core) {
            test_helper::print_test_result(true, "获取核心服务成功");
            
            // 测试服务基本功能
            const char* app_name = nullptr;
            if(SUCCEEDED(core->GetAppName(&app_name)) && app_name) {
                test_helper::print_test_result(true, std::string("应用名称: ") + app_name);
            } else {
                test_helper::print_test_result(false, "无法获取应用名称");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "无法获取核心服务");
            all_passed = false;
        }
        
        // 测试1.3: 播放控制服务
        auto* playback = fb2k_playback_control();
        if(playback) {
            test_helper::print_test_result(true, "获取播放控制服务成功");
            
            // 测试播放控制功能
            DWORD state = 0;
            if(SUCCEEDED(playback->GetPlaybackState(&state))) {
                test_helper::print_test_result(true, "播放状态查询成功");
            } else {
                test_helper::print_test_result(false, "播放状态查询失败");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "无法获取播放控制服务");
            all_passed = false;
        }
        
        // 测试1.4: 元数据库服务
        auto* metadb = fb2k_metadb();
        if(metadb) {
            test_helper::print_test_result(true, "获取元数据库服务成功");
        } else {
            test_helper::print_test_result(false, "无法获取元数据库服务");
            all_passed = false;
        }
        
        // 测试1.5: 配置管理服务
        auto* config = fb2k_config_manager();
        if(config) {
            test_helper::print_test_result(true, "获取配置管理服务成功");
        } else {
            test_helper::print_test_result(false, "无法获取配置管理服务");
            all_passed = false;
        }
        
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("COM系统异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试2: 组件系统测试
bool test_component_system(const test_config& config) {
    test_helper::print_test_header("组件系统测试");
    
    bool all_passed = true;
    
    try {
        // 初始化组件系统
        initialize_fb2k_core_services();
        initialize_fb2k_component_system();
        
        // 测试2.1: 获取组件管理器
        auto* manager = fb2k_get_component_manager();
        if(manager) {
            test_helper::print_test_result(true, "获取组件管理器成功");
            
            // 测试2.2: 扫描组件
            if(SUCCEEDED(manager->ScanComponents("components"))) {
                test_helper::print_test_result(true, "组件扫描成功");
            } else {
                test_helper::print_test_result(false, "组件扫描失败");
                all_passed = false;
            }
            
            // 测试2.3: 枚举组件
            DWORD count = 0;
            if(SUCCEEDED(manager->GetComponentCount(&count))) {
                test_helper::print_test_result(true, "组件数量: " + std::to_string(count));
            } else {
                test_helper::print_test_result(false, "无法获取组件数量");
                all_passed = false;
            }
            
            // 测试2.4: 组件类型枚举
            component_type* types = nullptr;
            DWORD type_count = 0;
            if(SUCCEEDED(manager->GetComponentTypes(&types, &type_count))) {
                test_helper::print_test_result(true, "组件类型数量: " + std::to_string(type_count));
            } else {
                test_helper::print_test_result(false, "无法获取组件类型");
                all_passed = false;
            }
            
        } else {
            test_helper::print_test_result(false, "无法获取组件管理器");
            all_passed = false;
        }
        
        shutdown_fb2k_component_system();
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("组件系统异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试3: ASIO输出设备测试
bool test_asio_output(const test_config& config) {
    test_helper::print_test_header("ASIO输出设备测试");
    
    bool all_passed = true;
    
    try {
        // 创建ASIO输出设备
        auto asio_output = create_asio_output();
        if(!asio_output) {
            test_helper::print_test_result(false, "无法创建ASIO输出设备");
            return false;
        }
        test_helper::print_test_result(true, "创建ASIO输出设备成功");
        
        // 测试3.1: 枚举ASIO驱动
        std::vector<asio_driver_info> drivers;
        if(asio_output->enum_drivers(drivers) && !drivers.empty()) {
            test_helper::print_test_result(true, "枚举ASIO驱动成功，发现 " + std::to_string(drivers.size()) + " 个驱动");
            
            // 显示驱动信息
            for(size_t i = 0; i < drivers.size() && i < 3; ++i) {
                std::cout << "  驱动[" << i << "]: " << drivers[i].name 
                         << " (" << drivers[i].description << ")" << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "无法枚举ASIO驱动");
            all_passed = false;
        }
        
        // 测试3.2: 加载第一个可用驱动
        if(!drivers.empty()) {
            if(asio_output->load_driver(drivers[0].id)) {
                test_helper::print_test_result(true, "加载ASIO驱动成功: " + drivers[0].name);
            } else {
                test_helper::print_test_result(false, "无法加载ASIO驱动");
                all_passed = false;
            }
        }
        
        // 测试3.3: 配置ASIO参数
        asio_output->set_buffer_size(512);
        asio_output->set_sample_rate(44100);
        test_helper::print_test_result(true, "配置ASIO参数成功");
        
        // 测试3.4: 获取ASIO信息
        std::cout << "\nASIO设备信息:" << std::endl;
        std::cout << "  当前驱动: " << asio_output->get_current_driver_name() << std::endl;
        std::cout << "  缓冲区大小: " << asio_output->get_buffer_size() << std::endl;
        std::cout << "  采样率: " << asio_output->get_sample_rate() << "Hz" << std::endl;
        std::cout << "  输入延迟: " << asio_output->get_input_latency() << " 采样" << std::endl;
        std::cout << "  输出延迟: " << asio_output->get_output_latency() << " 采样" << std::endl;
        
        // 测试3.5: 打开音频流（不实际播放）
        abort_callback_dummy abort;
        try {
            asio_output->open(44100, 2, 0, abort);
            test_helper::print_test_result(true, "打开ASIO音频流成功");
            
            // 测试音量控制
            asio_output->volume_set(0.5f);
            test_helper::print_test_result(true, "ASIO音量控制成功");
            
            // 关闭音频流
            asio_output->close(abort);
            test_helper::print_test_result(true, "关闭ASIO音频流成功");
            
        } catch(const std::exception& e) {
            test_helper::print_test_result(false, std::string("ASIO音频流异常: ") + e.what());
            all_passed = false;
        }
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("ASIO输出异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试4: VST桥接测试
bool test_vst_bridge(const test_config& config) {
    test_helper::print_test_header("VST桥接测试");
    
    bool all_passed = true;
    
    try {
        // 初始化VST桥接管理器
        auto& vst_manager = vst_bridge_manager::get_instance();
        if(!vst_manager.initialize()) {
            test_helper::print_test_result(false, "VST桥接管理器初始化失败");
            return false;
        }
        test_helper::print_test_result(true, "VST桥接管理器初始化成功");
        
        // 测试4.1: 扫描VST目录
        auto vst_dirs = vst_manager.get_vst_directories();
        if(!vst_dirs.empty()) {
            test_helper::print_test_result(true, "获取VST目录成功，发现 " + std::to_string(vst_dirs.size()) + " 个目录");
            
            // 扫描每个目录
            int total_plugins = 0;
            for(const auto& dir : vst_dirs) {
                auto plugins = vst_manager.scan_vst_plugins(dir);
                total_plugins += plugins.size();
            }
            test_helper::print_test_result(true, "扫描VST插件成功，发现 " + std::to_string(total_plugins) + " 个插件");
        } else {
            test_helper::print_test_result(false, "未找到VST目录");
            all_passed = false;
        }
        
        // 测试4.2: 加载VST插件（如果有指定路径）
        if(!config.vst_plugin_path.empty()) {
            auto* plugin = vst_manager.load_vst_plugin(config.vst_plugin_path);
            if(plugin) {
                test_helper::print_test_result(true, "加载VST插件成功: " + config.vst_plugin_path);
                
                // 测试插件信息
                std::cout << "\nVST插件信息:" << std::endl;
                std::cout << "  插件名称: " << plugin->get_plugin_name() << std::endl;
                std::cout << "  供应商: " << plugin->get_plugin_vendor() << std::endl;
                std::cout << "  版本: " << plugin->get_plugin_version() << std::endl;
                std::cout << "  输入声道: " << plugin->get_num_inputs() << std::endl;
                std::cout << "  输出声道: " << plugin->get_num_outputs() << std::endl;
                std::cout << "  参数数量: " << plugin->get_num_parameters() << std::endl;
                std::cout << "  预设数量: " << plugin->get_num_programs() << std::endl;
                
                // 测试参数获取
                auto params = plugin->get_parameter_info();
                if(!params.empty()) {
                    std::cout << "  参数信息:" << std::endl;
                    for(size_t i = 0; i < std::min(params.size(), size_t(5)); ++i) {
                        std::cout << "    [" << i << "] " << params[i].name 
                                 << " (" << params[i].min_value << " - " << params[i].max_value << ")" << std::endl;
                    }
                }
                
                // 卸载插件
                vst_manager.unload_vst_plugin(plugin);
                test_helper::print_test_result(true, "卸载VST插件成功");
                
            } else {
                test_helper::print_test_result(false, "无法加载指定VST插件");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(true, "未指定VST插件路径，跳过插件加载测试");
        }
        
        // 测试4.3: VST宿主功能
        std::cout << "\nVST宿主信息:" << std::endl;
        std::cout << "  宿主名称: " << vst_manager.vst_host_->get_host_name() << std::endl;
        std::cout << "  宿主版本: " << vst_manager.vst_host_->get_host_version() << std::endl;
        std::cout << "  宿主供应商: " << vst_manager.vst_host_->get_host_vendor() << std::endl;
        std::cout << "  缓冲区大小: " << vst_manager.get_vst_buffer_size() << std::endl;
        std::cout << "  采样率: " << vst_manager.get_vst_sample_rate() << "Hz" << std::endl;
        
        vst_manager.shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("VST桥接异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试5: 音频分析工具测试
bool test_audio_analyzer(const test_config& config) {
    test_helper::print_test_header("音频分析工具测试");
    
    bool all_passed = true;
    
    try {
        // 获取音频分析管理器
        auto& analysis_manager = get_audio_analysis_manager();
        
        // 测试5.1: 频谱分析仪
        auto spectrum_analyzer = std::make_unique<fb2k::spectrum_analyzer>();
        if(spectrum_analyzer) {
            test_helper::print_test_result(true, "创建频谱分析仪成功");
            
            // 初始化分析仪
            if(SUCCEEDED(spectrum_analyzer->Initialize())) {
                test_helper::print_test_result(true, "初始化频谱分析仪成功");
                
                // 配置分析仪
                spectrum_analyzer->set_fft_size(2048);
                spectrum_analyzer->set_window_type(1); // Hann窗
                spectrum_analyzer->set_analysis_mode(0); // 实时模式
                test_helper::print_test_result(true, "配置频谱分析仪成功");
                
                // 创建测试音频
                auto test_chunk = test_helper::create_test_chunk(44100, 2, 2048, 1000.0f);
                
                // 测试基本分析
                audio_features features;
                if(SUCCEEDED(spectrum_analyzer->analyze_chunk(test_chunk, features))) {
                    test_helper::print_test_result(true, "音频特征分析成功");
                    
                    std::cout << "\n音频特征:" << std::endl;
                    std::cout << "  RMS电平: " << std::fixed << std::setprecision(2) << features.rms_level << " dB" << std::endl;
                    std::cout << "  峰值电平: " << features.peak_level << " dB" << std::endl;
                    std::cout << "  响度: " << features.loudness << " LUFS" << std::endl;
                    std::cout << "  动态范围: " << features.dynamic_range << " dB" << std::endl;
                    std::cout << "  DC偏移: " << features.dc_offset << std::endl;
                    std::cout << "  立体声相关性: " << features.stereo_correlation << std::endl;
                } else {
                    test_helper::print_test_result(false, "音频特征分析失败");
                    all_passed = false;
                }
                
                // 测试频谱分析
                spectrum_data spectrum;
                if(SUCCEEDED(spectrum_analyzer->analyze_spectrum(test_chunk, spectrum))) {
                    test_helper::print_test_result(true, "频谱分析成功");
                    
                    std::cout << "\n频谱信息:" << std::endl;
                    std::cout << "  FFT大小: " << spectrum.fft_size << std::endl;
                    std::cout << "  频率分辨率: " << (spectrum.sample_rate / spectrum.fft_size) << " Hz" << std::endl;
                    std::cout << "  频点数量: " << spectrum.frequencies.size() << std::endl;
                    
                    // 测试频率带分析
                    double band_level = 0.0;
                    if(SUCCEEDED(spectrum_analyzer->get_frequency_band_level(frequency_band::midrange, band_level))) {
                        std::cout << "  中频带(500-2000Hz)电平: " << band_level << " dB" << std::endl;
                    }
                } else {
                    test_helper::print_test_result(false, "频谱分析失败");
                    all_passed = false;
                }
                
                // 测试实时分析
                real_time_analysis analysis;
                if(SUCCEEDED(spectrum_analyzer->get_real_time_analysis(analysis))) {
                    test_helper::print_test_result(true, "实时分析成功");
                } else {
                    test_helper::print_test_result(false, "实时分析失败");
                    all_passed = false;
                }
                
                // 生成分析报告
                std::string report;
                if(SUCCEEDED(spectrum_analyzer->generate_report(report))) {
                    test_helper::print_test_result(true, "生成分析报告成功");
                    if(config.verbose) {
                        std::cout << "\n分析报告预览:\n" << report.substr(0, 300) << "..." << std::endl;
                    }
                }
                
                spectrum_analyzer->Shutdown();
            } else {
                test_helper::print_test_result(false, "初始化频谱分析仪失败");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "无法创建频谱分析仪");
            all_passed = false;
        }
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("音频分析异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试6: 集成测试
bool test_integration(const test_config& config) {
    test_helper::print_test_header("集成测试");
    
    bool all_passed = true;
    
    try {
        // 初始化所有系统
        initialize_fb2k_core_services();
        initialize_fb2k_component_system();
        
        // 创建音频处理器
        auto processor = create_audio_processor();
        
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::realtime;
        processor_config.target_latency_ms = 10.0;
        processor_config.cpu_usage_limit_percent = 80.0f;
        processor_config.buffer_size = 1024;
        processor_config.enable_performance_monitoring = true;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "音频处理器初始化失败");
            return false;
        }
        test_helper::print_test_result(true, "音频处理器初始化成功");
        
        // 添加DSP效果器
        auto dsp_manager = std::make_unique<fb2k::dsp_manager>();
        dsp_config dsp_config;
        dsp_config.enable_standard_effects = true;
        dsp_config.enable_performance_monitoring = true;
        
        if(dsp_manager->initialize(dsp_config)) {
            auto eq = dsp_manager->create_equalizer_10band();
            auto reverb = dsp_manager->create_reverb();
            processor->add_dsp_effect(std::move(eq));
            processor->add_dsp_effect(std::move(reverb));
            test_helper::print_test_result(true, "添加DSP效果器成功");
        }
        
        // 测试6.1: 完整音频处理链路
        auto test_chunk = test_helper::create_test_chunk(44100, 2, 1024, 440.0f);
        audio_chunk output_chunk;
        abort_callback_dummy abort;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        bool process_success = processor->process_audio(test_chunk, output_chunk, abort);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time_ms = duration.count() / 1000.0;
        
        if(process_success) {
            test_helper::print_test_result(true, "完整音频处理链路成功，耗时: " + std::to_string(processing_time_ms) + "ms");
            
            // 验证输出
            float output_rms = output_chunk.calculate_rms();
            if(output_rms > 0.0f) {
                test_helper::print_test_result(true, "输出验证通过，RMS: " + std::to_string(output_rms));
            } else {
                test_helper::print_test_result(false, "输出为空");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "完整音频处理链路失败");
            all_passed = false;
        }
        
        // 测试6.2: 性能监控
        auto stats = processor->get_stats();
        test_helper::print_performance_stats(stats);
        
        // 测试6.3: 状态报告
        std::string status_report = processor->get_status_report();
        if(!status_report.empty()) {
            test_helper::print_test_result(true, "状态报告生成成功");
            if(config.verbose) {
                std::cout << "\n状态报告预览:\n" << status_report.substr(0, 400) << "..." << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "状态报告为空");
            all_passed = false;
        }
        
        // 测试6.4: 实时参数调节
        processor->set_realtime_parameter("Reverb", "room_size", 0.8f);
        float room_size = processor->get_realtime_parameter("Reverb", "room_size");
        
        if(std::abs(room_size - 0.8f) < 0.01f) {
            test_helper::print_test_result(true, "实时参数调节验证通过");
        } else {
            test_helper::print_test_result(false, "实时参数调节验证失败");
            all_passed = false;
        }
        
        processor->shutdown();
        shutdown_fb2k_component_system();
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("集成测试异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 性能基准测试
bool test_performance_benchmark(const test_config& config) {
    test_helper::print_test_header("性能基准测试");
    
    bool all_passed = true;
    
    try {
        // 初始化系统
        initialize_fb2k_core_services();
        initialize_fb2k_component_system();
        
        // 创建音频处理器
        auto processor = create_audio_processor();
        
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::high_fidelity;
        processor_config.target_latency_ms = 5.0;
        processor_config.cpu_usage_limit_percent = 90.0f;
        processor_config.buffer_size = 2048;
        processor_config.enable_performance_monitoring = true;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "音频处理器初始化失败");
            return false;
        }
        
        // 添加多个效果器进行压力测试
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
        
        // 性能测试
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
                processing_times.push_back(duration.count() / 1000.0); // 转换为毫秒
            }
        }
        
        if(!processing_times.empty()) {
            // 计算统计信息
            double total_time = 0.0;
            double min_time = processing_times[0];
            double max_time = processing_times[0];
            
            for(double time : processing_times) {
                total_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            
            double avg_time = total_time / processing_times.size();
            double rtf = (avg_time / (1024.0 / 44100 * 1000.0)); // 实时倍数
            
            std::cout << "\n性能基准结果:" << std::endl;
            std::cout << "  测试次数: " << processing_times.size() << std::endl;
            std::cout << "  平均处理时间: " << std::fixed << std::setprecision(3) << avg_time << "ms" << std::endl;
            std::cout << "  最小处理时间: " << min_time << "ms" << std::endl;
            std::cout << "  最大处理时间: " << max_time << "ms" << std::endl;
            std::cout << "  实时倍数: " << std::fixed << std::setprecision(2) << rtf << "x" << std::endl;
            
            if(rtf < 1.0) {
                test_helper::print_test_result(true, "性能基准测试通过（实时倍数 < 1.0）");
            } else {
                test_helper::print_test_result(false, "性能基准测试失败（实时倍数 >= 1.0）");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "性能基准测试无有效数据");
            all_passed = false;
        }
        
        processor->shutdown();
        shutdown_fb2k_component_system();
        shutdown_fb2k_core_services();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("性能基准测试异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

} // namespace fb2k

// 主测试函数
int main(int argc, char* argv[]) {
    using namespace fb2k;
    
    std::cout << "foobar2000兼容层 - 阶段1.4功能测试" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "测试时间: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    
    // 解析命令行参数
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
            std::cout << "用法: " << argv[0] << " [选项]" << std::endl;
            std::cout << "选项:" << std::endl;
            std::cout << "  --vst-plugin <path>    指定VST插件路径进行测试" << std::endl;
            std::cout << "  --duration <seconds>   设置测试持续时间 (默认: 5)" << std::endl;
            std::cout << "  --quiet                减少输出信息" << std::endl;
            std::cout << "  --help                 显示帮助信息" << std::endl;
            return 0;
        }
    }
    
    std::cout << "\n测试配置:" << std::endl;
    std::cout << "  VST插件路径: " << (config.vst_plugin_path.empty() ? "未指定" : config.vst_plugin_path) << std::endl;
    std::cout << "  测试持续时间: " << config.test_duration_seconds << "秒" << std::endl;
    std::cout << "  详细输出: " << (config.verbose ? "启用" : "禁用") << std::endl;
    
    // 运行所有测试
    std::vector<std::pair<std::string, std::function<bool(const test_config&)>>> tests = {
        {"COM系统测试", test_com_system},
        {"组件系统测试", test_component_system},
        {"ASIO输出设备测试", test_asio_output},
        {"VST桥接测试", test_vst_bridge},
        {"音频分析工具测试", test_audio_analyzer},
        {"集成测试", test_integration},
        {"性能基准测试", test_performance_benchmark}
    };
    
    int passed_count = 0;
    int total_count = tests.size();
    
    for(const auto& [test_name, test_func] : tests) {
        bool passed = test_func(config);
        if(passed) {
            passed_count++;
        }
        
        std::cout << "\n" << test_name << " 结果: " << (passed ? "通过" : "失败") << std::endl;
    }
    
    // 总结
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试总结:" << std::endl;
    std::cout << "  总测试数: " << total_count << std::endl;
    std::cout << "  通过测试: " << passed_count << std::endl;
    std::cout << "  失败测试: " << (total_count - passed_count) << std::endl;
    std::cout << "  通过率: " << std::fixed << std::setprecision(1) 
              << (passed_count * 100.0 / total_count) << "%" << std::endl;
    
    if(passed_count == total_count) {
        std::cout << "\n✓ 所有测试通过！阶段1.4功能正常。" << std::endl;
        std::cout << "\n阶段1.4实现了以下核心功能:" << std::endl;
        std::cout << "- 完整的foobar2000 COM接口体系" << std::endl;
        std::cout << "- 组件系统和插件加载器" << std::endl;
        std::cout << "- ASIO专业音频驱动支持" << std::endl;
        std::cout << "- VST插件桥接系统" << std::endl;
        std::cout << "- 高级音频分析工具" << std::endl;
        std::cout << "- 完整的音频处理链路集成" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ 部分测试失败，请检查错误信息。" << std::endl;
        return 1;
    }
}