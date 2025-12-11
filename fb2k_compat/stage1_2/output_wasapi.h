#pragma once

// 阶段1.2：WASAPI输出设备
// Windows Audio Session API (WASAPI) 输出实现

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
#include "output_interfaces.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace fb2k {

using Microsoft::WRL::ComPtr;

// WASAPI输出设备实现
class output_wasapi : public output_device_base {
private:
    // WASAPI接口
    ComPtr<IMMDevice> device_;
    ComPtr<IAudioClient> audio_client_;
    ComPtr<IAudioRenderClient> render_client_;
    ComPtr<IAudioClock> audio_clock_;
    
    // 设备信息
    std::wstring device_id_;
    std::wstring device_name_;
    bool is_default_device_;
    
    // 音频格式
    WAVEFORMATEX* mix_format_;
    WAVEFORMATEXTENSIBLE* mix_format_ext_;
    bool use_extensible_;
    
    // 缓冲管理
    audio_buffer buffer_;
    std::vector<float> convert_buffer_;
    std::vector<uint8_t> output_buffer_;
    
    // 线程管理
    std::unique_ptr<std::thread> render_thread_;
    std::atomic<bool> should_stop_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    // 状态管理
    std::atomic<uint32_t> buffer_frame_count_;
    std::atomic<uint32_t> padding_frames_;
    std::atomic<double> clock_frequency_;
    
    // 配置
    bool exclusive_mode_;
    bool event_driven_;
    size_t buffer_duration_ms_;
    size_t period_duration_ms_;
    
public:
    output_wasapi();
    ~output_wasapi() override;
    
    // 设备选择
    bool initialize(const wchar_t* device_id = nullptr);
    void shutdown();
    
    // output_device 接口实现
    bool open(uint32_t sample_rate, uint32_t channels, abort_callback& abort) override;
    void close(abort_callback& abort) override;
    void process_chunk(audio_chunk& chunk, abort_callback& abort) override;
    void flush(abort_callback& abort) override;
    
    bool can_update_format() const override { return true; }
    bool set_format(const output_format& format, abort_callback& abort) override;
    output_format get_current_format() const override;
    std::vector<output_format> get_supported_formats() const override;
    
    double get_latency() const override;
    size_t get_buffer_size() const override;
    bool set_buffer_size(size_t size, abort_callback& abort) override;
    
    const char* get_name() const override;
    const char* get_description() const override;
    output_device_caps get_device_caps() const override;
    
    bool supports_exclusive_mode() const override { return true; }
    bool set_exclusive_mode(bool exclusive, abort_callback& abort) override;
    bool get_exclusive_mode() const override { return exclusive_mode_; }
    
private:
    // WASAPI初始化
    bool initialize_wasapi(abort_callback& abort);
    void shutdown_wasapi();
    
    // 设备枚举
    bool enumerate_devices();
    bool get_default_device();
    
    // 音频格式处理
    bool get_mix_format();
    bool negotiate_audio_format(const output_format& format);
    HRESULT create_audio_format(const output_format& format, WAVEFORMATEX** format_out);
    void free_audio_format();
    
    // 音频客户端管理
    HRESULT initialize_audio_client(abort_callback& abort);
    HRESULT start_audio_client();
    HRESULT stop_audio_client();
    HRESULT reset_audio_client();
    
    // 渲染线程
    void render_thread_func();
    HRESULT render_audio();
    HRESULT get_current_padding(uint32_t& padding);
    HRESULT get_device_frequency(double& frequency);
    
    // 音频处理
    HRESULT write_audio_data(const float* data, size_t samples);
    HRESULT convert_and_write(const audio_chunk& chunk);
    
    // 格式转换
    HRESULT convert_to_wasapi_format(const audio_chunk& chunk, std::vector<uint8_t>& output);
    
    // 错误处理
    const char* wasapi_error_to_string(HRESULT hr);
    void log_wasapi_error(const char* operation, HRESULT hr);
    
    // 辅助函数
    REFERENCE_TIME milliseconds_to_reference_time(uint32_t ms) const {
        return static_cast<REFERENCE_TIME>(ms) * 10000; // 100ns units
    }
    
    uint32_t reference_time_to_milliseconds(REFERENCE_TIME ref_time) const {
        return static_cast<uint32_t>(ref_time / 10000); // 100ns units
    }
};

