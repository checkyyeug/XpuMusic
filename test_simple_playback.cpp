// Simple test to isolate the playback issue
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#endif

int main() {
    // Create test sine wave
    const int SAMPLE_RATE = 48000;
    const int DURATION_SECONDS = 3;
    const int FRAMES = SAMPLE_RATE * DURATION_SECONDS;
    const int CHANNELS = 2;

    std::vector<float> buffer(FRAMES * CHANNELS);

    for (int i = 0; i < FRAMES; ++i) {
        float sample = 0.3f * sin(2.0f * 3.14159f * 440.0f * i / SAMPLE_RATE);
        buffer[i * 2] = sample;
        buffer[i * 2 + 1] = sample;
    }

    std::cout << "Generated " << FRAMES << " frames of test audio" << std::endl;

#ifdef _WIN32
    // Initialize WASAPI
    HRESULT hr;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* client = nullptr;
    IAudioRenderClient* render = nullptr;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // Get default device
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                        (void**)&enumerator);

    enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                    nullptr, (void**)&client);

    // Get mix format
    WAVEFORMATEX* mixFormat = nullptr;
    client->GetMixFormat(&mixFormat);

    // Initialize for 48000Hz stereo float
    mixFormat->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    mixFormat->nSamplesPerSec = SAMPLE_RATE;
    mixFormat->nChannels = CHANNELS;
    mixFormat->wBitsPerSample = 32;
    mixFormat->nBlockAlign = (mixFormat->nChannels * mixFormat->wBitsPerSample) / 8;
    mixFormat->nAvgBytesPerSec = mixFormat->nSamplesPerSec * mixFormat->nBlockAlign;
    mixFormat->cbSize = 0;

    hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mixFormat, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize audio client: " << std::hex << hr << std::endl;
        return 1;
    }

    UINT32 bufferFrames;
    client->GetBufferSize(&bufferFrames);
    client->GetService(__uuidof(IAudioRenderClient), (void**)&render);

    std::cout << "WASAPI initialized!" << std::endl;
    std::cout << "Buffer frames: " << bufferFrames << std::endl;

    // Start playback
    client->Start();
    std::cout << "Playing 3 seconds of 440Hz tone..." << std::endl;

    // Playback loop
    UINT32 framePos = 0;
    int iterations = 0;

    while (framePos < FRAMES && iterations < 1000) { // Safety limit
        UINT32 padding;
        client->GetCurrentPadding(&padding);
        UINT32 available = bufferFrames - padding;

        if (available == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        BYTE* wasapiBuffer;
        hr = render->GetBuffer(available, &wasapiBuffer);
        if (FAILED(hr)) break;

        UINT32 framesToWrite = (std::min)(available, FRAMES - framePos);
        float* floatBuffer = reinterpret_cast<float*>(wasapiBuffer);

        // Copy audio data
        for (UINT32 i = 0; i < framesToWrite * CHANNELS; ++i) {
            floatBuffer[i] = buffer[framePos * CHANNELS + i];
        }

        render->ReleaseBuffer(framesToWrite, 0);
        framePos += framesToWrite;

        iterations++;

        // Show progress every second
        if (framePos % SAMPLE_RATE == 0) {
            int progress = (framePos * 100) / FRAMES;
            std::cout << "\rProgress: " << progress << "%  Iterations: " << iterations << std::flush;
        }
    }

    std::cout << "\nPlayback finished!" << std::endl;
    std::cout << "Total iterations: " << iterations << std::endl;

    // Cleanup
    client->Stop();
    render->Release();
    client->Release();
    device->Release();
    enumerator->Release();
    CoUninitialize();
#endif

    return 0;
}