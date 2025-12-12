/**
 * @file simple_wav_test.cpp
 * @brief Minimal WAV player test WITH DETAILED ERRORS
 */

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <cmath>
#include <atomic>
#include <vector>

#ifdef _WIN32
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

bool check_hr(HRESULT hr) { return hr == S_OK; }

void print_hr_error(const char* step, HRESULT hr) {
    std::cerr << "鉂?" << step << " failed: 0x" << std::hex << hr << std::dec << std::endl;
}

bool play_simple_wav(const char* filename) {
    std::cout << "Opening: " << filename << std::endl;
    
    std::ifstream f(filename, std::ios::binary);
    if (!f) {
        std::cerr << "Failed to open file" << std::endl;
        return false;
    }
    
    char header[44];
    f.read(header, 44);
    
    if (std::strncmp(header, "RIFF", 4) != 0) return false;
    if (std::strncmp(header+8, "WAVE", 4) != 0) return false;
    
    std::cout << "鉁?Valid WAV header" << std::endl;
    
    // Parse format
    uint16_t channels = *(uint16_t*)(header + 22);
    uint32_t sampleRate = *(uint32_t*)(header + 24);
    uint16_t bitsPerSample = *(uint16_t*)(header + 34);
    uint32_t dataSize = *(uint32_t*)(header + 40);
    
    std::cout << "鉁?Channels: " << channels << std::endl;
    std::cout << "鉁?Sample Rate: " << sampleRate << std::endl;
    std::cout << "鉁?Bits: " << bitsPerSample << std::endl;
    std::cout << "鉁?Data Size: " << dataSize << " bytes" << std::endl;
    
    // Read data
    std::vector<char> data(dataSize);
    f.read(data.data(), dataSize);
    std::cout << "鉁?Read " << f.gcount() << " bytes of audio" << std::endl;
    f.close();
    
    // Setup WASAPI with detailed error checking
    std::cout << "\nStep 1: CoInitialize..." << std::endl;
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        print_hr_error("CoInitialize", hr);
        return false;
    }
    std::cout << "鉁?COM initialized" << std::endl;
    
    std::cout << "Step 2: Create device enumerator..." << std::endl;
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&enumerator);
    if (FAILED(hr)) {
        print_hr_error("Create device enumerator", hr);
        return false;
    }
    std::cout << "鉁?Device enumerator created" << std::endl;
    
    std::cout << "Step 3: Get default audio endpoint..." << std::endl;
    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        print_hr_error("Get default audio endpoint", hr);
        enumerator->Release();
        return false;
    }
    std::cout << "鉁?Default audio endpoint obtained" << std::endl;
    
    std::cout << "Step 4: Activate audio client..." << std::endl;
    IAudioClient* client = nullptr;
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                         NULL, (void**)&client);
    if (FAILED(hr)) {
        print_hr_error("Activate audio client", hr);
        device->Release();
        enumerator->Release();
        return false;
    }
    std::cout << "鉁?Audio client activated" << std::endl;
    
    // Configure format
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = channels;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = bitsPerSample;
    wfx.nBlockAlign = (channels * bitsPerSample) / 8;
    wfx.nAvgBytesPerSec = sampleRate * wfx.nBlockAlign;
    
    std::cout << "\nStep 5: Initialize audio client..." << std::endl;
    std::cout << "  Format: PCM" << std::endl;
    std::cout << "  Channels: " << wfx.nChannels << std::endl;
    std::cout << "  Sample Rate: " << wfx.nSamplesPerSec << std::endl;
    std::cout << "  Bits: " << wfx.wBitsPerSample << std::endl;
    std::cout << "  Block Align: " << wfx.nBlockAlign << std::endl;
    
    hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                           0, 10000000, 0, &wfx, NULL);
    
    if (FAILED(hr)) {
        print_hr_error("Initialize audio client", hr);
        std::cout << "\nThis usually means:" << std::endl;
        std::cout << "  - The audio format is not supported" << std::endl;
        std::cout << "  - Another app has exclusive access" << std::endl;
        std::cout << "  - Audio drivers need updating" << std::endl;
        client->Release();
        device->Release();
        enumerator->Release();
        return false;
    }
    std::cout << "鉁?Audio client initialized" << std::endl;
    
    std::cout << "Step 6: Get render client..." << std::endl;
    IAudioRenderClient* render = nullptr;
    hr = client->GetService(__uuidof(IAudioRenderClient), (void**)&render);
    if (FAILED(hr)) {
        print_hr_error("Get render client", hr);
        client->Release();
        device->Release();
        enumerator->Release();
        return false;
    }
    std::cout << "鉁?Render client obtained" << std::endl;
    
    std::cout << "Step 7: Get buffer size..." << std::endl;
    UINT32 bufferSize = 0;
    client->GetBufferSize(&bufferSize);
    std::cout << "鉁?Buffer size: " << bufferSize << " frames" << std::endl;
    
    std::cout << "\n鈺斺晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晽" << std::endl;
    std::cout << "鈺? STARTING PLAYBACK...                       鈺? << std::endl;
    std::cout << "鈺? Volume should be at 50%                    鈺? << std::endl;
    std::cout << "鈺氣晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨暆" << std::endl;
    std::cout << std::endl;
    
    hr = client->Start();
    if (FAILED(hr)) {
        print_hr_error("Start audio", hr);
        render->Release();
        client->Release();
        device->Release();
        enumerator->Release();
        return false;
    }
    std::cout << "鉁?Audio playback started" << std::endl;
    
    // Stream audio data with small delay between chunks
    const char* data_ptr = data.data();
    const char* end_ptr = data_ptr + data.size();
    int totalSize = data.size();
    int bytesSent = 0;
    
    while (data_ptr < end_ptr) {
        UINT32 padding = 0;
        client->GetCurrentPadding(&padding);
        
        UINT32 available = bufferSize - padding;
        if (available > 0) {
            BYTE* buffer = nullptr;
            HRESULT hr = render->GetBuffer(available, &buffer);
            if (hr == S_OK && buffer) {
                UINT32 bytesNeeded = available * wfx.nBlockAlign;
                UINT32 bytesRemaining = (UINT32)(end_ptr - data_ptr);
                UINT32 bytesToCopy = (bytesNeeded < bytesRemaining) ? bytesNeeded : bytesRemaining;
                
                if (bytesToCopy > 0) {
                    memcpy(buffer, data_ptr, bytesToCopy);
                    data_ptr += bytesToCopy;
                    bytesSent += bytesToCopy;
                }
                
                render->ReleaseBuffer(available, 0);
            }
        }
        
        // Progress indicator
        if (bytesSent % (totalSize / 10) == 0) {
            int percent = (bytesSent * 100) / totalSize;
            std::cout << "  Progress: " << percent << "%" << std::endl;
        }
    }
    
    std::cout << "鉁?All data sent (" << bytesSent << " bytes)" << std::endl;
    
    // Wait for playback to complete (with timeout)
    float duration = (float)totalSize / wfx.nAvgBytesPerSec;
    std::cout << "  Waiting " << duration << " seconds for playback..." << std::endl;
    
    int wait_ms = (int)(duration * 1000) + 500; // +500ms buffer
    for (int i = 0; i < wait_ms / 100; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (i % 10 == 0) {
            std::cout << "." << std::flush;
        }
    }
    std::cout << std::endl;
    
    // Cleanup
    client->Stop();
    render->Release();
    client->Release();
    device->Release();
    enumerator->Release();
    
    std::cout << "\n鉁?Playback finished!" << std::endl;
    return true;
}
#endif