// WASAPI设备枚举器
class wasapi_device_enumerator {
public:
    struct wasapi_device_info {
        std::wstring id;
        std::wstring name;
        std::wstring description;
        bool is_default;
        bool is_immersive;
        output_device_caps caps;
    };
    
    static std::vector<wasapi_device_info> enumerate_devices() {
        std::vector<wasapi_device_info> devices;
        
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            return devices;
        }
        
        ComPtr<IMMDeviceEnumerator> device_enumerator;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                             CLSCTX_ALL, IID_PPV_ARGS(&device_enumerator));
        
        if(FAILED(hr)) {
            return devices;
        }
        
        // 获取默认设备
        ComPtr<IMMDevice> default_device;
        hr = device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &default_device);
        
        if(SUCCEEDED(hr)) {
            wasapi_device_info default_info;
            if(get_device_info(default_device.Get(), default_info)) {
                default_info.is_default = true;
                devices.push_back(default_info);
            }
        }
        
        // 枚举所有渲染设备
        ComPtr<IMMDeviceCollection> device_collection;
        hr = device_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &device_collection);
        
        if(SUCCEEDED(hr)) {
            UINT device_count = 0;
            device_collection->GetCount(&device_count);
            
            for(UINT i = 0; i < device_count; ++i) {
                ComPtr<IMMDevice> device;
                hr = device_collection->Item(i, &device);
                
                if(SUCCEEDED(hr)) {
                    wasapi_device_info info;
                    if(get_device_info(device.Get(), info)) {
                        // 避免重复添加默认设备
                        if(!info.is_default) {
                            devices.push_back(info);
                        }
                    }
                }
            }
        }
        
        return devices;
    }
    
private:
    static bool get_device_info(IMMDevice* device, wasapi_device_info& info) {
        if(!device) return false;
        
        // 获取设备ID
        LPWSTR device_id = nullptr;
        HRESULT hr = device->GetId(&device_id);
        if(FAILED(hr)) return false;
        
        info.id = device_id;
        CoTaskMemFree(device_id);
        
        // 获取设备属性
        ComPtr<IPropertyStore> prop_store;
        hr = device->OpenPropertyStore(STGM_READ, &prop_store);
        if(FAILED(hr)) return false;
        
        // 获取设备名称
        PROPVARIANT prop_var;
        PropVariantInit(&prop_var);
        hr = prop_store->GetValue(PKEY_Device_FriendlyName, &prop_var);
        if(SUCCEEDED(hr)) {
            info.name = prop_var.pwszVal;
            PropVariantClear(&prop_var);
        }
        
        // 获取设备描述
        PropVariantInit(&prop_var);
        hr = prop_store->GetValue(PKEY_Device_DeviceDesc, &prop_var);
        if(SUCCEEDED(hr)) {
            info.description = prop_var.pwszVal;
            PropVariantClear(&prop_var);
        }
        
        // 检查是否为沉浸式设备
        PropVariantInit(&prop_var);
        hr = prop_store->GetValue(PKEY_AudioEndpoint_Association, &prop_var);
        if(SUCCEEDED(hr)) {
            info.is_immersive = (prop_var.vt == VT_LPWSTR && 
                                std::wstring(prop_var.pwszVal).find(L"MSHPLUG") != std::wstring::npos);
            PropVariantClear(&prop_var);
        }
        
        // 获取设备能力
        info.caps = get_device_capabilities(device);
        
        return true;
    }
    
    static output_device_caps get_device_capabilities(IMMDevice* device) {
        output_device_caps caps;
        
        // 打开音频客户端获取格式信息
        ComPtr<IAudioClient> audio_client;
        HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                                     nullptr, reinterpret_cast<void**>(&audio_client));
        
        if(SUCCEEDED(hr)) {
            // 获取混合格式
            WAVEFORMATEX* mix_format = nullptr;
            hr = audio_client->GetMixFormat(&mix_format);
            
            if(SUCCEEDED(hr) && mix_format) {
                // 添加支持的格式
                output_format format;
                format.sample_rate = mix_format->nSamplesPerSec;
                format.channels = mix_format->nChannels;
                format.bits_per_sample = mix_format->wBitsPerSample;
                format.format = audio_format::float32; // WASAPI默认使用浮点
                
                if(format.is_valid()) {
                    caps.supported_formats.push_back(format);
                }
                
                // 尝试其他常见格式
                add_common_formats(caps, mix_format->nSamplesPerSec, mix_format->nChannels);
                
                CoTaskMemFree(mix_format);
            }
            
            // 获取设备周期信息
            REFERENCE_TIME default_period = 0;
            REFERENCE_TIME minimum_period = 0;
            hr = audio_client->GetDevicePeriod(&default_period, &minimum_period);
            
            if(SUCCEEDED(hr)) {
                caps.min_latency_ms = static_cast<double>(minimum_period) / 10000.0; // 转换为毫秒
                caps.max_latency_ms = static_cast<double>(default_period) / 10000.0;
            }
            
            // 检查独占模式支持
            caps.supports_exclusive_mode = true; // WASAPI支持独占模式
            caps.supports_event_driven = true;   // WASAPI支持事件驱动
        }
        
        return caps;
    }
    
    static void add_common_formats(output_device_caps& caps, uint32_t sample_rate, uint32_t channels) {
        // 添加常见的音频格式
        struct format_info {
            uint32_t rate;
            uint32_t channels;
            uint32_t bits;
            audio_format format;
        };
        
        const format_info common_formats[] = {
            {44100, 2, 16, audio_format::int16},
            {44100, 2, 24, audio_format::int24},
            {48000, 2, 16, audio_format::int16},
            {48000, 2, 24, audio_format::int24},
            {96000, 2, 24, audio_format::int24},
            {192000, 2, 24, audio_format::int24}
        };
        
        for(const auto& fmt : common_formats) {
            if(fmt.rate <= sample_rate && fmt.channels <= channels) {
                output_format format;
                format.sample_rate = fmt.rate;
                format.channels = fmt.channels;
                format.bits_per_sample = fmt.bits;
                format.format = fmt.format;
                
                if(format.is_valid()) {
                    caps.supported_formats.push_back(format);
                }
            }
        }
    }
};

