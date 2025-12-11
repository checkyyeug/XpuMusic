#pragma once

#include "../stage1_3/dsp_effect.h"
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

// VST SDK接口定义（简化版）

#define VST_FORCE_DEPRECATED 0

#ifdef _WIN32
#pragma pack(push, 8)
#endif

typedef long VstInt32;
typedef long long VstInt64;
typedef float VstFloat;
typedef double VstDouble;

// VST音频缓冲区格式
enum VstSpeakerArrangementType {
    kSpeakerArrMono = 1,
    kSpeakerArrStereo = 2,
    kSpeakerArrStereoSurround = 3,
    kSpeakerArrStereoCenter = 4,
    kSpeakerArrStereoSide = 5,
    kSpeakerArrStereoWide = 6,
    kSpeakerArr3_0 = 7,
    kSpeakerArr4_0 = 8,
    kSpeakerArr5_0 = 9,
    kSpeakerArr5_1 = 10,
    kSpeakerArr6_0 = 11,
    kSpeakerArr6_1 = 12,
    kSpeakerArr7_0 = 13,
    kSpeakerArr7_1 = 14
};

// VST处理精度
enum VstProcessPrecision {
    kProcessPrecision32 = 0,
    kProcessPrecision64 = 1
};

// VST插件标志
enum VstPluginFlags {
    kPluginHasEditor = 1 << 0,
    kPluginCanMono = 1 << 1,
    kPluginCanReplacing = 1 << 2,
    kPluginProgramsChunks = 1 << 5,
    kPluginIsSynth = 1 << 8,
    kPluginNoSoundInStop = 1 << 9,
    kPluginBufferSizeDependent = 1 << 12
};

// VST事件类型
enum VstEventType {
    kVstMidiType = 1,
    kVstAudioType = 2,
    kVstVideoType = 3,
    kVstParameterType = 4,
    kVstTriggerType = 5,
    kVstSysExType = 6
};

// VST事件结构
struct VstEvent {
    VstEventType type;
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    VstInt32 data1;
    VstInt32 data2;
    VstInt32 data3;
    VstInt32 data4;
    VstInt64 data5;
    void* data6;
    void* data7;
    void* data8;
};

// VST事件列表
struct VstEvents {
    VstInt32 numEvents;
    void* reserved;
    VstEvent* events[256];
};

// VST音频缓冲区
struct VstAudioBuffer {
    VstFloat** channels;
    VstInt32 numChannels;
    VstInt32 size;
};

// VST处理参数
struct VstProcessParameter {
    VstInt32 index;
    VstFloat value;
    VstInt32 flags;
    char label[64];
    char shortLabel[8];
    char units[16];
    VstInt32 precision;
    VstInt32 stepCount;
    VstFloat defaultValue;
    VstFloat minValue;
    VstFloat maxValue;
};

// VST插件信息
struct VstPluginInfo {
    char name[64];
    char vendor[64];
    VstInt32 version;
    VstInt32 uniqueID;
    VstInt32 pluginType;
    VstInt32 numInputs;
    VstInt32 numOutputs;
    VstInt32 numParameters;
    VstInt32 numPrograms;
    VstInt32 flags;
    VstInt32 initialDelay;
    char filePath[MAX_PATH];
};

// VST主机回调函数类型
typedef VstInt32 (*VstHostCallback)(VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt);

// VST主操作码
enum VstHostOpcodes {
    kVstAudioMasterAutomate = 0,
    kVstAudioMasterVersion = 1,
    kVstAudioMasterCurrentId = 2,
    kVstAudioMasterIdle = 3,
    kVstAudioMasterPinConnected = 4,
    kVstAudioMasterGetTime = 5,
    kVstAudioMasterProcessEvents = 6,
    kVstAudioMasterIOChanged = 7,
    kVstAudioMasterSizeWindow = 8,
    kVstAudioMasterGetSampleRate = 9,
    kVstAudioMasterGetBlockSize = 10,
    kVstAudioMasterGetInputLatency = 11,
    kVstAudioMasterGetOutputLatency = 12,
    kVstAudioMasterGetCurrentProcessLevel = 20,
    kVstAudioMasterGetAutomationState = 21
};

