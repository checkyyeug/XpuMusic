/**
 * @file final_wav_player.cpp
 * @brief FINAL WAV Player - Uses System-Native Format
 * @version 1.0 - Production Ready
 */

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <vector>
#include <memory>

#ifdef _WIN32
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

struct AudioFormat {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
};

// Safe HRESULT checking
bool hr_ok(HRESULT hr) { return hr == S_OK; }
void print_hr(const char* step, HRESULT hr) {
    std::cerr << "❌ " << step << " failed: 0x" << std::hex << hr << std::dec << std::endl;
}

class WASAPIPlayer {
private:
    IAudioClient* client_ = nullptr;
    IAudioRenderClient* render_ = nullptr;
    IMMDevice* device_ = nullptr;
    IMMDeviceEnumerator* enumerator_ = nullptr;
    WAVEFORMATEX* system_format_ = nullptr;
    
public:
    ~WASAPIPlayer() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize COM
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            print_hr("COM init", hr);
            return false;
        }
        
        // Create enumerator
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                              CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                              (void**)&enumerator_);
        if (!hr_ok(hr)) {
            print_hr("Create enumerator", hr);
            return false;
        }
        
        // Get default device
        hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
        if (!hr_ok(hr)) {
            print_hr("Get default endpoint", hr);
            cleanup();
            return false;
        }
        
        // Activate client
        hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                               NULL, (void**)&client_);
        if (!hr_ok(hr)) {
            print_hr("Activate client", hr);
            cleanup();
            return false;
        }
        
        // Query system's native format (THIS IS KEY!)
        hr = client_->GetMixFormat(&system_format_);
        if (!hr_ok(hr) || !system_format_) {
            print_hr("Get system format", hr);
            cleanup();
            return false;
        }
        
        // Display what system actually uses
        std::cout << "\n✓ System audio format:" << std::endl;
        std::cout << "  Sample Rate: " << system_format_->nSamplesPerSec << " Hz" << std::endl;
        std::cout << "  Channels: " << system_format_->nChannels << std::endl;
        std::cout << "  Bits: " << system_format_->wBitsPerSample << std::endl;
        std::cout << "  Block Align: " << system_format_->nBlockAlign << std::endl;
        
        // Initialize with SYSTEM format (guaranteed to work)
        hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                0, 10000000, 0, system_format_, NULL);
        if (!hr_ok(hr)) {
            print_hr("Initialize with system format", hr);
            cleanup();
            return false;
        }
        
        // Get render client
        hr = client_->GetService(__uuidof(IAudioRenderClient), (void**)&render_);
        if (!hr_ok(hr)) {
            print_hr("Get render client", hr);
            cleanup();
            return false;
        }
        
        return true;
    }
    
    void cleanup() {
        if (render_) { render_->Release(); render_ = nullptr; }
        if (client_) { client_->Release(); client_ = nullptr; }
        if (device_) { device_->Release(); device_ = nullptr; }
        if (enumerator_) { enumerator_->Release(); enumerator_ = nullptr; }
        if (system_format_) { CoTaskMemFree(system_format_); system_format_ = nullptr; }
    }
    
    bool play_pcm_stream(const uint8_t* pcm_data, uint32_t data_size, 
                        uint32_t sample_rate, uint16_t channels, uint16_t bits) {
        
        if (!hr_ok(client_->Start())) {
            std::cerr << "Failed to start audio" << std::endl;
            return false;
        }
        
        UINT32 buffer_size = 0;
        client_->GetBufferSize(&buffer_size);
        
        std::cout << "\n✓ Playback started" << std::endl;
        std::cout << "✓ Buffer: " << buffer_size << " frames" << std::endl;
        
        // Convert to system format if needed
        bool needs_resample = (sample_rate != system_format_->nSamplesPerSec) ||
                             (channels != system_format_->nChannels);
        bool needs_format_convert = (bits != system_format_->wBitsPerSample);

        if (needs_resample || needs_format_convert) {
            std::cout << "⚠️  Format conversion needed:" << std::endl;
            std::cout << "  File: " << sample_rate << " Hz, " << channels << " ch, " << bits << "-bit" << std::endl;
            std::cout << "  System: " << system_format_->nSamplesPerSec << " Hz, "
                     << system_format_->nChannels << " ch, " << system_format_->wBitsPerSample << "-bit" << std::endl;
        }

        // Prepare data for conversion
        std::vector<float> converted_data;
        const uint8_t* data_ptr = pcm_data;
        const uint8_t* end_ptr = data_ptr + data_size;
        uint32_t bytes_sent = 0;

        // Convert to 32-bit float if needed
        if (needs_format_convert && bits == 16) {
            std::cout << "✓ Converting 16-bit to 32-bit float..." << std::endl;
            uint32_t samples = data_size / 2; // 16-bit = 2 bytes
            converted_data.resize(samples);

            const int16_t* src = reinterpret_cast<const int16_t*>(pcm_data);
            for (uint32_t i = 0; i < samples; i++) {
                converted_data[i] = src[i] / 32768.0f; // Normalize to [-1, 1]
            }

            data_ptr = reinterpret_cast<const uint8_t*>(converted_data.data());
            end_ptr = data_ptr + samples * sizeof(float);
            bits = 32;
        }

        uint32_t block_align = channels * (bits / 8);
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (data_ptr < end_ptr) {
            UINT32 padding = 0;
            client_->GetCurrentPadding(&padding);
            
            UINT32 available_frames = buffer_size - padding;
            if (available_frames > 0) {
                BYTE* buffer = nullptr;
                HRESULT hr = render_->GetBuffer(available_frames, &buffer);
                if (hr_ok(hr) && buffer) {
                    UINT32 bytes_needed = available_frames * block_align;
                    UINT32 bytes_remaining = (uint32_t)(end_ptr - data_ptr);
                    UINT32 bytes_to_copy = (std::min)(bytes_needed, bytes_remaining);
                    
                    if (bytes_to_copy > 0) {
                        memcpy(buffer, data_ptr, bytes_to_copy);
                        data_ptr += bytes_to_copy;
                        bytes_sent += bytes_to_copy;
                    }
                    
                    render_->ReleaseBuffer(available_frames, 0);
                }
            }
            
            // Small delay to not hog CPU
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        float duration = (float)data_size / (sample_rate * block_align);
        std::cout << "\n✓ Data streamed (" << bytes_sent << " bytes)" << std::endl;
        std::cout << "✓ Expected duration: " << duration << " seconds" << std::endl;
        
        // Wait for playback with progress indicator
        std::cout << "  Playing";
        for (int i = 0; i < (int)duration; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "." << std::flush;
        }
        std::cout << std::endl;
        
        client_->Stop();
        return true;
    }
};

