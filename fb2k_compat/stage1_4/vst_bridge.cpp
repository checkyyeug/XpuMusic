#include "vst_bridge.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace fb2k {

// vst_plugin_wrapper瀹炵幇
vst_plugin_wrapper::vst_plugin_wrapper()
    : vst_effect_(nullptr)
    , vst_dll_(nullptr)
    , plugin_loaded_(false)
    , current_sample_rate_(44100.0)
    , current_block_size_(512)
    , editor_open_(false)
    , editor_window_(nullptr)
    , current_program_(0)
    , total_samples_processed_(0)
    , total_processing_time_(0.0) {
    
    // 鍒濆鍖朌SP鏁堟灉鍣ㄥ弬鏁?
    auto params = create_default_dsp_params();
    params.type = dsp_effect_type::vst_plugin;
    params.name = "VST Plugin";
    params.description = "VST plugin wrapper";
    params.latency_ms = 0.0;
    params.cpu_usage_estimate = 10.0f;
    
    set_params(params);
}

vst_plugin_wrapper::~vst_plugin_wrapper() {
    unload_plugin();
}

bool vst_plugin_wrapper::load_plugin(const std::string& dll_path) {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (plugin_loaded_) {
        unload_plugin();
    }
    
    // 鍔犺浇VST DLL
    vst_dll_ = LoadLibraryA(dll_path.c_str());
    if (!vst_dll_) {
        std::cerr << "[VST Bridge] 鍔犺浇VST DLL澶辫触: " << dll_path << std::endl;
        return false;
    }
    
    // 鑾峰彇VST涓诲嚱鏁?
    VstPluginMainFunc main_func = (VstPluginMainFunc)GetProcAddress(vst_dll_, "VSTPluginMain");
    if (!main_func) {
        // 灏濊瘯鏃х殑鍏ュ彛鐐瑰悕绉?
        main_func = (VstPluginMainFunc)GetProcAddress(vst_dll_, "main");
    }
    
    if (!main_func) {
        std::cerr << "[VST Bridge] 鎵句笉鍒癡ST鍏ュ彛鐐? " << dll_path << std::endl;
        FreeLibrary(vst_dll_);
        vst_dll_ = nullptr;
        return false;
    }
    
    // 鍒涘缓VST鏁堟灉瀹炰緥
    vst_effect_ = main_func(host_callback);
    if (!vst_effect_ || vst_effect_->magic != 'VstP') {
        std::cerr << "[VST Bridge] 鍒涘缓VST鏁堟灉澶辫触: " << dll_path << std::endl;
        FreeLibrary(vst_dll_);
        vst_dll_ = nullptr;
        return false;
    }
    
    // 娉ㄥ唽鎻掍欢鍒板涓?
    vst_host::get_instance().register_plugin(vst_effect_, this);
    
    vst_plugin_path_ = dll_path;
    plugin_loaded_ = true;
    
    // 鍒濆鍖栨彃浠?
    if (!initialize_vst_plugin()) {
        unload_plugin();
        return false;
    }
    
    std::cout << "[VST Bridge] VST鎻掍欢鍔犺浇鎴愬姛: " << dll_path << std::endl;
    return true;
}

void vst_plugin_wrapper::unload_plugin() {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!plugin_loaded_) {
        return;
    }
    
    // 鍏抽棴缂栬緫鍣?
    if (editor_open_) {
        close_editor();
    }
    
    // 鍏抽棴鎻掍欢
    if (vst_effect_) {
        call_dispatcher(kVstEffectClose, 0, 0, nullptr, 0.0f);
        
        // 浠庡涓绘敞閿€
        vst_host::get_instance().unregister_plugin(vst_effect_);
        
        vst_effect_ = nullptr;
    }
    
    // 鍗歌浇DLL
    if (vst_dll_) {
        FreeLibrary(vst_dll_);
        vst_dll_ = nullptr;
    }
    
    plugin_loaded_ = false;
    vst_plugin_path_.clear();
    parameter_values_.clear();
    parameter_info_.clear();
    programs_.clear();
    
    std::cout << "[VST Bridge] VST鎻掍欢宸插嵏杞? << std::endl;
}

std::string vst_plugin_wrapper::get_plugin_name() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_) return "";
    
    char name[64] = {0};
    call_dispatcher(kVstEffectGetEffectName, 0, 0, name, 0.0f);
    return std::string(name);
}

