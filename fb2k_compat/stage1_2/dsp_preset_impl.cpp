#include "dsp_interfaces.h"
#include <cstring>
#include <algorithm>

namespace fb2k {

// dsp_preset_impl 具体实现（已在头文件中部分实现）
// 这里是完整的序列化/反序列化实现

void dsp_preset_impl::serialize(std::vector<uint8_t>& data) const {
    data.clear();
    
    // 二进制序列化格式:
    // [magic:4][version:4][name_len:4][name][float_count:4][float_params][string_count:4][string_params]
    
    const uint32_t magic = 0x46504244; // "FPBD" (Foobar2000 DSP Binary Data)
    const uint32_t version = 1;
    const uint32_t name_len = static_cast<uint32_t>(name_.length());
    
    // 魔数和版本
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&magic), 
               reinterpret_cast<const uint8_t*>(&magic) + sizeof(magic));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&version),
               reinterpret_cast<const uint8_t*>(&version) + sizeof(version));
    
    // 名称
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&name_len),
               reinterpret_cast<const uint8_t*>(&name_len) + sizeof(name_len));
    data.insert(data.end(), name_.begin(), name_.end());
    
    // 浮点参数
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
    
    // 字符串参数
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
    
    reset(); // 先重置
    
    if(size < 12) return false; // 最小大小：magic + version + name_len
    
    size_t pos = 0;
    
    // 读取魔数和版本
    uint32_t magic, version;
    std::memcpy(&magic, data + pos, sizeof(magic));
    pos += sizeof(magic);
    
    std::memcpy(&version, data + pos, sizeof(version));
    pos += sizeof(version);
    
    // 验证魔数
    if(magic != 0x46504244) return false; // "FPBD"
    if(version != 1) return false;
    
    // 读取名称
    uint32_t name_len;
    std::memcpy(&name_len, data + pos, sizeof(name_len));
    pos += sizeof(name_len);
    
    if(pos + name_len > size) return false;
    
    name_.assign(reinterpret_cast<const char*>(data + pos), name_len);
    pos += name_len;
    
    // 读取浮点参数
    uint32_t float_count;
    if(pos + sizeof(float_count) > size) return false;
    std::memcpy(&float_count, data + pos, sizeof(float_count));
    pos += sizeof(float_count);
    
    for(uint32_t i = 0; i < float_count; ++i) {
        // 读取参数名长度
        uint32_t key_len;
        if(pos + sizeof(key_len) > size) return false;
        std::memcpy(&key_len, data + pos, sizeof(key_len));
        pos += sizeof(key_len);
        
        if(pos + key_len > size) return false;
        std::string key(reinterpret_cast<const char*>(data + pos), key_len);
        pos += key_len;
        
        // 读取参数值
        float value;
        if(pos + sizeof(value) > size) return false;
        std::memcpy(&value, data + pos, sizeof(value));
        pos += sizeof(value);
        
        float_params_[key] = value;
    }
    
    // 读取字符串参数
    uint32_t string_count;
    if(pos + sizeof(string_count) > size) return false;
    std::memcpy(&string_count, data + pos, sizeof(string_count));
    pos += sizeof(string_count);
    
    for(uint32_t i = 0; i < string_count; ++i) {
        // 读取参数名长度
        uint32_t key_len;
        if(pos + sizeof(key_len) > size) return false;
        std::memcpy(&key_len, data + pos, sizeof(key_len));
        pos += sizeof(key_len);
        
        if(pos + key_len > size) return false;
        std::string key(reinterpret_cast<const char*>(data + pos), key_len);
        pos += key_len;
        
        // 读取参数值长度
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

// dsp_chain 实现
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

// dsp_config_helper 实现
std::unique_ptr<dsp_preset> dsp_config_helper::create_basic_preset(const char* name) {
    return std::make_unique<dsp_preset_impl>(name ? name : "Basic");
}

std::unique_ptr<dsp_preset> dsp_config_helper::create_equalizer_preset(
    const std::string& name, 
    const std::vector<float>& bands) {
    
    auto preset = std::make_unique<dsp_preset_impl>(name);
    
    // 设置均衡器参数
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
            return false; // 缺少必需参数
        }
        
        float value = preset.get_parameter_float(param.name.c_str());
        if(value < param.min_value || value > param.max_value) {
            return false; // 参数值超出范围
        }
    }
    
    return true;
}

// dsp_utils 实现
float dsp_utils::estimate_cpu_usage(const dsp_chain& chain) {
    float total_usage = 0.0f;
    
    for(size_t i = 0; i < chain.get_effect_count(); ++i) {
        dsp* effect = chain.get_effect(i);
        if(effect) {
            // 这里需要根据具体DSP效果估算CPU占用
            // 简化实现：每个效果基础占用1%
            total_usage += 1.0f;
        }
    }
    
    return total_usage;
}

bool dsp_utils::validate_dsp_chain(const dsp_chain& chain) {
    for(size_t i = 0; i < chain.get_effect_count(); ++i) {
        dsp* effect = chain.get_effect(i);
        if(!effect) {
            return false; // 有空效果器
        }
        
        if(!effect->is_valid()) {
            return false; // 效果器无效
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