#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <fstream>

// 包含必要的头文件
#include "dsp_equalizer.h"
#include "dsp_reverb.h"
#include "dsp_manager.h"
#include "output_wasapi.h"
#include "audio_processor.h"
#include "audio_block_impl.h"
#include "abort_callback.h"

namespace fb2k {

// 测试配置
struct test_config {
    uint32_t sample_rate = 44100;
    uint32_t channels = 2;
    size_t test_duration_seconds = 5;
    bool enable_dsp = true;
    bool enable_output = false; // 默认禁用输出以避免声音
    bool verbose = true;
};

// 测试音频源
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
        
        // 创建测试音频数据
        size_t samples_per_chunk = 512;
        chunk.set_sample_count(samples_per_chunk);
        chunk.set_channels(channels_);
        chunk.set_sample_rate(sample_rate_);
        
        float* data = chunk.get_data();
        float phase_increment = 2.0f * M_PI * frequency_ / sample_rate_;
        
        for(size_t i = 0; i < samples_per_chunk; ++i) {
            float sample = std::sin(current_phase_);
            
            // 写入所有声道
            for(uint32_t ch = 0; ch < channels_; ++ch) {
                data[i * channels_ + ch] = sample * 0.5f; // 降低音量
            }
            
            current_phase_ += phase_increment;
            if(current_phase_ >= 2.0f * M_PI) {
                current_phase_ -= 2.0f * M_PI;
            }
        }
        
        return true;
    }
    
    bool is_eof() const override {
        return false; // 无限生成测试音频
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

// 测试音频输出
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
        
        // 记录统计信息
        total_samples_written_ += chunk.get_sample_count() * chunk.get_channels();
        chunk_count_++;
        
        // 计算RMS电平
        float rms = chunk.calculate_rms();
        
        // 每100个块输出一次统计
        if(chunk_count_ % 100 == 0) {
            std::cout << "[TestSink] " << name_ << " - 块数: " << chunk_count_ 
                      << ", 总采样: " << total_samples_written_ 
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

// 测试辅助函数
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
        std::cout << "  丢包数: " << stats.dropout_count << std::endl;
        std::cout << "  错误数: " << stats.error_count << std::endl;
    }
    
    static bool save_test_audio(const audio_chunk& chunk, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if(!file.is_open()) {
            return false;
        }
        
        // 简化的WAV文件头（实际应该使用完整的WAV格式）
        const float* data = chunk.get_data();
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        
        // 写入数据（这里简化处理，实际应该写入完整的WAV文件）
        file.write(reinterpret_cast<const char*>(data), total_samples * sizeof(float));
        
        file.close();
        return true;
    }
};

