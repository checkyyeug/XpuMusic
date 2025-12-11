#pragma once

// 阶段1.2：DSP接口定义
// DSP效果器系统接口，符合foobar2000规范

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "audio_chunk.h"
#include "../stage1_1/real_minihost.h"

namespace fb2k {

// DSP配置参数
struct dsp_config_param {
    std::string name;
    std::string description;
    float default_value;
    float min_value;
    float max_value;
    float step_value;
    
    dsp_config_param(const std::string& n, const std::string& desc, 
                    float def, float min, float max, float step)
        : name(n), description(desc), default_value(def), 
          min_value(min), max_value(max), step_value(step) {}
};

// DSP预设接口 - 符合foobar2000规范
class dsp_preset : public ServiceBase {
public:
    // 基础管理
    virtual void reset() = 0;
    virtual bool is_valid() const = 0;
    virtual void copy(const dsp_preset& source) = 0;
    
    // 名称管理
    virtual const char* get_name() const = 0;
    virtual void set_name(const char* name) = 0;
    
    // 参数管理
    virtual bool has_parameter(const char* name) const = 0;
    virtual float get_parameter_float(const char* name) const = 0;
    virtual void set_parameter_float(const char* name, float value) = 0;
    virtual const char* get_parameter_string(const char* name) const = 0;
    virtual void set_parameter_string(const char* name, const char* value) = 0;
    
    // 序列化
    virtual void serialize(std::vector<uint8_t>& data) const = 0;
    virtual bool deserialize(const uint8_t* data, size_t size) = 0;
    
    // 比较
    virtual bool operator==(const dsp_preset& other) const = 0;
    virtual bool operator!=(const dsp_preset& other) const = 0;
};

// DSP效果器接口 - 符合foobar2000规范
class dsp : public ServiceBase {
public:
    // 生命周期管理
    virtual bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                            uint32_t channels) = 0;
    virtual void reset() = 0;
    
    // 音频处理
    virtual void run(audio_chunk& chunk, abort_callback& abort) = 0;
    
    // 配置管理
    virtual void get_preset(dsp_preset& preset) const = 0;
    virtual void set_preset(const dsp_preset& preset) = 0;
    
    // 参数信息
    virtual std::vector<dsp_config_param> get_config_params() const = 0;
    
    // 状态信息
    virtual bool need_track_change_mark() const = 0;
    virtual double get_latency() const = 0;
    virtual const char* get_name() const = 0;
    virtual const char* get_description() const = 0;
    
    // 能力查询
    virtual bool can_work_with(const audio_chunk& chunk) const = 0;
    virtual bool supports_format(uint32_t sample_rate, uint32_t channels) const = 0;
};

// DSP预设具体实现
class dsp_preset_impl : public dsp_preset {
private:
    std::string name_;
    std::map<std::string, float> float_params_;
    std::map<std::string, std::string> string_params_;
    bool is_valid_;
    
public:
    dsp_preset_impl() : is_valid_(false) {}
    explicit dsp_preset_impl(const std::string& name) : name_(name), is_valid_(true) {}
    
    // IUnknown实现
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, __uuidof(dsp_preset))) {
            *ppvObject = static_cast<dsp_preset*>(this);
            return S_OK;
        }
        return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
    }
    
    // 基础管理
    void reset() override {
        name_.clear();
        float_params_.clear();
        string_params_.clear();
        is_valid_ = false;
    }
    
    bool is_valid() const override {
        return is_valid_;
    }
    
    void copy(const dsp_preset& source) override {
        if(&source == this) return;
        
        name_ = source.get_name();
        is_valid_ = source.is_valid();
        
        // 复制参数（简化实现）
        // 实际实现需要遍历source的所有参数
    }
    
    // 名称管理
    const char* get_name() const override {
        return name_.c_str();
    }
    
    void set_name(const char* name) override {
        if(name) name_ = name;
    }
    
    // 参数管理
    bool has_parameter(const char* name) const override {
        if(!name) return false;
        std::string param_name(name);
        return float_params_.find(param_name) != float_params_.end() ||
               string_params_.find(param_name) != string_params_.end();
    }
    
    float get_parameter_float(const char* name) const override {
        if(!name) return 0.0f;
        std::string param_name(name);
        auto it = float_params_.find(param_name);
        return it != float_params_.end() ? it->second : 0.0f;
    }
    
    void set_parameter_float(const char* name, float value) override {
        if(!name) return;
        float_params_[name] = value;
    }
    
    const char* get_parameter_string(const char* name) const override {
        if(!name) return "";
        std::string param_name(name);
        auto it = string_params_.find(param_name);
        return it != string_params_.end() ? it->second.c_str() : "";
    }
    
    void set_parameter_string(const char* name, const char* value) override {
        if(!name || !value) return;
        string_params_[name] = value;
    }
    
    // 序列化
    void serialize(std::vector<uint8_t>& data) const override {
        data.clear();
        
        // 简单的二进制序列化格式
        // 格式: [name_length][name][float_count][float_params][string_count][string_params]
        
        // 名称
        uint32_t name_len = static_cast<uint32_t>(name_.length());
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&name_len), 
                   reinterpret_cast<const uint8_t*>(&name_len) + sizeof(name_len));
        data.insert(data.end(), name_.begin(), name_.end());
        
        // 浮点参数
        uint32_t float_count = static_cast<uint32_t>(float_params_.size());
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&float_count),
                   reinterpret_cast<const uint8_t*>(&float_count) + sizeof(float_count));
        
        for(const auto& [key, value] : float_params_) {
            uint32_t key_len = static_cast<uint32_t>(key.length());
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&key_len),
                       reinterpret_cast<const uint8_t*>(&key_len) + sizeof(key_len));
            data.insert(data.end(), key.begin(), key.end());
            
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value),
                       reinterpret_cast<const uint8_t*>(&value) + sizeof(value));
        }
        
        // 字符串参数
        uint32_t string_count = static_cast<uint32_t>(string_params_.size());
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&string_count),
                   reinterpret_cast<const uint8_t*>(&string_count) + sizeof(string_count));
        
        for(const auto& [key, value] : string_params_) {
            uint32_t key_len = static_cast<uint32_t>(key.length());
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&key_len),
                       reinterpret_cast<const uint8_t*>(&key_len) + sizeof(key_len));
            data.insert(data.end(), key.begin(), key.end());
            
            uint32_t value_len = static_cast<uint32_t>(value.length());
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&value_len),
                       reinterpret_cast<const uint8_t*>(&value_len) + sizeof(value_len));
            data.insert(data.end(), value.begin(), value.end());
        }
    }
    
    bool deserialize(const uint8_t* data, size_t size) override {
        if(!data || size == 0) return false;
        
        reset(); // 先重置
        
        // 简化的反序列化实现
        // 实际实现需要完整的解析逻辑
        
        is_valid_ = true;
        return true;
    }
    
    // 比较
    bool operator==(const dsp_preset& other) const override {
        if(!is_valid_ || !other.is_valid()) return false;
        if(name_ != other.get_name()) return false;
        
        // 需要比较所有参数
        // 简化实现
        return true;
    }
    
    bool operator!=(const dsp_preset& other) const override {
        return !(*this == other);
    }
};

