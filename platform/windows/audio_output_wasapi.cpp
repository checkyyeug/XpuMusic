#include "mp_audio_output.h"

#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <comdef.h>
#endif

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <cstring>
#include <memory>

namespace mp {
namespace platform {

#ifdef _WIN32

// RAII wrapper for COM initialization
class ComInitializer {
public:
    ComInitializer() {
        hr_ = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr_) && hr_ != RPC_E_CHANGED_MODE) {
            std::cerr << "COM initialization failed: 0x" << std::hex << hr_ << std::endl;
        }
    }
    
    ~ComInitializer() {
        if (SUCCEEDED(hr_)) {
            CoUninitialize();
        }
    }
    
    bool succeeded() const { return SUCCEEDED(hr_); }
    
private:
    HRESULT hr_;
};

// WASAPI audio output implementation
class AudioOutputWASAPI : public IAudioOutput {
public:
    AudioOutputWASAPI() 
        : volume_(1.0f)
        , running_(false)
        , sample_rate_(0)
        , channels_(0)
        , format_(SampleFormat::Float32)
        , buffer_frames_(0)
        , callback_(nullptr)
        , user_data_(nullptr)
        , audio_client_(nullptr)
        , render_client_(nullptr)
        , device_(nullptr)
        , event_handle_(nullptr)
        , com_initialized_(false) {
    }
    
    ~AudioOutputWASAPI() override {
        close();
    }
    
