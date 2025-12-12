#include "audio_output_factory.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   WASAPI Audio Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Create audio output
    mp::IAudioOutput* audio_output = mp::platform::create_platform_audio_output();
    if (!audio_output) {
        std::cerr << "Failed to create audio output" << std::endl;
        return 1;
    }
    
    // Enumerate devices
    const mp::AudioDeviceInfo* devices = nullptr;
    size_t device_count = 0;
    mp::Result result = audio_output->enumerate_devices(&devices, &device_count);
    
    if (result != mp::Result::Success || device_count == 0) {
        std::cerr << "Failed to enumerate audio devices" << std::endl;
        delete audio_output;
        return 1;
    }
    
    std::cout << "Found " << device_count << " audio device(s):" << std::endl;
    for (size_t i = 0; i < device_count; ++i) {
        std::cout << "  - " << devices[i].name 
                  << " (" << devices[i].max_channels << " channels, "
                  << devices[i].default_sample_rate << " Hz)" << std::endl;
    }
    std::cout << std::endl;
    
    // Test audio playback
    std::cout << "Testing audio playback (2 second 440 Hz tone)..." << std::endl;
    
    struct AudioContext {
        float phase = 0.0f;
        float frequency = 440.0f;
        float sample_rate = 48000.0f;
    };
    
    AudioContext audio_ctx;
    
    auto audio_callback = [](void* buffer, size_t frames, void* user_data) {
        AudioContext* ctx = static_cast<AudioContext*>(user_data);
        float* output = static_cast<float*>(buffer);
        
        float phase_increment = 2.0f * static_cast<float>(M_PI) * ctx->frequency / ctx->sample_rate;
        
        for (size_t i = 0; i < frames; ++i) {
            float sample = 0.2f * std::sin(ctx->phase);
            output[i * 2] = sample;
            output[i * 2 + 1] = sample;
            
            ctx->phase += phase_increment;
            if (ctx->phase > 2.0f * static_cast<float>(M_PI)) {
                ctx->phase -= 2.0f * static_cast<float>(M_PI);
            }
        }
    };
    
    mp::AudioOutputConfig audio_config = {};
    audio_config.device_id = nullptr;
    audio_config.sample_rate = 48000;
    audio_config.channels = 2;
    audio_config.format = mp::SampleFormat::Float32;
    audio_config.buffer_frames = 1024;
    audio_config.callback = audio_callback;
    audio_config.user_data = &audio_ctx;
    
    audio_ctx.sample_rate = static_cast<float>(audio_config.sample_rate);
    
    result = audio_output->open(audio_config);
    if (result != mp::Result::Success) {
        std::cerr << "Failed to open audio device" << std::endl;
        delete audio_output;
        return 1;
    }
    
    std::cout << "Audio device opened successfully" << std::endl;
    std::cout << "Latency: " << audio_output->get_latency() << " ms" << std::endl;
    
    result = audio_output->start();
    if (result != mp::Result::Success) {
        std::cerr << "Failed to start playback" << std::endl;
        audio_output->close();
        delete audio_output;
        return 1;
    }
    
    std::cout << "Playback started..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    audio_output->stop();
    std::cout << "Playback stopped" << std::endl;
    
    audio_output->close();
    delete audio_output;
    
    std::cout << std::endl;
    std::cout << "Test completed successfully!" << std::endl;
    
    return 0;
}
