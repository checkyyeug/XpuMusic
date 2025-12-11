#include "dsp_manager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace fb2k {

// DSP管理器实现

dsp_manager::dsp_manager() 
    : use_multithreading_(false),
      preset_manager_(std::make_unique<dsp_preset_manager>()),
      performance_monitor_(std::make_unique<dsp_performance_monitor>()) {
}

dsp_manager::~dsp_manager() {
    shutdown();
}

bool dsp_manager::initialize(const dsp_config& config) {
    std::cout << "[DSPManager] 初始化DSP管理器..." << std::endl;
    
    config_ = config;
    
    // 初始化性能监控
    if(config_.enable_performance_monitoring) {
        performance_monitor_->start_monitoring();
    }
    
    // 初始化多线程处理器
    if(config_.enable_multithreading) {
        thread_pool_ = std::make_unique<multithreaded_dsp_processor>(config_.max_threads);
        if(!thread_pool_->start()) {
            std::cerr << "[DSPManager] 多线程处理器启动失败" << std::endl;
            return false;
        }
        use_multithreading_ = true;
        std::cout << "[DSPManager] 多线程处理器已启动，线程数: " << config_.max_threads << std::endl;
    }
    
    // 加载标准效果器
    if(config_.enable_standard_effects) {
        load_standard_effects();
    }
    
    std::cout << "[DSPManager] DSP管理器初始化完成" << std::endl;
    std::cout << "[DSPManager] 配置: " << std::endl;
    std::cout << "  - 多线程: " << (config_.enable_multithreading ? "启用" : "禁用") << std::endl;
    std::cout << "  - 性能监控: " << (config_.enable_performance_monitoring ? "启用" : "禁用") << std::endl;
    std::cout << "  - 内存池: " << (config_.memory_pool_size / (1024*1024)) << "MB" << std::endl;
    
    return true;
}

void dsp_manager::shutdown() {
    std::cout << "[DSPManager] 关闭DSP管理器..." << std::endl;
    
    // 停止性能监控
    if(performance_monitor_) {
        performance_monitor_->stop_monitoring();
    }
    
    // 停止多线程处理器
    if(thread_pool_) {
        thread_pool_->stop();
        use_multithreading_ = false;
    }
    
    // 清理效果器
    clear_effects();
    
    std::cout << "[DSPManager] DSP管理器已关闭" << std::endl;
}

bool dsp_manager::add_effect(std::unique_ptr<dsp_effect_advanced> effect) {
    if(!effect) {
        return false;
    }
    
    if(effects_.size() >= config_.max_effects) {
        std::cerr << "[DSPManager] 效果器数量已达上限: " << config_.max_effects << std::endl;
        return false;
    }
    
    effects_.push_back(std::move(effect));
    
    std::cout << "[DSPManager] 添加效果器: " << effects_.back()->get_name() << std::endl;
    return true;
}

bool dsp_manager::remove_effect(size_t index) {
    if(index >= effects_.size()) {
        return false;
    }
    
    std::cout << "[DSPManager] 移除效果器: " << effects_[index]->get_name() << std::endl;
    effects_.erase(effects_.begin() + index);
    return true;
}

void dsp_manager::clear_effects() {
    std::cout << "[DSPManager] 清除所有效果器（数量: " << effects_.size() << ")" << std::endl;
    effects_.clear();
}

size_t dsp_manager::get_effect_count() const {
    return effects_.size();
}

dsp_effect_advanced* dsp_manager::get_effect(size_t index) {
    return index < effects_.size() ? effects_[index].get() : nullptr;
}

const dsp_effect_advanced* dsp_manager::get_effect(size_t index) const {
    return index < effects_.size() ? effects_[index].get() : nullptr;
}

