#pragma once

// 闃舵1.2锛歐ASAPI杈撳嚭璁惧
// Windows Audio Session API (WASAPI) 杈撳嚭瀹炵幇

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

// WASAPI杈撳嚭璁惧瀹炵幇
class output_wasapi : public output_device_base {
private:
    // WASAPI鎺ュ彛
    ComPtr<IMMDevice> device_;
    ComPtr<IAudioClient> audio_client_;
    ComPtr<IAudioRenderClient> render_client_;
    ComPtr<IAudioClock> audio_clock_;
    
    // 璁惧淇℃伅
    std::wstring device_id_;
    std::wstring device_name_;
    bool is_default_device_;
    
    // 闊抽鏍煎紡
    WAVEFORMATEX* mix_format_;
    WAVEFORMATEXTENSIBLE* mix_format_ext_;
    bool use_extensible_;
    
    // 缂撳啿绠＄悊
    audio_buffer buffer_;
    std::vector<float> convert_buffer_;
    std::vector<uint8_t> output_buffer_;
    
    // 绾跨▼绠＄悊
    std::unique_ptr<std::thread> render_thread_;
    std::atomic<bool> should_stop_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    // 鐘舵€佺鐞?
    std::atomic<uint32_t> buffer_frame_count_;
    std::atomic<uint32_t> padding_frames_;
    std::atomic<double> clock_frequency_;
    
    // 閰嶇疆
    bool exclusive_mode_;
    bool event_driven_;
    size_t buffer_duration_ms_;
    size_t period_duration_ms_;
    
public:
    output_wasapi();
    ~output_wasapi() override;
    
    // 璁惧閫夋嫨
    bool initialize(const wchar_t* device_id = nullptr);
    void shutdown();
    
    // output_device 鎺ュ彛瀹炵幇
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
    // WASAPI鍒濆鍖?
    bool initialize_wasapi(abort_callback& abort);
    void shutdown_wasapi();
    
    // 璁惧鏋氫妇
    bool enumerate_devices();
    bool get_default_device();
    
    // 闊抽鏍煎紡澶勭悊
    bool get_mix_format();
    bool negotiate_audio_format(const output_format& format);
    HRESULT create_audio_format(const output_format& format, WAVEFORMATEX** format_out);
    void free_audio_format();
    
    // 闊抽瀹㈡埛绔鐞?
    HRESULT initialize_audio_client(abort_callback& abort);
    HRESULT start_audio_client();
    HRESULT stop_audio_client();
    HRESULT reset_audio_client();
    
    // 娓叉煋绾跨▼
    void render_thread_func();
    HRESULT render_audio();
    HRESULT get_current_padding(uint32_t& padding);
    HRESULT get_device_frequency(double& frequency);
    
    // 闊抽澶勭悊
    HRESULT write_audio_data(const float* data, size_t samples);
    HRESULT convert_and_write(const audio_chunk& chunk);
    
    // 鏍煎紡杞崲
    HRESULT convert_to_wasapi_format(const audio_chunk& chunk, std::vector<uint8_t>& output);
    
    // 閿欒澶勭悊
    const char* wasapi_error_to_string(HRESULT hr);
    void log_wasapi_error(const char* operation, HRESULT hr);
    
    // 杈呭姪鍑芥暟
    REFERENCE_TIME milliseconds_to_reference_time(uint32_t ms) const {
        return static_cast<REFERENCE_TIME>(ms) * 10000; // 100ns units
    }
    
    uint32_t reference_time_to_milliseconds(REFERENCE_TIME ref_time) const {
        return static_cast<uint32_t>(ref_time / 10000); // 100ns units
    }
};

