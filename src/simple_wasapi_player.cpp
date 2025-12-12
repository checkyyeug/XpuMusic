/**
 * @file simple_wasapi_player.cpp
 * @brief Simple WASAPI player without event-driven mode
 * @date 2025-12-12
 */

#include <iostream>
#include <fstream>
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
#include <conio.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#endif

// WAV header
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

class SimpleWASAPIPlayer {
private:
    IMMDeviceEnumerator* enumerator_;
    IMMDevice* device_;
    IAudioClient* client_;
    IAudioRenderClient* render_;
    WAVEFORMATEX format_;
    uint32_t buffer_size_;
    std::vector<float> audio_data_;
    uint32_t audio_frames_;
    uint32_t audio_channels_;
    uint32_t audio_sample_rate_;

public:
    SimpleWASAPIPlayer()
        : enumerator_(nullptr), device_(nullptr), client_(nullptr)
        , render_(nullptr), buffer_size_(0), audio_frames_(0)
        , audio_channels_(0), audio_sample_rate_(0) {
        memset(&format_, 0, sizeof(format_));
    }

    ~SimpleWASAPIPlayer() {
        cleanup();
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

        audio_channels_ = header.channels;
        audio_sample_rate_ = header.sample_rate;
        audio_frames_ = header.data_size / (header.channels * header.bits / 8);
        audio_data_.resize(audio_frames_ * audio_channels_);

        // Convert to float
        if (header.bits == 16) {
            std::vector<int16_t> temp(audio_frames_ * audio_channels_);
            file.read(reinterpret_cast<char*>(temp.data()), header.data_size);

            for (size_t i = 0; i < temp.size(); ++i) {
                audio_data_[i] = static_cast<float>(temp[i]) / 32768.0f;
            }
        } else if (header.bits == 32) {
            file.read(reinterpret_cast<char*>(audio_data_.data()), header.data_size);
        } else {
            std::cerr << "Error: Unsupported bit depth: " << header.bits << std::endl;
            return false;
        }

        std::cout << "Loaded: " << filename << std::endl;
        std::cout << "  Format: " << header.sample_rate << " Hz, "
                  << header.channels << " channels, " << header.bits << "-bit" << std::endl;
        std::cout << "  Duration: " << static_cast<double>(audio_frames_) / header.sample_rate
                  << " seconds" << std::endl;

        return true;
    }

