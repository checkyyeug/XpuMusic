/**
 * @file standalone_music_player_fixed.cpp
 * @brief Fixed version of standalone music player with resampling
 * @date 2025-12-12
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <comdef.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "avrt.lib")
#endif

// Simple WAV header structure
struct WAVHeader {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits;
    char data[4];
    uint32_t data_size;
};

// Audio buffer structure
struct AudioBuffer {
    std::vector<float> data;
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t frames;

    AudioBuffer() : sample_rate(44100), channels(2), frames(0) {}
};

// Simple linear resampler
class LinearResampler {
private:
    double ratio_;
    double position_;

public:
    LinearResampler() : ratio_(1.0), position_(0.0) {}

    void set_ratio(double input_rate, double output_rate) {
        ratio_ = input_rate / output_rate;
        position_ = 0.0;
    }

    uint32_t process(const float* input, uint32_t input_frames,
                     float* output, uint32_t max_output_frames,
                     uint32_t channels) {
        uint32_t output_frames = 0;

        for (uint32_t out_frame = 0; out_frame < max_output_frames; ++out_frame) {
            uint32_t input_index = static_cast<uint32_t>(position_);
            double frac = position_ - input_index;

            if (input_index + 1 >= input_frames) {
                break;
            }

            for (uint32_t ch = 0; ch < channels; ++ch) {
                float sample0 = input[input_index * channels + ch];
                float sample1 = input[(input_index + 1) * channels + ch];
                output[out_frame * channels + ch] = sample0 + (sample1 - sample0) * frac;
            }

            position_ += ratio_;
            output_frames++;
        }

        return output_frames;
    }

    void reset() {
        position_ = 0.0;
    }
};

// WASAPI Audio Output
class WASAPIOutput {
private:
    IMMDeviceEnumerator* enumerator_;
    IMMDevice* device_;
    IAudioClient* client_;
    IAudioRenderClient* render_;
    WAVEFORMATEX format_;
    uint32_t buffer_size_;
    HANDLE audio_thread_;
    volatile bool playing_;
    AudioBuffer buffer_;
    LinearResampler resampler_;

public:
    WASAPIOutput()
        : enumerator_(nullptr), device_(nullptr), client_(nullptr)
        , render_(nullptr), audio_thread_(nullptr)
        , playing_(false), buffer_size_(0) {
        memset(&format_, 0, sizeof(format_));
    }

    ~WASAPIOutput() {
        stop();
        cleanup();
    }

    bool initialize(uint32_t sample_rate, uint32_t channels) {
        std::cout << "Initializing WASAPI..." << std::endl;

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
            return false;
        }
        std::cout << "COM initialized successfully" << std::endl;

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                            CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                            (void**)&enumerator_);
        if (FAILED(hr)) {
            std::cerr << "Failed to create MMDeviceEnumerator: " << std::hex << hr << std::endl;
            return false;
        }
        std::cout << "MMDeviceEnumerator created" << std::endl;

        hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get default audio endpoint: " << std::hex << hr << std::endl;
            return false;
        }
        std::cout << "Default audio endpoint obtained" << std::endl;

        hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                              nullptr, (void**)&client_);
        if (FAILED(hr)) {
            std::cerr << "Failed to activate audio client: " << std::hex << hr << std::endl;
            return false;
        }
        std::cout << "Audio client activated" << std::endl;

        // Get device format info
        WAVEFORMATEX* device_format = nullptr;
        hr = client_->GetMixFormat(&device_format);
        if (FAILED(hr)) {
            std::cerr << "Failed to get mix format: " << std::hex << hr << std::endl;
            return false;
        }
        std::cout << "Device format: " << device_format->nSamplesPerSec << " Hz, "
                  << device_format->nChannels << " channels" << std::endl;

        // Try the most compatible format first: 16-bit PCM at 48kHz
        format_.wFormatTag = WAVE_FORMAT_PCM;
        format_.nChannels = 2;  // Force stereo
        format_.nSamplesPerSec = 48000;  // Most common rate
        format_.wBitsPerSample = 16;
        format_.nBlockAlign = (format_.nChannels * format_.wBitsPerSample) / 8;
        format_.nAvgBytesPerSec = format_.nSamplesPerSec * format_.nBlockAlign;
        format_.cbSize = 0;

        std::cout << "Trying to initialize with format: 48000 Hz, 2 channels, 16-bit PCM" << std::endl;

        hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                0,  // No event callback
                                0, 0, &format_, nullptr);
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize with 48kHz format: " << std::hex << hr << std::endl;

            // Try 44.1kHz
            format_.nSamplesPerSec = 44100;
            format_.nAvgBytesPerSec = format_.nSamplesPerSec * format_.nBlockAlign;

            std::cout << "Trying 44100 Hz..." << std::endl;
            hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                    0, 0, 0, &format_, nullptr);
            if (FAILED(hr)) {
                std::cerr << "Failed to initialize with 44.1kHz format: " << std::hex << hr << std::endl;

                // Try device's native sample rate but keep PCM
                format_.nSamplesPerSec = device_format->nSamplesPerSec;
                format_.nAvgBytesPerSec = format_.nSamplesPerSec * format_.nBlockAlign;

                std::cout << "Trying device sample rate: " << device_format->nSamplesPerSec << " Hz" << std::endl;
                hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                        0, 0, 0, &format_, nullptr);
                if (FAILED(hr)) {
                    std::cerr << "All format attempts failed: " << std::hex << hr << std::endl;
                    CoTaskMemFree(device_format);
                    return false;
                }
            }
        }

        std::cout << "Successfully initialized WASAPI!" << std::endl;
        std::cout << "Using format: " << format_.nSamplesPerSec << " Hz, "
                  << format_.nChannels << " channels" << std::endl;

        CoTaskMemFree(device_format);

        hr = client_->GetBufferSize(&buffer_size_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get buffer size: " << std::hex << hr << std::endl;
            return false;
        }
        std::cout << "Buffer size: " << buffer_size_ << " frames" << std::endl;

        hr = client_->GetService(__uuidof(IAudioRenderClient),
                                (void**)&render_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get render client: " << std::hex << hr << std::endl;
            return false;
        }

        return true;
    }

    bool load_wav(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (strncmp(header.riff, "RIFF", 4) != 0 ||
            strncmp(header.wave, "WAVE", 4) != 0) {
            std::cerr << "Error: Not a valid WAV file" << std::endl;
            return false;
        }

        buffer_.sample_rate = header.sample_rate;
        buffer_.channels = header.channels;
        buffer_.frames = header.data_size / (header.channels * header.bits / 8);
        buffer_.data.resize(buffer_.frames * buffer_.channels);

        // Read and convert to float
        if (header.bits == 16) {
            std::vector<int16_t> temp(buffer_.frames * buffer_.channels);
            file.read(reinterpret_cast<char*>(temp.data()), header.data_size);

            for (size_t i = 0; i < temp.size(); ++i) {
                buffer_.data[i] = static_cast<float>(temp[i]) / 32768.0f;
            }
        } else if (header.bits == 32) {
            file.read(reinterpret_cast<char*>(buffer_.data.data()), header.data_size);
        } else {
            std::cerr << "Error: Unsupported bit depth: " << header.bits << std::endl;
            return false;
        }

        std::cout << "Loaded: " << filename << std::endl;
        std::cout << "  Format: " << header.sample_rate << " Hz, "
                  << header.channels << " channels, " << header.bits << "-bit" << std::endl;
        std::cout << "  Duration: " << static_cast<double>(buffer_.frames) / header.sample_rate
                  << " seconds" << std::endl;

        return true;
    }

    void play() {
        if (!buffer_.data.empty() && !playing_) {
            playing_ = true;
            audio_thread_ = CreateThread(nullptr, 0, &audio_thread_proc, this, 0, nullptr);
        }
    }

    void stop() {
        if (playing_) {
            playing_ = false;
            if (audio_thread_) {
                WaitForSingleObject(audio_thread_, INFINITE);
                CloseHandle(audio_thread_);
                audio_thread_ = nullptr;
            }
        }
    }

private:
    void cleanup() {
        if (render_) render_->Release();
        if (client_) client_->Release();
        if (device_) device_->Release();
        if (enumerator_) enumerator_->Release();
        CoUninitialize();
    }

    static DWORD WINAPI audio_thread_proc(LPVOID param) {
        WASAPIOutput* This = static_cast<WASAPIOutput*>(param);
        return This->audio_loop();
    }

    DWORD audio_loop() {
        // Set thread priority
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        // Setup resampler if needed
        resampler_.set_ratio(buffer_.sample_rate, format_.nSamplesPerSec);

        std::vector<float> temp_buffer(buffer_size_ * format_.nChannels);
        uint32_t current_frame = 0;

        client_->Start();
        std::cout << "Playback started... Press Enter to stop" << std::endl;

        while (playing_) {
            UINT32 padding;
            client_->GetCurrentPadding(&padding);

            UINT32 frames_available = buffer_size_ - padding;
            if (frames_available == 0) {
                Sleep(10);
                continue;
            }

            BYTE* wasapi_buffer;
            HRESULT hr = render_->GetBuffer(frames_available, &wasapi_buffer);
            if (FAILED(hr)) break;

            int16_t* pcm_buffer = reinterpret_cast<int16_t*>(wasapi_buffer);

            if (current_frame < buffer_.frames) {
                uint32_t frames_to_process = (std::min<uint32_t>)(
                    frames_available,
                    buffer_.frames - current_frame
                );

                if (buffer_.sample_rate == format_.nSamplesPerSec) {
                    // No resampling needed
                    for (uint32_t i = 0; i < frames_to_process; ++i) {
                        for (uint32_t ch = 0; ch < format_.nChannels; ++ch) {
                            uint32_t src_idx = current_frame * buffer_.channels;
                            if (format_.nChannels == buffer_.channels) {
                                pcm_buffer[i * format_.nChannels + ch] =
                                    static_cast<int16_t>(buffer_.data[src_idx + i * format_.nChannels + ch] * 32767.0f);
                            } else if (buffer_.channels == 1 && format_.nChannels == 2) {
                                float sample = buffer_.data[current_frame + i];
                                pcm_buffer[i * 2] = static_cast<int16_t>(sample * 32767.0f);
                                pcm_buffer[i * 2 + 1] = pcm_buffer[i * 2];
                            }
                        }
                    }
                } else {
                    // Resample then convert to PCM
                    uint32_t output_frames = resampler_.process(
                        &buffer_.data[current_frame * buffer_.channels],
                        buffer_.frames - current_frame,
                        temp_buffer.data(),
                        frames_available,
                        buffer_.channels
                    );

                    for (uint32_t i = 0; i < output_frames * format_.nChannels; ++i) {
                        pcm_buffer[i] = static_cast<int16_t>(temp_buffer[i] * 32767.0f);
                    }
                    frames_to_process = output_frames;
                }

                current_frame += frames_to_process;
            } else {
                // Fill with silence
                memset(pcm_buffer, 0, frames_available * format_.nBlockAlign);
                playing_ = false;
            }

            render_->ReleaseBuffer(frames_available, 0);
        }

        client_->Stop();
        return 0;
    }
};

// Main function
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "   Fixed Standalone Music Player           " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    WASAPIOutput player;

    std::string filename = argv[1];

    if (!player.load_wav(filename)) {
        std::cerr << "Failed to load file: " << filename << std::endl;
        return 1;
    }

    std::cout << std::endl << "Initializing audio output..." << std::endl;

    if (!player.initialize(44100, 2)) {
        std::cerr << "Failed to initialize audio output" << std::endl;
        return 1;
    }

    player.play();

    // Wait for user input
    std::cout << std::endl;
    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    std::cout << "Stopping playback..." << std::endl;
    player.stop();

    std::cout << "Playback completed!" << std::endl;

    return 0;
}