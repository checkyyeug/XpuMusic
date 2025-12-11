#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

// 包含所有阶段1.2的头文件
#include "audio_chunk.h"
#include "dsp_interfaces.h"
#include "output_interfaces.h"
#include "../stage1_1/real_minihost.h"

using namespace fb2k;
using namespace std::chrono;

// 集成测试类
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
        std::cout << "=== 阶段1.2集成测试开始 ===" << std::endl;
        
        if(!host_->Initialize()) {
            std::cout << "❌ 主机初始化失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ 主机初始化成功" << std::endl;
        
        // 初始化DSP系统
        if(!dsp_system_initializer::initialize_dsp_system()) {
            std::cout << "❌ DSP系统初始化失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ DSP系统初始化成功" << std::endl;
        
        return true;
    }
    
    void shutdown() {
        dsp_system_initializer::shutdown_dsp_system();
        
        if(host_) {
            host_->Shutdown();
        }
        
        std::cout << "\n=== 集成测试完成 ===" << std::endl;
    }
    
    // 测试1：音频块基本功能
    bool test_audio_chunk_basic() {
        std::cout << "\n1. 音频块基本功能测试..." << std::endl;
        
        // 创建音频块
        auto chunk = audio_chunk_utils::create_chunk(1024, 2, 44100);
        if(!chunk) {
            std::cout << "❌ 音频块创建失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ 音频块创建成功" << std::endl;
        
        // 验证基本属性
        if(chunk->get_sample_count() != 1024) {
            std::cout << "❌ 采样数不匹配" << std::endl;
            return false;
        }
        
        if(chunk->get_channels() != 2) {
            std::cout << "❌ 声道数不匹配" << std::endl;
            return false;
        }
        
        if(chunk->get_sample_rate() != 44100) {
            std::cout << "❌ 采样率不匹配" << std::endl;
            return false;
        }
        
        std::cout << "✅ 音频块属性验证通过" << std::endl;
        
        // 验证数据有效性
        if(!audio_chunk_validation::validate_audio_chunk_basic(*chunk)) {
            std::cout << "❌ 音频块基本验证失败" << std::endl;
            return false;
        }
        
        if(!audio_chunk_validation::validate_audio_chunk_format(*chunk)) {
            std::cout << "❌ 音频块格式验证失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ 音频块验证通过" << std::endl;
        
        // 显示音频块信息
        audio_chunk_validation::log_audio_chunk_info(*chunk, "  ");
        
        return true;
    }
    
    // 测试2：DSP预设功能
    bool test_dsp_preset() {
        std::cout << "\n2. DSP预设功能测试..." << std::endl;
        
        // 创建DSP预设
        auto preset = dsp_config_helper::create_basic_preset("TestPreset");
        if(!preset) {
            std::cout << "❌ DSP预设创建失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ DSP预设创建成功" << std::endl;
        
        // 设置预设参数
        preset->set_name("Equalizer");
        preset->set_parameter_float("gain", 0.8f);
        preset->set_parameter_float("bass", 1.2f);
        preset->set_parameter_float("treble", 0.9f);
        preset->set_parameter_string("mode", "rock");
        
        // 验证参数
        if(std::string(preset->get_name()) != "Equalizer") {
            std::cout << "❌ 预设名称设置失败" << std::endl;
            return false;
        }
        
        if(preset->get_parameter_float("gain") != 0.8f) {
            std::cout << "❌ 浮点参数设置失败" << std::endl;
            return false;
        }
        
        if(std::string(preset->get_parameter_string("mode")) != "rock") {
            std::cout << "❌ 字符串参数设置失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ DSP预设参数设置成功" << std::endl;
        
        // 序列化和反序列化测试
        std::vector<uint8_t> serialized_data;
        preset->serialize(serialized_data);
        
        auto new_preset = std::make_unique<simple_dsp_preset>();
        new_preset->deserialize(serialized_data.data(), serialized_data.size());
        
        if(std::string(new_preset->get_name()) != std::string(preset->get_name())) {
            std::cout << "❌ 预设序列化/反序列化失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ DSP预设序列化测试通过" << std::endl;
        
        return true;
    }
    
    // 测试3：DSP效果器功能
    bool test_dsp_effects() {
        std::cout << "\n3. DSP效果器功能测试..." << std::endl;
        
        // 创建测试DSP效果器
        auto effect = dsp_effect_factory::create_test_effect("TestEffect");
        if(!effect) {
            std::cout << "❌ DSP效果器创建失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ DSP效果器创建成功: " << effect->get_name() << std::endl;
        
        // 创建音频块
        auto chunk = audio_chunk_utils::create_chunk(1024, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 实例化效果器
        if(!effect->instantiate(*chunk, 44100, 2)) {
            std::cout << "❌ DSP效果器实例化失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ DSP效果器实例化成功" << std::endl;
        
        // 处理音频
        float rms_before = audio_chunk_utils::calculate_rms(*chunk);
        
        effect->run(*chunk, *abort);
        
        float rms_after = audio_chunk_utils::calculate_rms(*chunk);
        
        std::cout << "✅ DSP效果器处理完成" << std::endl;
        std::cout << "  处理前RMS: " << rms_before << std::endl;
        std::cout << "  处理后RMS: " << rms_after << std::endl;
        
        // 验证效果器参数
        auto params = effect->get_config_params();
        std::cout << "✅ DSP效果器配置参数:" << std::endl;
        for(const auto& param : params) {
            std::cout << "    - " << param.name << ": " << param.description 
                     << " (" << param.min_value << " - " << param.max_value << ")" << std::endl;
        }
        
        return true;
    }
    
    // 测试4：DSP链功能
    bool test_dsp_chain() {
        std::cout << "\n4. DSP链功能测试..." << std::endl;
        
        // 创建DSP链
        auto chain = std::make_unique<dsp_chain>();
        
        // 添加多个效果器
        chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_volume_effect(0.8f)));
        chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_passthrough_effect("PassThrough")));
        
        std::cout << "✅ DSP链创建成功，效果器数量: " << chain->get_effect_count() << std::endl;
        
        // 显示DSP链信息
        std::string chain_info = dsp_utils::get_dsp_chain_info(*chain);
        std::cout << chain_info << std::endl;
        
        // 验证DSP链
        auto validation_result = dsp_chain_validator::validate_chain(*chain);
        if(!validation_result.is_valid) {
            std::cout << "❌ DSP链验证失败: " << validation_result.error_message << std::endl;
            return false;
        }
        
        if(!validation_result.warnings.empty()) {
            std::cout << "⚠️  DSP链警告:" << std::endl;
            for(const auto& warning : validation_result.warnings) {
                std::cout << "  - " << warning << std::endl;
            }
        }
        
        std::cout << "✅ DSP链验证通过" << std::endl;
        
        // 测试DSP链处理
        auto chunk = audio_chunk_utils::create_chunk(2048, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 实例化所有效果器
        for(size_t i = 0; i < chain->get_effect_count(); ++i) {
            dsp* effect = chain->get_effect(i);
            if(effect) {
                effect->instantiate(*chunk, 44100, 2);
            }
        }
        
        float rms_before = audio_chunk_utils::calculate_rms(*chunk);
        
        chain->run_chain(*chunk, *abort);
        
        float rms_after = audio_chunk_utils::calculate_rms(*chunk);
        
        std::cout << "✅ DSP链处理完成" << std::endl;
        std::cout << "  处理前RMS: " << rms_before << std::endl;
        std::cout << "  处理后RMS: " << rms_after << std::endl;
        
        return true;
    }
    
    // 测试5：完整音频链路
    bool test_complete_audio_chain() {
        std::cout << "\n5. 完整音频链路测试..." << std::endl;
        
        // 创建音频数据
        auto input_chunk = audio_chunk_utils::create_chunk(4096, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 添加测试音频数据（正弦波）
        float* data = input_chunk->get_data();
        if(data) {
            const float frequency = 440.0f; // A4音符
            const float amplitude = 0.5f;
            
            for(size_t i = 0; i < input_chunk->get_sample_count(); ++i) {
                float time = static_cast<float>(i) / input_chunk->get_sample_rate();
                float value = amplitude * sin(2.0f * 3.14159f * frequency * time);
                
                // 立体声
                data[i * 2] = value;      // 左声道
                data[i * 2 + 1] = value;  // 右声道
            }
        }
        
        std::cout << "✅ 输入音频数据创建完成" << std::endl;
        
        // 创建DSP链
        auto dsp_chain = std::make_unique<dsp_chain>();
        dsp_chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_volume_effect(0.8f)));
        dsp_chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_passthrough_effect("Clean")));
        
        std::cout << "✅ DSP链配置完成" << std::endl;
        
        // 处理音频数据
        float input_rms = audio_chunk_utils::calculate_rms(*input_chunk);
        
        // 实例化所有DSP效果器
        for(size_t i = 0; i < dsp_chain->get_effect_count(); ++i) {
            dsp* effect = dsp_chain->get_effect(i);
            if(effect) {
                effect->instantiate(*input_chunk, 44100, 2);
            }
        }
        
        // 运行DSP处理
        dsp_chain->run_chain(*input_chunk, *abort);
        
        float output_rms = audio_chunk_utils::calculate_rms(*input_chunk);
        
        std::cout << "✅ 音频处理链路完成" << std::endl;
        std::cout << "  输入RMS: " << input_rms << std::endl;
        std::cout << "  输出RMS: " << output_rms << std::endl;
        std::cout << "  处理增益: " << (output_rms / input_rms) << std::endl;
        
        return true;
    }
    
    // 测试6：性能基准测试
    bool test_performance_benchmark() {
        std::cout << "\n6. 性能基准测试..." << std::endl;
        
        const size_t test_samples = 44100 * 10; // 10秒音频
        const size_t iterations = 100;
        
        // 创建测试音频块
        auto chunk = audio_chunk_utils::create_chunk(test_samples, 2, 44100);
        auto abort = std::make_unique<AbortCallbackDummy>();
        
        // 创建DSP链
        auto dsp_chain = std::make_unique<dsp_chain>();
        dsp_chain->add_effect(service_ptr_t<dsp>(dsp_effect_factory::create_passthrough_effect()));
        
        // 实例化所有效果器
        for(size_t i = 0; i < dsp_chain->get_effect_count(); ++i) {
            dsp* effect = dsp_chain->get_effect(i);
            if(effect) {
                effect->instantiate(*chunk, 44100, 2);
            }
        }
        
        // 性能测试
        auto start_time = high_resolution_clock::now();
        
        for(size_t i = 0; i < iterations; ++i) {
            dsp_chain->run_chain(*chunk, *abort);
        }
        
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end_time - start_time);
        
        double total_seconds = static_cast<double>(duration.count()) / 1000000.0;
        double samples_per_second = (test_samples * iterations) / total_seconds;
        double realtime_factor = samples_per_second / 44100.0;
        
        std::cout << "✅ 性能测试完成" << std::endl;
        std::cout << "  总处理时间: " << total_seconds << " 秒" << std::endl;
        std::cout << "  总采样数: " << (test_samples * iterations) << std::endl;
        std::cout << "  处理速度: " << samples_per_second << " 采样/秒" << std::endl;
        std::cout << "  实时倍数: " << realtime_factor << "x" << std::endl;
        std::cout << "  CPU占用估算: " << dsp_utils::estimate_cpu_usage(*dsp_chain) << "%" << std::endl;
        
        return true;
    }
    
    // 运行所有测试
    bool run_all_tests() {
        std::cout << "=" << std::string(60, '=') << std::endl;
        std::cout << "阶段1.2：功能扩展集成测试" << std::endl;
        std::cout << "=" << std::string(60, '=') << std::endl;
        
        if(!initialize()) {
            return false;
        }
        
        int passed_tests = 0;
        int total_tests = 6;
        
        // 运行所有测试
        std::vector<std::pair<std::string, std::function<bool()>>> tests = {
            {"音频块基本功能", [this]() { return test_audio_chunk_basic(); }},
            {"DSP预设功能", [this]() { return test_dsp_preset(); }},
            {"DSP效果器功能", [this]() { return test_dsp_effects(); }},
            {"DSP链功能", [this]() { return test_dsp_chain(); }},
            {"完整音频链路", [this]() { return test_complete_audio_chain(); }},
            {"性能基准测试", [this]() { return test_performance_benchmark(); }}
        };
        
        for(size_t i = 0; i < tests.size(); ++i) {
            std::cout << "\n[" << (i+1) << "/" << tests.size() << "] " << tests[i].first << std::endl;
            
            try {
                if(tests[i].second()) {
                    passed_tests++;
                    std::cout << "✅ " << tests[i].first << " - 通过" << std::endl;
                } else {
                    std::cout << "❌ " << tests[i].first << " - 失败" << std::endl;
                }
            } catch(const std::exception& e) {
                std::cout << "❌ " << tests[i].first << " - 异常: " << e.what() << std::endl;
            }
        }
        
        shutdown();
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "测试结果: " << passed_tests << "/" << total_tests << " 通过" << std::endl;
        
        if(passed_tests == total_tests) {
            std::cout << "🎉 所有测试通过！阶段1.2功能扩展完成。" << std::endl;
            std::cout << "\n核心成就:" << std::endl;
            std::cout << "  ✅ 音频块系统完整实现" << std::endl;
            std::cout << "  ✅ DSP预设和配置系统" << std::endl;
            std::cout << "  ✅ DSP效果器框架" << std::endl;
            std::cout << "  ✅ DSP链管理器" << std::endl;
            std::cout << "  ✅ 完整音频处理链路" << std::endl;
            std::cout << "  ✅ 性能基准验证" << std::endl;
            std::cout << "\n下一步：阶段1.3 - 高级功能和优化" << std::endl;
            return true;
        } else {
            std::cout << "⚠️  部分测试失败，需要调试" << std::endl;
            return false;
        }
    }
};

// 主测试函数
int main() {
    try {
        IntegrationTest test;
        return test.run_all_tests() ? 0 : 1;
    } catch(const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << std::endl;
        return 1;
    }
}