    bool initialize() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize COM" << std::endl;
            return false;
        }

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                            CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                            (void**)&enumerator_);
        if (FAILED(hr)) {
            std::cerr << "Failed to create MMDeviceEnumerator" << std::endl;
            return false;
        }

        hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get default audio endpoint" << std::endl;
            return false;
        }

        hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                              nullptr, (void**)&client_);
        if (FAILED(hr)) {
            std::cerr << "Failed to activate audio client" << std::endl;
            return false;
        }

        // Get device format
        WAVEFORMATEX* device_format = nullptr;
        hr = client_->GetMixFormat(&device_format);
        if (FAILED(hr)) {
            std::cerr << "Failed to get mix format" << std::endl;
            return false;
        }

        // Use a compatible format - 16-bit PCM
        format_.wFormatTag = WAVE_FORMAT_PCM;
        format_.nChannels = 2;  // Force stereo
        format_.nSamplesPerSec = 48000;  // Common sample rate
        format_.wBitsPerSample = 16;
        format_.nBlockAlign = (format_.nChannels * format_.wBitsPerSample) / 8;
        format_.nAvgBytesPerSec = format_.nSamplesPerSec * format_.nBlockAlign;
        format_.cbSize = 0;

        // Initialize without event-driven mode
        hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                0,  // No event flag
                                0, 0, &format_, nullptr);
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize audio client with format: "
                      << format_.nSamplesPerSec << " Hz" << std::endl;

            // Try device format
            std::cout << "Trying device format..." << std::endl;
            format_ = *device_format;
            format_.wFormatTag = WAVE_FORMAT_PCM;
            format_.wBitsPerSample = 16;
            format_.nBlockAlign = (format_.nChannels * format_.wBitsPerSample) / 8;
            format_.nAvgBytesPerSec = format_.nSamplesPerSec * format_.nBlockAlign;
            format_.cbSize = 0;

            hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                    0, 0, 0, &format_, nullptr);
            if (FAILED(hr)) {
                std::cerr << "Failed with device format too" << std::endl;
                CoTaskMemFree(device_format);
                return false;
            }
        }

        std::cout << "Initialized with format: " << format_.nSamplesPerSec
                  << " Hz, " << format_.nChannels << " channels" << std::endl;

        CoTaskMemFree(device_format);

        hr = client_->GetBufferSize(&buffer_size_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get buffer size" << std::endl;
            return false;
        }

        hr = client_->GetService(__uuidof(IAudioRenderClient),
                                (void**)&render_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get render client" << std::endl;
            return false;
        }

        return true;
    }

    void play() {
        if (!client_ || !render_) return;

        client_->Start();
        std::cout << "\nPlaying... Press Enter to stop\n" << std::endl;

        uint32_t current_frame = 0;
        double sample_ratio = static_cast<double>(audio_sample_rate_) / format_.nSamplesPerSec;

        while (current_frame < audio_frames_) {
            UINT32 padding;
            client_->GetCurrentPadding(&padding);

            UINT32 frames_available = buffer_size_ - padding;
            if (frames_available == 0) {
                Sleep(10);
                continue;
            }

            BYTE* buffer = nullptr;
            HRESULT hr = render_->GetBuffer(frames_available, &buffer);
            if (FAILED(hr)) break;

            int16_t* pcm_buffer = reinterpret_cast<int16_t*>(buffer);
            UINT32 frames_to_write = frames_available;

            // Process each frame
            for (UINT32 i = 0; i < frames_to_write; ++i) {
                if (current_frame >= audio_frames_) {
                    frames_to_write = i;
                    break;
                }

                double src_pos = current_frame * sample_ratio;
                uint32_t src_idx = static_cast<uint32_t>(src_pos);
                double frac = src_pos - src_idx;

                if (src_idx + 1 >= audio_frames_) break;

                // Linear interpolation
                for (UINT32 ch = 0; ch < format_.nChannels; ++ch) {
                    float sample = 0.0f;

                    if (audio_channels_ == 1 && format_.nChannels == 2) {
                        // Mono to stereo
                        float s0 = audio_data_[src_idx];
                        float s1 = audio_data_[src_idx + 1];
                        sample = s0 + (s1 - s0) * frac;
                        pcm_buffer[i * format_.nChannels + ch] = static_cast<int16_t>(sample * 32767.0f);
                    } else if (audio_channels_ == 2 && format_.nChannels == 2) {
                        // Stereo to stereo
                        float s0 = audio_data_[src_idx * 2 + ch];
                        float s1 = audio_data_[(src_idx + 1) * 2 + ch];
                        sample = s0 + (s1 - s0) * frac;
                        pcm_buffer[i * format_.nChannels + ch] = static_cast<int16_t>(sample * 32767.0f);
                    }
                }
                current_frame++;
            }

            render_->ReleaseBuffer(frames_to_write, 0);

            // Check if user pressed Enter
            if (_kbhit() && _getch() == '\r') {
                std::cout << "\nStopping..." << std::endl;
                break;
            }
        }

        client_->Stop();
    }

private:
    void cleanup() {
        if (render_) render_->Release();
        if (client_) client_->Release();
        if (device_) device_->Release();
        if (enumerator_) enumerator_->Release();
        CoUninitialize();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "   Simple WASAPI Player    " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    SimpleWASAPIPlayer player;

    if (!player.load_wav(argv[1])) {
        return 1;
    }

    std::cout << "\nInitializing audio..." << std::endl;
    if (!player.initialize()) {
        std::cerr << "Failed to initialize audio" << std::endl;
        return 1;
    }

    player.play();

    std::cout << "Playback completed!" << std::endl;

    return 0;
}