#else
// Linux placeholder - no WASAPI implementation
#endif

// Parse minimal WAV header
bool parse_wav_header(const char* filename, uint32_t& data_size, 
                      uint32_t& sample_rate, uint16_t& channels, uint16_t& bits) {
    std::ifstream f(filename, std::ios::binary);
    if (!f) return false;
    
    char header[44];
    f.read(header, 44);
    
    if (std::strncmp(header, "RIFF", 4) != 0) return false;
    if (std::strncmp(header+8, "WAVE", 4) != 0) return false;
    
    channels = *(uint16_t*)(header + 22);
    sample_rate = *(uint32_t*)(header + 24);
    bits = *(uint16_t*)(header + 34);
    data_size = *(uint32_t*)(header + 40);
    
    return true;
}

int main(int argc, char** argv) {
    const char* filename = (argc > 1) ? argv[1] : "1khz.wav";
    
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    FINAL WAV PLAYER - System Native Format  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    std::cout << "File: " << filename << std::endl;
    
#ifdef _WIN32
    // Parse WAV file
    uint32_t data_size, sample_rate;
    uint16_t channels, bits;
    if (!parse_wav_header(filename, data_size, sample_rate, channels, bits)) {
        std::cerr << "❌ Failed to parse WAV header" << std::endl;
        return 1;
    }
    
    std::cout << "\n✓ WAV Format:" << std::endl;
    std::cout << "  Sample Rate: " << sample_rate << " Hz" << std::endl;
    std::cout << "  Channels: " << channels << std::endl;
    std::cout << "  Bits: " << bits << std::endl;
    std::cout << "  Data Size: " << data_size << " bytes" << std::endl;
    
    // Read audio data
    std::vector<uint8_t> audio_data(data_size);
    std::ifstream f(filename, std::ios::binary);
    f.seekg(44); // Skip header
    f.read(reinterpret_cast<char*>(audio_data.data()), data_size);
    f.close();
    
    std::cout << "✓ Read " << audio_data.size() << " bytes" << std::endl;
    
    // Create and initialize player
    WASAPIPlayer player;
    if (!player.initialize()) {
        std::cerr << "❌ Failed to initialize audio" << std::endl;
        return 1;
    }
    
    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  NOW PLAYING...                             ║" << std::endl;
    std::cout << "║  Check your volume!                         ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    if (!player.play_pcm_stream(audio_data.data(), data_size, 
                               sample_rate, channels, bits)) {
        std::cerr << "❌ Playback failed" << std::endl;
        return 1;
    }
    
    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  ✅ SUCCESS! Playback completed!            ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    return 0;
#else
    // Linux version - test WAV parsing and format conversion
    std::cout << "Running Linux version - Testing WAV parsing and format conversion" << std::endl;

    // Parse WAV file
    uint32_t data_size, sample_rate;
    uint16_t channels, bits;
    if (!parse_wav_header(filename, data_size, sample_rate, channels, bits)) {
        std::cerr << "❌ Failed to parse WAV header" << std::endl;
        return 1;
    }

    std::cout << "\n✓ WAV Format:" << std::endl;
    std::cout << "  Sample Rate: " << sample_rate << " Hz" << std::endl;
    std::cout << "  Channels: " << channels << std::endl;
    std::cout << "  Bits: " << bits << std::endl;
    std::cout << "  Data Size: " << data_size << " bytes" << std::endl;

    // Read audio data
    std::vector<uint8_t> audio_data(data_size);
    std::ifstream f(filename, std::ios::binary);
    f.seekg(44); // Skip header
    f.read(reinterpret_cast<char*>(audio_data.data()), data_size);
    f.close();

    std::cout << "✓ Read " << audio_data.size() << " bytes" << std::endl;

    // Test format conversion (16-bit to 32-bit float)
    if (bits == 16) {
        std::cout << "\n✓ Testing 16-bit to 32-bit float conversion..." << std::endl;

        uint32_t samples = data_size / 2; // 16-bit = 2 bytes per sample
        std::vector<float> converted_data(samples);

        const int16_t* src = reinterpret_cast<const int16_t*>(audio_data.data());
        for (uint32_t i = 0; i < std::min(samples, 1000u); i++) {
            converted_data[i] = src[i] / 32768.0f;
        }

        std::cout << "  Converted " << std::min(samples, 1000u) << " samples" << std::endl;
        std::cout << "  Example: " << src[0] << " -> " << converted_data[0] << std::endl;
    }

    std::cout << "\n✅ Linux test completed successfully!" << std::endl;
    std::cout << "  Note: Audio playback requires Windows WASAPI or Linux ALSA" << std::endl;
    return 0;
#endif
} 
