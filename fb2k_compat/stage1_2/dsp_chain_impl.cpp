#include "dsp_interfaces.h"
#include <algorithm>
#include <numeric>

namespace fb2k {

// dsp_chain 瀹屾暣瀹炵幇锛堝凡鍦ㄥご鏂囦欢涓儴鍒嗗疄鐜帮級
// 杩欓噷鏄畬鏁寸殑DSP閾剧鐞嗗疄鐜?

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
    
    // 纭繚鎵€鏈夋晥鏋滃櫒閮藉凡瀹炰緥鍖?
    for(auto& effect : effects_) {
        if(effect.is_valid()) {
            if(!effect->instantiate(chunk, chunk.get_sample_rate(), chunk.get_channels())) {
                return; // 瀹炰緥鍖栧け璐?
            }
        }
    }
    
    // 澶勭悊闊抽鏁版嵁
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

// 楂樼骇DSP閾惧姛鑳?

void dsp_chain::optimize_chain() {
    // 绉婚櫎鏃犳晥鐨勬晥鏋滃櫒
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
                      [](const service_ptr_t<dsp>& effect) {
                          return !effect.is_valid() || !effect->is_valid();
                      }),
        effects_.end()
    );
    
    // 鍙互鍦ㄨ繖閲屾坊鍔犳洿澶氱殑浼樺寲閫昏緫
    // 渚嬪锛氬悎骞剁浉浼肩殑鏁堟灉鍣紝閲嶆柊鎺掑簭绛?
}

bool dsp_chain::reorder_effects(const std::vector<size_t>& new_order) {
    if(new_order.size() != effects_.size()) {
        return false;
    }
    
    // 楠岃瘉鏂伴『搴忕殑鏈夋晥鎬?
    std::vector<bool> used(effects_.size(), false);
    for(size_t index : new_order) {
        if(index >= effects_.size() || used[index]) {
            return false;
        }
        used[index] = true;
    }
    
    // 鍒涘缓鏂伴『搴忕殑鏁堟灉鍣ㄥ垪琛?
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
    
    // 杩欓噷鍙互瀹炵幇灏咲SP閾鹃厤缃鍑轰负棰勮鐨勯€昏緫
    // 鍖呮嫭鎵€鏈夋晥鏋滃櫒鐨勫弬鏁板拰椤哄簭
    
    // 绠€鍖栧疄鐜帮細鍙鍑哄熀鏈俊鎭?
    preset->set_parameter_string("chain_info", get_dsp_chain_info(*this).c_str());
    
    return preset;
}

bool dsp_chain::import_chain_preset(const dsp_preset& preset) {
    // 杩欓噷鍙互瀹炵幇浠庨璁惧鍏SP閾鹃厤缃殑閫昏緫
    // 鍖呮嫭鍒涘缓鏁堟灉鍣ㄥ疄渚嬪拰璁剧疆鍙傛暟
    
    // 绠€鍖栧疄鐜帮細鍙鍏ュ熀鏈俊鎭?
    const char* info = preset.get_parameter_string("chain_info");
    if(info) {
        // 瑙ｆ瀽淇℃伅骞堕噸寤篋SP閾?
        // 瀹為檯瀹炵幇闇€瑕佸畬鏁寸殑瑙ｆ瀽閫昏緫
        return true;
    }
    
    return false;
}

// DSP鎬ц兘鍒嗘瀽鍣?
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

// DSP閾炬瀯寤哄櫒 - 绠€鍖朌SP閾剧殑鏋勫缓
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
        
        // 杩欓噷闇€瑕佹牴鎹晥鏋滃櫒鍚嶇О鍒涘缓鍏蜂綋鐨凞SP瀹炰緥
        // 骞跺簲鐢ㄩ璁惧弬鏁?
        
        // 绠€鍖栧疄鐜帮細鍙褰曢厤缃?
        for(const auto& [name, preset] : effects_) {
            // 瀹為檯瀹炵幇闇€瑕佸垱寤哄搴旂殑DSP瀹炰緥
            printf("DSP Chain Builder: Adding %s effect\n", name.c_str());
        }
        
        return chain;
    }
    
    void clear() {
        effects_.clear();
    }
};

// DSP閾鹃獙璇佸櫒
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
        
        // 妫€鏌ョ┖閾?
        if(chain.get_effect_count() == 0) {
            result.warnings.push_back("DSP閾句负绌猴紝娌℃湁鍚敤浠讳綍鏁堟灉");
            return result;
        }
        
        // 妫€鏌ユ棤鏁堟晥鏋滃櫒
        for(size_t i = 0; i < chain.get_effect_count(); ++i) {
            dsp* effect = chain.get_effect(i);
            if(!effect) {
                result.is_valid = false;
                result.error_message = "DSP閾句腑鍖呭惈绌虹殑鏁堟灉鍣?;
                return result;
            }
            
            if(!effect->is_valid()) {
                result.is_valid = false;
                result.error_message = "DSP閾句腑鍖呭惈鏃犳晥鐨勬晥鏋滃櫒: " + std::string(effect->get_name());
                return result;
            }
        }
        
        // 妫€鏌ュ欢杩熺疮绉?
        double total_latency = chain.get_total_latency();
        if(total_latency > 100.0) { // 100ms闃堝€?
            result.warnings.push_back("DSP閾炬€诲欢杩熻繃楂? " + std::to_string(total_latency) + " ms");
        }
        
        // 妫€鏌ラ煶杞ㄥ彉鍖栨爣璁?
        if(chain.need_track_change_mark()) {
            result.warnings.push_back("DSP閾鹃渶瑕侀煶杞ㄥ彉鍖栨爣璁帮紝鍙兘褰卞搷鏃犵紳鎾斁");
        }
        
        return result;
    }
    
    static bool validate_dsp_effect(const dsp& effect) {
        // 鍩虹楠岃瘉
        if(!effect.is_valid()) {
            return false;
        }
        
        // 妫€鏌ュ繀闇€鐨勬柟娉?
        // 杩欓噷鍙互娣诲姞鏇村鐨勯獙璇侀€昏緫
        
        return true;
    }
};

} // namespace fb2k