std::string vst_plugin_wrapper::get_plugin_vendor() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_) return "";
    
    char vendor[64] = {0};
    call_dispatcher(kVstEffectGetVendorString, 0, 0, vendor, 0.0f);
    return std::string(vendor);
}

std::string vst_plugin_wrapper::get_plugin_version() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_) return "";
    
    VstInt32 version = call_dispatcher(kVstEffectGetVendorVersion, 0, 0, nullptr, 0.0f);
    
    std::stringstream ss;
    ss << HIWORD(version) << "." << LOWORD(version);
    return ss.str();
}

int vst_plugin_wrapper::get_num_inputs() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    return vst_effect_ ? vst_effect_->numInputs : 0;
}

int vst_plugin_wrapper::get_num_outputs() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    return vst_effect_ ? vst_effect_->numOutputs : 0;
}

int vst_plugin_wrapper::get_num_parameters() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    return vst_effect_ ? vst_effect_->numParams : 0;
}

int vst_plugin_wrapper::get_num_programs() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    return vst_effect_ ? vst_effect_->numPrograms : 0;
}

std::vector<vst_parameter_info> vst_plugin_wrapper::get_parameter_info() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    return parameter_info_;
}

float vst_plugin_wrapper::get_parameter_value(int index) const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_ || index < 0 || index >= vst_effect_->numParams) {
        return 0.0f;
    }
    
    return call_get_parameter(index);
}

void vst_plugin_wrapper::set_parameter_value(int index, float value) {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_ || index < 0 || index >= vst_effect_->numParams) {
        return;
    }
    
    // 闄愬埗鍙傛暟鑼冨洿
    value = std::max(0.0f, std::min(1.0f, value));
    
    call_set_parameter(index, value);
}

std::string vst_plugin_wrapper::get_parameter_label(int index) const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_ || index < 0 || index >= vst_effect_->numParams) {
        return "";
    }
    
    char label[64] = {0};
    call_dispatcher(kVstEffectGetParamLabel, index, 0, label, 0.0f);
    return std::string(label);
}

std::string vst_plugin_wrapper::get_parameter_display(int index) const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_ || index < 0 || index >= vst_effect_->numParams) {
        return "";
    }
    
    char display[64] = {0};
    call_dispatcher(kVstEffectGetParamDisplay, index, 0, display, 0.0f);
    return std::string(display);
}

bool vst_plugin_wrapper::process_audio(const float** inputs, float** outputs, int num_samples) {
    if (!plugin_loaded_ || !vst_effect_ || !vst_effect_->process) {
        return false;
    }
    
    // 璁剧疆闊抽缂撳啿鍖?
    VstAudioBuffer in_buffer;
    in_buffer.channels = const_cast<float**>(inputs);
    in_buffer.numChannels = vst_effect_->numInputs;
    in_buffer.size = num_samples;
    
    VstAudioBuffer out_buffer;
    out_buffer.channels = outputs;
    out_buffer.numChannels = vst_effect_->numOutputs;
    out_buffer.size = num_samples;
    
    // 澶勭悊闊抽
    auto start_time = std::chrono::high_resolution_clock::now();
    
    typedef void (*VstProcessFunc)(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames);
    VstProcessFunc process_func = (VstProcessFunc)vst_effect_->process;
    
    process_func(vst_effect_, const_cast<float**>(inputs), outputs, num_samples);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    total_samples_processed_ += num_samples;
    total_processing_time_ += duration.count() / 1000.0; // 杞崲涓烘绉?
    
    return true;
}

bool vst_plugin_wrapper::get_chunk(std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_) return false;
    
    void* chunk_data = nullptr;
    VstInt32 chunk_size = call_dispatcher(kVstEffectGetChunk, 0, 0, &chunk_data, 0.0f);
    
    if (chunk_size > 0 && chunk_data) {
        data.resize(chunk_size);
        std::memcpy(data.data(), chunk_data, chunk_size);
        return true;
    }
    
    return false;
}

bool vst_plugin_wrapper::set_chunk(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!vst_effect_) return false;
    
    return call_dispatcher(kVstEffectSetChunk, 0, data.size(), const_cast<void*>(static_cast<const void*>(data.data())), 0.0f) == 1;
}

bool vst_plugin_wrapper::has_editor() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    return vst_effect_ && (vst_effect_->flags & kPluginHasEditor);
}