bool dsp_manager::process_chain(audio_chunk& chunk, abort_callback& abort) {
    if(effects_.empty() || abort.is_aborting()) {
        return true;
    }
    
    // 记录处理开始
    if(performance_monitor_) {
        performance_monitor_->record_processing_start();
    }
    
    bool success = true;
    
    try {
        if(use_multithreading_ && effects_.size() > 1) {
            success = process_chain_multithread(chunk, abort);
        } else {
            success = process_chain_singlethread(chunk, abort);
        }
    } catch(const std::exception& e) {
        std::cerr << "[DSPManager] 处理链异常: " << e.what() << std::endl;
        if(performance_monitor_) {
            performance_monitor_->record_error("processing_exception");
        }
        success = false;
    }
    
    // 记录处理结束
    if(performance_monitor_) {
        performance_monitor_->record_processing_end(chunk.get_sample_count() * chunk.get_channels());
    }
    
    return success;
}

bool dsp_manager::process_chain_singlethread(audio_chunk& chunk, abort_callback& abort) {
    // 单线程处理
    for(auto& effect : effects_) {
        if(!effect || effect->is_bypassed()) {
            continue;
        }
        
        if(abort.is_aborting()) {
            return false;
        }
        
        // 实例化效果器（如果需要）
        if(!effect->instantiate(chunk, chunk.get_sample_rate(), chunk.get_channels())) {
            std::cerr << "[DSPManager] 效果器实例化失败: " << effect->get_name() << std::endl;
            continue;
        }
        
        // 处理音频
        effect->run(chunk, abort);
        
        if(abort.is_aborting()) {
            return false;
        }
    }
    
    return true;
}

bool dsp_manager::process_chain_multithread(audio_chunk& chunk, abort_callback& abort) {
    // 多线程处理（简化实现）
    // 实际实现需要更复杂的任务分割和同步
    
    std::vector<std::unique_ptr<dsp_task>> tasks;
    
    // 为每个效果器创建任务
    for(auto& effect : effects_) {
        if(!effect || effect->is_bypassed()) {
            continue;
        }
        
        // 这里简化处理：每个效果器作为一个独立任务
        // 实际实现可能需要分割音频数据
        auto task = std::make_unique<dsp_task>(&chunk, &chunk, nullptr, &abort);
        tasks.push_back(std::move(task));
    }
    
    // 提交任务到线程池
    for(auto& task : tasks) {
        thread_pool_->submit_task(std::move(task));
    }
    
    // 等待所有任务完成
    thread_pool_->wait_for_completion();
    
    return true;
}