// VST插件操作码
enum VstPluginOpcodes {
    kVstEffectOpen = 0,
    kVstEffectClose = 1,
    kVstEffectSetProgram = 2,
    kVstEffectGetProgram = 3,
    kVstEffectSetProgramName = 4,
    kVstEffectGetProgramName = 5,
    kVstEffectGetParamLabel = 6,
    kVstEffectGetParamDisplay = 7,
    kVstEffectGetParamName = 8,
    kVstEffectSetSampleRate = 10,
    kVstEffectSetBlockSize = 11,
    kVstEffectMainsChanged = 12,
    kVstEffectEditGetRect = 13,
    kVstEffectEditOpen = 14,
    kVstEffectEditClose = 15,
    kVstEffectEditIdle = 19,
    kVstEffectProcess = 24,
    kVstEffectProcessEvents = 25,
    kVstEffectSetParameter = 26,
    kVstEffectGetParameter = 27,
    kVstEffectGetProgramNameIndexed = 29,
    kVstEffectGetInputProperties = 33,
    kVstEffectGetOutputProperties = 34,
    kVstEffectGetPlugCategory = 35,
    kVstEffectSetSpeakerArrangement = 36,
    kVstEffectGetProgramNameIndexed = 29,
    kVstEffectGetProgramNameIndexed = 29,
    kVstEffectBeginSetProgram = 38,
    kVstEffectEndSetProgram = 39,
    kVstEffectGetVU = 48,
    kVstEffectCanDo = 51,
    kVstEffectGetTailSize = 52,
    kVstEffectGetEffectName = 54,
    kVstEffectGetVendorString = 55,
    kVstEffectGetProductString = 56,
    kVstEffectGetVendorVersion = 57,
    kVstEffectVendorSpecific = 58,
    kVstEffectGetIcon = 59,
    kVstEffectSetViewPosition = 60,
    kVstEffectGetParameterProperties = 61,
    kVstEffectKeysRequired = 62,
    kVstEffectGetVstVersion = 63
};

// VST时间信息
struct VstTimeInfo {
    double samplePos;
    double sampleRate;
    double nanoSeconds;
    double ppqPos;
    double tempo;
    double barStartPos;
    VstInt32 timeSigNumerator;
    VstInt32 timeSigDenominator;
    VstInt32 flags;
};

// VST时间信息标志
enum VstTimeInfoFlags {
    kVstTransportChanged = 1,
    kVstTransportPlaying = 2,
    kVstTransportCycleActive = 4,
    kVstTransportRecording = 8,
    kVstAutomationWriting = 16,
    kVstAutomationReading = 32
};

// AEffect结构 - VST插件主结构
typedef struct AEffect {
    VstInt32 magic;           // 'VstP'
    VstInt32 dispatcher;      // 函数指针
    VstInt32 process;         // 函数指针
    VstInt32 setParameter;    // 函数指针
    VstInt32 getParameter;    // 函数指针
    VstInt32 numPrograms;
    VstInt32 numParams;
    VstInt32 numInputs;
    VstInt32 numOutputs;
    VstInt32 flags;
    VstIntPtr resvd1;
    VstIntPtr resvd2;
    VstInt32 initialDelay;
    VstInt32 realQualities;
    VstInt32 offQualities;
    VstFloat ioRatio;
    void* object;
    void* user;
    VstInt32 uniqueID;
    VstInt32 version;
    VstHostCallback hostCallback;
} AEffect;

// VST插件主函数类型
typedef AEffect* (*VstPluginMainFunc)(VstHostCallback hostCallback);

namespace fb2k {

// VST参数信息
struct vst_parameter_info {
    std::string name;
    std::string label;
    std::string units;
    float default_value;
    float min_value;
    float max_value;
    float step_size;
    bool is_automatable;
    bool is_discrete;
    int discrete_steps;
};

// VST程序（预设）信息
struct vst_program_info {
    std::string name;
    std::vector<float> parameter_values;
};

// VST插件包装器接口
class vst_plugin_wrapper : public dsp_effect_advanced {
public:
    vst_plugin_wrapper();
    ~vst_plugin_wrapper() override;
    
    // 初始化和配置
    bool load_plugin(const std::string& dll_path);
    void unload_plugin();
    bool is_plugin_loaded() const { return plugin_loaded_; }
    
    // 插件信息
    std::string get_plugin_name() const;
    std::string get_plugin_vendor() const;
    std::string get_plugin_version() const;
    int get_num_inputs() const;
    int get_num_outputs() const;
    int get_num_parameters() const;
    int get_num_programs() const;
    
    // 参数管理
    std::vector<vst_parameter_info> get_parameter_info() const;
    float get_parameter_value(int index) const;
    void set_parameter_value(int index, float value);
    std::string get_parameter_label(int index) const;
    std::string get_parameter_display(int index) const;
    
    // 程序（预设）管理
    std::vector<vst_program_info> get_programs() const;
    int get_current_program() const;
    void set_current_program(int index);
    std::string get_program_name(int index) const;
    void set_program_name(int index, const std::string& name);
    
    // 音频处理
    bool process_audio(const float** inputs, float** outputs, int num_samples);
    void process_events(const VstEvents* events);
    
    // 配置
    bool get_chunk(std::vector<uint8_t>& data);
    bool set_chunk(const std::vector<uint8_t>& data);
    
