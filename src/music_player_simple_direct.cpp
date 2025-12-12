/**
 * @file music_player_simple_direct.cpp
 * @brief Simple music player with direct WASAPI playback (no dependencies)
 * @date 2025-12-13
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#endif

// Simple WAV header
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

#ifdef _WIN32
class DirectWASAPIPlayer {
private:
    IMMDeviceEnumerator* enumerator_;
    IMMDevice* device_;
    IAudioClient* client_;
    IAudioRenderClient* render_;
    WAVEFORMATEX format_;
    UINT32 buffer_frame_count_;

public:
    DirectWASAPIPlayer() : enumerator_(nullptr), device_(nullptr), client_(nullptr), render_(nullptr) {
        memset(&format_, 0, sizeof(format_));
    }

    ~DirectWASAPIPlayer() {
        stop();
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
            std::cerr << "Failed to create device enumerator" << std::endl;
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

        // Force to 48kHz stereo 32-bit float (most compatible)
        format_.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format_.nSamplesPerSec = 48000;
        format_.nChannels = 2;
        format_.wBitsPerSample = 32;
        format_.nBlockAlign = (format_.nChannels * format_.wBitsPerSample) / 8;
        format_.nAvgBytesPerSec = format_.nSamplesPerSec * format_.nBlockAlign;
        format_.cbSize = 0;

        hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, &format_, nullptr);
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize audio client with 48kHz: " << std::hex << hr << std::endl;

            // Try with device format instead
            WAVEFORMATEX* device_format = nullptr;
            hr = client_->GetMixFormat(&device_format);
            if (SUCCEEDED(hr)) {
                hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, device_format, nullptr);
                CoTaskMemFree(device_format);
            }

            if (FAILED(hr)) {
                std::cerr << "Failed to initialize audio client with device format" << std::endl;
                return false;
            }
        }

        hr = client_->GetBufferSize(&buffer_frame_count_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get buffer size" << std::endl;
            return false;
        }

        hr = client_->GetService(__uuidof(IAudioRenderClient), (void**)&render_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get render client" << std::endl;
            return false;
        }

        std::cout << "WASAPI initialized successfully!" << std::endl;
        std::cout << "Format: " << format_.nSamplesPerSec << "Hz, "
                  << format_.nChannels << " channels, "
                  << format_.wBitsPerSample << "-bit float" << std::endl;
        std::cout << "Buffer size: " << buffer_frame_count_ << " frames" << std::endl;

        return true;
    }

    bool play_wav(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (strncmp(header.riff, "RIFF", 4) != 0 ||
            strncmp(header.wave, "WAVE", 4) != 0) {
            std::cerr << "Invalid WAV file" << std::endl;
            return false;
        }

        std::cout << "\n=== WAV File Information ===" << std::endl;
        std::cout << "File: " << filename << std::endl;
        std::cout << "Sample Rate: " << header.sample_rate << " Hz" << std::endl;
        std::cout << "Channels: " << header.channels << std::endl;
        std::cout << "Bits: " << header.bits << "-bit" << std::endl;
        std::cout << "Data Size: " << header.data_size << " bytes" << std::endl;
        std::cout << "Duration: " << (header.data_size / (header.sample_rate * header.channels * (header.bits / 8))) << " seconds" << std::endl;

        // Read audio data
        std::vector<uint8_t> audio_data(header.data_size);
        file.read(reinterpret_cast<char*>(audio_data.data()), header.data_size);

        // Start playback
        std::cout << "\n=== Starting Playback ===" << std::endl;
        HRESULT hr = client_->Start();
        if (FAILED(hr)) {
            std::cerr << "Failed to start playback" << std::endl;
            return false;
        }

        // Playback parameters
        UINT32 src_sample_rate = header.sample_rate;
        UINT32 src_channels = header.channels;
        UINT32 src_bits = header.bits;
        UINT32 src_bytes_per_frame = src_channels * (src_bits / 8);
        UINT32 total_frames = header.data_size / src_bytes_per_frame;
        UINT32 current_frame = 0;

        // Resampling ratio
        double ratio = (double)format_.nSamplesPerSec / src_sample_rate;
        double position = 0.0;

        std::cout << "Playing...\n" << std::endl;

        // Playback loop
        while (current_frame < total_frames) {
            UINT32 padding;
            hr = client_->GetCurrentPadding(&padding);
            if (FAILED(hr)) break;

            UINT32 frames_available = buffer_frame_count_ - padding;
            if (frames_available == 0) {
                Sleep(1);
                continue;
            }

            BYTE* buffer;
            hr = render_->GetBuffer(frames_available, &buffer);
            if (FAILED(hr)) break;

            float* float_buffer = reinterpret_cast<float*>(buffer);

            // Convert audio data
            for (UINT32 i = 0; i < frames_available && current_frame < total_frames; i++) {
                position = current_frame * ratio;

                if (src_bits == 16) {
                    const int16_t* src16 = reinterpret_cast<const int16_t*>(
                        audio_data.data() + (UINT32)position * src_bytes_per_frame);

                    if (src_channels == 1 && format_.nChannels == 2) {
                        // Mono to stereo
                        float sample = static_cast<float>(src16[0]) / 32768.0f;
                        float_buffer[i * 2] = sample;
                        float_buffer[i * 2 + 1] = sample;
                    } else if (src_channels == 2 && format_.nChannels == 2) {
                        // Stereo to stereo
                        float_buffer[i * 2] = static_cast<float>(src16[0]) / 32768.0f;
                        float_buffer[i * 2 + 1] = static_cast<float>(src16[1]) / 32768.0f;
                    } else {
                        // Default
                        float_buffer[i * format_.nChannels] = 0.0f;
                    }
                } else if (src_bits == 32) {
                    const float* src32 = reinterpret_cast<const float*>(
                        audio_data.data() + (UINT32)position * src_bytes_per_frame);

                    if (src_channels == 1 && format_.nChannels == 2) {
                        // Mono to stereo
                        float_buffer[i * 2] = src32[0];
                        float_buffer[i * 2 + 1] = src32[0];
                    } else if (src_channels == 2 && format_.nChannels == 2) {
                        float_buffer[i * 2] = src32[0];
                        float_buffer[i * 2 + 1] = src32[1];
                    } else {
                        // Default
                        float_buffer[i * format_.nChannels] = 0.0f;
                    }
                }

                current_frame++;
                position += ratio;

                // Check if we've exceeded the audio data
                if ((UINT32)position * src_bytes_per_frame >= audio_data.size()) {
                    current_frame = total_frames; // Force exit
                }
            }

            // Fill remaining buffer with silence if needed
            UINT32 frames_written = (frames_available < (total_frames - current_frame)) ? frames_available : (total_frames - current_frame);
            if (frames_written < frames_available) {
                memset(float_buffer + frames_written * format_.nChannels,
                       0, (frames_available - frames_written) * sizeof(float) * format_.nChannels);
            }

            hr = render_->ReleaseBuffer(frames_available, 0);
            if (FAILED(hr)) break;

            // Progress indicator
            int progress = (current_frame * 100) / total_frames;
            if (progress % 10 == 0) {
                std::cout << "\rProgress: " << progress << "%" << std::flush;
            }
        }

        std::cout << "\rProgress: 100%" << std::endl;

        // Stop playback
        client_->Stop();
        std::cout << "\nPlayback completed!" << std::endl;

        return true;
    }

    void stop() {
        if (render_) {
            render_->Release();
            render_ = nullptr;
        }
        if (client_) {
            client_->Stop();
            client_->Release();
            client_ = nullptr;
        }
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }
        if (enumerator_) {
            enumerator_->Release();
            enumerator_ = nullptr;
        }
        CoUninitialize();
    }
};
#endif // _WIN32

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "   Direct WASAPI Music Player   " << std::endl;
    std::cout << "   (No external dependencies)    " << std::endl;
    std::cout << "========================================" << std::endl;

#ifdef _WIN32
    DirectWASAPIPlayer player;

    if (!player.initialize()) {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return 1;
    }

    if (!player.play_wav(argv[1])) {
        std::cerr << "Failed to play file" << std::endl;
        return 1;
    }
#else
    std::cout << "Windows audio not supported on this platform" << std::endl;
    return 1;
#endif

    return 0;
}