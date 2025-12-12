#include "dsp_interfaces.h"
#include <cstring>
#include <algorithm>
#include <sstream>

namespace fb2k {

// 绠€鍖栫殑DSP棰勮瀹炵幇
// 鐢ㄤ簬娴嬭瘯鍜屾紨绀哄熀鏈姛鑳?

class simple_dsp_preset : public dsp_preset {
private:
    std::string name_;
    std::map<std::string, float> float_params_;
    std::map<std::string, std::string> string_params_;
    bool is_valid_;
    
public:
    simple_dsp_preset() : is_valid_(false) {}
    explicit simple_dsp_preset(const std::string& name) : name_(name), is_valid_(true) {}
    
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
        
        // 澶嶅埗娴偣鍙傛暟
        float_params_.clear();
        // 杩欓噷闇€瑕侀亶鍘唖ource鐨勬墍鏈塮loat鍙傛暟
        // 绠€鍖栧疄鐜帮細鍙鍒跺凡鐭ュ弬鏁?
        
        // 澶嶅埗瀛楃涓插弬鏁?
        string_params_.clear();
        // 绠€鍖栧疄鐜?
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
    
    // 搴忓垪鍖?- 绠€鍖朖SON鏍煎紡
    void serialize(std::vector<uint8_t>& data) const override {
        data.clear();
        
        // 鍒涘缓JSON鏍煎紡鐨勫瓧绗︿覆
        std::stringstream json;
        json << "{\n";
        json << "  \"name\": \"" << name_ << "\",\n";
        json << "  \"valid\": " << (is_valid_ ? "true" : "false") << ",\n";
        
        // 娴偣鍙傛暟
        json << "  \"float_params\": {\n";
        bool first = true;
        for(const auto& [key, value] : float_params_) {
            if(!first) json << ",\n";
            json << "    \"" << key << "\": " << value;
            first = false;
        }
        json << "\n  },\n";
        
        // 瀛楃涓插弬鏁?
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
        
        reset(); // 鍏堥噸缃?
        
        std::string json_str(reinterpret_cast<const char*>(data), size);
        
        // 绠€鍖栫殑JSON瑙ｆ瀽
        // 瀹為檯瀹炵幇闇€瑕佸畬鏁寸殑JSON瑙ｆ瀽鍣?
        
        // 鏌ユ壘鍚嶇О
        size_t name_pos = json_str.find("\"name\":");
        if(name_pos != std::string::npos) {
            size_t name_start = json_str.find("\"", name_pos + 7) + 1;
            size_t name_end = json_str.find("\"", name_start);
            if(name_end != std::string::npos) {
                name_ = json_str.substr(name_start, name_end - name_start);
            }
        }
        
        // 鏌ユ壘valid鏍囧織
        size_t valid_pos = json_str.find("\"valid\":");
        if(valid_pos != std::string::npos) {
            size_t valid_start = valid_pos + 8;
            if(json_str.substr(valid_start, 4) == "true") {
                is_valid_ = true;
            }
        }
        
        // 绠€鍖栧弬鏁拌В鏋愶紙瀹為檯瀹炵幇闇€瑕佸畬鏁碕SON瑙ｆ瀽锛?
        // 杩欓噷鍙槸婕旂ず鍩烘湰鎬濊矾
        
        is_valid_ = true;
        return true;
    }
    
    // 姣旇緝
    bool operator==(const dsp_preset& other) const override {
        if(!is_valid_ || !other.is_valid()) return false;
        if(name_ != other.get_name()) return false;
        
        // 闇€瑕佹瘮杈冩墍鏈夊弬鏁?
        // 绠€鍖栧疄鐜帮細鍙瘮杈僨loat鍙傛暟鐨勬暟閲?
        return float_params_.size() == 0; // 绠€鍖栨瘮杈?
    }
    
    bool operator!=(const dsp_preset& other) const override {
        return !(*this == other);
    }
};

// 鍩虹DSP鏁堟灉鍣ㄥ疄鐜帮紙鐢ㄤ簬娴嬭瘯锛?
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
    
    // IUnknown瀹炵幇
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, __uuidof(dsp))) {
            *ppvObject = static_cast<dsp*>(this);
            return S_OK;
        }
        return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
    }
    
    // DSP鎺ュ彛瀹炵幇
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
        
        // 绠€鍗曠殑澧炵泭鏁堟灉锛堢敤浜庢祴璇曪級
        float* data = chunk.get_data();
        size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
        
        if(data) {
            for(size_t i = 0; i < total_samples; ++i) {
                data[i] *= gain_;
            }
        }
        
        printf("[%s] 澶勭悊闊抽鏁版嵁: %zu 閲囨牱, 澧炵泭: %.2f\n", 
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
        return false; // 涓嶉渶瑕侀煶杞ㄥ彉鍖栨爣璁?
    }
    
    double get_latency() const override {
        return 0.0; // 鏃犲欢杩?
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

// DSP鏁堟灉鍣ㄥ伐鍘?
class dsp_effect_factory {
public:
    static std::unique_ptr<dsp> create_test_effect(const std::string& name = "TestDSP") {
        return std::make_unique<test_dsp_effect>(name);
    }
    
    static std::unique_ptr<dsp> create_passthrough_effect(const std::string& name = "PassThrough") {
        // 鍒涘缓涓€涓洿閫氭晥鏋滃櫒锛堜笉鏀瑰彉闊抽锛?
        auto effect = std::make_unique<test_dsp_effect>(name);
        
        // 鍒涘缓棰勮骞惰缃负鐩撮€氾紙澧炵泭=1.0锛?
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
        
        // 璁剧疆鍧囪　鍣ㄥ弬鏁?
        for(size_t i = 0; i < bands.size(); ++i) {
            std::string param_name = "band_" + std::to_string(i);
            preset->set_parameter_float(param_name.c_str(), bands[i]);
        }
        
        effect->set_preset(*preset);
        return effect;
    }
};

// DSP绯荤粺鍒濆鍖栧櫒
class dsp_system_initializer {
public:
    static bool initialize_dsp_system() {
        printf("[DSP] 鍒濆鍖朌SP绯荤粺...\n");
        
        // 杩欓噷鍙互娣诲姞DSP绯荤粺鐨勫垵濮嬪寲閫昏緫
        // 渚嬪锛氭敞鍐屾爣鍑咲SP鏁堟灉锛屽垵濮嬪寲DSP搴撶瓑
        
        printf("[DSP] DSP绯荤粺鍒濆鍖栧畬鎴怽n");
        return true;
    }
    
    static void shutdown_dsp_system() {
        printf("[DSP] 鍏抽棴DSP绯荤粺\n");
        
        // 娓呯悊DSP绯荤粺璧勬簮
    }
};

} // namespace fb2k