bool vst_plugin_wrapper::open_editor(void* parent_window) {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (!has_editor() || editor_open_) {
        return false;
    }
    
    editor_window_ = parent_window;
    
    // 鑾峰彇缂栬緫鍣ㄥぇ灏?
    struct EditorRect {
        short top, left, bottom, right;
    } rect;
    
    if (call_dispatcher(kVstEffectEditGetRect, 0, 0, &rect, 0.0f) == 1) {
        // 鎵撳紑缂栬緫鍣?
        if (call_dispatcher(kVstEffectEditOpen, 0, 0, parent_window, 0.0f) == 1) {
            editor_open_ = true;
            return true;
        }
    }
    
    return false;
}

void vst_plugin_wrapper::close_editor() {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (editor_open_ && vst_effect_) {
        call_dispatcher(kVstEffectEditClose, 0, 0, nullptr, 0.0f);
        editor_open_ = false;
        editor_window_ = nullptr;
    }
}

// dsp_effect_advanced 瀹炵幇
bool vst_plugin_wrapper::instantiate(audio_chunk& chunk, uint32_t sample_rate, uint32_t channels) {
    if (!plugin_loaded_) return false;
    
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    // 璁剧疆閲囨牱鐜?
    if (current_sample_rate_ != sample_rate) {
        current_sample_rate_ = sample_rate;
        call_dispatcher(kVstEffectSetSampleRate, 0, 0, nullptr, static_cast<float>(sample_rate));
    }
    
    // 璁剧疆鍧楀ぇ灏?
    int block_size = chunk.get_sample_count();
    if (current_block_size_ != block_size) {
        current_block_size_ = block_size;
        call_dispatcher(kVstEffectSetBlockSize, 0, block_size, nullptr, 0.0f);
    }
    
    // 妫€鏌/O閰嶇疆
    int plugin_inputs = get_num_inputs();
    int plugin_outputs = get_num_outputs();
    
    if (channels != plugin_inputs && plugin_inputs != 0) {
        // 灏濊瘯閰嶇疆鎻掍欢鐨勫０閬撳竷灞€
        call_dispatcher(kVstEffectSetSpeakerArrangement, 0, channels, nullptr, 0.0f);
    }
    
    // 鍚敤鎻掍欢
    call_dispatcher(kVstEffectMainsChanged, 0, 1, nullptr, 0.0f);
    
    // 鎭㈠鍙傛暟鍊?
    if (!parameter_values_.empty()) {
        for (size_t i = 0; i < parameter_values_.size() && i < vst_effect_->numParams; ++i) {
            call_set_parameter(static_cast<VstInt32>(i), parameter_values_[i]);
        }
    }
    
    return true;
}