// WASAPI输出设备具体实现（框架）
// 完整的实现需要大量的WASAPI相关代码
// 这里提供接口定义和基础框架

output_wasapi::output_wasapi() 
    : mix_format_(nullptr), mix_format_ext_(nullptr), use_extensible_(false),
      exclusive_mode_(false), event_driven_(true), 
      buffer_duration_ms_(100), period_duration_ms_(10), should_stop_(false) {
}

output_wasapi::~output_wasapi() {
    shutdown();
}

bool output_wasapi::initialize(const wchar_t* device_id) {
    device_id_ = device_id ? device_id : L"";
    
    if(!enumerate_devices()) {
        return false;
    }
    
    if(device_id_.empty()) {
        if(!get_default_device()) {
            return false;
        }
    }
    
    return true;
}

void output_wasapi::shutdown() {
    if(is_open()) {
        abort_callback_dummy abort;
        close(abort);
    }
}

bool output_wasapi::open(uint32_t sample_rate, uint32_t channels, abort_callback& abort) {
    if(is_open()) {
        return false; // 已经打开
    }
    
    set_state(output_state::opening);
    
    // 初始化WASAPI
    if(!initialize_wasapi(abort)) {
        set_state(output_state::error);
        return false;
    }
    
    set_state(output_state::open);
    return true;
}

void output_wasapi::close(abort_callback& abort) {
    if(!is_open()) return;
    
    set_state(output_state::closed);
    shutdown_wasapi();
}

void output_wasapi::process_chunk(audio_chunk& chunk, abort_callback& abort) {
    if(!is_open() || abort.is_aborting()) {
        return;
    }
    
    if(chunk.is_empty()) return;
    
    // 这里应该实现音频数据的处理和输出
    // 包括格式转换、缓冲管理等
    
    // 简化实现：直接处理
    HRESULT hr = convert_and_write(chunk);
    if(FAILED(hr)) {
        // 错误处理
    }
}

void output_wasapi::flush(abort_callback& abort) {
    if(!is_open() || abort.is_aborting()) {
        return;
    }
    
    // 刷新剩余的音频数据
    if(render_client_) {
        UINT32 padding = 0;
        if(SUCCEEDED(get_current_padding(padding))) {
            if(padding > 0) {
                // 等待播放完成
                // 实际实现需要更复杂的逻辑
            }
        }
    }
}

