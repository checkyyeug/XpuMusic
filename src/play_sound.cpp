/**
 * @file play_sound.cpp
 * @brief Minimal audio playback test - bypass everything
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstring>

#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#include <shlwapi.h>
#endif

// Direct test - no core engine
typedef enum {
    Result_Success = 0,
    Result_Failure = 1
} SimpleResult;

// Minimal audio callback
typedef void (*AudioCallback)(void* buffer, size_t frames, void* user_data);

// Simple sine wave generation
struct AudioContext {
    float phase = 0.0f;
    float frequency = 440.0f;  // A4 note
    float sample_rate = 44100.0f;
    float amplitude = 0.3f;    // 30% volume
};

void generate_sine_wave(void* buffer, size_t frames, void* user_data) {
    AudioContext* ctx = static_cast<AudioContext*>(user_data);
    float* output = static_cast<float*>(buffer);
    
    const float pi = 3.14159265358979323846f;
    float phase_increment = 2.0f * pi * ctx->frequency / ctx->sample_rate;
    
    for (size_t i = 0; i < frames; ++i) {
        float sample = ctx->amplitude * std::sin(ctx->phase);
        output[i * 2] = sample;      // Left channel
        output[i * 2 + 1] = sample;  // Right channel
        
        ctx->phase += phase_increment;
        if (ctx->phase > 2.0f * pi) {
            ctx->phase -= 2.0f * pi;
        }
    }
}

#ifdef _WIN32
// Windows WASAPI direct test
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

SimpleResult test_wasapi_direct() {
    std::cout << "\n=== Testing Windows WASAPI Directly ===" << std::endl;
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "Failed to initialize COM" << std::endl;
        return Result_Failure;
    }
    
    // Get default audio device
    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, 
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&pEnumerator);
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create device enumerator" << std::endl;
        CoUninitialize();
        return Result_Failure;
    }
    
    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to get default audio endpoint" << std::endl;
        pEnumerator->Release();
        CoUninitialize();
        return Result_Failure;
    }
    
    std::cout << "鉁?Audio device obtained" << std::endl;
    
    // Get audio client
    IAudioClient* pAudioClient = NULL;
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                           NULL, (void**)&pAudioClient);
    if (FAILED(hr)) {
        std::cerr << "Failed to activate audio client" << std::endl;
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return Result_Failure;
    }
    
    // Get device format
    WAVEFORMATEX* pwfx = NULL;
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        std::cerr << "Failed to get mix format" << std::endl;
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return Result_Failure;
    }
    
    std::cout << "鉁?Audio format: " << pwfx->nSamplesPerSec << " Hz, " 
              << pwfx->nChannels << " channels" << std::endl;
    
    // Initialize audio client
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  0,
                                  10000000,  // 1 second buffer
                                  0,
                                  pwfx,
                                  NULL);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize audio client: 0x" << std::hex << hr << std::endl;
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return Result_Failure;
    }
    
    // Get buffer size
    UINT32 bufferFrameCount;
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    if (FAILED(hr)) {
        std::cerr << "Failed to get buffer size" << std::endl;
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return Result_Failure;
    }
    
    std::cout << "鉁?Buffer size: " << bufferFrameCount << " frames" << std::endl;
    
    // Get render client
    IAudioRenderClient* pRenderClient = NULL;
    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient),
                                  (void**)&pRenderClient);
    if (FAILED(hr)) {
        std::cerr << "Failed to get render client" << std::endl;
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return Result_Failure;
    }
    
    // Start audio
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "Failed to start audio client" << std::endl;
        pRenderClient->Release();
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return Result_Failure;
    }
    
    std::cout << "鉁?Audio started - Playing 2 second tone..." << std::endl;
    
    // Generate and play audio
    AudioContext ctx;
    ctx.sample_rate = static_cast<float>(pwfx->nSamplesPerSec);
    ctx.frequency = 440.0f;
    ctx.amplitude = 0.3f;
    
    BYTE* pData;
    UINT32 numFramesPadding;
    UINT32 numFramesAvailable;
    
    auto start_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(2);
    
    while (std::chrono::steady_clock::now() - start_time < duration) {
        // Get current padding
        hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
        if (FAILED(hr)) break;
        
        numFramesAvailable = bufferFrameCount - numFramesPadding;
        
        if (numFramesAvailable > 0) {
            // Get buffer
            hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
            if (FAILED(hr)) break;
            
            // Generate audio
            generate_sine_wave(pData, numFramesAvailable, &ctx);
            
            // Release buffer
            hr = pRenderClient->ReleaseBuffer(numFramesAvailable, 0);
            if (FAILED(hr)) break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Cleanup
    pAudioClient->Stop();
    pRenderClient->Release();
    CoTaskMemFree(pwfx);
    pAudioClient->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();
    
    std::cout << "鉁?Audio playback complete!" << std::endl;
    return Result_Success;
}
#endif

int main(int argc, char** argv) {
    std::cout << "==============================================" << std::endl;
    std::cout << "   Direct Audio Playback Test" << std::endl;
    std::cout << "   Bypassing CoreEngine - Testing WASAPI" << std::endl;
    std::cout << "==============================================" << std::endl;
    
#ifdef _WIN32
    std::cout << "Windows detected - Testing WASAPI directly...\n" << std::endl;
    
    SimpleResult result = test_wasapi_direct();
    
    if (result == Result_Success) {
        std::cout << "\n鉁?SUCCESS! Audio played through WASAPI!" << std::endl;
        std::cout << "The audio stack is working - problem is in CoreEngine init." << std::endl;
    } else {
        std::cout << "\n鉂?Failed to play audio via WASAPI." << std::endl;
    }
#else
    std::cout << "Non-Windows platform - Audio test skipped." << std::endl;
    std::cout << "Platform: " << 
        #ifdef __linux__
            "Linux"
        #elif defined(__APPLE__)
            "macOS"
        #else
            "Unknown"
        #endif
              << std::endl;
#endif
    
    std::cout << "\n==============================================" << std::endl;
    
    return 0;
}
