/**
 * @file audio_output_wasapi.cpp
 * @brief WASAPI audio output implementation for Windows
 * @date 2025-12-10
 */

#ifdef AUDIO_BACKEND_WASAPI

#include "audio_output.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <stdexcept>
#include <iostream>

namespace audio {

/**
 * @brief WASAPI audio output implementation for Windows
 */
class AudioOutputWASAPI final : public IAudioOutput {
private:
    IAudioClient* client_;
    IAudioRenderClient* render_client_;
    WAVEFORMATEX* wave_format_;
    HANDLE audio_event_;
    HANDLE stop_event_;
    bool is_open_;
    int buffer_size_;
    int latency_;
    unsigned int sample_rate_;
    unsigned int channels_;

    // Helper to check HRESULT
    void check_hresult(HRESULT hr, const char* operation) {
        if (FAILED(hr)) {
            std::cerr << "WASAPI Error: " << operation << " failed with HRESULT 0x"
                      << std::hex << hr << std::endl;
            throw std::runtime_error(operation);
        }
    }

public:
    AudioOutputWASAPI()
        : client_(nullptr)
        , render_client_(nullptr)
        , wave_format_(nullptr)
        , audio_event_(nullptr)
        , stop_event_(nullptr)
        , is_open_(false)
        , buffer_size_(0)
        , latency_(0)
        , sample_rate_(44100)
        , channels_(2) {
    }

    ~AudioOutputWASAPI() override {
        close();
    }

    bool open(const AudioFormat& format) override {
        try {
            sample_rate_ = format.sample_rate;
            channels_ = format.channels;

            // Initialize COM
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
                check_hresult(hr, "CoInitializeEx");
            }

            // Get default audio endpoint
            IMMDeviceEnumerator* enumerator = nullptr;
            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  (void**)&enumerator);
            check_hresult(hr, "CoCreateInstance(MMDeviceEnumerator)");

            IMMDevice* device = nullptr;
            hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
            enumerator->Release();
            check_hresult(hr, "GetDefaultAudioEndpoint");

            // Activate audio client
            hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                                 nullptr, (void**)&client_);
            device->Release();
            check_hresult(hr, "Activate audio client");

            // Get mix format
            hr = client_->GetMixFormat(&wave_format_);
            check_hresult(hr, "GetMixFormat");

            // Adjust format to our requirements
            wave_format_->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            wave_format_->nChannels = channels_;
            wave_format_->nSamplesPerSec = sample_rate_;
            wave_format_->wBitsPerSample = 16;
            wave_format_->nBlockAlign = (wave_format_->nChannels *
                                         wave_format_->wBitsPerSample) / 8;
            wave_format_->nAvgBytesPerSec = wave_format_->nSamplesPerSec *
                                           wave_format_->nBlockAlign;

            if (wave_format_->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
                WAVEFORMATEXTENSIBLE* ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(wave_format_);
                ext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
                ext->Samples.wValidBitsPerSample = 16;
                ext->dwChannelMask = (channels_ == 1) ? KSAUDIO_SPEAKER_MONO
                                      : (channels_ == 2) ? KSAUDIO_SPEAKER_STEREO
                                      : KSAUDIO_SPEAKER_STEREO;
            }