// 测试1: 基础DSP效果器测试
bool test_basic_dsp_effects(const test_config& config) {
    test_helper::print_test_header("基础DSP效果器测试");
    
    bool all_passed = true;
    
    try {
        // 创建DSP管理器
        auto dsp_manager = std::make_unique<dsp_manager>();
        dsp_config dsp_config;
        dsp_config.enable_standard_effects = true;
        dsp_config.enable_performance_monitoring = true;
        
        if(!dsp_manager->initialize(dsp_config)) {
            test_helper::print_test_result(false, "DSP管理器初始化失败");
            return false;
        }
        
        // 测试1.1: 创建均衡器
        auto eq = dsp_manager->create_equalizer_10band();
        if(!eq) {
            test_helper::print_test_result(false, "创建10段均衡器失败");
            all_passed = false;
        } else {
            dsp_manager->add_effect(std::move(eq));
            test_helper::print_test_result(true, "创建并添加10段均衡器");
        }
        
        // 测试1.2: 创建混响
        auto reverb = dsp_manager->create_reverb();
        if(!reverb) {
            test_helper::print_test_result(false, "创建混响效果器失败");
            all_passed = false;
        } else {
            dsp_manager->add_effect(std::move(reverb));
            test_helper::print_test_result(true, "创建并添加混响效果器");
        }
        
        // 测试1.3: 创建压缩器
        auto compressor = dsp_manager->create_compressor();
        if(!compressor) {
            test_helper::print_test_result(false, "创建压缩器失败");
            all_passed = false;
        } else {
            dsp_manager->add_effect(std::move(compressor));
            test_helper::print_test_result(true, "创建并添加压缩器");
        }
        
        // 测试1.4: 验证效果器数量
        size_t effect_count = dsp_manager->get_effect_count();
        if(effect_count == 3) {
            test_helper::print_test_result(true, "效果器数量验证: " + std::to_string(effect_count));
        } else {
            test_helper::print_test_result(false, "效果器数量不匹配: " + std::to_string(effect_count) + " != 3");
            all_passed = false;
        }
        
        // 测试1.5: 生成DSP报告
        std::string dsp_report = dsp_manager->generate_dsp_report();
        if(!dsp_report.empty()) {
            test_helper::print_test_result(true, "生成DSP报告成功");
            if(config.verbose) {
                std::cout << "\nDSP报告预览:\n" << dsp_report.substr(0, 200) << "..." << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "DSP报告为空");
            all_passed = false;
        }
        
        dsp_manager->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试2: 高级混响效果器测试
bool test_advanced_reverb(const test_config& config) {
    test_helper::print_test_header("高级混响效果器测试");
    
    bool all_passed = true;
    
    try {
        // 创建混响参数
        dsp_effect_params reverb_params;
        reverb_params.type = dsp_effect_type::reverb;
        reverb_params.name = "Advanced Reverb";
        reverb_params.is_enabled = true;
        reverb_params.is_bypassed = false;
        reverb_params.cpu_usage_estimate = 15.0f;
        reverb_params.latency_ms = 10.0;
        
        // 创建高级混响效果器
        auto reverb = std::make_unique<dsp_reverb_advanced>(reverb_params);
        
        // 测试2.1: 加载不同预设
        reverb->load_room_preset(0.3f); // 小房间
        test_helper::print_test_result(true, "加载小房间预设");
        
        reverb->load_hall_preset(0.7f); // 大厅
        test_helper::print_test_result(true, "加载大厅预设");
        
        reverb->load_plate_preset(); // 板式混响
        test_helper::print_test_result(true, "加载板式混响预设");
        
        reverb->load_cathedral_preset(); // 大教堂
        test_helper::print_test_result(true, "加载大教堂预设");
        
        // 测试2.2: 参数调节
        reverb->set_room_size(0.6f);
        reverb->set_damping(0.4f);
        reverb->set_wet_level(0.3f);
        reverb->set_dry_level(0.7f);
        reverb->set_width(0.8f);
        reverb->set_predelay(20.0f);
        reverb->set_decay_time(2.5f);
        reverb->set_diffusion(0.8f);
        
        test_helper::print_test_result(true, "混响参数调节");
        
        // 测试2.3: 调制功能
        reverb->set_modulation_rate(0.5f);
        reverb->set_modulation_depth(0.2f);
        reverb->enable_modulation(true);
        test_helper::print_test_result(true, "启用调制功能");
        
        // 测试2.4: 滤波功能
        reverb->enable_filtering(true);
        test_helper::print_test_result(true, "启用滤波功能");
        
        // 测试2.5: 实例化和处理
        audio_chunk test_chunk;
        test_chunk.set_sample_count(512);
        test_chunk.set_channels(config.channels);
        test_chunk.set_sample_rate(config.sample_rate);
        
        // 填充测试数据
        float* data = test_chunk.get_data();
        for(size_t i = 0; i < 512 * config.channels; ++i) {
            data[i] = std::sin(2.0f * M_PI * 440.0f * i / config.sample_rate) * 0.5f;
        }
        
        abort_callback_dummy abort;
        
        if(reverb->instantiate(test_chunk, config.sample_rate, config.channels)) {
            test_helper::print_test_result(true, "混响效果器实例化成功");
            
            // 处理音频
            reverb->run(test_chunk, abort);
            test_helper::print_test_result(true, "混响音频处理成功");
            
            // 验证输出
            float output_rms = test_chunk.calculate_rms();
            if(output_rms > 0.0f) {
                test_helper::print_test_result(true, "混响输出验证通过，RMS: " + std::to_string(output_rms));
            } else {
                test_helper::print_test_result(false, "混响输出为空");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "混响效果器实例化失败");
            all_passed = false;
        }
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试3: 音频处理器集成测试
bool test_audio_processor_integration(const test_config& config) {
    test_helper::print_test_header("音频处理器集成测试");
    
    bool all_passed = true;
    
    try {
        // 创建音频处理器
        auto processor = create_audio_processor();
        
        // 配置音频处理器
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::realtime;
        processor_config.target_latency_ms = 10.0;
        processor_config.cpu_usage_limit_percent = 80.0f;
        processor_config.buffer_size = 1024;
        processor_config.max_dsp_effects = 16;
        processor_config.enable_performance_monitoring = true;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "音频处理器初始化失败");
            return false;
        }
        test_helper::print_test_result(true, "音频处理器初始化成功");
        
        // 添加DSP效果器
        auto eq = dsp_manager().create_equalizer_10band();
        auto reverb = dsp_manager().create_reverb();
        
        processor->add_dsp_effect(std::move(eq));
        processor->add_dsp_effect(std::move(reverb));
        test_helper::print_test_result(true, "添加DSP效果器成功");
        
        // 测试3.1: 基本音频处理
        audio_chunk input_chunk;
        input_chunk.set_sample_count(512);
        input_chunk.set_channels(config.channels);
        input_chunk.set_sample_rate(config.sample_rate);
        
        // 生成测试音频
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
            test_helper::print_test_result(true, "音频处理成功，耗时: " + std::to_string(processing_time_ms) + "ms");
            
            // 验证输出
            float output_rms = output_chunk.calculate_rms();
            if(output_rms > 0.0f) {
                test_helper::print_test_result(true, "输出验证通过，RMS: " + std::to_string(output_rms));
            } else {
                test_helper::print_test_result(false, "输出为空");
                all_passed = false;
            }
        } else {
            test_helper::print_test_result(false, "音频处理失败");
            all_passed = false;
        }
        
        // 测试3.2: 音量控制
        processor->set_volume(0.5f);
        processor->process_audio(input_chunk, output_chunk, abort);
        float half_volume_rms = output_chunk.calculate_rms();
        
        processor->set_volume(1.0f);
        processor->process_audio(input_chunk, output_chunk, abort);
        float full_volume_rms = output_chunk.calculate_rms();
        
        if(std::abs(half_volume_rms - full_volume_rms * 0.5f) < full_volume_rms * 0.1f) {
            test_helper::print_test_result(true, "音量控制验证通过");
        } else {
            test_helper::print_test_result(false, "音量控制验证失败");
            all_passed = false;
        }
        
        // 测试3.3: 静音功能
        processor->set_mute(true);
        processor->process_audio(input_chunk, output_chunk, abort);
        float muted_rms = output_chunk.calculate_rms();
        
        processor->set_mute(false);
        
        if(muted_rms < 0.001f) {
            test_helper::print_test_result(true, "静音功能验证通过");
        } else {
            test_helper::print_test_result(false, "静音功能验证失败");
            all_passed = false;
        }
        
        // 测试3.4: 实时参数调节
        processor->set_realtime_parameter("Reverb", "room_size", 0.7f);
        float room_size = processor->get_realtime_parameter("Reverb", "room_size");
        
        if(std::abs(room_size - 0.7f) < 0.01f) {
            test_helper::print_test_result(true, "实时参数调节验证通过");
        } else {
            test_helper::print_test_result(false, "实时参数调节验证失败");
            all_passed = false;
        }
        
        // 测试3.5: 性能统计
        auto stats = processor->get_stats();
        if(stats.total_samples_processed > 0) {
            test_helper::print_test_result(true, "性能统计收集成功");
            test_helper::print_performance_stats(stats);
        } else {
            test_helper::print_test_result(false, "性能统计为空");
            all_passed = false;
        }
        
        // 测试3.6: 状态报告
        std::string status_report = processor->get_status_report();
        if(!status_report.empty()) {
            test_helper::print_test_result(true, "状态报告生成成功");
            if(config.verbose) {
                std::cout << "\n状态报告预览:\n" << status_report.substr(0, 300) << "..." << std::endl;
            }
        } else {
            test_helper::print_test_result(false, "状态报告为空");
            all_passed = false;
        }
        
        processor->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试4: 音频流处理测试
bool test_audio_stream_processing(const test_config& config) {
    test_helper::print_test_header("音频流处理测试");
    
    bool all_passed = true;
    
    try {
        // 创建音频处理器
        auto processor = create_audio_processor();
        
        audio_processor_config processor_config;
        processor_config.processing_mode = processing_mode::realtime;
        processor_config.target_latency_ms = 10.0;
        processor_config.cpu_usage_limit_percent = 80.0f;
        processor_config.buffer_size = 1024;
        
        if(!processor->initialize(processor_config)) {
            test_helper::print_test_result(false, "音频处理器初始化失败");
            return false;
        }
        
        // 添加效果器
        auto eq = dsp_manager().create_equalizer_10band();
        auto reverb = dsp_manager().create_reverb();
        processor->add_dsp_effect(std::move(eq));
        processor->add_dsp_effect(std::move(reverb));
        
        // 创建音频源和输出
        auto audio_source = std::make_unique<test_audio_source>(config.sample_rate, config.channels, 440.0f);
        auto audio_sink = std::make_unique<test_audio_sink>("StreamTest");
        
        // 测试4.1: 基本流处理
        abort_callback_dummy abort;
        abort.set_timeout(1000); // 1秒超时
        
        std::cout << "开始流处理测试..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();
        
        bool stream_success = processor->process_stream(audio_source.get(), audio_sink.get(), abort);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if(stream_success) {
            test_helper::print_test_result(true, "流处理成功，耗时: " + std::to_string(duration.count()) + "ms");
            test_helper::print_test_result(true, "处理采样数: " + std::to_string(audio_sink->get_total_samples()));
        } else {
            test_helper::print_test_result(false, "流处理失败");
            all_passed = false;
        }
        
        // 测试4.2: 中断处理
        auto abort_source = std::make_unique<test_audio_source>(config.sample_rate, config.channels, 880.0f);
        auto abort_sink = std::make_unique<test_audio_sink>("AbortTest");
        
        // 设置快速中断
        abort_callback_dummy fast_abort;
        fast_abort.set_timeout(100); // 100ms后中断
        
        bool abort_success = processor->process_stream(abort_source.get(), abort_sink.get(), fast_abort);
        
        if(!abort_success && fast_abort.is_aborting()) {
            test_helper::print_test_result(true, "中断处理验证通过");
        } else {
            test_helper::print_test_result(false, "中断处理验证失败");
            all_passed = false;
        }
        
        // 测试4.3: 性能监控
        auto final_stats = processor->get_stats();
        if(final_stats.total_samples_processed > 0) {
            test_helper::print_test_result(true, "流处理性能统计收集成功");
            test_helper::print_performance_stats(final_stats);
        } else {
            test_helper::print_test_result(false, "流处理性能统计为空");
            all_passed = false;
        }
        
        processor->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 测试5: 性能基准测试
bool test_performance_benchmark(const test_config& config) {
    test_helper::print_test_header("性能基准测试");
    
    bool all_passed = true;
    
    try {
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
        for(int i = 0; i < 5; ++i) {
            auto eq = dsp_manager().create_equalizer_10band();
            auto reverb = dsp_manager().create_reverb();
            processor->add_dsp_effect(std::move(eq));
            processor->add_dsp_effect(std::move(reverb));
        }
        
        // 测试5.1: 处理速度基准
        audio_chunk test_chunk;
        test_chunk.set_sample_count(1024);
        test_chunk.set_channels(config.channels);
        test_chunk.set_sample_rate(config.sample_rate);
        
        // 生成复杂的测试信号
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
            double rtf = (avg_time / (1024.0 / config.sample_rate * 1000.0)); // 实时倍数
            
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
        
        // 测试5.2: 内存使用检查（简化版）
        auto final_stats = processor->get_stats();
        if(final_stats.error_count == 0) {
            test_helper::print_test_result(true, "处理过程中无错误");
        } else {
            test_helper::print_test_result(false, "处理过程中出现错误: " + std::to_string(final_stats.error_count));
            all_passed = false;
        }
        
        processor->shutdown();
        
    } catch(const std::exception& e) {
        test_helper::print_test_result(false, std::string("异常: ") + e.what());
        all_passed = false;
    }
    
    return all_passed;
}

// 主测试函数
int main(int argc, char* argv[]) {
    std::cout << "foobar2000兼容层 - 阶段1.3功能测试" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "测试时间: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    
    // 解析命令行参数
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
            std::cout << "用法: " << argv[0] << " [选项]" << std::endl;
            std::cout << "选项:" << std::endl;
            std::cout << "  --sample-rate <rate>   设置测试采样率 (默认: 44100)" << std::endl;
            std::cout << "  --channels <num>       设置测试声道数 (默认: 2)" << std::endl;
            std::cout << "  --duration <seconds>   设置测试持续时间 (默认: 5)" << std::endl;
            std::cout << "  --enable-output        启用音频输出（会产生声音）" << std::endl;
            std::cout << "  --quiet                减少输出信息" << std::endl;
            std::cout << "  --help                 显示帮助信息" << std::endl;
            return 0;
        }
    }
    
    std::cout << "\n测试配置:" << std::endl;
    std::cout << "  采样率: " << config.sample_rate << "Hz" << std::endl;
    std::cout << "  声道数: " << config.channels << std::endl;
    std::cout << "  测试持续时间: " << config.test_duration_seconds << "秒" << std::endl;
    std::cout << "  音频输出: " << (config.enable_output ? "启用" : "禁用") << std::endl;
    std::cout << "  详细输出: " << (config.verbose ? "启用" : "禁用") << std::endl;
    
    // 运行所有测试
    std::vector<std::pair<std::string, std::function<bool(const test_config&)>>> tests = {
        {"基础DSP效果器测试", test_basic_dsp_effects},
        {"高级混响效果器测试", test_advanced_reverb},
        {"音频处理器集成测试", test_audio_processor_integration},
        {"音频流处理测试", test_audio_stream_processing},
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
        std::cout << "\n✓ 所有测试通过！阶段1.3功能正常。" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ 部分测试失败，请检查错误信息。" << std::endl;
        return 1;
    }
}

} // namespace fb2k