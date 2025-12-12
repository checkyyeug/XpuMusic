#include "dsp_interfaces.h"
#include <cstring>
#include <algorithm>

namespace fb2k {

// dsp_preset_impl 鍏蜂綋瀹炵幇锛堝凡鍦ㄥご鏂囦欢涓儴鍒嗗疄鐜帮級
// 杩欓噷鏄畬鏁寸殑搴忓垪鍖?鍙嶅簭鍒楀寲瀹炵幇

void dsp_preset_impl::serialize(std::vector<uint8_t>& data) const {
    data.clear();
    
    // 浜岃繘鍒跺簭鍒楀寲鏍煎紡:
    // [magic:4][version:4][name_len:4][name][float_count:4][float_params][string_count:4][string_params]
    
    const uint32_t magic = 0x46504244; // "FPBD" (Foobar2000 DSP Binary Data)
    const uint32_t version = 1;
    const uint32_t name_len = static_cast<uint32_t>(name_.length());
    
    // 榄旀暟鍜岀増鏈?
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&magic), 
               reinterpret_cast<const uint8_t*>(&magic) + sizeof(magic));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&version),
               reinterpret_cast<const uint8_t*>(&version) + sizeof(version));
    
    // 鍚嶇О
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&name_len),
               reinterpret_cast<const uint8_t*>(&name_len) + sizeof(name_len));
    data.insert(data.end(), name_.begin(), name_.end());
    
    // 娴偣鍙傛暟
    const uint32_t float_count = static_cast<uint32_t>(float_params_.size());
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&float_count),
               reinterpret_cast<const uint8_t*>(&float_count) + sizeof(float_count));
    
    for(const auto& [key, value] : float_params_) {
        const uint32_t key_len = static_cast<uint32_t>(key.length());
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&key_len),
                   reinterpret_cast<const uint8_t*>(&key_len) + sizeof(key_len));
        data.insert(data.end(), key.begin(), key.end());
        
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value),
                   reinterpret_cast<const uint8_t*>(&value) + sizeof(value));
    }
    
    // 瀛楃涓插弬鏁?
    const uint32_t string_count = static_cast<uint32_t>(string_params_.size());
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&string_count),
               reinterpret_cast<const uint8_t*>(&string_count) + sizeof(string_count));
    
    for(const auto& [key, value] : string_params_) {
        const uint32_t key_len = static_cast<uint32_t>(key.length());
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&key_len),
                   reinterpret_cast<const uint8_t*>(&key_len) + sizeof(key_len));
        data.insert(data.end(), key.begin(), key.end());
        
        const uint32_t value_len = static_cast<uint32_t>(value.length());
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value_len),
                   reinterpret_cast<const uint8_t*>(&value_len) + sizeof(value_len));
        data.insert(data.end(), value.begin(), value.end());
    }
}

bool dsp_preset_impl::deserialize(const uint8_t* data, size_t size) {
    if(!data || size == 0) return false;
    
    reset(); // 鍏堥噸缃?
    
    if(size < 12) return false; // 鏈€灏忓ぇ灏忥細magic + version + name_len
    
    size_t pos = 0;
    
    // 璇诲彇榄旀暟鍜岀増鏈?
    uint32_t magic, version;
    std::memcpy(&magic, data + pos, sizeof(magic));
    pos += sizeof(magic);
    
    std::memcpy(&version, data + pos, sizeof(version));
    pos += sizeof(version);
    
    // 楠岃瘉榄旀暟
    if(magic != 0x46504244) return false; // "FPBD"
    if(version != 1) return false;
    
    // 璇诲彇鍚嶇О
    uint32_t name_len;
    std::memcpy(&name_len, data + pos, sizeof(name_len));
    pos += sizeof(name_len);
    
    if(pos + name_len > size) return false;
    
    name_.assign(reinterpret_cast<const char*>(data + pos), name_len);
    pos += name_len;
    
    // 璇诲彇娴偣鍙傛暟
    uint32_t float_count;
    if(pos + sizeof(float_count) > size) return false;
    std::memcpy(&float_count, data + pos, sizeof(float_count));
    pos += sizeof(float_count);
    
    for(uint32_t i = 0; i < float_count; ++i) {
        // 璇诲彇鍙傛暟鍚嶉暱搴?
        uint32_t key_len;
        if(pos + sizeof(key_len) > size) return false;
        std::memcpy(&key_len, data + pos, sizeof(key_len));
        pos += sizeof(key_len);
        
        if(pos + key_len > size) return false;
        std::string key(reinterpret_cast<const char*>(data + pos), key_len);
        pos += key_len;
        
        // 璇诲彇鍙傛暟鍊?
        float value;
        if(pos + sizeof(value) > size) return false;
        std::memcpy(&value, data + pos, sizeof(value));
        pos += sizeof(value);
        
        float_params_[key] = value;
    }
    
    // 璇诲彇瀛楃涓插弬鏁?
    uint32_t string_count;
    if(pos + sizeof(string_count) > size) return false;
    std::memcpy(&string_count, data + pos, sizeof(string_count));
    pos += sizeof(string_count);
    
    for(uint32_t i = 0; i < string_count; ++i) {
        // 璇诲彇鍙傛暟鍚嶉暱搴?
        uint32_t key_len;
        if(pos + sizeof(key_len) > size) return false;
        std::memcpy(&key_len, data + pos, sizeof(key_len));
        pos += sizeof(key_len);
        
        if(pos + key_len > size) return false;
        std::string key(reinterpret_cast<const char*>(data + pos), key_len);
        pos += key_len;
        
        // 璇诲彇鍙傛暟鍊奸暱搴?
        uint32_t value_len;
        if(pos + sizeof(value_len) > size) return false;
        std::memcpy(&value_len, data + pos, sizeof(value_len));
        pos += sizeof(value_len);
        
        if(pos + value_len > size) return false;
        std::string value(reinterpret_cast<const char*>(data + pos), value_len);
        pos += value_len;
        
        string_params_[key] = value;
    }
    
    is_valid_ = true;
    return true;
}