void vst_plugin_wrapper::run(audio_chunk& chunk, abort_callback& abort) {
    if (!plugin_loaded_ || abort.is_aborting()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    int num_samples = chunk.get_sample_count();
    int num_channels = chunk.get_channels();
    float* data = chunk.get_data();
    
    // 鍑嗗杈撳叆杈撳嚭缂撳啿鍖?
    std::vector<float*> inputs(vst_effect_->numInputs);
    std::vector<float*> outputs(vst_effect_->numOutputs);
    std::vector<std::vector<float>> input_buffers;
    std::vector<std::vector<float>> output_buffers;
    
    // 鍒嗛厤缂撳啿鍖?
    for (int i = 0; i < vst_effect_->numInputs; ++i) {
        input_buffers.emplace_back(num_samples, 0.0f);
        inputs[i] = input_buffers[i].data();
    }
    
    for (int i = 0; i < vst_effect_->numOutputs; ++i) {
        output_buffers.emplace_back(num_samples, 0.0f);
        outputs[i] = output_buffers[i].data();
    }
    
    // 濉厖杈撳叆缂撳啿鍖?
    if (vst_effect_->numInputs > 0 && num_channels > 0) {
        for (int i = 0; i < std::min(vst_effect_->numInputs, num_channels); ++i) {
            for (int j = 0; j < num_samples; ++j) {
                inputs[i][j] = data[j * num_channels + i];
            }
        }
    }
    
    // 澶勭悊闊抽
    process_audio(const_cast<const float**>(inputs.data()), outputs.data(), num_samples);
    
    // 灏嗚緭鍑哄鍒跺洖鍘熺紦鍐插尯
    if (vst_effect_->numOutputs > 0) {
        for (int i = 0; i < std::min(vst_effect_->numOutputs, num_channels); ++i) {
            for (int j = 0; j < num_samples; ++j) {
                data[j * num_channels + i] = outputs[i][j];
            }
        }
    }
}

void vst_plugin_wrapper::reset() {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    if (vst_effect_) {
        // 閲嶇疆鎻掍欢鐘舵€?
        call_dispatcher(kVstEffectMainsChanged, 0, 0, nullptr, 0.0f);
        call_dispatcher(kVstEffectMainsChanged, 0, 1, nullptr, 0.0f);
    }
    
    total_samples_processed_ = 0;
    total_processing_time_ = 0.0;
}

void vst_plugin_wrapper::set_realtime_parameter(const std::string& param_name, float value) {
    // 鏌ユ壘鍙傛暟绱㈠紩
    for (size_t i = 0; i < parameter_info_.size(); ++i) {
        if (parameter_info_[i].name == param_name) {
            set_parameter_value(static_cast<int>(i), value);
            return;
        }
    }
}

float vst_plugin_wrapper::get_realtime_parameter(const std::string& param_name) const {
    // 鏌ユ壘鍙傛暟绱㈠紩
    for (size_t i = 0; i < parameter_info_.size(); ++i) {
        if (parameter_info_[i].name == param_name) {
            return get_parameter_value(static_cast<int>(i));
        }
    }
    return 0.0f;
}

std::vector<parameter_info> vst_plugin_wrapper::get_realtime_parameters() const {
    std::lock_guard<std::mutex> lock(vst_mutex_);
    
    std::vector<parameter_info> params;
    
    for (size_t i = 0; i < parameter_info_.size(); ++i) {
        parameter_info info;
        info.name = parameter_info_[i].name;
        info.value = parameter_values_[i];
        info.min_value = parameter_info_[i].min_value;
        info.max_value = parameter_info_[i].max_value;
        info.default_value = parameter_info_[i].default_value;
        params.push_back(info);
    }
    
    return params;
}

// VST鎿嶄綔杈呭姪鍑芥暟
VstInt32 vst_plugin_wrapper::call_dispatcher(VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt) {
    if (!vst_effect_ || !vst_effect_->dispatcher) {
        return 0;
    }
    
    typedef VstInt32 (*VstDispatcherFunc)(AEffect* effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt);
    VstDispatcherFunc dispatcher_func = (VstDispatcherFunc)vst_effect_->dispatcher;
    
    return dispatcher_func(vst_effect_, opcode, index, value, ptr, opt);
}

void vst_plugin_wrapper::call_set_parameter(VstInt32 index, VstFloat parameter) {
    if (!vst_effect_ || !vst_effect_->setParameter) {
        return;
    }
    
    typedef void (*VstSetParameterFunc)(AEffect* effect, VstInt32 index, VstFloat parameter);
    VstSetParameterFunc set_param_func = (VstSetParameterFunc)vst_effect_->setParameter;
    
    set_param_func(vst_effect_, index, parameter);
}

VstFloat vst_plugin_wrapper::call_get_parameter(VstInt32 index) {
    if (!vst_effect_ || !vst_effect_->getParameter) {
        return 0.0f;
    }
    
    typedef VstFloat (*VstGetParameterFunc)(AEffect* effect, VstInt32 index);
    VstGetParameterFunc get_param_func = (VstGetParameterFunc)vst_effect_->getParameter;
    
    return get_param_func(vst_effect_, index);
}

// VST涓绘満鍥炶皟
VstInt32 VSTCALLBACK vst_plugin_wrapper::host_callback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt) {
    // 鑾峰彇瀵瑰簲鐨勬彃浠跺寘瑁呭櫒
    vst_plugin_wrapper* wrapper = vst_host::get_instance().get_plugin_from_effect(effect);
    if (wrapper) {
        return wrapper->handle_host_callback(opcode, index, value, ptr, opt);
    }
    
    return 0;
}

VstInt32 vst_plugin_wrapper::handle_host_callback(VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt) {
    switch (opcode) {
        case kVstAudioMasterVersion:
            return 2400; // VST 2.4
            
        case kVstAudioMasterGetSampleRate:
            return static_cast<VstInt32>(vst_host::get_instance().get_sample_rate());
            
        case kVstAudioMasterGetBlockSize:
            return vst_host::get_instance().get_block_size();
            
        case kVstAudioMasterGetTime:
            return reinterpret_cast<VstInt32>(vst_host::get_instance().get_time_info());
            
        case kVstAudioMasterProcessEvents:
            // 澶勭悊VST浜嬩欢
            if (ptr) {
                VstEvents* events = static_cast<VstEvents*>(ptr);
                // 杩欓噷搴旇澶勭悊MIDI绛変簨浠?
            }
            return 1;
            
        case kVstAudioMasterAutomate:
            // 鍙傛暟鑷姩鍖?
            if (index >= 0 && index < vst_effect_->numParams) {
                // 杩欓噷搴旇澶勭悊鍙傛暟鑷姩鍖?
            }
            return 1;
            
        default:
            return 0;
    }
}