    // 编辑器支持
    bool has_editor() const;
    bool open_editor(void* parent_window);
    void close_editor();
    bool get_editor_size(int& width, int& height) const;
    void resize_editor(int width, int height);
    
    // 实时参数
    void set_realtime_parameter(const std::string& param_name, float value) override;
    float get_realtime_parameter(const std::string& param_name) const override;
    std::vector<parameter_info> get_realtime_parameters() const override;
    
protected:
    // dsp_effect_advanced 实现
    bool instantiate(audio_chunk& chunk, uint32_t sample_rate, uint32_t channels) override;
    void run(audio_chunk& chunk, abort_callback& abort) override;
    void reset() override;
    
private:
    // VST插件实例
    AEffect* vst_effect_;
    HMODULE vst_dll_;
    std::string vst_plugin_path_;
    bool plugin_loaded_;
    
    // 音频格式
    double current_sample_rate_;
    int current_block_size_;
    
    // 参数缓存
    std::vector<float> parameter_values_;
    std::vector<vst_parameter_info> parameter_info_;
    
    // 程序缓存
    std::vector<vst_program_info> programs_;
    int current_program_;
    
    // 编辑器状态
    bool editor_open_;
    void* editor_window_;
    
    // 性能统计
    std::atomic<int64_t> total_samples_processed_;
    std::atomic<double> total_processing_time_;
    
    // 线程安全
    mutable std::mutex vst_mutex_;
    
    // VST主机回调
    static VstInt32 VSTCALLBACK host_callback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt);
    VstInt32 handle_host_callback(VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt);
    
    // VST操作辅助函数
    VstInt32 call_dispatcher(VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, VstFloat opt);
    void call_set_parameter(VstInt32 index, VstFloat parameter);
    VstFloat call_get_parameter(VstInt32 index);
    
    // 初始化和配置
    bool initialize_vst_plugin();
    bool configure_vst_plugin();
    void extract_plugin_info();
    void extract_parameter_info();
    void extract_program_info();
    
    // 音频格式转换
    void convert_audio_format(const audio_chunk& chunk);
    void convert_audio_format_to_chunk(float** outputs, audio_chunk& chunk);
    
    // 错误处理
    void handle_vst_error(const std::string& operation, VstInt32 error_code);
    std::string get_vst_error_string(VstInt32 error_code) const;
};

// VST主机实现
class vst_host {
public:
    vst_host();
    ~vst_host();
    
    // 主机管理
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // 插件管理
    vst_plugin_wrapper* load_plugin(const std::string& dll_path);
    void unload_plugin(vst_plugin_wrapper* plugin);
    std::vector<std::string> scan_plugin_directory(const std::string& directory);
    
    // 宿主信息
    std::string get_host_name() const { return host_name_; }
    std::string get_host_version() const { return host_version_; }
    std::string get_host_vendor() const { return host_vendor_; }
    
    // 全局设置
    void set_sample_rate(double sample_rate) { host_sample_rate_ = sample_rate; }
    double get_sample_rate() const { return host_sample_rate_; }
    void set_block_size(int block_size) { host_block_size_ = block_size; }
    int get_block_size() const { return host_block_size_; }
    
    // 时间信息
    VstTimeInfo* get_time_info() { return &time_info_; }
    void update_time_info(double sample_pos, double sample_rate, double tempo);
    
    // 插件回调管理
    void register_plugin(AEffect* effect, vst_plugin_wrapper* plugin);
    void unregister_plugin(AEffect* effect);
    vst_plugin_wrapper* get_plugin_from_effect(AEffect* effect);
    
private:
    std::string host_name_;
    std::string host_version_;
    std::string host_vendor_;
    
    double host_sample_rate_;
    int host_block_size_;
    VstProcessPrecision host_process_precision_;
    
    VstTimeInfo time_info_;
    
    std::map<AEffect*, vst_plugin_wrapper*> registered_plugins_;
    std::mutex host_mutex_;
    
    bool initialized_;
    
    // 插件扫描
    bool is_vst_plugin(const std::string& file_path);
    std::string get_plugin_name_from_dll(const std::string& dll_path);
};

// VST桥接管理器
class vst_bridge_manager {
public:
    static vst_bridge_manager& get_instance();
    
    // 桥接管理
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // VST插件管理
    vst_plugin_wrapper* load_vst_plugin(const std::string& vst_path);
    void unload_vst_plugin(vst_plugin_wrapper* plugin);
    std::vector<std::string> scan_vst_plugins(const std::string& directory);
    std::vector<std::string> get_vst_plugin_paths() const;
    
    // DSP效果器创建
    std::unique_ptr<dsp_effect_advanced> create_vst_effect(const std::string& vst_path);
    std::vector<std::unique_ptr<dsp_effect_advanced>> create_vst_effects_from_directory(const std::string& directory);
    