            // Initialize audio client
            REFERENCE_TIME buffer_duration = 10000000; // 1 second
            hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                   AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                   buffer_duration, 0, wave_format_, nullptr);
            check_hresult(hr, "Initialize audio client");

            // Create event handle
            audio_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!audio_event_) {
                throw std::runtime_error("CreateEvent failed");
            }

            stop_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!stop_event_) {
                CloseHandle(audio_event_);
                throw std::runtime_error("CreateEvent (stop) failed");
            }

            // Set event handle
            hr = client_->SetEventHandle(audio_event_);
            check_hresult(hr, "SetEventHandle");

            // Get render client
            hr = client_->GetService(__uuidof(IAudioRenderClient),
                                     (void**)&render_client_);
            check_hresult(hr, "GetService(IAudioRenderClient)");

            // Get buffer size
            hr = client_->GetBufferSize(&buffer_size_);
            check_hresult(hr, "GetBufferSize");

            // Calculate latency
            REFERENCE_TIME latency;
            hr = client_->GetStreamLatency(&latency);
            if (SUCCEEDED(hr)) {
                latency_ = static_cast<int>(latency / 10000); // Convert to ms
            } else {
                latency_ = 100; // Default estimate
            }

            // Start the stream
            hr = client_->Start();
            check_hresult(hr, "Start stream");

            is_open_ = true;
            return true;

        } catch (const std::exception& e) {
            std::cerr << "Failed to open WASAPI: " << e.what() << std::endl;
            close();
            return false;
        }
    }

    void close() override {
        if (client_) {
            client_->Stop();
        }

        if (render_client_) {
            render_client_->Release();
            render_client_ = nullptr;
        }

        if (client_) {
            client_->Release();
            client_ = nullptr;
        }

        if (wave_format_) {
            CoTaskMemFree(wave_format_);
            wave_format_ = nullptr;
        }

        if (audio_event_) {
            CloseHandle(audio_event_);
            audio_event_ = nullptr;
        }

        if (stop_event_) {
            CloseHandle(stop_event_);
            stop_event_ = nullptr;
        }

        is_open_ = false;
    }

    int write(const float* buffer, int frames) override {
        if (!is_open_) return 0;

        try {
            // Wait for buffer to be available
            HANDLE handles[2] = { audio_event_, stop_event_ };
            DWORD result = WaitForMultipleObjects(2, handles, FALSE, 2000);

            if (result == WAIT_OBJECT_0 + 1) {
                // Stop event
                return 0;
            } else if (result != WAIT_OBJECT_0) {
                // Timeout or error
                return 0;
            }

            // Get available buffer space
            UINT32 padding;
            HRESULT hr = client_->GetCurrentPadding(&padding);
            if (FAILED(hr)) return 0;

            UINT32 frames_available = buffer_size_ - padding;
            UINT32 frames_to_write = min(frames, frames_available);

            if (frames_to_write > 0) {
                BYTE* data = nullptr;
                hr = render_client_->GetBuffer(frames_to_write, &data);
                if (SUCCEEDED(hr)) {
                    // Convert float to 16-bit PCM
                    int16_t* pcm = reinterpret_cast<int16_t*>(data);
                    for (UINT32 i = 0; i < frames_to_write * channels_; i++) {
                        float sample = buffer[i];
                        if (sample > 1.0f) sample = 1.0f;
                        if (sample < -1.0f) sample = -1.0f;
                        pcm[i] = static_cast<int16_t>(sample * 32767.0f);
                    }

                    hr = render_client_->ReleaseBuffer(frames_to_write, 0);
                    if (SUCCEEDED(hr)) {
                        return frames_to_write;
                    }
                }
            }

            return 0;

        } catch (...) {
            return 0;
        }
    }

    int get_latency() const override {
        return latency_;
    }

    int get_buffer_size() const override {
        return buffer_size_;
    }

    bool is_ready() const override {
        return is_open_ && client_ && render_client_;
    }
};

// Factory function for WASAPI backend
std::unique_ptr<IAudioOutput> create_wasapi_audio_output() {
    return std::make_unique<AudioOutputWASAPI>();
}

} // namespace audio

#else // AUDIO_BACKEND_WASAPI not defined

#include "audio_output.h"
#include <iostream>

namespace audio {

// Fallback to stub if WASAPI not available
std::unique_ptr<IAudioOutput> create_wasapi_audio_output() {
    std::cout << "WASAPI backend not compiled, falling back to stub" << std::endl;
    return create_audio_output(); // This will create stub
}

#endif // AUDIO_BACKEND_WASAPI