// 鍒濆鍖朧ST鎻掍欢
bool vst_plugin_wrapper::initialize_vst_plugin() {
    if (!vst_effect_) return false;
    
    // 鎵撳紑鎻掍欢
    if (call_dispatcher(kVstEffectOpen, 0, 0, nullptr, 0.0f) != 1) {
        std::cerr << "[VST Bridge] 鎵撳紑VST鎻掍欢澶辫触" << std::endl;
        return false;
    }
    
    // 鎻愬彇鎻掍欢淇℃伅
    extract_plugin_info();
    extract_parameter_info();
    extract_program_info();
    
    // 璁剧疆榛樿闊抽鏍煎紡
    call_dispatcher(kVstEffectSetSampleRate, 0, 0, nullptr, 44100.0f);
    call_dispatcher(kVstEffectSetBlockSize, 0, 512, nullptr, 0.0f);
    
    return true;
}

void vst_plugin_wrapper::extract_plugin_info() {
    if (!vst_effect_) return;
    
    // 鑾峰彇鎻掍欢鍚嶇О
    char name[64] = {0};
    call_dispatcher(kVstEffectGetEffectName, 0, 0, name, 0.0f);
    
    // 鏇存柊DSP鍙傛暟
    auto params = get_params();
    params.name = std::string(name);
    params.description = "VST Plugin: " + std::string(name);
    set_params(params);
}

void vst_plugin_wrapper::extract_parameter_info() {
    if (!vst_effect_) return;
    
    parameter_info_.clear();
    parameter_values_.clear();
    
    int num_params = vst_effect_->numParams;
    parameter_values_.resize(num_params);
    
    for (int i = 0; i < num_params; ++i) {
        vst_parameter_info info;
        
        // 鑾峰彇鍙傛暟鍚嶇О
        char name[64] = {0};
        call_dispatcher(kVstEffectGetParamName, i, 0, name, 0.0f);
        info.name = std::string(name);
        
        // 鑾峰彇鍙傛暟鏍囩
        char label[64] = {0};
        call_dispatcher(kVstEffectGetParamLabel, i, 0, label, 0.0f);
        info.label = std::string(label);
        
        // 鑾峰彇鍙傛暟鏄剧ず鍊?
        char display[64] = {0};
        call_dispatcher(kVstEffectGetParamDisplay, i, 0, display, 0.0f);
        
        // 鑾峰彇褰撳墠鍙傛暟鍊?
        float current_value = call_get_parameter(i);
        info.default_value = 0.5f; // 榛樿涓棿鍊?
        info.min_value = 0.0f;
        info.max_value = 1.0f;
        info.step_size = 0.01f;
        info.is_automatable = true;
        info.is_discrete = false;
        info.discrete_steps = 0;
        
        parameter_info_.push_back(info);
        parameter_values_[i] = current_value;
    }
}

void vst_plugin_wrapper::extract_program_info() {
    if (!vst_effect_) return;
    
    programs_.clear();
    
    int num_programs = vst_effect_->numPrograms;
    for (int i = 0; i < num_programs; ++i) {
        vst_program_info program;
        
        // 鑾峰彇绋嬪簭鍚嶇О
        char name[64] = {0};
        call_dispatcher(kVstEffectGetProgramNameIndexed, i, 0, name, 0.0f);
        program.name = std::string(name);
        
        // 淇濆瓨褰撳墠绋嬪簭鍙傛暟鍊?
        if (i == 0) {
            // 淇濆瓨褰撳墠鍙傛暟鐘舵€佷綔涓虹涓€涓璁?
            program.parameter_values.resize(parameter_values_.size());
            for (size_t j = 0; j < parameter_values_.size(); ++j) {
                program.parameter_values[j] = parameter_values_[j];
            }
        }
        
        programs_.push_back(program);
    }
    
    current_program_ = 0;
}

// vst_host瀹炵幇
vst_host::vst_host()
    : host_name_("foobar2000 VST Host")
    , host_version_("1.0.0")
    , host_vendor_("FB2K Compatible")
    , host_sample_rate_(44100.0)
    , host_block_size_(512)
    , host_process_precision_(kProcessPrecision32)
    , initialized_(false) {
    
    // 鍒濆鍖栨椂闂翠俊鎭?
    std::memset(&time_info_, 0, sizeof(time_info_));
    time_info_.sampleRate = host_sample_rate_;
    time_info_.flags = kVstTransportPlaying;
}

