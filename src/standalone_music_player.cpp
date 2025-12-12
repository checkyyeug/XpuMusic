/**
 * @file standalone_music_player.cpp
 * @brief Standalone music player with resampling support (no SDK dependencies)
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

#ifndef CLCTX_ALL
#define CLCTX_ALL CLCTX_ALL
#endif
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
    WAVEFORMATEX* format_;
    HANDLE audio_thread_;
    volatile bool playing_;
    AudioBuffer buffer_;
    LinearResampler resampler_;

public:
    WASAPIOutput()
        : enumerator_(nullptr), device_(nullptr), client_(nullptr)
        , render_(nullptr), format_(nullptr), audio_thread_(nullptr)
        , playing_(false) {
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

        // Get device format
        hr = client_->GetMixFormat(&format_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get mix format: " << std::hex << hr << std::endl;
            return false;
        }
        // Store the original device format
        WAVEFORMATEX* device_format = format_;
        std::cout << "Device format: " << device_format->nSamplesPerSec << " Hz, "
                  << device_format->nChannels << " channels" << std::endl;

        // First, free the device format to avoid memory leak
        CoTaskMemFree(format_);

        // Create our own format structure
        format_ = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
        if (!format_) return false;

        // Try our desired format first
        format_->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format_->nChannels = channels;
        format_->nSamplesPerSec = sample_rate;
        format_->wBitsPerSample = 32;
        format_->nBlockAlign = (format_->nChannels * format_->wBitsPerSample) / 8;
        format_->nAvgBytesPerSec = format_->nSamplesPerSec * format_->nBlockAlign;
        format_->cbSize = 0;

        std::cout << "Trying to initialize with format: " << sample_rate << " Hz, "
                  << channels << " channels, 32-bit float" << std::endl;

        hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                0, 0, format_, nullptr);
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize audio client: " << std::hex << hr << std::endl;

            // Free our format
            free(format_);

            // Last resort: try 16-bit integer format based on device format
            std::cout << "Trying 16-bit integer format..." << std::endl;
            format_ = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
            if (!format_) {
                std::cerr << "Failed to allocate memory for format" << std::endl;
                CoTaskMemFree(device_format);
                return false;
            }

            // Use device's parameters but with PCM format
            format_->wFormatTag = WAVE_FORMAT_PCM;
            format_->nChannels = device_format->nChannels;
            format_->nSamplesPerSec = device_format->nSamplesPerSec;
            format_->wBitsPerSample = 16;
            format_->nBlockAlign = (format_->nChannels * format_->wBitsPerSample) / 8;
            format_->nAvgBytesPerSec = format_->nSamplesPerSec * format_->nBlockAlign;
            format_->cbSize = 0;

            hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                    AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                    0, 0, format_, nullptr);
            if (FAILED(hr)) {
                std::cerr << "Failed to initialize with 16-bit format: " << std::hex << hr << std::endl;
                CoTaskMemFree(device_format);
                free(format_);
                return false;
            }
            std::cout << "Successfully initialized with 16-bit format: "
                      << format_->nSamplesPerSec << " Hz, "
                      << format_->nChannels << " channels" << std::endl;

            // Clean up device_format
            CoTaskMemFree(device_format);
        } else {
            std::cout << "Successfully initialized with desired format" << std::endl;
            // Clean up device_format since we don't need it
            CoTaskMemFree(device_format);
        }
        std::cout << "Audio client initialized" << std::endl;

        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!event) {
            std::cerr << "Failed to create event" << std::endl;
            return false;
        }

        hr = client_->SetEventHandle(event);
        if (FAILED(hr)) {
            std::cerr << "Failed to set event handle: " << std::hex << hr << std::endl;
            return false;
        }

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

        std::cout << "WASAPI initialized successfully!" << std::endl;
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
    uint32_t buffer_size_;

    void cleanup() {
        if (format_) free(format_);
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
        resampler_.set_ratio(buffer_.sample_rate, format_->nSamplesPerSec);

        std::vector<float> temp_buffer(buffer_size_ * format_->nChannels);
        uint32_t current_frame = 0;

        client_->Start();

        while (playing_) {
            UINT32 padding;
            client_->GetCurrentPadding(&padding);

            UINT32 frames_available = buffer_size_ - padding;
            if (frames_available == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            BYTE* wasapi_buffer;
            HRESULT hr = render_->GetBuffer(frames_available, &wasapi_buffer);
            if (FAILED(hr)) break;

            float* output_buffer = reinterpret_cast<float*>(wasapi_buffer);

            if (current_frame < buffer_.frames) {
                uint32_t frames_to_process = (std::min<uint32_t>)(
                    frames_available,
                    buffer_.frames - current_frame
                );

                // Handle different output formats
                if (format_->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
                    // Float output
                    float* float_buffer = reinterpret_cast<float*>(wasapi_buffer);

                    if (buffer_.sample_rate == format_->nSamplesPerSec) {
                        // No resampling needed
                        if (format_->nChannels == buffer_.channels) {
                            memcpy(float_buffer,
                                   &buffer_.data[current_frame * buffer_.channels],
                                   frames_to_process * format_->nChannels * sizeof(float));
                        } else if (buffer_.channels == 1 && format_->nChannels == 2) {
                            // Mono to stereo
                            for (uint32_t i = 0; i < frames_to_process; ++i) {
                                float sample = buffer_.data[current_frame + i];
                                float_buffer[i * 2] = sample;
                                float_buffer[i * 2 + 1] = sample;
                            }
                        }
                    } else {
                        // Resample
                        uint32_t output_frames = resampler_.process(
                            &buffer_.data[current_frame * buffer_.channels],
                            buffer_.frames - current_frame,
                            temp_buffer.data(),
                            frames_available,
                            buffer_.channels
                        );

                        memcpy(float_buffer, temp_buffer.data(),
                               output_frames * format_->nChannels * sizeof(float));
                        frames_to_process = output_frames;
                    }
                } else if (format_->wFormatTag == WAVE_FORMAT_PCM) {
                    // 16-bit PCM output
                    int16_t* pcm_buffer = reinterpret_cast<int16_t*>(wasapi_buffer);

                    if (buffer_.sample_rate == format_->nSamplesPerSec) {
                        // No resampling needed
                        for (uint32_t i = 0; i < frames_to_process * format_->nChannels; ++i) {
                            uint32_t src_idx = current_frame * buffer_.channels;
                            if (format_->nChannels == buffer_.channels) {
                                pcm_buffer[i] = static_cast<int16_t>(buffer_.data[src_idx + i] * 32767.0f);
                            } else if (buffer_.channels == 1 && format_->nChannels == 2) {
                                // Mono to stereo
                                float sample = buffer_.data[current_frame + i/2];
                                pcm_buffer[i] = static_cast<int16_t>(sample * 32767.0f);
                            }
                        }
                    } else {
                        // Resample then convert to PCM
                        std::vector<float> resampled(frames_available * format_->nChannels);
                        uint32_t output_frames = resampler_.process(
                            &buffer_.data[current_frame * buffer_.channels],
                            buffer_.frames - current_frame,
                            resampled.data(),
                            frames_available,
                            buffer_.channels
                        );

                        for (uint32_t i = 0; i < output_frames * format_->nChannels; ++i) {
                            pcm_buffer[i] = static_cast<int16_t>(resampled[i] * 32767.0f);
                        }
                        frames_to_process = output_frames;
                    }
                }

                current_frame += frames_to_process;
            } else {
                // Fill with silence
                memset(output_buffer, 0, frames_available * format_->nBlockAlign);
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
    std::cout << "   Standalone Music Player with Resampling   " << std::endl;
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

    std::cout << "Starting playback... Press Enter to stop" << std::endl;
    player.play();

    // Wait for user input
    std::cin.get();

    std::cout << "Stopping playback..." << std::endl;
    player.stop();

    std::cout << "Playback completed!" << std::endl;

    return 0;
}