// 标准效果器工厂
std::unique_ptr<dsp_effect_advanced> dsp_manager::create_equalizer_10band() {
    auto params = create_default_eq_params();
    params.type = dsp_effect_type::equalizer;
    params.name = "10-Band Equalizer";
    params.description = "Professional 10-band parametric equalizer";
    
    auto eq = std::make_unique<dsp_equalizer_10band>(params);
    eq->load_iso_preset();
    
    std::cout << "[DSPManager] 创建10段均衡器" << std::endl;
    return eq;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_equalizer_31band() {
    auto params = create_default_eq_params();
    params.type = dsp_effect_type::equalizer;
    params.name = "31-Band Equalizer";
    params.description = "Professional 31-band graphic equalizer";
    
    auto eq = std::make_unique<dsp_equalizer_advanced>(params);
    
    // 添加31个ISO频段
    const float iso_31_freqs[31] = {
        20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f, 125.0f, 160.0f,
        200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f, 800.0f, 1000.0f, 1250.0f, 1600.0f,
        2000.0f, 2500.0f, 3150.0f, 4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f
    };
    
    for(size_t i = 0; i < 31; ++i) {
        eq_band_params band_params;
        band_params.frequency = iso_31_freqs[i];
        band_params.gain = 0.0f;
        band_params.bandwidth = 1.0f;
        band_params.type = filter_type::peak;
        band_params.is_enabled = true;
        band_params.name = "ISO_" + std::to_string(i);
        
        eq->add_band(band_params);
    }
    
    std::cout << "[DSPManager] 创建31段均衡器" << std::endl;
    return eq;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_reverb() {
    auto params = create_default_reverb_params();
    params.type = dsp_effect_type::reverb;
    params.name = "Reverb";
    params.description = "Professional reverb effect";
    params.latency_ms = 10.0; // 10ms延迟
    
    auto reverb = std::make_unique<dsp_reverb_advanced>(params);
    reverb->load_room_preset(0.5f); // 中等房间
    
    std::cout << "[DSPManager] 创建混响效果器" << std::endl;
    return reverb;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_compressor() {
    auto params = create_default_compressor_params();
    params.type = dsp_effect_type::compressor;
    params.name = "Compressor";
    params.description = "Dynamic range compressor";
    
    // 这里应该实现压缩器效果器
    // 目前返回一个占位符
    auto compressor = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 创建压缩器效果器" << std::endl;
    return compressor;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_limiter() {
    auto params = create_default_limiter_params();
    params.type = dsp_effect_type::limiter;
    params.name = "Limiter";
    params.description = "Peak limiter for loudness control";
    
    // 这里应该实现限制器效果器
    auto limiter = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 创建限制器效果器" << std::endl;
    return limiter;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_volume_control() {
    auto params = create_default_volume_params();
    params.type = dsp_effect_type::volume;
    params.name = "Volume Control";
    params.description = "Advanced volume control with limiting";
    
    // 这里应该实现高级音量控制效果器
    auto volume = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 创建音量控制效果器" << std::endl;
    return volume;
}

// 高级效果器
std::unique_ptr<dsp_effect_advanced> dsp_manager::create_convolver() {
    auto params = create_default_convolver_params();
    params.type = dsp_effect_type::convolver;
    params.name = "Convolver";
    params.description = "Impulse response convolver";
    params.latency_ms = 50.0; // 较高的延迟
    
    // 这里应该实现卷积效果器
    auto convolver = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 创建卷积效果器" << std::endl;
    return convolver;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_crossfeed() {
    auto params = create_default_crossfeed_params();
    params.type = dsp_effect_type::crossfeed;
    params.name = "Crossfeed";
    params.description = "Headphone crossfeed for natural sound";
    
    // 这里应该实现交叉馈送效果器
    auto crossfeed = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 创建交叉馈送效果器" << std::endl;
    return crossfeed;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_resampler(uint32_t target_rate) {
    auto params = create_default_resampler_params();
    params.type = dsp_effect_type::resampler;
    params.name = "Resampler";
    params.description = "High-quality audio resampler";
    
    // 这里应该实现重采样效果器
    auto resampler = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 创建重采样效果器，目标采样率: " << target_rate << std::endl;
    return resampler;
}

// 预设管理
bool dsp_manager::save_effect_preset(size_t effect_index, const std::string& preset_name) {
    if(effect_index >= effects_.size() || !preset_manager_) {
        return false;
    }
    
    auto* effect = effects_[effect_index].get();
    if(!effect) {
        return false;
    }
    
    // 创建预设
    auto preset = std::make_unique<dsp_preset_impl>(preset_name);
    effect->get_preset(*preset);
    
    // 保存预设
    return preset_manager_->save_preset(preset_name, *preset);
}

bool dsp_manager::load_effect_preset(size_t effect_index, const std::string& preset_name) {
    if(effect_index >= effects_.size() || !preset_manager_) {
        return false;
    }
    
    auto* effect = effects_[effect_index].get();
    if(!effect) {
        return false;
    }
    
    // 加载预设
    auto preset = std::make_unique<dsp_preset_impl>();
    if(!preset_manager_->load_preset(preset_name, *preset)) {
        return false;
    }
    
    // 应用预设
    effect->set_preset(*preset);
    return true;
}

std::vector<std::string> dsp_manager::get_available_presets() const {
    if(!preset_manager_) {
        return {};
    }
    return preset_manager_->get_preset_list();
}

// 性能监控
void dsp_manager::start_performance_monitoring() {
    if(performance_monitor_) {
        performance_monitor_->start_monitoring();
    }
}

void dsp_manager::stop_performance_monitoring() {
    if(performance_monitor_) {
        performance_monitor_->stop_monitoring();
    }
}

dsp_performance_stats dsp_manager::get_overall_stats() const {
    if(performance_monitor_) {
        return performance_monitor_->get_stats();
    }
    return dsp_performance_stats();
}

std::vector<dsp_performance_stats> dsp_manager::get_all_effect_stats() const {
    std::vector<dsp_performance_stats> stats;
    stats.reserve(effects_.size());
    
    for(const auto& effect : effects_) {
        if(effect) {
            stats.push_back(effect->get_performance_stats());
        }
    }
    
    return stats;
}

// 配置管理
void dsp_manager::update_config(const dsp_config& config) {
    std::cout << "[DSPManager] 更新配置" << std::endl;
    config_ = config;
    
    // 根据新配置调整系统
    if(config.enable_multithreading && !use_multithreading_) {
        // 启动多线程
        thread_pool_ = std::make_unique<multithreaded_dsp_processor>(config.max_threads);
        thread_pool_->start();
        use_multithreading_ = true;
    } else if(!config.enable_multithreading && use_multithreading_) {
        // 停止多线程
        thread_pool_->stop();
        use_multithreading_ = false;
    }
}

// 实用工具
float dsp_manager::estimate_total_cpu_usage() const {
    float total_usage = 0.0f;
    
    for(const auto& effect : effects_) {
        if(effect) {
            total_usage += effect->get_cpu_usage();
        }
    }
    
    return total_usage;
}

double dsp_manager::estimate_total_latency() const {
    double total_latency = 0.0;
    
    for(const auto& effect : effects_) {
        if(effect) {
            total_latency += effect->get_params().latency_ms;
        }
    }
    
    return total_latency;
}

bool dsp_manager::validate_dsp_chain() const {
    // 检查效果器链的有效性
    for(const auto& effect : effects_) {
        if(!effect || !effect->is_valid()) {
            return false;
        }
    }
    
    // 检查是否有重复的效果器类型
    std::set<dsp_effect_type> seen_types;
    for(const auto& effect : effects_) {
        if(effect) {
            if(seen_types.count(effect->get_params().type) > 0) {
                // 某些效果器类型不应该重复
                if(effect->get_params().type == dsp_effect_type::limiter) {
                    return false;
                }
            }
            seen_types.insert(effect->get_params().type);
        }
    }
    
    return true;
}

std::string dsp_manager::generate_dsp_report() const {
    std::stringstream report;
    report << "DSP系统报告:\n";
    report << "===================\n\n";
    
    // 系统信息
    report << "系统配置:\n";
    report << "  效果器数量: " << effects_.size() << "\n";
    report << "  多线程: " << (use_multithreading_ ? "启用" : "禁用") << "\n";
    report << "  性能监控: " << (performance_monitor_ ? "启用" : "禁用") << "\n";
    report << "  内存池: " << (config_.memory_pool_size / (1024*1024)) << "MB\n\n";
    
    // 效果器列表
    report << "效果器列表:\n";
    for(size_t i = 0; i < effects_.size(); ++i) {
        const auto* effect = effects_[i].get();
        if(effect) {
            report << "  [" << i << "] " << effect->get_name() << "\n";
            report << "      类型: " << static_cast<int>(effect->get_params().type) << "\n";
            report << "      延迟: " << effect->get_params().latency_ms << "ms\n";
            report << "      CPU占用: " << effect->get_cpu_usage() << "%\n";
            report << "      状态: " << (effect->is_enabled() ? "启用" : "禁用") 
                   << (effect->is_bypassed() ? " (旁路)" : "") << "\n\n";
        }
    }
    
    // 性能统计
    if(performance_monitor_ && performance_monitor_->is_monitoring()) {
        auto stats = performance_monitor_->get_stats();
        report << "性能统计:\n";
        report << "  总采样数: " << stats.total_samples_processed << "\n";
        report << "  总处理时间: " << stats.total_processing_time_ms << "ms\n";
        report << "  平均处理时间: " << stats.average_time_ms << "ms\n";
        report << "  CPU使用率: " << stats.cpu_usage_percent << "%\n";
        report << "  实时倍数: " << performance_monitor_->get_realtime_factor() << "x\n";
    }
    
    // 性能建议
    auto warnings = get_performance_warnings();
    if(!warnings.empty()) {
        report << "\n性能警告:\n";
        for(const auto& warning : warnings) {
            report << "  - " << warning << "\n";
        }
    }
    
    return report.str();
}

// 标准DSP效果器链
std::unique_ptr<dsp_chain> dsp_manager::create_standard_dsp_chain() {
    auto chain = std::make_unique<dsp_chain>();
    
    // 添加标准效果器
    chain->add_effect(service_ptr_t<dsp>(create_equalizer_10band()));
    chain->add_effect(service_ptr_t<dsp>(create_volume_control()));
    
    std::cout << "[DSPManager] 创建标准DSP链" << std::endl;
    return chain;
}

// 加载标准效果器
void dsp_manager::load_standard_effects() {
    std::cout << "[DSPManager] 加载标准效果器..." << std::endl;
    
    // 添加基础效果器
    add_effect(create_equalizer_10band());
    add_effect(create_reverb());
    add_effect(create_volume_control());
    
    std::cout << "[DSPManager] 标准效果器加载完成（数量: " << effects_.size() << ")" << std::endl;
}

// 辅助函数
dsp_effect_params dsp_manager::create_default_eq_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::equalizer;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 5.0f; // 估算5% CPU占用
    params.latency_ms = 0.1; // 0.1ms延迟
    
    // 配置参数
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"gain", "Overall Gain", 0.0f, -24.0f, 24.0f, 0.1f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_reverb_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::reverb;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 15.0f; // 估算15% CPU占用
    params.latency_ms = 10.0; // 10ms延迟
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"room_size", "Room Size", 0.5f, 0.0f, 1.0f, 0.01f},
        {"damping", "Damping", 0.5f, 0.0f, 1.0f, 0.01f},
        {"wet_level", "Wet Level", 0.3f, 0.0f, 1.0f, 0.01f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_compressor_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::compressor;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 8.0f;
    params.latency_ms = 2.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"threshold", "Threshold", -20.0f, -60.0f, 0.0f, 1.0f},
        {"ratio", "Ratio", 4.0f, 1.0f, 20.0f, 0.1f},
        {"attack", "Attack", 10.0f, 0.1f, 100.0f, 0.1f},
        {"release", "Release", 100.0f, 10.0f, 1000.0f, 1.0f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_limiter_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::limiter;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 10.0f;
    params.latency_ms = 5.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"threshold", "Threshold", -3.0f, -20.0f, 0.0f, 0.1f},
        {"release", "Release", 50.0f, 1.0f, 500.0f, 1.0f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_volume_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::volume;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 1.0f;
    params.latency_ms = 0.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"volume", "Volume", 0.0f, -60.0f, 12.0f, 0.1f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_convolver_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::convolver;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 30.0f;
    params.latency_ms = 50.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"impulse_response", "Impulse Response", 0.0f, 0.0f, 1.0f, 1.0f},
        {"mix", "Mix Level", 0.5f, 0.0f, 1.0f, 0.01f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_crossfeed_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::crossfeed;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 3.0f;
    params.latency_ms = 1.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"intensity", "Crossfeed Intensity", 0.5f, 0.0f, 1.0f, 0.01f},
        {"frequency", "Crossfeed Frequency", 700.0f, 100.0f, 2000.0f, 10.0f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_resampler_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::resampler;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 20.0f;
    params.latency_ms = 20.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"target_rate", "Target Sample Rate", 48000.0f, 8000.0f, 192000.0f, 100.0f},
        {"quality", "Resampling Quality", 0.8f, 0.0f, 1.0f, 0.01f}
    };
    
    return params;
}

std::vector<std::string> dsp_manager::get_performance_warnings() const {
    std::vector<std::string> warnings;
    
    if(!performance_monitor_) {
        return warnings;
    }
    
    auto stats = performance_monitor_->get_stats();
    
    // CPU使用率过高
    if(stats.cpu_usage_percent > config_.target_cpu_usage * 1.5f) {
        warnings.push_back("CPU使用率过高，考虑减少效果器或优化算法");
    }
    
    // 延迟过高
    if(estimate_total_latency() > config_.max_latency_ms * 1.5f) {
        warnings.push_back("总延迟过高，考虑减少效果器数量");
    }
    
    // 内存使用过高
    // 这里可以添加内存使用检查
    
    // 错误率过高
    if(stats.error_count > 0) {
        warnings.push_back("处理过程中出现错误，建议检查效果器配置");
    }
    
    return warnings;
}

} // namespace fb2k