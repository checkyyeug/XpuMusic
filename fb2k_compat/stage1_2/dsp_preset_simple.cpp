#include "dsp_interfaces.h"
#include <cstring>
#include <algorithm>
#include <sstream>

namespace fb2k {

// 简化的DSP预设实现
// 用于测试和演示基本功能

class simple_dsp_preset : public dsp_preset {
private:
    std::string name_;
    std::map<std::string, float> float_params_;
    std::map<std::string, std::string> string_params_;
    bool is_valid_;
    
public:
    simple_dsp_preset() : is_valid_(false) {}
    explicit simple_dsp_preset(const std::string& name) : name_(name), is_valid_(true) {}
    
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
        
        // 复制浮点参数
        float_params_.clear();
        // 这里需要遍历source的所有float参数
        // 简化实现：只复制已知参数
        
        // 复制字符串参数
        string_params_.clear();
        // 简化实现
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
    
    // 序列化 - 简化JSON格式
    void serialize(std::vector<uint8_t>& data) const override {
        data.clear();
        
        // 创建JSON格式的字符串
        std::stringstream json;
        json << "{\n";
        json << "  \"name\": \"" << name_ << "\",\n";
        json << "  \"valid\": " << (is_valid_ ? "true" : "false") << ",\n";
        
        // 浮点参数
        json << "  \"float_params\": {\n";
        bool first = true;
        for(const auto& [key, value] : float_params_) {
            if(!first) json << ",\n";
            json << "    \"" << key << "\": " << value;
            first = false;
        }
        json << "\n  },\n";
        
        // 字符串参数
        json << "  \"string_params\": {\n";
        first = true;
        for(const auto& [key, value] : string_params_) {
            if(!first) json << ",\n";
            json << "    \"" << key << "\": \"" << value << "\"";
            first = false;
        }
        json << "\n  }\n";
        json << "}";
        
        std::string json_str = json.str();
        data.assign(json_str.begin(), json_str.end());
    }
    
    bool deserialize(const uint8_t* data, size_t size) override {
        if(!data || size == 0) return false;
        
        reset(); // 先重置
        
        std::string json_str(reinterpret_cast<const char*>(data), size);
        
        // 简化的JSON解析
        // 实际实现需要完整的JSON解析器
        
        // 查找名称
        size_t name_pos = json_str.find("\"name\":");
        if(name_pos != std::string::npos) {
            size_t name_start = json_str.find("\"", name_pos + 7) + 1;
            size_t name_end = json_str.find("\"", name_start);
            if(name_end != std::string::npos) {
                name_ = json_str.substr(name_start, name_end - name_start);
            }
        }
        
        // 查找valid标志
        size_t valid_pos = json_str.find("\"valid\":");
        if(valid_pos != std::string::npos) {
            size_t valid_start = valid_pos + 8;
            if(json_str.substr(valid_start, 4) == "true") {
                is_valid_ = true;
            }
        }
        
        // 简化参数解析（实际实现需要完整JSON解析）
        // 这里只是演示基本思路
        
        is_valid_ = true;
        return true;
    }
    
    // 比较
    bool operator==(const dsp_preset& other) const override {
        if(!is_valid_ || !other.is_valid()) return false;
        if(name_ != other.get_name()) return false;
        
        // 需要比较所有参数
        // 简化实现：只比较float参数的数量
        return float_params_.size() == 0; // 简化比较
    }
    
    bool operator!=(const dsp_preset& other) const override {
        return !(*this == other);
    }
};

// 基础DSP效果器实现（用于测试）
class test_dsp_effect : public dsp {
private:
    std::string name_;
    bool is_instantiated_;
    uint32_t sample_rate_;
    uint32_t channels_;
    float gain_;
    
public:
    test_dsp_effect(const std::string& name = "TestDSP") 
        : name_(name), is_instantiated_(false), sample_rate_(44100), channels_(2), gain_(1.0f) {}
    