bool output_wasapi::set_format(const output_format& format, abort_callback& abort) {
    if(!output_device_utils::validate_output_format(*this, format)) {
        return false;
    }
    
    // 如果设备已打开，需要重新配置
    if(is_open()) {
        close(abort);
        
        // 重新打开并设置格式
        if(!open(format.sample_rate, format.channels, abort)) {
            return false;
        }
    }
    
    current_format_ = format;
    return true;
}

output_format output_wasapi::get_current_format() const {
    return current_format_;
}

std::vector<output_format> output_wasapi::get_supported_formats() const {
    output_device_caps caps = get_device_caps();
    return caps.supported_formats;
}

double output_wasapi::get_latency() const {
    if(!is_open() || !audio_client_) {
        return 0.0;
    }
    
    REFERENCE_TIME latency = 0;
    HRESULT hr = audio_client_->GetStreamLatency(&latency);
    if(FAILED(hr)) {
        return 0.0;
    }
    
    return static_cast<double>(latency) / 10000.0; // 转换为毫秒
}

size_t output_wasapi::get_buffer_size() const {
    return buffer_.get_capacity();
}

bool output_wasapi::set_buffer_size(size_t size, abort_callback& abort) {
    if(is_open()) {
        return false; // 设备已打开，不能更改缓冲大小
    }
    
    // 重新创建缓冲
    buffer_ = audio_buffer(size);
    return true;
}

const char* output_wasapi::get_name() const {
    return "WASAPI Output";
}

const char* output_wasapi::get_description() const {
    return "Windows Audio Session API (WASAPI) audio output device";
}

output_device_caps output_wasapi::get_device_caps() const {
    output_device_caps caps;
    caps.name = "WASAPI";
    caps.description = "Windows Audio Session API";
    caps.min_latency_ms = 1.0;
    caps.max_latency_ms = 100.0;
    caps.supports_exclusive_mode = true;
    caps.supports_event_driven = true;
    
    // 添加支持的格式
    caps.supported_formats.push_back(output_format(44100, 2, 16, audio_format::int16));
    caps.supported_formats.push_back(output_format(44100, 2, 24, audio_format::int24));
    caps.supported_formats.push_back(output_format(48000, 2, 16, audio_format::int16));
    caps.supported_formats.push_back(output_format(48000, 2, 24, audio_format::int24));
    caps.supported_formats.push_back(output_format(44100, 2, 32, audio_format::float32));
    caps.supported_formats.push_back(output_format(48000, 2, 32, audio_format::float32));
    
    return caps;
}

bool output_wasapi::set_exclusive_mode(bool exclusive, abort_callback& abort) {
    if(is_open()) {
        return false; // 设备已打开，不能更改模式
    }
    
    exclusive_mode_ = exclusive;
    return true;
}

// 辅助函数
const char* output_wasapi::wasapi_error_to_string(HRESULT hr) {
    switch(hr) {
        case AUDCLNT_E_NOT_INITIALIZED: return "音频客户端未初始化";
        case AUDCLNT_E_ALREADY_INITIALIZED: return "音频客户端已初始化";
        case AUDCLNT_E_WRONG_ENDPOINT_TYPE: return "错误的端点类型";
        case AUDCLNT_E_DEVICE_INVALIDATED: return "设备无效";
        case AUDCLNT_E_NOT_STOPPED: return "设备未停止";
        case AUDCLNT_E_BUFFER_TOO_LARGE: return "缓冲区过大";
        case AUDCLNT_E_BUFFER_SIZE_ERROR: return "缓冲区大小错误";
        case AUDCLNT_E_CPUUSAGE_EXCEEDED: return "CPU使用率超限";
        case AUDCLNT_E_BUFFER_OPERATION_PENDING: return "缓冲区操作挂起";
        case AUDCLNT_E_SERVICE_NOT_RUNNING: return "服务未运行";
        default: return "未知WASAPI错误";
    }
}

void output_wasapi::log_wasapi_error(const char* operation, HRESULT hr) {
    printf("[WASAPI] %s 失败: %s (0x%08X)\n", 
           operation, wasapi_error_to_string(hr), hr);
}

HRESULT output_wasapi::convert_and_write(const audio_chunk& chunk) {
    if(!render_client_) {
        return E_FAIL;
    }
    
    // 这里应该实现完整的音频数据转换和写入逻辑
    // 包括格式转换、缓冲管理等
    
    return S_OK;
}

} // namespace fb2k