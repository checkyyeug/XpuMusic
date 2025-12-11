#include "dsp_interfaces.h"
#include <algorithm>
#include <numeric>

namespace fb2k {

// dsp_chain 完整实现（已在头文件中部分实现）
// 这里是完整的DSP链管理实现

dsp_chain::dsp_chain() : sample_rate_(44100), channels_(2) {}

void dsp_chain::add_effect(service_ptr_t<dsp> effect) {
    if(effect.is_valid()) {
        effects_.push_back(effect);
    }
}

void dsp_chain::remove_effect(size_t index) {
    if(index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void dsp_chain::clear_effects() {
    effects_.clear();
    temp_buffers_.clear();
}

size_t dsp_chain::get_effect_count() const {
    return effects_.size();
}

dsp* dsp_chain::get_effect(size_t index) {
    return index < effects_.size() ? effects_[index].get() : nullptr;
}

void dsp_chain::run_chain(audio_chunk& chunk, abort_callback& abort) {
    if(effects_.empty() || abort.is_aborting()) {
        return;
    }
    
    // 确保所有效果器都已实例化
    for(auto& effect : effects_) {
        if(effect.is_valid()) {
            if(!effect->instantiate(chunk, chunk.get_sample_rate(), chunk.get_channels())) {
                return; // 实例化失败
            }
        }
    }
    
    // 处理音频数据
    audio_chunk* current_chunk = &chunk;
    
    for(size_t i = 0; i < effects_.size() && !abort.is_aborting(); ++i) {
        if(effects_[i].is_valid()) {
            effects_[i]->run(*current_chunk, abort);
        }
    }
}

void dsp_chain::reset_all() {
    for(auto& effect : effects_) {
        if(effect.is_valid()) {
            effect->reset();
        }
    }
}

double dsp_chain::get_total_latency() const {
    double total_latency = 0.0;
    for(const auto& effect : effects_) {
        if(effect.is_valid()) {
            total_latency += effect->get_latency();
        }
    }
    return total_latency;
}

bool dsp_chain::need_track_change_mark() const {
    for(const auto& effect : effects_) {
        if(effect.is_valid() && effect->need_track_change_mark()) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> dsp_chain::get_effect_names() const {
    std::vector<std::string> names;
    for(const auto& effect : effects_) {
        if(effect.is_valid()) {
            names.push_back(effect->get_name());
        }
    }
    return names;
}

// 高级DSP链功能

void dsp_chain::optimize_chain() {
    // 移除无效的效果器
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
                      [](const service_ptr_t<dsp>& effect) {
                          return !effect.is_valid() || !effect->is_valid();
                      }),
        effects_.end()
    );
    
    // 可以在这里添加更多的优化逻辑
    // 例如：合并相似的效果器，重新排序等
}

bool dsp_chain::reorder_effects(const std::vector<size_t>& new_order) {
    if(new_order.size() != effects_.size()) {
        return false;
    }
    
    // 验证新顺序的有效性
    std::vector<bool> used(effects_.size(), false);
    for(size_t index : new_order) {
        if(index >= effects_.size() || used[index]) {
            return false;
        }
        used[index] = true;
    }
    
    // 创建新顺序的效果器列表
    std::vector<service_ptr_t<dsp>> new_effects;
    new_effects.reserve(effects_.size());
    
    for(size_t index : new_order) {
        new_effects.push_back(effects_[index]);
    }
    
    effects_ = std::move(new_effects);
    return true;
}

std::unique_ptr<dsp_preset> dsp_chain::export_chain_preset(const char* name) const {
    auto preset = std::make_unique<dsp_preset_impl>(name);
    
    // 这里可以实现将DSP链配置导出为预设的逻辑
    // 包括所有效果器的参数和顺序
    
    // 简化实现：只导出基本信息
    preset->set_parameter_string("chain_info", get_dsp_chain_info(*this).c_str());
    
    return preset;
}

bool dsp_chain::import_chain_preset(const dsp_preset& preset) {
    // 这里可以实现从预设导入DSP链配置的逻辑
    // 包括创建效果器实例和设置参数
    
    // 简化实现：只导入基本信息
    const char* info = preset.get_parameter_string("chain_info");
    if(info) {
        // 解析信息并重建DSP链
        // 实际实现需要完整的解析逻辑
        return true;
    }
    
    return false;
}

// DSP性能分析器
class dsp_performance_analyzer {
private:
    struct effect_stats {
        size_t call_count = 0;
        double total_time_ms = 0.0;
        double avg_time_ms = 0.0;
        double min_time_ms = 0.0;
        double max_time_ms = 0.0;
    };
    
    std::map<std::string, effect_stats> stats_;
    
public:
    void record_effect_call(const std::string& effect_name, double time_ms) {
        auto& stat = stats_[effect_name];
        stat.call_count++;
        stat.total_time_ms += time_ms;
        
        if(stat.call_count == 1) {
            stat.min_time_ms = time_ms;
            stat.max_time_ms = time_ms;
        } else {
            stat.min_time_ms = std::min(stat.min_time_ms, time_ms);
            stat.max_time_ms = std::max(stat.max_time_ms, time_ms);
        }
        
        stat.avg_time_ms = stat.total_time_ms / stat.call_count;
    }
    
    std::string generate_performance_report() const {
        std::string report = "DSP Performance Report:\n";
        
        for(const auto& [name, stat] : stats_) {
            report += "  " + name + ":\n";
            report += "    Calls: " + std::to_string(stat.call_count) + "\n";
            report += "    Avg Time: " + std::to_string(stat.avg_time_ms) + " ms\n";
            report += "    Min Time: " + std::to_string(stat.min_time_ms) + " ms\n";
            report += "    Max Time: " + std::to_string(stat.max_time_ms) + " ms\n";
            report += "    Total Time: " + std::to_string(stat.total_time_ms) + " ms\n";
        }
        
        return report;
    }
    
    void reset_stats() {
        stats_.clear();
    }
};

// DSP链构建器 - 简化DSP链的构建
class dsp_chain_builder {
private:
    std::vector<std::pair<std::string, std::unique_ptr<dsp_preset>>> effects_;
    
public:
    dsp_chain_builder& add_equalizer(const std::vector<float>& bands) {
        auto preset = dsp_config_helper::create_equalizer_preset("Equalizer", bands);
        effects_.emplace_back("Equalizer", std::move(preset));
        return *this;
    }
    
    dsp_chain_builder& add_volume(float volume) {
        auto preset = dsp_config_helper::create_volume_preset(volume);
        effects_.emplace_back("Volume", std::move(preset));
        return *this;
    }
    
    dsp_chain_builder& add_effect(const std::string& name, 
                                 std::unique_ptr<dsp_preset> preset) {
        effects_.emplace_back(name, std::move(preset));
        return *this;
    }
    
    std::unique_ptr<dsp_chain> build() {
        auto chain = std::make_unique<dsp_chain>();
        
        // 这里需要根据效果器名称创建具体的DSP实例
        // 并应用预设参数
        
        // 简化实现：只记录配置
        for(const auto& [name, preset] : effects_) {
            // 实际实现需要创建对应的DSP实例
            printf("DSP Chain Builder: Adding %s effect\n", name.c_str());
        }
        
        return chain;
    }
    
    void clear() {
        effects_.clear();
    }
};

// DSP链验证器
class dsp_chain_validator {
public:
    struct validation_result {
        bool is_valid;
        std::string error_message;
        std::vector<std::string> warnings;
    };
    
    static validation_result validate_chain(const dsp_chain& chain) {
        validation_result result;
        result.is_valid = true;
        
        // 检查空链
        if(chain.get_effect_count() == 0) {
            result.warnings.push_back("DSP链为空，没有启用任何效果");
            return result;
        }
        
        // 检查无效效果器
        for(size_t i = 0; i < chain.get_effect_count(); ++i) {
            dsp* effect = chain.get_effect(i);
            if(!effect) {
                result.is_valid = false;
                result.error_message = "DSP链中包含空的效果器";
                return result;
            }
            
            if(!effect->is_valid()) {
                result.is_valid = false;
                result.error_message = "DSP链中包含无效的效果器: " + std::string(effect->get_name());
                return result;
            }
        }
        
        // 检查延迟累积
        double total_latency = chain.get_total_latency();
        if(total_latency > 100.0) { // 100ms阈值
            result.warnings.push_back("DSP链总延迟过高: " + std::to_string(total_latency) + " ms");
        }
        
        // 检查音轨变化标记
        if(chain.need_track_change_mark()) {
            result.warnings.push_back("DSP链需要音轨变化标记，可能影响无缝播放");
        }
        
        return result;
    }
    
    static bool validate_dsp_effect(const dsp& effect) {
        // 基础验证
        if(!effect.is_valid()) {
            return false;
        }
        
        // 检查必需的方法
        // 这里可以添加更多的验证逻辑
        
        return true;
    }
};

} // namespace fb2k