    // IUnknown实现
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, __uuidof(dsp))) {
            *ppvObject = static_cast<dsp*>(this);
            return S_OK;
        }
        return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
    }
    
    // DSP接口实现
    bool instantiate(audio_chunk& chunk, uint32_t sample_rate, uint32_t channels) override {
        sample_rate_ = sample_rate;
        channels_ = channels;
        is_instantiated_ = true;
        return true;
    }
    
    void reset() override {
        is_instantiated_ = false;
        gain_ = 1.0f;
    }
    
    void run(audio_chunk& chunk, abort_callback& abort) override {
        if(!is_instantiated_ || abort.is_aborting()) {
            return;
        }
        
        if(chunk.is_empty()) return;
        
        // 简单的增益效果（用于测试）
        float* data = chunk.get_data();
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        
        if(data) {
            for(size_t i = 0; i < total_samples; ++i) {
                data[i] *= gain_;
            }
        }
        
        printf("[%s] 处理音频数据: %zu 采样, 增益: %.2f\n", 
               name_.c_str(), chunk.get_sample_count(), gain_);
    }
    
    void get_preset(dsp_preset& preset) const override {
        preset.set_name(name_.c_str());
        preset.set_parameter_float("gain", gain_);
    }
    
    void set_preset(const dsp_preset& preset) override {
        gain_ = preset.get_parameter_float("gain");
    }
    
    std::vector<dsp_config_param> get_config_params() const override {
        std::vector<dsp_config_param> params;
        params.emplace_back("gain", "Gain", 1.0f, 0.0f, 2.0f, 0.1f);
        return params;
    }
    
    bool need_track_change_mark() const override {
        return false; // 不需要音轨变化标记
    }
    
    double get_latency() const override {
        return 0.0; // 无延迟
    }
    
    const char* get_name() const override {
        return name_.c_str();
    }
    
    const char* get_description() const override {
        return "Test DSP effect for framework validation";
    }
    
    bool can_work_with(const audio_chunk& chunk) const override {
        return chunk.get_sample_rate() == sample_rate_ && chunk.get_channels() == channels_;
    }
    
    bool supports_format(uint32_t sample_rate, uint32_t channels) const override {
        return sample_rate >= 8000 && sample_rate <= 192000 &&
               channels >= 1 && channels <= 8;
    }
};

// DSP效果器工厂
class dsp_effect_factory {
public:
    static std::unique_ptr<dsp> create_test_effect(const std::string& name = "TestDSP") {
        return std::make_unique<test_dsp_effect>(name);
    }
    
    static std::unique_ptr<dsp> create_passthrough_effect(const std::string& name = "PassThrough") {
        // 创建一个直通效果器（不改变音频）
        auto effect = std::make_unique<test_dsp_effect>(name);
        
        // 创建预设并设置为直通（增益=1.0）
        auto preset = std::make_unique<simple_dsp_preset>(name);
        preset->set_parameter_float("gain", 1.0f);
        effect->set_preset(*preset);
        
        return effect;
    }
    
    static std::unique_ptr<dsp> create_volume_effect(float volume = 1.0f) {
        auto effect = std::make_unique<test_dsp_effect>("Volume");
        
        auto preset = std::make_unique<simple_dsp_preset>("Volume");
        preset->set_parameter_float("gain", volume);
        effect->set_preset(*preset);
        
        return effect;
    }
    
    static std::unique_ptr<dsp> create_equalizer_effect(const std::vector<float>& bands) {
        auto effect = std::make_unique<test_dsp_effect>("Equalizer");
        
        auto preset = std::make_unique<simple_dsp_preset>("Equalizer");
        
        // 设置均衡器参数
        for(size_t i = 0; i < bands.size(); ++i) {
            std::string param_name = "band_" + std::to_string(i);
            preset->set_parameter_float(param_name.c_str(), bands[i]);
        }
        
        effect->set_preset(*preset);
        return effect;
    }
};

// DSP系统初始化器
class dsp_system_initializer {
public:
    static bool initialize_dsp_system() {
        printf("[DSP] 初始化DSP系统...\n");
        
        // 这里可以添加DSP系统的初始化逻辑
        // 例如：注册标准DSP效果，初始化DSP库等
        
        printf("[DSP] DSP系统初始化完成\n");
        return true;
    }
    
    static void shutdown_dsp_system() {
        printf("[DSP] 关闭DSP系统\n");
        
        // 清理DSP系统资源
    }
};

} // namespace fb2k