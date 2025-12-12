/**
 * @file xpumusic_plugin_sdk.h
 * @brief XpuMusic 鎻掍欢SDK - 鏍稿績鎺ュ彛瀹氫箟
 * @date 2025-12-10
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstring>

// 骞冲彴鐩稿叧瀹氫箟
#ifdef _WIN32
    #ifdef XPUMUSIC_PLUGIN_BUILD
        #define XPUMUSIC_PLUGIN_API __declspec(dllexport)
    #else
        #define XPUMUSIC_PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define XPUMUSIC_PLUGIN_API __attribute__((visibility("default")))
#endif

// API鐗堟湰
#define XPUMUSIC_PLUGIN_API_VERSION 1

namespace xpumusic {

// 鍓嶅悜澹版槑
class IPlugin;
class IPluginFactory;

// 鎻掍欢绫诲瀷鏋氫妇
enum class PluginType {
    Unknown = 0,
    AudioDecoder = 1,  // 闊抽瑙ｇ爜鍣?
    DSPEffect = 2,     // DSP鏁堟灉鍣?
    AudioOutput = 3,   // 闊抽杈撳嚭
    Visualization = 4, // 鍙鍖?
    Metadata = 5       // 鍏冩暟鎹鐞?
};

// 鎻掍欢淇℃伅缁撴瀯
struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    PluginType type;
    uint32_t api_version;
    std::vector<std::string> supported_formats;

    PluginInfo() : type(PluginType::Unknown), api_version(0) {}
};

// 闊抽鏍煎紡
struct AudioFormat {
    int sample_rate = 0;
    int channels = 0;
    int bits_per_sample = 0;
    bool is_float = false;
    int64_t channel_mask = 0;

    bool operator==(const AudioFormat& other) const {
        return sample_rate == other.sample_rate &&
               channels == other.channels &&
               bits_per_sample == other.bits_per_sample &&
               is_float == other.is_float;
    }

    bool operator!=(const AudioFormat& other) const {
        return !(*this == other);
    }
};

// 闊抽缂撳啿鍖?
struct AudioBuffer {
    float* data = nullptr;
    int frames = 0;
    int channels = 0;

    AudioBuffer() = default;
    AudioBuffer(float* d, int f, int c) : data(d), frames(f), channels(c) {}

    size_t size_bytes() const {
        return frames * channels * sizeof(float);
    }

    void clear() {
        if (data && frames > 0 && channels > 0) {
            memset(data, 0, size_bytes());
        }
    }
};

// 鍏冩暟鎹」
struct MetadataItem {
    std::string key;
    std::string value;

    MetadataItem() = default;
    MetadataItem(const std::string& k, const std::string& v) : key(k), value(v) {}
};

// 鎻掍欢鐘舵€佹灇涓?
enum class PluginState {
    Uninitialized,
    Initialized,
    Active,
    Error
};

// 鎻掍欢鍩虹被
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // 鎻掍欢鐢熷懡鍛ㄦ湡
    virtual bool initialize() = 0;
    virtual void finalize() = 0;

    // 鑾峰彇鎻掍欢淇℃伅
    virtual PluginInfo get_info() const = 0;

    // 鑾峰彇/璁剧疆鐘舵€?
    virtual PluginState get_state() const;
    virtual void set_state(PluginState state);

    // 閿欒澶勭悊
    virtual std::string get_last_error() const;

protected:
    PluginState state_ = PluginState::Uninitialized;
    std::string last_error_;

    // Helper method for derived classes
    void set_error(const std::string& error);
};

// 杈撳叆瑙ｇ爜鍣ㄦ帴鍙?
class IAudioDecoder : public IPlugin {
public:
    virtual ~IAudioDecoder() = default;

    // 鏍煎紡鏀寔
    virtual bool can_decode(const std::string& file_path) = 0;
    virtual std::vector<std::string> get_supported_extensions() = 0;

    // 瑙ｇ爜鎿嶄綔
    virtual bool open(const std::string& file_path) = 0;
    virtual int decode(AudioBuffer& buffer, int max_frames) = 0;
    virtual bool seek(int64_t sample_pos) = 0;
    virtual void close() = 0;

    // 闊抽淇℃伅
    virtual AudioFormat get_format() const = 0;
    virtual int64_t get_length() const = 0;
    virtual double get_duration() const = 0;

    // 鍏冩暟鎹?
    virtual std::vector<MetadataItem> get_metadata() = 0;
    virtual std::string get_metadata_value(const std::string& key) = 0;

    // 浣嶇疆淇℃伅
    virtual int64_t get_position() const = 0;
    virtual bool is_eof() const = 0;
};

// DSP鏁堟灉鍣ㄦ帴鍙?
class IDSPProcessor : public IPlugin {
public:
    virtual ~IDSPProcessor() = default;

    // 澶勭悊閰嶇疆
    virtual bool configure(const AudioFormat& input_format,
                          const AudioFormat& output_format) = 0;

    // 澶勭悊闊抽
    virtual int process(const AudioBuffer& input,
                       AudioBuffer& output) = 0;

    // 鍙傛暟鎺у埗
    virtual void set_parameter(const std::string& name, double value) = 0;
    virtual double get_parameter(const std::string& name) = 0;
    virtual std::vector<std::string> get_parameter_names() = 0;

    // 閲嶇疆
    virtual void reset() = 0;

    // 寤惰繜淇℃伅
    virtual int get_latency_samples() const = 0;
};

// 杈撳嚭璁惧鎺ュ彛
class IAudioOutput : public IPlugin {
public:
    virtual ~IAudioOutput() = default;

    // 璁惧绠＄悊
    virtual std::vector<std::string> get_devices() = 0;
    virtual bool open_device(const std::string& device_id,
                            const AudioFormat& format) = 0;
    virtual void close_device() = 0;

    // 闊抽杈撳嚭
    virtual int write(const AudioBuffer& buffer) = 0;
    virtual int get_latency() = 0;
    virtual void flush() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;

    // 璁惧淇℃伅
    virtual AudioFormat get_device_format(const std::string& device_id) = 0;
    virtual std::string get_current_device() const = 0;
};

// 鍙鍖栨彃浠舵帴鍙?
class IVisualization : public IPlugin {
public:
    virtual ~IVisualization() = default;

    // 娓叉煋璁剧疆
    virtual bool initialize_render(int width, int height) = 0;
    virtual void finalize_render() = 0;

    // 闊抽鏁版嵁杈撳叆
    virtual void process_audio(const AudioBuffer& buffer) = 0;

    // 娓叉煋
    virtual void render(void* target, int stride) = 0;

    // 閰嶇疆
    virtual void set_background_color(uint32_t color) = 0;
    virtual void set_foreground_color(uint32_t color) = 0;
};

// 鎻掍欢宸ュ巶鎺ュ彛
class IPluginFactory {
public:
    virtual ~IPluginFactory() = default;

    // 鍒涘缓鎻掍欢瀹炰緥
    virtual std::unique_ptr<IPlugin> create() = 0;

    // 鑾峰彇鎻掍欢淇℃伅
    virtual PluginInfo get_info() const = 0;

    // 妫€鏌ュ吋瀹规€?
    virtual bool is_compatible(uint32_t host_api_version) const {
        return get_info().api_version <= host_api_version;
    }
};

// 鐗瑰寲鐨勫伐鍘傛帴鍙?
template<typename Interface>
class ITypedPluginFactory : public IPluginFactory {
public:
    virtual ~ITypedPluginFactory() = default;

    // 鍒涘缓绫诲瀷鍖栫殑鎻掍欢瀹炰緥
    virtual std::unique_ptr<Interface> create_typed() = 0;

    // 鍩虹被鎺ュ彛瀹炵幇
    std::unique_ptr<IPlugin> create() override {
        return std::unique_ptr<IPlugin>(create_typed().release());
    }
};

// 鎻掍欢娉ㄥ唽鍥炶皟
using PluginRegisterCallback = std::function<void(std::unique_ptr<IPluginFactory>)>;

// 鎻掍欢瀵煎嚭瀹?
#define XPUMUSIC_DECLARE_PLUGIN(PluginClass, FactoryClass) \
    extern "C" { \
        XPUMUSIC_PLUGIN_API xpumusic::IPluginFactory* xpumusic_create_plugin_factory() { \
            return new FactoryClass(); \
        } \
        XPUMUSIC_PLUGIN_API const char* xpumusic_get_plugin_name() { \
            static PluginClass dummy; \
            return dummy.get_info().name.c_str(); \
        } \
    }

// 渚垮埄瀹?
#define XPUMUSIC_EXPORT_AUDIO_PLUGIN(PluginClass) \
    XPUMUSIC_DECLARE_PLUGIN(PluginClass, PluginClass##Factory)

#define XPUMUSIC_EXPORT_DSP_PLUGIN(PluginClass) \
    XPUMUSIC_DECLARE_PLUGIN(PluginClass, PluginClass##Factory)

} // namespace xpumusic