vst_host::~vst_host() {
    shutdown();
}

bool vst_host::initialize() {
    if (initialized_) return true;
    
    std::cout << "[VST Host] 鍒濆鍖朧ST瀹夸富" << std::endl;
    
    // 璁剧疆榛樿VST鐩綍
    std::vector<std::string> default_dirs = vst_utils::get_default_vst_directories();
    for (const auto& dir : default_dirs) {
        if (std::filesystem::exists(dir)) {
            scan_plugin_directory(dir);
        }
    }
    
    initialized_ = true;
    return true;
}

void vst_host::shutdown() {
    if (!initialized_) return;
    
    std::cout << "[VST Host] 鍏抽棴VST瀹夸富" << std::endl;
    
    // 鍗歌浇鎵€鏈夋彃浠?
    std::lock_guard<std::mutex> lock(host_mutex_);
    for (auto& [effect, plugin] : registered_plugins_) {
        if (plugin) {
            plugin->unload_plugin();
        }
    }
    registered_plugins_.clear();
    
    initialized_ = false;
}

vst_plugin_wrapper* vst_host::load_plugin(const std::string& dll_path) {
    if (!initialized_) return nullptr;
    
    auto* plugin = new vst_plugin_wrapper();
    if (plugin->load_plugin(dll_path)) {
        return plugin;
    }
    
    delete plugin;
    return nullptr;
}

void vst_host::unload_plugin(vst_plugin_wrapper* plugin) {
    if (plugin) {
        plugin->unload_plugin();
        delete plugin;
    }
}