    Result enumerate_devices(const AudioDeviceInfo** devices, size_t* count) override {
        ComInitializer com;
        if (!com.succeeded()) {
            return Result::Error;
        }
        
        IMMDeviceEnumerator* enumerator = nullptr;
        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void**)&enumerator
        );
        
        if (FAILED(hr)) {
            std::cerr << "Failed to create device enumerator: 0x" << std::hex << hr << std::endl;
            return Result::Error;
        }
        
        // Get default device for now (full enumeration can be added later)
        IMMDevice* default_device = nullptr;
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &default_device);
        enumerator->Release();
        
        if (FAILED(hr)) {
            std::cerr << "Failed to get default audio endpoint: 0x" << std::hex << hr << std::endl;
            return Result::Error;
        }
        
        // Get device name
        IPropertyStore* props = nullptr;
        hr = default_device->OpenPropertyStore(STGM_READ, &props);
        
        static std::wstring device_name_wide;
        static std::string device_name;
        
        if (SUCCEEDED(hr)) {
            PROPVARIANT var_name;
            PropVariantInit(&var_name);
            
            hr = props->GetValue(PKEY_Device_FriendlyName, &var_name);
            if (SUCCEEDED(hr)) {
                device_name_wide = var_name.pwszVal;
                // Convert wide string to narrow string
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, device_name_wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
                device_name.resize(size_needed - 1);
                WideCharToMultiByte(CP_UTF8, 0, device_name_wide.c_str(), -1, &device_name[0], size_needed, nullptr, nullptr);
            }
            
            PropVariantClear(&var_name);
            props->Release();
        }
        
        default_device->Release();
        
        // Return default device info
        static AudioDeviceInfo default_info = {
            "default",
            device_name.empty() ? "Default WASAPI Device" : device_name.c_str(),
            2,      // stereo
            48000,  // 48 kHz
            true
        };
        
        static AudioDeviceInfo* device_list[] = { &default_info };
        *devices = device_list[0];
        *count = 1;
        
        return Result::Success;
    }
    
    Result open(const AudioOutputConfig& config) override {
        if (running_) {
            close();
        }
        
        // Initialize COM for this thread
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            std::cerr << "COM initialization failed: 0x" << std::hex << hr << std::endl;
            return Result::Error;
        }
        com_initialized_ = SUCCEEDED(hr);
        
        // Store configuration
        callback_ = config.callback;
        user_data_ = config.user_data;
        sample_rate_ = config.sample_rate;
        channels_ = config.channels;
        format_ = config.format;
        buffer_frames_ = config.buffer_frames;
        
        // Create device enumerator
        IMMDeviceEnumerator* enumerator = nullptr;
        hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void**)&enumerator
        );
        
        if (FAILED(hr)) {
            std::cerr << "Failed to create device enumerator: 0x" << std::hex << hr << std::endl;
            cleanup_com();
            return Result::Error;
        }
        
        // Get default audio endpoint
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
        enumerator->Release();
        
        if (FAILED(hr)) {
            std::cerr << "Failed to get default audio endpoint: 0x" << std::hex << hr << std::endl;
            cleanup_com();
            return Result::Error;
        }
        
        // Activate audio client
        hr = device_->Activate(
            __uuidof(IAudioClient),
            CLSCTX_ALL,
            nullptr,
            (void**)&audio_client_
        );
        
        if (FAILED(hr)) {
            std::cerr << "Failed to activate audio client: 0x" << std::hex << hr << std::endl;
            cleanup_resources();
            return Result::Error;
        }
        
        // Set up audio format with EXTENSIBLE support for modern Windows
        WAVEFORMATEXTENSIBLE wave_format = {};
        wave_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wave_format.Format.nChannels = channels_;
        wave_format.Format.nSamplesPerSec = sample_rate_;
        wave_format.Format.wBitsPerSample = 32; // Float32
        wave_format.Format.nBlockAlign = (wave_format.Format.nChannels * wave_format.Format.wBitsPerSample) / 8;
        wave_format.Format.nAvgBytesPerSec = wave_format.Format.nSamplesPerSec * wave_format.Format.nBlockAlign;
        wave_format.Format.cbSize = sizeof(wave_format) - sizeof(WAVEFORMATEX);
        wave_format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        wave_format.Samples.wValidBitsPerSample = 32;
        wave_format.dwChannelMask = (channels_ == 1) ? KSAUDIO_SPEAKER_MONO
                                                   : (channels_ == 2) ? (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
                                                   : KSAUDIO_SPEAKER_STEREO; // Default to stereo
        
        // Calculate buffer duration (in 100-nanosecond units)
        // Use 10ms as default minimum
        REFERENCE_TIME buffer_duration = 100000; // 10ms in 100-nanosecond units
        
        // Initialize audio client in shared mode (more compatible)
        hr = audio_client_->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            buffer_duration,
            0,
            (WAVEFORMATEX*)&wave_format,
            nullptr
        );
        
        // If that failed, try to get the closest supported format
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize with requested format: 0x" << std::hex << hr << std::endl;
            
            // Try to get the mix format (the format closest to what the device supports)
            WAVEFORMATEX* closest_format = nullptr;
            hr = audio_client_->GetMixFormat(&closest_format);
            if (SUCCEEDED(hr) && closest_format) {
                std::cout << "Trying mix format: " << closest_format->nSamplesPerSec << " Hz, "
                          << closest_format->nChannels << " channels" << std::endl;
                
                // Try again with mix format
                hr = audio_client_->Initialize(
                    AUDCLNT_SHAREMODE_SHARED,
                    AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                    buffer_duration,
                    0,
                    closest_format,
                    nullptr
                );
                
                if (SUCCEEDED(hr)) {
                    // Update our configuration to match
                    sample_rate_ = closest_format->nSamplesPerSec;
                    channels_ = closest_format->nChannels;
                }
                
                CoTaskMemFree(closest_format);
            }
        }
        
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize audio client: 0x" << std::hex << hr << std::endl;
            cleanup_resources();
            return Result::Error;
        }
        
        // Get actual buffer size
        UINT32 actual_buffer_frames;
        hr = audio_client_->GetBufferSize(&actual_buffer_frames);
        if (SUCCEEDED(hr)) {
            buffer_frames_ = actual_buffer_frames;
        }
        
        // Create event handle
        event_handle_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!event_handle_) {
            std::cerr << "Failed to create event handle" << std::endl;
            cleanup_resources();
            return Result::Error;
        }
        
        // Set event handle
        hr = audio_client_->SetEventHandle(event_handle_);
        if (FAILED(hr)) {
            std::cerr << "Failed to set event handle: 0x" << std::hex << hr << std::endl;
            cleanup_resources();
            return Result::Error;
        }
        
        // Get render client
        hr = audio_client_->GetService(__uuidof(IAudioRenderClient), (void**)&render_client_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get render client: 0x" << std::hex << hr << std::endl;
            cleanup_resources();
            return Result::Error;
        }
        
        return Result::Success;
    }
    
    Result start() override {
        if (!audio_client_ || running_) {
            return Result::Error;
        }
        
        running_ = true;
        
        // Start playback thread
        playback_thread_ = std::thread(&AudioOutputWASAPI::playback_loop, this);
        
        // Start audio client
        HRESULT hr = audio_client_->Start();
        if (FAILED(hr)) {
            std::cerr << "Failed to start audio client: 0x" << std::hex << hr << std::endl;
            running_ = false;
            if (playback_thread_.joinable()) {
                playback_thread_.join();
            }
            return Result::Error;
        }
        
        return Result::Success;
    }
    
    Result stop() override {
        if (!audio_client_ || !running_) {
            return Result::Error;
        }
        
        running_ = false;
        
        // Signal event to wake up playback thread
        if (event_handle_) {
            SetEvent(event_handle_);
        }
        
        // Wait for playback thread to finish
        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }
        
        // Stop audio client
        HRESULT hr = audio_client_->Stop();
        if (FAILED(hr)) {
            std::cerr << "Failed to stop audio client: 0x" << std::hex << hr << std::endl;
            return Result::Error;
        }
        
        return Result::Success;
    }
    
    void close() override {
        if (running_) {
            stop();
        }
        
        cleanup_resources();
    }
    
    uint32_t get_latency() const override {
        if (!audio_client_) {
            return 0;
        }
        
        REFERENCE_TIME latency;
        HRESULT hr = audio_client_->GetStreamLatency(&latency);
        if (FAILED(hr)) {
            return 0;
        }
        
        // Convert from 100-nanosecond units to milliseconds
        return static_cast<uint32_t>(latency / 10000);
    }
    
    Result set_volume(float volume) override {
        volume_ = volume;
        if (volume_ < 0.0f) volume_ = 0.0f;
        if (volume_ > 1.0f) volume_ = 1.0f;
        return Result::Success;
    }
    
    float get_volume() const override {
        return volume_;
    }
    