    // 配置管理
    void set_vst_directories(const std::vector<std::string>& directories);
    std::vector<std::string> get_vst_directories() const;
    void add_vst_directory(const std::string& directory);
    void remove_vst_directory(const std::string& directory);
    
    // 性能设置
    void set_vst_buffer_size(int buffer_size);
    int get_vst_buffer_size() const;
    void set_vst_sample_rate(double sample_rate);
    double get_vst_sample_rate() const;
    
private:
    vst_bridge_manager();
    ~vst_bridge_manager();
    vst_bridge_manager(const vst_bridge_manager&) = delete;
    vst_bridge_manager& operator=(const vst_bridge_manager&) = delete;
    
    std::unique_ptr<vst_host> vst_host_;
    std::vector<std::string> vst_directories_;
    std::map<std::string, vst_plugin_wrapper*> loaded_plugins_;
    std::mutex bridge_mutex_;
    
    bool initialized_;
    int vst_buffer_size_;
    double vst_sample_rate_;
    
    void scan_all_directories();
    bool validate_vst_path(const std::string& path) const;
};

// VST参数自动化
class vst_parameter_automation {
public:
    vst_parameter_automation(vst_plugin_wrapper* plugin);
    ~vst_parameter_automation();
    
    // 自动化录制
    void start_recording();
    void stop_recording();
    bool is_recording() const { return recording_; }
    
    // 自动化播放
    void start_playback();
    void stop_playback();
    bool is_playing() const { return playing_; }
    
    // 参数自动化
    void record_parameter_change(int parameter_index, float value, double time);
    void apply_automation(double current_time);
    
    // 自动化数据管理
    void clear_automation();
    bool save_automation(const std::string& file_path);
    bool load_automation(const std::string& file_path);
    
    // 编辑功能
    void add_automation_point(int parameter_index, double time, float value);
    void remove_automation_point(int parameter_index, int point_index);
    void move_automation_point(int parameter_index, int point_index, double new_time, float new_value);
    
private:
    struct automation_point {
        double time;
        float value;
        bool is_tempered; // 是否经过手动调整
    };
    
    struct parameter_automation {
        int parameter_index;
        std::vector<automation_point> points;
        std::string parameter_name;
    };
    
    vst_plugin_wrapper* plugin_;
    std::map<int, parameter_automation> automation_data_;
    std::mutex automation_mutex_;
    
    bool recording_;
    bool playing_;
    double start_time_;
    double current_time_;
    
    void interpolate_parameter_value(int parameter_index, double time, float& value);
    float calculate_interpolated_value(const std::vector<automation_point>& points, double time);
};

// VST预设管理器
class vst_preset_manager {
public:
    vst_preset_manager(vst_plugin_wrapper* plugin);
    ~vst_preset_manager();
    
    // 预设管理
    bool save_preset(const std::string& name);
    bool load_preset(const std::string& name);
    bool delete_preset(const std::string& name);
    std::vector<std::string> get_preset_list() const;
    bool preset_exists(const std::string& name) const;
    
    // 预设文件管理
    bool export_preset(const std::string& name, const std::string& file_path);
    bool import_preset(const std::string& file_path, const std::string& name);
    bool export_bank(const std::string& file_path);
    bool import_bank(const std::string& file_path);
    
    // 工厂预设
    void load_factory_presets();
    bool has_factory_presets() const;
    std::vector<std::string> get_factory_preset_names() const;
    
private:
    vst_plugin_wrapper* plugin_;
    std::map<std::string, std::vector<uint8_t>> presets_;
    std::map<std::string, std::vector<uint8_t>> factory_presets_;
    mutable std::mutex preset_mutex_;
    
    std::string get_preset_file_path(const std::string& name) const;
    std::string get_preset_directory() const;
    bool ensure_preset_directory_exists() const;
    
    std::vector<uint8_t> serialize_preset() const;
    bool deserialize_preset(const std::vector<uint8_t>& data);
};

// VST错误处理
class vst_error_handler {
public:
    static std::string get_vst_error_string(VstInt32 error_code);
    static void handle_vst_error(const std::string& operation, VstInt32 error_code);
    static bool is_recoverable_error(VstInt32 error_code);
    static void log_vst_error(const std::string& context, VstInt32 error_code);
};

// VST工具函数
namespace vst_utils {
    std::string get_vst_directory();
    std::vector<std::string> get_default_vst_directories();
    bool is_vst_plugin(const std::string& file_path);
    std::string get_plugin_name_from_path(const std::string& path);
    VstInt32 float_to_vst_param(float value);
    float vst_param_to_float(VstInt32 value);
    bool validate_vst_plugin(const std::string& path);
    std::string get_vst_error_category(VstInt32 error_code);
}

} // namespace fb2k

#ifdef _WIN32
#pragma pack(pop)
#endif