std::vector<std::string> vst_host::scan_plugin_directory(const std::string& directory) {
    std::vector<std::string> plugins;
    
    if (!std::filesystem::exists(directory)) {
        return plugins;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".dll") {
                    if (is_vst_plugin(file_path)) {
                        plugins.push_back(file_path);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[VST Host] 鎵弿鎻掍欢鐩綍澶辫触: " << e.what() << std::endl;
    }
    
    return plugins;
}

void vst_host::register_plugin(AEffect* effect, vst_plugin_wrapper* plugin) {
    std::lock_guard<std::mutex> lock(host_mutex_);
    registered_plugins_[effect] = plugin;
}

void vst_host::unregister_plugin(AEffect* effect) {
    std::lock_guard<std::mutex> lock(host_mutex_);
    registered_plugins_.erase(effect);
}

vst_plugin_wrapper* vst_host::get_plugin_from_effect(AEffect* effect) {
    std::lock_guard<std::mutex> lock(host_mutex_);
    auto it = registered_plugins_.find(effect);
    return (it != registered_plugins_.end()) ? it->second : nullptr;
}

void vst_host::update_time_info(double sample_pos, double sample_rate, double tempo) {
    time_info_.samplePos = sample_pos;
    time_info_.sampleRate = sample_rate;
    time_info_.tempo = tempo;
}

bool vst_host::is_vst_plugin(const std::string& file_path) {
    // 妫€鏌LL鏄惁瀵煎嚭VST鍏ュ彛鐐?
    HMODULE dll = LoadLibraryA(file_path.c_str());
    if (!dll) return false;
    
    bool is_vst = (GetProcAddress(dll, "VSTPluginMain") != nullptr) ||
                  (GetProcAddress(dll, "main") != nullptr);
    
    FreeLibrary(dll);
    return is_vst;
}

// vst_bridge_manager瀹炵幇
vst_bridge_manager& vst_bridge_manager::get_instance() {
    static vst_bridge_manager instance;
    return instance;
}

vst_bridge_manager::vst_bridge_manager()
    : initialized_(false)
    , vst_buffer_size_(512)
    , vst_sample_rate_(44100.0) {
    
    // 璁剧疆榛樿VST鐩綍
    vst_directories_ = vst_utils::get_default_vst_directories();
}

vst_bridge_manager::~vst_bridge_manager() {
    shutdown();
}

bool vst_bridge_manager::initialize() {
    if (initialized_) return true;
    
    std::cout << "[VST Bridge] 鍒濆鍖朧ST妗ユ帴绠＄悊鍣? << std::endl;
    
    // 鍒涘缓VST瀹夸富
    vst_host_ = std::make_unique<vst_host>();
    if (!vst_host_->initialize()) {
        std::cerr << "[VST Bridge] VST瀹夸富鍒濆鍖栧け璐? << std::endl;
        return false;
    }
    
    // 閰嶇疆瀹夸富
    vst_host_->set_sample_rate(vst_sample_rate_);
    vst_host_->set_block_size(vst_buffer_size_);
    
    initialized_ = true;
    std::cout << "[VST Bridge] VST妗ユ帴绠＄悊鍣ㄥ垵濮嬪寲鎴愬姛" << std::endl;
    
    return true;
}

void vst_bridge_manager::shutdown() {
    if (!initialized_) return;
    
    std::cout << "[VST Bridge] 鍏抽棴VST妗ユ帴绠＄悊鍣? << std::endl;
    
    // 鍗歌浇鎵€鏈夋彃浠?
    {
        std::lock_guard<std::mutex> lock(bridge_mutex_);
        for (auto& [path, plugin] : loaded_plugins_) {
            if (plugin) {
                plugin->unload_plugin();
            }
        }
        loaded_plugins_.clear();
    }
    
    // 鍏抽棴VST瀹夸富
    if (vst_host_) {
        vst_host_->shutdown();
        vst_host_.reset();
    }
    
    initialized_ = false;
}

vst_plugin_wrapper* vst_bridge_manager::load_vst_plugin(const std::string& vst_path) {
    if (!initialized_) return nullptr;
    
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    
    // 妫€鏌ユ槸鍚﹀凡鍔犺浇
    auto it = loaded_plugins_.find(vst_path);
    if (it != loaded_plugins_.end()) {
        return it->second;
    }
    
    // 楠岃瘉璺緞
    if (!validate_vst_path(vst_path)) {
        return nullptr;
    }
    
    // 鍔犺浇鎻掍欢
    auto* plugin = vst_host_->load_plugin(vst_path);
    if (plugin) {
        loaded_plugins_[vst_path] = plugin;
        std::cout << "[VST Bridge] VST鎻掍欢鍔犺浇鎴愬姛: " << vst_path << std::endl;
    }
    
    return plugin;
}

void vst_bridge_manager::unload_vst_plugin(vst_plugin_wrapper* plugin) {
    if (!plugin || !initialized_) return;
    
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    
    // 鏌ユ壘骞跺嵏杞芥彃浠?
    for (auto it = loaded_plugins_.begin(); it != loaded_plugins_.end(); ++it) {
        if (it->second == plugin) {
            vst_host_->unload_plugin(plugin);
            loaded_plugins_.erase(it);
            std::cout << "[VST Bridge] VST鎻掍欢鍗歌浇鎴愬姛" << std::endl;
            break;
        }
    }
}

std::vector<std::string> vst_bridge_manager::scan_vst_plugins(const std::string& directory) {
    if (!initialized_) return {};
    
    return vst_host_->scan_plugin_directory(directory);
}

std::vector<std::string> vst_bridge_manager::get_vst_plugin_paths() const {
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    
    std::vector<std::string> paths;
    for (const auto& [path, plugin] : loaded_plugins_) {
        if (plugin) {
            paths.push_back(path);
        }
    }
    
    return paths;
}

std::unique_ptr<dsp_effect_advanced> vst_bridge_manager::create_vst_effect(const std::string& vst_path) {
    auto* plugin = load_vst_plugin(vst_path);
    if (!plugin) {
        return nullptr;
    }
    
    // 鍒涘缓DSP鏁堟灉鍣ㄥ寘瑁呭櫒
    auto effect = std::make_unique<dsp_effect_advanced>(plugin->get_params());
    
    // 杩欓噷搴旇璁剧疆VST鎻掍欢瀹炰緥鍒版晥鏋滃櫒涓?
    // 绠€鍖栧疄鐜帮細鐩存帴杩斿洖VST鍖呰鍣?
    return std::unique_ptr<dsp_effect_advanced>(plugin);
}

void vst_bridge_manager::set_vst_directories(const std::vector<std::string>& directories) {
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    vst_directories_ = directories;
}

std::vector<std::string> vst_bridge_manager::get_vst_directories() const {
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    return vst_directories_;
}

void vst_bridge_manager::add_vst_directory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    
    if (std::find(vst_directories_.begin(), vst_directories_.end(), directory) == vst_directories_.end()) {
        vst_directories_.push_back(directory);
    }
}

void vst_bridge_manager::remove_vst_directory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(bridge_mutex_);
    
    auto it = std::find(vst_directories_.begin(), vst_directories_.end(), directory);
    if (it != vst_directories_.end()) {
        vst_directories_.erase(it);
    }
}

