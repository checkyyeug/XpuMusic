#pragma once

// 闃舵1.2锛欴SP鎺ュ彛瀹氫箟
// DSP鏁堟灉鍣ㄧ郴缁熸帴鍙ｏ紝绗﹀悎foobar2000瑙勮寖

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "audio_chunk.h"
#include "../stage1_1/real_minihost.h"

namespace fb2k {

// DSP閰嶇疆鍙傛暟
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

// DSP棰勮鎺ュ彛 - 绗﹀悎foobar2000瑙勮寖
class dsp_preset : public ServiceBase {
public:
    // 鍩虹绠＄悊
    virtual void reset() = 0;
    virtual bool is_valid() const = 0;
    virtual void copy(const dsp_preset& source) = 0;
    
    // 鍚嶇О绠＄悊
    virtual const char* get_name() const = 0;
    virtual void set_name(const char* name) = 0;
    
    // 鍙傛暟绠＄悊
    virtual bool has_parameter(const char* name) const = 0;
    virtual float get_parameter_float(const char* name) const = 0;
    virtual void set_parameter_float(const char* name, float value) = 0;
    virtual const char* get_parameter_string(const char* name) const = 0;
    virtual void set_parameter_string(const char* name, const char* value) = 0;
    
    // 搴忓垪鍖?
    virtual void serialize(std::vector<uint8_t>& data) const = 0;
    virtual bool deserialize(const uint8_t* data, size_t size) = 0;
    
    // 姣旇緝
    virtual bool operator==(const dsp_preset& other) const = 0;
    virtual bool operator!=(const dsp_preset& other) const = 0;
};

// DSP鏁堟灉鍣ㄦ帴鍙?- 绗﹀悎foobar2000瑙勮寖
class dsp : public ServiceBase {
public:
    // 鐢熷懡鍛ㄦ湡绠＄悊
    virtual bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                            uint32_t channels) = 0;
    virtual void reset() = 0;
    
    // 闊抽澶勭悊
    virtual void run(audio_chunk& chunk, abort_callback& abort) = 0;
    
    // 閰嶇疆绠＄悊
    virtual void get_preset(dsp_preset& preset) const = 0;
    virtual void set_preset(const dsp_preset& preset) = 0;
    
    // 鍙傛暟淇℃伅
    virtual std::vector<dsp_config_param> get_config_params() const = 0;
    
    // 鐘舵€佷俊鎭?
    virtual bool need_track_change_mark() const = 0;
    virtual double get_latency() const = 0;
    virtual const char* get_name() const = 0;
    virtual const char* get_description() const = 0;
    
    // 鑳藉姏鏌ヨ
    virtual bool can_work_with(const audio_chunk& chunk) const = 0;
    virtual bool supports_format(uint32_t sample_rate, uint32_t channels) const = 0;
};

// DSP棰勮鍏蜂綋瀹炵幇
class dsp_preset_impl : public dsp_preset {
private:
    std::string name_;
    std::map<std::string, float> float_params_;
    std::map<std::string, std::string> string_params_;
    bool is_valid_;
    
public:
    dsp_preset_impl() : is_valid_(false) {}
    explicit dsp_preset_impl(const std::string& name) : name_(name), is_valid_(true) {}
    
    // IUnknown瀹炵幇
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, __uuidof(dsp_preset))) {
            *ppvObject = static_cast<dsp_preset*>(this);
            return S_OK;
        }
        return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
    }
    
    // 鍩虹绠＄悊
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
        
        // 澶嶅埗鍙傛暟锛堢畝鍖栧疄鐜帮級
        // 瀹為檯瀹炵幇闇€瑕侀亶鍘唖ource鐨勬墍鏈夊弬鏁?
    }
    
    // 鍚嶇О绠＄悊
    const char* get_name() const override {
        return name_.c_str();
    }
    
    void set_name(const char* name) override {
        if(name) name_ = name;
    }
    
    // 鍙傛暟绠＄悊
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
    
    // 搴忓垪鍖?
    void serialize(std::vector<uint8_t>& data) const override {
        data.clear();
        
        // 绠€鍗曠殑浜岃繘鍒跺簭鍒楀寲鏍煎紡
        // 鏍煎紡: [name_length][name][float_count][float_params][string_count][string_params]
        
        // 鍚嶇О
        uint32_t name_len = static_cast<uint32_t>(name_.length());
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&name_len), 
                   reinterpret_cast<const uint8_t*>(&name_len) + sizeof(name_len));
        data.insert(data.end(), name_.begin(), name_.end());
        
        // 娴偣鍙傛暟
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
        
        // 瀛楃涓插弬鏁?
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
        
        reset(); // 鍏堥噸缃?
        
        // 绠€鍖栫殑鍙嶅簭鍒楀寲瀹炵幇
        // 瀹為檯瀹炵幇闇€瑕佸畬鏁寸殑瑙ｆ瀽閫昏緫
        
        is_valid_ = true;
        return true;
    }
    
    // 姣旇緝
    bool operator==(const dsp_preset& other) const override {
        if(!is_valid_ || !other.is_valid()) return false;
        if(name_ != other.get_name()) return false;
        
        // 闇€瑕佹瘮杈冩墍鏈夊弬鏁?
        // 绠€鍖栧疄鐜?
        return true;
    }
    
    bool operator!=(const dsp_preset& other) const override {
        return !(*this == other);
    }
};