private:
    void playback_loop() {
        // Set thread priority to high for real-time audio
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        
        // Allocate buffer for audio data
        std::vector<float> temp_buffer(buffer_frames_ * channels_);
        
        while (running_) {
            // Wait for event signal
            DWORD wait_result = WaitForSingleObject(event_handle_, 1000);
            
            if (wait_result != WAIT_OBJECT_0) {
                if (running_) {
                    std::cerr << "Event wait timeout or error" << std::endl;
                }
                continue;
            }
            
            if (!running_) {
                break;
            }
            
            // Get available buffer space
            UINT32 padding;
            HRESULT hr = audio_client_->GetCurrentPadding(&padding);
            if (FAILED(hr)) {
                std::cerr << "Failed to get current padding: 0x" << std::hex << hr << std::endl;
                continue;
            }
            
            UINT32 frames_available = buffer_frames_ - padding;
            if (frames_available == 0) {
                continue;
            }
            
            // Get buffer from WASAPI
            BYTE* buffer_data;
            hr = render_client_->GetBuffer(frames_available, &buffer_data);
            if (FAILED(hr)) {
                std::cerr << "Failed to get buffer: 0x" << std::hex << hr << std::endl;
                continue;
            }
            
            // Get audio data from callback
            if (callback_) {
                temp_buffer.resize(frames_available * channels_);
                callback_(temp_buffer.data(), frames_available, user_data_);
                
                // Apply volume
                if (volume_ != 1.0f) {
                    for (size_t i = 0; i < temp_buffer.size(); ++i) {
                        temp_buffer[i] *= volume_;
                    }
                }
                
                // Copy to WASAPI buffer
                std::memcpy(buffer_data, temp_buffer.data(), frames_available * channels_ * sizeof(float));
            } else {
                // Fill with silence if no callback
                std::memset(buffer_data, 0, frames_available * channels_ * sizeof(float));
            }
            
            // Release buffer
            hr = render_client_->ReleaseBuffer(frames_available, 0);
            if (FAILED(hr)) {
                std::cerr << "Failed to release buffer: 0x" << std::hex << hr << std::endl;
            }
        }
    }
    
    void cleanup_resources() {
        if (render_client_) {
            render_client_->Release();
            render_client_ = nullptr;
        }
        
        if (audio_client_) {
            audio_client_->Release();
            audio_client_ = nullptr;
        }
        
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }
        
        if (event_handle_) {
            CloseHandle(event_handle_);
            event_handle_ = nullptr;
        }
        
        cleanup_com();
    }
    
    void cleanup_com() {
        if (com_initialized_) {
            CoUninitialize();
            com_initialized_ = false;
        }
    }
    
    float volume_;
    std::atomic<bool> running_;
    std::thread playback_thread_;
    
    uint32_t sample_rate_;
    uint32_t channels_;
    SampleFormat format_;
    uint32_t buffer_frames_;
    
    AudioCallback callback_;
    void* user_data_;
    
    IAudioClient* audio_client_;
    IAudioRenderClient* render_client_;
    IMMDevice* device_;
    HANDLE event_handle_;
    bool com_initialized_;
};

#else

// Stub implementation for non-Windows platforms
class AudioOutputWASAPI : public IAudioOutput {
public:
    Result enumerate_devices(const AudioDeviceInfo** devices, size_t* count) override {
        return Result::NotSupported;
    }
    
    Result open(const AudioOutputConfig& config) override {
        return Result::NotSupported;
    }
    
    Result start() override {
        return Result::NotSupported;
    }
    
    Result stop() override {
        return Result::NotSupported;
    }
    
    void close() override {}
    
    uint32_t get_latency() const override {
        return 0;
    }
    
    Result set_volume(float volume) override {
        return Result::NotSupported;
    }
    
    float get_volume() const override {
        return 0.0f;
    }
};

#endif

}} // namespace mp::platform

// Factory function
extern "C" mp::IAudioOutput* create_wasapi_output() {
    return new mp::platform::AudioOutputWASAPI();
}
