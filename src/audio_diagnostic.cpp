/**
 * @file audio_diagnostic.cpp
 * @brief Audio system diagnostic tool
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <atomic>

#ifdef _WIN32
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "shlwapi.lib")

// Diagnostic audio callback
class AudioDiagnostic {
private:
    IAudioClient* client_ = nullptr;
    IAudioRenderClient* render_ = nullptr;
    std::atomic<bool> should_stop_{false};
    
public:
    ~AudioDiagnostic() {
        stop();
    }
    
    bool initialize() {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            std::cerr << "❌ Failed to initialize COM" << std::endl;
            return false;
        }
        
        std::cout << "✓ COM initialized" << std::endl;
        
        // Get device enumerator
        IMMDeviceEnumerator* enumerator = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                              CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                              (void**)&enumerator);
        
        if (FAILED(hr)) {
            std::cerr << "❌ Failed to create device enumerator: 0x" << std::hex << hr << std::endl;
            return false;
        }
        
        std::cout << "✓ Device enumerator created" << std::endl;
        
        // Get default audio device
        IMMDevice* device = nullptr;
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        
        if (FAILED(hr)) {
            std::cerr << "❌ Failed to get default audio endpoint: 0x" << std::hex << hr << std::endl;
            enumerator->Release();
            return false;
        }
        
        std::cout << "✓ Default audio device obtained" << std::endl;
        
        // Get device name
        IPropertyStore* props = nullptr;
        hr = device->OpenPropertyStore(STGM_READ, &props);
        if (SUCCEEDED(hr)) {
            PROPVARIANT pv;
            PropVariantInit(&pv);
            hr = props->GetValue(PKEY_Device_FriendlyName, &pv);
            if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR) {
                char friendly_name[256];
                WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, friendly_name, 256, NULL, NULL);
                std::cout << "✓ Device: " << friendly_name << std::endl;
                PropVariantClear(&pv);
            }
            props->Release();
        }
        
        // Activate audio client
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                             NULL, (void**)&client_);
        
        if (FAILED(hr)) {
            std::cerr << "❌ Failed to activate audio client: 0x" << std::hex << hr << std::endl;
            device->Release();
            enumerator->Release();
            return false;
        }
        
        std::cout << "✓ Audio client activated" << std::endl;
        
        // Get mix format
        WAVEFORMATEX* mix_format = nullptr;
        hr = client_->GetMixFormat(&mix_format);
        
        if (FAILED(hr)) {
            std::cerr << "❌ Failed to get mix format: 0x" << std::hex << hr << std::endl;
            device->Release();
            enumerator->Release();
            client_->Release();
            return false;
        }
        
        std::cout << "✓ Mix format obtained:" << std::endl;
        std::cout << "  - Sample Rate: " << mix_format->nSamplesPerSec << " Hz" << std::endl;
        std::cout << "  - Channels: " << mix_format->nChannels << std::endl;
        std::cout << "  - Bits per Sample: " << mix_format->wBitsPerSample << std::endl;
        std::cout << "  - Format Tag: " << mix_format->wFormatTag << std::endl;
        
        // Try to use 44.1kHz stereo float for compatibility
        WAVEFORMATEXTENSIBLE wfx = {};
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.nChannels = 2;
        wfx.Format.nSamplesPerSec = 44100;
        wfx.Format.wBitsPerSample = 32;
        wfx.Format.nBlockAlign = wfx.Format.nChannels * wfx.Format.wBitsPerSample / 8;
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Format.cbSize = sizeof(wfx) - sizeof(WAVEFORMATEX);
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        wfx.Samples.wValidBitsPerSample = 32;
        wfx.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
        
        std::cout << "\nAttempting to initialize with:" << std::endl;
        std::cout << "  - Sample Rate: " << wfx.Format.nSamplesPerSec << " Hz" << std::endl;
        std::cout << "  - Channels: " << wfx.Format.nChannels << std::endl;
        std::cout << "  - Format: Float32" << std::endl;
        
        hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                0, 10000000, 0, (WAVEFORMATEX*)&wfx, NULL);
        
        if (FAILED(hr)) {
            std::cout << "⚠️  Failed with custom format, trying system default..." << std::endl;
            hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                    0, 10000000, 0, mix_format, NULL);
            
            if (FAILED(hr)) {
                std::cerr << "❌ Failed to initialize audio client: 0x" << std::hex << hr << std::endl;
                CoTaskMemFree(mix_format);
                device->Release();
                enumerator->Release();
                client_->Release();
                return false;
            }
        }
        
        CoTaskMemFree(mix_format);
        std::cout << "✓ Audio client initialized" << std::endl;
        
        // Get buffer size
        UINT32 buffer_size = 0;
        hr = client_->GetBufferSize(&buffer_size);
        if (SUCCEEDED(hr)) {
            std::cout << "✓ Buffer size: " << buffer_size << " frames" << std::endl;
        }
        
        // Get render client
        hr = client_->GetService(__uuidof(IAudioRenderClient), (void**)&render_);
        if (FAILED(hr)) {
            std::cerr << "❌ Failed to get render client: 0x" << std::hex << hr << std::endl;
            device->Release();
            enumerator->Release();
            client_->Release();
            return false;
        }
        
        std::cout << "✓ Render client obtained" << std::endl;
        device->Release();
        enumerator->Release();
        
        return true;
    }
    
    void stop() {
        should_stop_ = true;
        if (client_) {
            client_->Stop();
        }
    }
    
    void play_tone(float frequency, float duration_seconds, float amplitude = 0.5f) {
        std::cout << "\n================================================" << std::endl;
        std::cout << "Playing test tone: " << frequency << " Hz for " << duration_seconds << " seconds" << std::endl;
        std::cout << "Amplitude: " << (amplitude * 100) << "%" << std::endl;
        std::cout << "================================================" << std::endl;
        std::cout << "You should hear a continuous tone now..." << std::endl;
        std::cout << std::endl;
        
        HRESULT hr = client_->Start();
        if (FAILED(hr)) {
            std::cerr << "❌ Failed to start audio: 0x" << std::hex << hr << std::endl;
            return;
        }
        
        std::cout << "✓ Audio started" << std::endl;
        
        UINT32 buffer_size = 0;
        client_->GetBufferSize(&buffer_size);
        
        const float pi = 3.14159265358979323846f;
        float phase = 0.0f;
        auto start_time = std::chrono::steady_clock::now();
        int frames_played = 0;
        
        while (!should_stop_ && 
               std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count() < duration_seconds) {
            UINT32 padding = 0;
            client_->GetCurrentPadding(&padding);
            
            UINT32 available = buffer_size - padding;
            if (available > 0) {
                BYTE* data = nullptr;
                HRESULT hr = render_->GetBuffer(available, &data);
                if (SUCCEEDED(hr)) {
                    float* samples = reinterpret_cast<float*>(data);
                    float phase_increment = 2.0f * pi * frequency / 44100.0f;
                    
                    for (UINT32 i = 0; i < available; ++i) {
                        float sample = amplitude * sin(phase);
                        samples[i * 2] = sample;       // Left
                        samples[i * 2 + 1] = sample;   // Right
                        phase += phase_increment;
                        if (phase > 2.0f * pi) phase -= 2.0f * pi;
                    }
                    
                    render_->ReleaseBuffer(available, 0);
                    frames_played += available;
                }
            }
            
            // Small delay to not hog CPU
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        std::cout << "✓ Playback complete (" << frames_played << " frames played)" << std::endl;
        client_->Stop();
    }
};
#endif

int main() {
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Audio System Diagnostic Tool             ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
#ifdef _WIN32
    AudioDiagnostic audio;
    
    std::cout << "Step 1: Initializing audio system..." << std::endl;
    if (!audio.initialize()) {
        std::cerr << "\n❌ FAILED - Audio system could not be initialized" << std::endl;
        std::cerr << "Possible causes:" << std::endl;
        std::cerr << "  - No audio device connected" << std::endl;
        std::cerr << "  - Audio drivers not installed" << std::endl;
        std::cerr << "  - Another application has exclusive access" << std::endl;
        std::cerr << "  - Windows Audio Service is stopped" << std::endl;
        return 1;
    }
    
    std::cout << "\n✓ Audio system initialized successfully!" << std::endl;
    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  CHECK YOUR VOLUME: Set to 50% now!         ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Enter when ready...";
    std::cin.get();
    
    // Play 1kHz tone (easier to hear than 440Hz)
    std::cout << "\nPlaying test tone...\n" << std::endl;
    audio.play_tone(1000.0f, 3.0f, 0.8f);  // 1kHz, 3 seconds, 80% volume
    
    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  DID YOU HEAR THE TONE?                      ║" << std::endl;
    std::cout << "║  [YES] Audio system working                  ║" << std::endl;
    std::cout << "║  [NO]  Check volume, speakers, drivers       ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    return 0;
#else
    std::cout << "This diagnostic tool is Windows-only (WASAPI)." << std::endl;
    std::cout << "Test on Windows with WASAPI to verify audio works." << std::endl;
    return 1;
#endif
}