// DSP链管理器
class dsp_chain {
private:
    std::vector<service_ptr_t<dsp>> effects_;
    std::vector<std::unique_ptr<audio_chunk>> temp_buffers_;
    uint32_t sample_rate_;
    uint32_t channels_;
    
public:
    dsp_chain() : sample_rate_(44100), channels_(2) {}
    
    void add_effect(service_ptr_t<dsp> effect) {
        if(effect.is_valid()) {
            effects_.push_back(effect);
        }
    }
    
    void remove_effect(size_t index) {
        if(index < effects_.size()) {
            effects_.erase(effects_.begin() + index);
        }
    }
    
    void clear_effects() {
        effects_.clear();
        temp_buffers_.clear();
    }
    
    size_t get_effect_count() const {
        return effects_.size();
    }
    
    dsp* get_effect(size_t index) {
        return index < effects_.size() ? effects_[index].get() : nullptr;
    }
    
    // 运行DSP链
    void run_chain(audio_chunk& chunk, abort_callback& abort) {
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
    
    // 重置所有效果器
    void reset_all() {
        for(auto& effect : effects_) {
            if(effect.is_valid()) {
                effect->reset();
            }
        }
    }
    
    // 获取总延迟
    double get_total_latency() const {
        double total_latency = 0.0;
        for(const auto& effect : effects_) {
            if(effect.is_valid()) {
                total_latency += effect->get_latency();
            }
        }
        return total_latency;
    }
    
    // 检查是否需要音轨变化标记
    bool need_track_change_mark() const {
        for(const auto& effect : effects_) {
            if(effect.is_valid() && effect->need_track_change_mark()) {
                return true;
            }
        }
        return false;
    }
    
    // 获取所有效果器的名称
    std::vector<std::string> get_effect_names() const {
        std::vector<std::string> names;
        for(const auto& effect : effects_) {
            if(effect.is_valid()) {
                names.push_back(effect->get_name());
            }
        }
        return names;
    }
};

// DSP配置助手
class dsp_config_helper {
public:
    // 创建基础DSP预设
    static std::unique_ptr<dsp_preset> create_basic_preset(const char* name) {
        auto preset = std::make_unique<dsp_preset_impl>(name);
        return preset;
    }
    
    // 创建均衡器预设
    static std::unique_ptr<dsp_preset> create_equalizer_preset(
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
    
    // 创建音量控制预设
    static std::unique_ptr<dsp_preset> create_volume_preset(float volume) {
        auto preset = std::make_unique<dsp_preset_impl>("Volume");
        preset->set_parameter_float("volume", volume);
        return preset;
    }
    
    // 验证DSP配置
    static bool validate_dsp_config(const dsp& effect, const dsp_preset& preset) {
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
};

// DSP工具函数
class dsp_utils {
public:
    // 创建测试DSP（用于验证框架）
    static std::unique_ptr<dsp> create_test_dsp(const char* name = "TestDSP") {
        // 这里应该返回具体的DSP实现
        // 目前返回nullptr，实际实现需要具体的DSP类
        return nullptr;
    }
    
    // 创建静音DSP（无效果）
    static std::unique_ptr<dsp> create_passthrough_dsp(const char* name = "PassThrough") {
        // 返回一个直通DSP，不改变音频数据
        return nullptr;
    }
    
    // 计算DSP链的总CPU占用（估算）
    static float estimate_cpu_usage(const dsp_chain& chain) {
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
    
    // 验证DSP链配置
    static bool validate_dsp_chain(const dsp_chain& chain) {
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
    
    // 获取DSP链的详细信息
    static std::string get_dsp_chain_info(const dsp_chain& chain) {
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
};

} // namespace fb2k