// dsp_chain 瀹炵幇
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

// dsp_config_helper 瀹炵幇
std::unique_ptr<dsp_preset> dsp_config_helper::create_basic_preset(const char* name) {
    return std::make_unique<dsp_preset_impl>(name ? name : "Basic");
}

std::unique_ptr<dsp_preset> dsp_config_helper::create_equalizer_preset(
    const std::string& name, 
    const std::vector<float>& bands) {
    
    auto preset = std::make_unique<dsp_preset_impl>(name);
    
    // 璁剧疆鍧囪　鍣ㄥ弬鏁?
    for(size_t i = 0; i < bands.size(); ++i) {
        std::string param_name = "band_" + std::to_string(i);
        preset->set_parameter_float(param_name.c_str(), bands[i]);
    }
    
    return preset;
}

std::unique_ptr<dsp_preset> dsp_config_helper::create_volume_preset(float volume) {
    auto preset = std::make_unique<dsp_preset_impl>("Volume");
    preset->set_parameter_float("volume", volume);
    return preset;
}

bool dsp_config_helper::validate_dsp_config(const dsp& effect, const dsp_preset& preset) {
    auto params = effect.get_config_params();
    
    for(const auto& param : params) {
        if(!preset.has_parameter(param.name.c_str())) {
            return false; // 缂哄皯蹇呴渶鍙傛暟
        }
        
        float value = preset.get_parameter_float(param.name.c_str());
        if(value < param.min_value || value > param.max_value) {
            return false; // 鍙傛暟鍊艰秴鍑鸿寖鍥?
        }
    }
    
    return true;
}

// dsp_utils 瀹炵幇
float dsp_utils::estimate_cpu_usage(const dsp_chain& chain) {
    float total_usage = 0.0f;
    
    for(size_t i = 0; i < chain.get_effect_count(); ++i) {
        dsp* effect = chain.get_effect(i);
        if(effect) {
            // 杩欓噷闇€瑕佹牴鎹叿浣揇SP鏁堟灉浼扮畻CPU鍗犵敤
            // 绠€鍖栧疄鐜帮細姣忎釜鏁堟灉鍩虹鍗犵敤1%
            total_usage += 1.0f;
        }
    }
    
    return total_usage;
}

bool dsp_utils::validate_dsp_chain(const dsp_chain& chain) {
    for(size_t i = 0; i < chain.get_effect_count(); ++i) {
        dsp* effect = chain.get_effect(i);
        if(!effect) {
            return false; // 鏈夌┖鏁堟灉鍣?
        }
        
        if(!effect->is_valid()) {
            return false; // 鏁堟灉鍣ㄦ棤鏁?
        }
    }
    
    return true;
}

std::string dsp_utils::get_dsp_chain_info(const dsp_chain& chain) {
    std::string info = "DSP Chain Info:\n";
    info += "  Effect Count: " + std::to_string(chain.get_effect_count()) + "\n";
    info += "  Total Latency: " + std::to_string(chain.get_total_latency()) + " ms\n";
    info += "  Need Track Change: " + std::string(chain.need_track_change_mark() ? "Yes" : "No") + "\n";
    
    auto names = chain.get_effect_names();
    info += "  Effects:\n";
    for(const auto& name : names) {
        info += "    - " + name + "\n";
    }
    
    return info;
}