// WASAPI璁惧鏋氫妇鍣?
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
        
        // 鑾峰彇榛樿璁惧
        ComPtr<IMMDevice> default_device;
        hr = device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &default_device);
        
        if(SUCCEEDED(hr)) {
            wasapi_device_info default_info;
            if(get_device_info(default_device.Get(), default_info)) {
                default_info.is_default = true;
                devices.push_back(default_info);
            }
        }
        
        // 鏋氫妇鎵€鏈夋覆鏌撹澶?
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
                        // 閬垮厤閲嶅娣诲姞榛樿璁惧
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
        
        // 鑾峰彇璁惧ID
        LPWSTR device_id = nullptr;
        HRESULT hr = device->GetId(&device_id);
        if(FAILED(hr)) return false;
        
        info.id = device_id;
        CoTaskMemFree(device_id);
        
        // 鑾峰彇璁惧灞炴€?
        ComPtr<IPropertyStore> prop_store;
        hr = device->OpenPropertyStore(STGM_READ, &prop_store);
        if(FAILED(hr)) return false;
        
        // 鑾峰彇璁惧鍚嶇О
        PROPVARIANT prop_var;
        PropVariantInit(&prop_var);
        hr = prop_store->GetValue(PKEY_Device_FriendlyName, &prop_var);
        if(SUCCEEDED(hr)) {
            info.name = prop_var.pwszVal;
            PropVariantClear(&prop_var);
        }
        
        // 鑾峰彇璁惧鎻忚堪
        PropVariantInit(&prop_var);
        hr = prop_store->GetValue(PKEY_Device_DeviceDesc, &prop_var);
        if(SUCCEEDED(hr)) {
            info.description = prop_var.pwszVal;
            PropVariantClear(&prop_var);
        }
        
        // 妫€鏌ユ槸鍚︿负娌夋蹈寮忚澶?
        PropVariantInit(&prop_var);
        hr = prop_store->GetValue(PKEY_AudioEndpoint_Association, &prop_var);
        if(SUCCEEDED(hr)) {
            info.is_immersive = (prop_var.vt == VT_LPWSTR && 
                                std::wstring(prop_var.pwszVal).find(L"MSHPLUG") != std::wstring::npos);
            PropVariantClear(&prop_var);
        }
        
        // 鑾峰彇璁惧鑳藉姏
        info.caps = get_device_capabilities(device);
        
        return true;
    }
    
    static output_device_caps get_device_capabilities(IMMDevice* device) {
        output_device_caps caps;
        
        // 鎵撳紑闊抽瀹㈡埛绔幏鍙栨牸寮忎俊鎭?
        ComPtr<IAudioClient> audio_client;
        HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                                     nullptr, reinterpret_cast<void**>(&audio_client));
        
        if(SUCCEEDED(hr)) {
            // 鑾峰彇娣峰悎鏍煎紡
            WAVEFORMATEX* mix_format = nullptr;
            hr = audio_client->GetMixFormat(&mix_format);
            
            if(SUCCEEDED(hr) && mix_format) {
                // 娣诲姞鏀寔鐨勬牸寮?
                output_format format;
                format.sample_rate = mix_format->nSamplesPerSec;
                format.channels = mix_format->nChannels;
                format.bits_per_sample = mix_format->wBitsPerSample;
                format.format = audio_format::float32; // WASAPI榛樿浣跨敤娴偣
                
                if(format.is_valid()) {
                    caps.supported_formats.push_back(format);
                }
                
                // 灏濊瘯鍏朵粬甯歌鏍煎紡
                add_common_formats(caps, mix_format->nSamplesPerSec, mix_format->nChannels);
                
                CoTaskMemFree(mix_format);
            }
            
            // 鑾峰彇璁惧鍛ㄦ湡淇℃伅
            REFERENCE_TIME default_period = 0;
            REFERENCE_TIME minimum_period = 0;
            hr = audio_client->GetDevicePeriod(&default_period, &minimum_period);
            
            if(SUCCEEDED(hr)) {
                caps.min_latency_ms = static_cast<double>(minimum_period) / 10000.0; // 杞崲涓烘绉?
                caps.max_latency_ms = static_cast<double>(default_period) / 10000.0;
            }
            
            // 妫€鏌ョ嫭鍗犳ā寮忔敮鎸?
            caps.supports_exclusive_mode = true; // WASAPI鏀寔鐙崰妯″紡
            caps.supports_event_driven = true;   // WASAPI鏀寔浜嬩欢椹卞姩
        }
        
        return caps;
    }
    
    static void add_common_formats(output_device_caps& caps, uint32_t sample_rate, uint32_t channels) {
        // 娣诲姞甯歌鐨勯煶棰戞牸寮?
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

// WASAPI杈撳嚭璁惧鍏蜂綋瀹炵幇锛堟鏋讹級
// 瀹屾暣鐨勫疄鐜伴渶瑕佸ぇ閲忕殑WASAPI鐩稿叧浠ｇ爜
// 杩欓噷鎻愪緵鎺ュ彛瀹氫箟鍜屽熀纭€妗嗘灦

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
        return false; // 宸茬粡鎵撳紑
    }
    
    set_state(output_state::opening);
    
    // 鍒濆鍖朩ASAPI
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
    
    // 杩欓噷搴旇瀹炵幇闊抽鏁版嵁鐨勫鐞嗗拰杈撳嚭
    // 鍖呮嫭鏍煎紡杞崲銆佺紦鍐茬鐞嗙瓑
    
    // 绠€鍖栧疄鐜帮細鐩存帴澶勭悊
    HRESULT hr = convert_and_write(chunk);
    if(FAILED(hr)) {
        // 閿欒澶勭悊
    }
}

