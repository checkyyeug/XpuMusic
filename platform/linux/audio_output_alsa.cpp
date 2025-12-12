#include "mp_audio_output.h"
#ifndef NO_ALSA
#include <alsa/asoundlib.h>
#endif
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <cstring>

namespace mp {
namespace platform {

// ALSA audio output implementation
class AudioOutputALSA : public IAudioOutput {
public:
    AudioOutputALSA() 
        : handle_(nullptr)
        , volume_(1.0f)
        , running_(false)
        , callback_(nullptr)
        , user_data_(nullptr)
        , buffer_frames_(0)
        , sample_rate_(0)
        , channels_(0)
        , format_(SampleFormat::Float32) {}
    
    ~AudioOutputALSA() override { close(); }
    
    Result enumerate_devices(const AudioDeviceInfo** devices, size_t* count) override {
        // Stub implementation - return default device
        static AudioDeviceInfo default_device = {
            "default",
            "Default ALSA Device",
            2,      // stereo
            44100,  // 44.1 kHz
            true
        };
        
        static AudioDeviceInfo* device_list[] = { &default_device };
        *devices = device_list[0];
        *count = 1;
        
        return Result::Success;
    }
    
    Result open(const AudioOutputConfig& config) override {
#ifdef NO_ALSA
        std::cerr << "ALSA not available, using stub implementation" << std::endl;
        return Result::NotSupported;
#else
        if (handle_) {
            close();
        }
        
        // Store configuration
        callback_ = config.callback;
        user_data_ = config.user_data;
        buffer_frames_ = config.buffer_frames;
        sample_rate_ = config.sample_rate;
        channels_ = config.channels;
        format_ = config.format;
        
        // Open PCM device
        const char* device = config.device_id ? config.device_id : "default";
        int err = snd_pcm_open(&handle_, device, SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            std::cerr << "Cannot open audio device: " << snd_strerror(err) << std::endl;
            return Result::Error;
        }
        
        // Configure hardware parameters
        snd_pcm_hw_params_t* hw_params;
        snd_pcm_hw_params_alloca(&hw_params);
        
        err = snd_pcm_hw_params_any(handle_, hw_params);
        if (err < 0) {
            std::cerr << "Cannot initialize hardware parameters: " << snd_strerror(err) << std::endl;
            snd_pcm_close(handle_);
            handle_ = nullptr;
            return Result::Error;
        }
        
        // Set access type
        err = snd_pcm_hw_params_set_access(handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) {
            std::cerr << "Cannot set access type: " << snd_strerror(err) << std::endl;
            snd_pcm_close(handle_);
            handle_ = nullptr;
            return Result::Error;
        }
        
        // Set sample format
        snd_pcm_format_t alsa_format;
        switch (config.format) {
            case SampleFormat::Int16:
                alsa_format = SND_PCM_FORMAT_S16_LE;
                break;
            case SampleFormat::Int24:
                alsa_format = SND_PCM_FORMAT_S24_LE;
                break;
            case SampleFormat::Int32:
                alsa_format = SND_PCM_FORMAT_S32_LE;
                break;
            case SampleFormat::Float32:
                alsa_format = SND_PCM_FORMAT_FLOAT_LE;
                break;
            default:
                std::cerr << "Unsupported sample format" << std::endl;
                snd_pcm_close(handle_);
                handle_ = nullptr;
                return Result::NotSupported;
        }
        
        err = snd_pcm_hw_params_set_format(handle_, hw_params, alsa_format);
        if (err < 0) {
            std::cerr << "Cannot set sample format: " << snd_strerror(err) << std::endl;
            snd_pcm_close(handle_);
            handle_ = nullptr;
            return Result::Error;
        }
        
        // Set channel count
        err = snd_pcm_hw_params_set_channels(handle_, hw_params, config.channels);
        if (err < 0) {
            std::cerr << "Cannot set channel count: " << snd_strerror(err) << std::endl;
            snd_pcm_close(handle_);
            handle_ = nullptr;
            return Result::Error;
        }
        
        // Set sample rate
        unsigned int rate = config.sample_rate;
        err = snd_pcm_hw_params_set_rate_near(handle_, hw_params, &rate, 0);
        if (err < 0) {
            std::cerr << "Cannot set sample rate: " << snd_strerror(err) << std::endl;
            snd_pcm_close(handle_);
            handle_ = nullptr;
            return Result::Error;
        }
        
        if (rate != config.sample_rate) {
            std::cout << "Actual sample rate: " << rate << " Hz (requested: " 
                      << config.sample_rate << " Hz)" << std::endl;
        }
        
        // Set buffer size
        snd_pcm_uframes_t buffer_size = config.buffer_frames * 4; // 4x buffer for safety
        err = snd_pcm_hw_params_set_buffer_size_near(handle_, hw_params, &buffer_size);
        if (err < 0) {
            std::cerr << "Cannot set buffer size: " << snd_strerror(err) << std::endl;
        }
        
        // Set period size
        snd_pcm_uframes_t period_size = config.buffer_frames;
        err = snd_pcm_hw_params_set_period_size_near(handle_, hw_params, &period_size, 0);
        if (err < 0) {
            std::cerr << "Cannot set period size: " << snd_strerror(err) << std::endl;
        }
        
        // Apply hardware parameters
        err = snd_pcm_hw_params(handle_, hw_params);
        if (err < 0) {
            std::cerr << "Cannot set hardware parameters: " << snd_strerror(err) << std::endl;
            snd_pcm_close(handle_);
            handle_ = nullptr;
            return Result::Error;
        }
        
        // Prepare device
        err = snd_pcm_prepare(handle_);
        if (err < 0) {
            std::cerr << "Cannot prepare audio device: " << snd_strerror(err) << std::endl;
            snd_pcm_close(handle_);
            handle_ = nullptr;
            return Result::Error;
        }
        
        std::cout << "ALSA audio output opened successfully" << std::endl;
        std::cout << "  Sample rate: " << rate << " Hz" << std::endl;
        std::cout << "  Channels: " << config.channels << std::endl;
        std::cout << "  Buffer frames: " << buffer_size << std::endl;
        std::cout << "  Period frames: " << period_size << std::endl;
        
        return Result::Success;
#endif
    }
    