// DSP閾剧鐞嗗櫒
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
    
    // 杩愯DSP閾?
    void run_chain(audio_chunk& chunk, abort_callback& abort) {
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
    
    // 閲嶇疆鎵€鏈夋晥鏋滃櫒
    void reset_all() {
        for(auto& effect : effects_) {
            if(effect.is_valid()) {
                effect->reset();
            }
        }
    }
    
    // 鑾峰彇鎬诲欢杩?
    double get_total_latency() const {
        double total_latency = 0.0;
        for(const auto& effect : effects_) {
            if(effect.is_valid()) {
                total_latency += effect->get_latency();
            }
        }
        return total_latency;
    }
    
    // 妫€鏌ユ槸鍚﹂渶瑕侀煶杞ㄥ彉鍖栨爣璁?
    bool need_track_change_mark() const {
        for(const auto& effect : effects_) {
            if(effect.is_valid() && effect->need_track_change_mark()) {
                return true;
            }
        }
        return false;
    }
    
    // 鑾峰彇鎵€鏈夋晥鏋滃櫒鐨勫悕绉?
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

// DSP閰嶇疆鍔╂墜
class dsp_config_helper {
public:
    // 鍒涘缓鍩虹DSP棰勮
    static std::unique_ptr<dsp_preset> create_basic_preset(const char* name) {
        auto preset = std::make_unique<dsp_preset_impl>(name);
        return preset;
    }
    
    // 鍒涘缓鍧囪　鍣ㄩ璁?
    static std::unique_ptr<dsp_preset> create_equalizer_preset(
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
    
    // 鍒涘缓闊抽噺鎺у埗棰勮
    static std::unique_ptr<dsp_preset> create_volume_preset(float volume) {
        auto preset = std::make_unique<dsp_preset_impl>("Volume");
        preset->set_parameter_float("volume", volume);
        return preset;
    }
    
    // 楠岃瘉DSP閰嶇疆
    static bool validate_dsp_config(const dsp& effect, const dsp_preset& preset) {
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
};

// DSP宸ュ叿鍑芥暟
class dsp_utils {
public:
    // 鍒涘缓娴嬭瘯DSP锛堢敤浜庨獙璇佹鏋讹級
    static std::unique_ptr<dsp> create_test_dsp(const char* name = "TestDSP") {
        // 杩欓噷搴旇杩斿洖鍏蜂綋鐨凞SP瀹炵幇
        // 鐩墠杩斿洖nullptr锛屽疄闄呭疄鐜伴渶瑕佸叿浣撶殑DSP绫?
        return nullptr;
    }
    
    // 鍒涘缓闈欓煶DSP锛堟棤鏁堟灉锛?
    static std::unique_ptr<dsp> create_passthrough_dsp(const char* name = "PassThrough") {
        // 杩斿洖涓€涓洿閫欴SP锛屼笉鏀瑰彉闊抽鏁版嵁
        return nullptr;
    }
    
    // 璁＄畻DSP閾剧殑鎬籆PU鍗犵敤锛堜及绠楋級
    static float estimate_cpu_usage(const dsp_chain& chain) {
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
    
    // 楠岃瘉DSP閾鹃厤缃?
    static bool validate_dsp_chain(const dsp_chain& chain) {
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
    
    // 鑾峰彇DSP閾剧殑璇︾粏淇℃伅
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