void output_wasapi::flush(abort_callback& abort) {
    if(!is_open() || abort.is_aborting()) {
        return;
    }
    
    // 鍒锋柊鍓╀綑鐨勯煶棰戞暟鎹?
    if(render_client_) {
        UINT32 padding = 0;
        if(SUCCEEDED(get_current_padding(padding))) {
            if(padding > 0) {
                // 绛夊緟鎾斁瀹屾垚
                // 瀹為檯瀹炵幇闇€瑕佹洿澶嶆潅鐨勯€昏緫
            }
        }
    }
}

bool output_wasapi::set_format(const output_format& format, abort_callback& abort) {
    if(!output_device_utils::validate_output_format(*this, format)) {
        return false;
    }
    
    // 濡傛灉璁惧宸叉墦寮€锛岄渶瑕侀噸鏂伴厤缃?
    if(is_open()) {
        close(abort);
        
        // 閲嶆柊鎵撳紑骞惰缃牸寮?
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
    
    return static_cast<double>(latency) / 10000.0; // 杞崲涓烘绉?
}

size_t output_wasapi::get_buffer_size() const {
    return buffer_.get_capacity();
}

bool output_wasapi::set_buffer_size(size_t size, abort_callback& abort) {
    if(is_open()) {
        return false; // 璁惧宸叉墦寮€锛屼笉鑳芥洿鏀圭紦鍐插ぇ灏?
    }
    
    // 閲嶆柊鍒涘缓缂撳啿
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
    
    // 娣诲姞鏀寔鐨勬牸寮?
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
        return false; // 璁惧宸叉墦寮€锛屼笉鑳芥洿鏀规ā寮?
    }
    
    exclusive_mode_ = exclusive;
    return true;
}

// 杈呭姪鍑芥暟
const char* output_wasapi::wasapi_error_to_string(HRESULT hr) {
    switch(hr) {
        case AUDCLNT_E_NOT_INITIALIZED: return "闊抽瀹㈡埛绔湭鍒濆鍖?;
        case AUDCLNT_E_ALREADY_INITIALIZED: return "闊抽瀹㈡埛绔凡鍒濆鍖?;
        case AUDCLNT_E_WRONG_ENDPOINT_TYPE: return "閿欒鐨勭鐐圭被鍨?;
        case AUDCLNT_E_DEVICE_INVALIDATED: return "璁惧鏃犳晥";
        case AUDCLNT_E_NOT_STOPPED: return "璁惧鏈仠姝?;
        case AUDCLNT_E_BUFFER_TOO_LARGE: return "缂撳啿鍖鸿繃澶?;
        case AUDCLNT_E_BUFFER_SIZE_ERROR: return "缂撳啿鍖哄ぇ灏忛敊璇?;
        case AUDCLNT_E_CPUUSAGE_EXCEEDED: return "CPU浣跨敤鐜囪秴闄?;
        case AUDCLNT_E_BUFFER_OPERATION_PENDING: return "缂撳啿鍖烘搷浣滄寕璧?;
        case AUDCLNT_E_SERVICE_NOT_RUNNING: return "鏈嶅姟鏈繍琛?;
        default: return "鏈煡WASAPI閿欒";
    }
}

void output_wasapi::log_wasapi_error(const char* operation, HRESULT hr) {
    printf("[WASAPI] %s 澶辫触: %s (0x%08X)\n", 
           operation, wasapi_error_to_string(hr), hr);
}

HRESULT output_wasapi::convert_and_write(const audio_chunk& chunk) {
    if(!render_client_) {
        return E_FAIL;
    }
    
    // 杩欓噷搴旇瀹炵幇瀹屾暣鐨勯煶棰戞暟鎹浆鎹㈠拰鍐欏叆閫昏緫
    // 鍖呮嫭鏍煎紡杞崲銆佺紦鍐茬鐞嗙瓑
    
    return S_OK;
}

} // namespace fb2k