    Result start() override {
#ifdef NO_ALSA
        return Result::NotSupported;
#else
        if (!handle_) {
            return Result::NotInitialized;
        }
        
        if (running_) {
            return Result::Success; // Already running
        }
        
        running_ = true;
        
        // Start playback thread
        playback_thread_ = std::thread([this]() {
            playback_loop();
        });
        
        std::cout << "ALSA audio playback started" << std::endl;
        return Result::Success;
#endif
    }
    
    Result stop() override {
#ifdef NO_ALSA
        return Result::NotSupported;
#else
        if (!running_) {
            return Result::Success;
        }
        
        running_ = false;
        
        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }
        
        if (handle_) {
            snd_pcm_drop(handle_);
        }
        
        std::cout << "ALSA audio playback stopped" << std::endl;
        return Result::Success;
#endif
    }
    
    void close() override {
#ifndef NO_ALSA
        stop();
        
        if (handle_) {
            snd_pcm_close(handle_);
            handle_ = nullptr;
        }
        
        callback_ = nullptr;
        user_data_ = nullptr;
#endif
    }
    
    uint32_t get_latency() const override {
#ifdef NO_ALSA
        return 50;
#else
        if (!handle_ || sample_rate_ == 0) {
            return 50;
        }
        
        // Calculate latency based on buffer size
        return (buffer_frames_ * 1000) / sample_rate_;
#endif
    }
    
    Result set_volume(float volume) override {
        volume_ = volume;
        return Result::Success;
    }
    
    float get_volume() const override {
        return volume_;
    }
    
private:
#ifndef NO_ALSA
    void playback_loop() {
        // Calculate buffer size in bytes
        size_t bytes_per_sample = 4; // Float32
        switch (format_) {
            case SampleFormat::Int16: bytes_per_sample = 2; break;
            case SampleFormat::Int24: bytes_per_sample = 3; break;
            case SampleFormat::Int32: bytes_per_sample = 4; break;
            case SampleFormat::Float32: bytes_per_sample = 4; break;
            case SampleFormat::Float64: bytes_per_sample = 8; break;
            default: bytes_per_sample = 4; break;
        }
        
        size_t buffer_size = buffer_frames_ * channels_ * bytes_per_sample;
        std::vector<uint8_t> buffer(buffer_size);
        
        while (running_) {
            // Call user callback to get audio data
            if (callback_) {
                callback_(buffer.data(), buffer_frames_, user_data_);
                
                // Apply volume
                if (volume_ != 1.0f && format_ == SampleFormat::Float32) {
                    float* samples = reinterpret_cast<float*>(buffer.data());
                    size_t total_samples = buffer_frames_ * channels_;
                    for (size_t i = 0; i < total_samples; i++) {
                        samples[i] *= volume_;
                    }
                }
            } else {
                // No callback, fill with silence
                std::memset(buffer.data(), 0, buffer_size);
            }
            
            // Write to ALSA
            snd_pcm_sframes_t frames = snd_pcm_writei(handle_, buffer.data(), buffer_frames_);
            
            if (frames < 0) {
                // Handle underrun
                if (frames == -EPIPE) {
                    std::cerr << "ALSA underrun occurred" << std::endl;
                    snd_pcm_prepare(handle_);
                } else if (frames == -ESTRPIPE) {
                    std::cerr << "ALSA suspended, resuming..." << std::endl;
                    while ((frames = snd_pcm_resume(handle_)) == -EAGAIN) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    if (frames < 0) {
                        snd_pcm_prepare(handle_);
                    }
                } else {
                    std::cerr << "ALSA write error: " << snd_strerror(frames) << std::endl;
                    break;
                }
            } else if (frames < (snd_pcm_sframes_t)buffer_frames_) {
                std::cerr << "Short write: " << frames << " / " << buffer_frames_ << std::endl;
            }
        }
    }
#endif
    
#ifndef NO_ALSA
    snd_pcm_t* handle_;
#else
    void* handle_;
#endif
    float volume_;
    std::atomic<bool> running_;
    std::thread playback_thread_;
    
    AudioCallback callback_;
    void* user_data_;
    uint32_t buffer_frames_;
    uint32_t sample_rate_;
    uint32_t channels_;
    SampleFormat format_;
};

}} // namespace mp::platform

// Factory function
extern "C" mp::IAudioOutput* create_alsa_output() {
    return new mp::platform::AudioOutputALSA();
}