int main(int argc, char** argv) {
    std::cout << "鈺斺晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晽" << std::endl;
    std::cout << "鈺?   WAV PLAYER WITH DETAILED ERRORS          鈺? << std::endl;
    std::cout << "鈺氣晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨暆" << std::endl;
    std::cout << std::endl;
    
    const char* filename = (argc > 1) ? argv[1] : "1khz.wav";
    
#ifdef _WIN32
    bool success = play_simple_wav(filename);
    
    if (success) {
        std::cout << "\n鈺斺晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晽" << std::endl;
        std::cout << "鈺? 鉁?SUCCESS! Audio playback complete!        鈺? << std::endl;
        std::cout << "鈺氣晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨暆" << std::endl;
        std::cout << "\nIf you heard nothing:" << std::endl;
        std::cout << "  1. Check system volume (bottom-right speaker icon)" << std::endl;
        std::cout << "  2. Ensure speakers/headphones are connected" << std::endl;
        std::cout << "  3. Make sure audio isn't muted" << std::endl;
        return 0;
    } else {
        std::cout << "\n鈺斺晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晽" << std::endl;
        std::cout << "鈺? 鉂?FAILED - See errors above               鈺? << std::endl;
        std::cout << "鈺氣晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨暆" << std::endl;
        return 1;
    }
#else
    std::cout << "鉂?Windows-only test (WASAPI)" << std::endl;
    return 1;
#endif
}