void vst_bridge_manager::set_vst_buffer_size(int buffer_size) {
    vst_buffer_size_ = buffer_size;
    if (vst_host_) {
        vst_host_->set_block_size(buffer_size);
    }
}

int vst_bridge_manager::get_vst_buffer_size() const {
    return vst_buffer_size_;
}

void vst_bridge_manager::set_vst_sample_rate(double sample_rate) {
    vst_sample_rate_ = sample_rate;
    if (vst_host_) {
        vst_host_->set_sample_rate(sample_rate);
    }
}

double vst_bridge_manager::get_vst_sample_rate() const {
    return vst_sample_rate_;
}

bool vst_bridge_manager::validate_vst_path(const std::string& path) const {
    if (!std::filesystem::exists(path)) {
        return false;
    }
    
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }
    
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".dll";
}

// VST宸ュ叿鍑芥暟瀹炵幇
namespace vst_utils {

std::string get_vst_directory() {
    char buffer[MAX_PATH];
    if (GetEnvironmentVariableA("VST_PLUGINS", buffer, MAX_PATH) > 0) {
        return std::string(buffer);
    }
    
    return "";
}

std::vector<std::string> get_default_vst_directories() {
    std::vector<std::string> directories;
    
    // 绯荤粺VST鐩綍
    directories.push_back("C:\\Program Files\\VSTPlugins");
    directories.push_back("C:\\Program Files\\Common Files\\VST2");
    directories.push_back("C:\\Program Files\\Common Files\\Steinberg\\VST2");
    
    // 鐢ㄦ埛VST鐩綍
    char user_profile[MAX_PATH];
    if (GetEnvironmentVariableA("USERPROFILE", user_profile, MAX_PATH) > 0) {
        directories.push_back(std::string(user_profile) + "\\VSTPlugins");
    }
    
    // 浠庢敞鍐岃〃鑾峰彇VST鐩綍
    HKEY hkey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\VST", 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
        char value[MAX_PATH];
        DWORD value_size = MAX_PATH;
        DWORD value_type;
        
        if (RegQueryValueExA(hkey, "VSTPluginsPath", nullptr, &value_type, (LPBYTE)value, &value_size) == ERROR_SUCCESS) {
            directories.push_back(std::string(value));
        }
        
        RegCloseKey(hkey);
    }
    
    return directories;
}

bool is_vst_plugin(const std::string& file_path) {
    // 妫€鏌ユ枃浠舵墿灞曞悕
    std::string extension = std::filesystem::path(file_path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension != ".dll") {
        return false;
    }
    
    // 妫€鏌ユ槸鍚﹀鍑篤ST鍏ュ彛鐐?
    HMODULE dll = LoadLibraryA(file_path.c_str());
    if (!dll) return false;
    
    bool is_vst = (GetProcAddress(dll, "VSTPluginMain") != nullptr) ||
                  (GetProcAddress(dll, "main") != nullptr);
    
    FreeLibrary(dll);
    return is_vst;
}

std::string get_plugin_name_from_path(const std::string& path) {
    return std::filesystem::path(path).stem().string();
}

VstInt32 float_to_vst_param(float value) {
    return static_cast<VstInt32>(value * 16777216.0f); // 24浣嶇簿搴?
}

float vst_param_to_float(VstInt32 value) {
    return static_cast<float>(value) / 16777216.0f;
}

bool validate_vst_plugin(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }
    
    if (!is_vst_plugin(path)) {
        return false;
    }
    
    // 灏濊瘯鍔犺浇鎻掍欢杩涜楠岃瘉
    HMODULE dll = LoadLibraryA(path.c_str());
    if (!dll) return false;
    
    VstPluginMainFunc main_func = (VstPluginMainFunc)GetProcAddress(dll, "VSTPluginMain");
    if (!main_func) {
        main_func = (VstPluginMainFunc)GetProcAddress(dll, "main");
    }
    
    bool valid = (main_func != nullptr);
    
    FreeLibrary(dll);
    return valid;
}

std::string get_vst_error_category(VstInt32 error_code) {
    if (error_code >= 0) return "Success";
    
    switch (error_code) {
        case -1: return "General Error";
        case -2: return "Invalid Parameter";
        case -3: return "Out of Memory";
        case -4: return "File Not Found";
        case -5: return "Plugin Not Initialized";
        default: return "Unknown Error";
    }
}

} // namespace vst_utils

} // namespace fb2k