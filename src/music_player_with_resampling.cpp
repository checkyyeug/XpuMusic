/**
 * @file music_player_with_resampling.cpp
 * @brief Music player with integrated sample rate conversion
 * @date 2025-12-11
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
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

// Include our resampling components
#include "audio/enhanced_sample_rate_converter.h"
#include "audio/cubic_resampler.h"

// Helper function to check HRESULT safely
inline bool check_hresult_ok(HRESULT hr) {
    return hr == S_OK;
}

// WAV file header structure
#pragma pack(push, 1)
struct WavHeader {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_size;
};
#pragma pack(pop)

/**
 * @brief Enhanced audio format conversion with resampling support
 * Supports: Various bit depths, sample rates, and channel configurations
 */
class AudioFormatConverter {
private:
    std::unique_ptr<audio::EnhancedSampleRateConverter> resampler_;
    bool conversion_message_shown;
    
public:
    AudioFormatConverter() : conversion_message_shown(false) {
        resampler_ = std::make_unique<audio::EnhancedSampleRateConverter>(audio::ResampleQuality::Good);
    }
    
    /**
     * Convert audio format with resampling if needed
     */
    UINT32 convert_audio_format(const uint8_t* src, uint8_t* dst, UINT32 frames,
                                const WavHeader& wav_header, const WAVEFORMATEX& wasapi_format) {
        
        // Validate parameters
        if (!src || !dst || frames == 0) {
            return 0;
        }
        
        // Get format information
        UINT32 src_rate = wav_header.sample_rate;
        UINT32 dst_rate = wasapi_format.nSamplesPerSec;
        UINT32 src_channels = wav_header.num_channels;
        UINT32 dst_channels = wasapi_format.nChannels;
        UINT32 src_bits = wav_header.bits_per_sample;
        UINT32 dst_bits = wasapi_format.wBitsPerSample;
        
        // Check if we need resampling
        bool need_resampling = (src_rate != dst_rate);
        bool need_bit_depth_conversion = (src_bits != dst_bits);
        bool need_channel_conversion = (src_channels != dst_channels);
        
        // Step 1: Convert input to float format for processing
        std::vector<float> float_input;
        convert_to_float(src, float_input, frames, src_channels, src_bits);
        
        // Step 2: Resample if needed
        std::vector<float> resampled_buffer;
        const float* processing_buffer = float_input.data();
        UINT32 processing_frames = frames;
        
        if (need_resampling) {
            if (!resampler_->initialize(src_rate, dst_rate, src_channels)) {
                std::cerr << "Failed to initialize resampler: " << src_rate << "Hz -> " << dst_rate << "Hz" << std::endl;
                return 0;
            }
            
            // Calculate output buffer size (add 20% margin for safety)
            double ratio = static_cast<double>(dst_rate) / src_rate;
            UINT32 max_output_frames = static_cast<UINT32>(frames * ratio * 1.2 + 1);

            // Limit the output buffer size to prevent excessive memory usage
            const UINT32 MAX_BUFFER_SIZE = 65536;
            if (max_output_frames > MAX_BUFFER_SIZE) {
                max_output_frames = MAX_BUFFER_SIZE;
            }

            resampled_buffer.resize(max_output_frames * dst_channels);
            
            // Perform resampling
            int output_frames = resampler_->convert(float_input.data(), frames, 
                                                   resampled_buffer.data(), max_output_frames);
            
            if (output_frames <= 0) {
                std::cerr << "Resampling failed" << std::endl;
                return 0;
            }
            
            processing_buffer = resampled_buffer.data();
            processing_frames = output_frames;
        }
        
        // Step 3: Channel conversion if needed
        std::vector<float> channel_converted_buffer;
        if (need_channel_conversion) {
            convert_channels(processing_buffer, channel_converted_buffer, 
                           processing_frames, src_channels, dst_channels);
            processing_buffer = channel_converted_buffer.data();
        }
        
        // Step 4: Convert to output format
        UINT32 output_frames = convert_from_float(processing_buffer, dst, processing_frames, 
                                                 dst_channels, dst_bits, wasapi_format.wFormatTag);
        
        // Show conversion message only once
        if (!conversion_message_shown && (need_resampling || need_bit_depth_conversion || need_channel_conversion)) {
            std::cout << "鉁?Converting audio: " << src_rate << "Hz " << src_bits << "-bit " 
                      << src_channels << "ch 鈫?" << dst_rate << "Hz " << dst_bits << "-bit " 
                      << dst_channels << "ch";
            if (need_resampling) std::cout << " (resampled)";
            std::cout << std::endl;
            conversion_message_shown = true;
        }
        
        return output_frames;
    }
    
private:
    /**
     * Convert input data to 32-bit float format
     */
    void convert_to_float(const uint8_t* src, std::vector<float>& dst, UINT32 frames,
                         UINT32 channels, UINT32 bits) {
        // 边界检查
        if (!src || frames == 0 || channels == 0) {
            dst.clear();
            return;
        }

        dst.resize(frames * channels);
        const size_t total_samples = frames * channels;

        if (bits == 16) {
            const int16_t* src_16 = reinterpret_cast<const int16_t*>(src);
            for (size_t i = 0; i < total_samples && i < dst.size(); i++) {
                dst[i] = src_16[i] * (1.0f / 32768.0f);
            }
        } else if (bits == 24) {
            const uint8_t* src_24 = src;
            for (size_t i = 0; i < total_samples && i < dst.size(); i++) {
                int32_t sample = (src_24[i*3] << 8) | (src_24[i*3+1] << 16) | (src_24[i*3+2] << 24);
                dst[i] = sample * (1.0f / 2147483648.0f);
            }
        } else if (bits == 32) {
            if (src && reinterpret_cast<const uint32_t*>(src)[0] & 0x80000000) {
                // Assume float format
                const float* src_float = reinterpret_cast<const float*>(src);
                size_t copy_count = (total_samples < dst.size()) ? total_samples : dst.size();
                std::copy(src_float, src_float + copy_count, dst.begin());
            } else {
                // Assume integer format
                const int32_t* src_32 = reinterpret_cast<const int32_t*>(src);
                for (size_t i = 0; i < total_samples && i < dst.size(); i++) {
                    dst[i] = src_32[i] * (1.0f / 2147483648.0f);
                }
            }
        } else {
            std::fill(dst.begin(), dst.end(), 0.0f);
        }
    }
    
    /**
     * Convert channels (mono/stereo conversion)
     */
    void convert_channels(const float* src, std::vector<float>& dst, UINT32 frames,
                         UINT32 src_channels, UINT32 dst_channels) {
        if (src_channels == dst_channels) {
            dst.assign(src, src + frames * src_channels);
        } else if (src_channels == 1 && dst_channels == 2) {
            // Mono to stereo
            dst.resize(frames * 2);
            for (UINT32 i = 0; i < frames; i++) {
                dst[i * 2] = src[i];        // Left
                dst[i * 2 + 1] = src[i];    // Right
            }
        } else if (src_channels == 2 && dst_channels == 1) {
            // Stereo to mono - average channels
            dst.resize(frames);
            for (UINT32 i = 0; i < frames; i++) {
                dst[i] = (src[i * 2] + src[i * 2 + 1]) / 2.0f;
            }
        } else {
            // Unsupported - copy first channels
            dst.resize(frames * dst_channels);
            for (UINT32 i = 0; i < frames; i++) {
                for (UINT32 ch = 0; ch < dst_channels; ch++) {
                    dst[i * dst_channels + ch] = (ch < src_channels) ? src[i * src_channels + ch] : 0.0f;
                }
            }
        }
    }
    
    /**
     * Convert from float to output format
     */
    UINT32 convert_from_float(const float* src, uint8_t* dst, UINT32 frames,
                             UINT32 channels, UINT32 bits, WORD format_tag) {
        if (format_tag == WAVE_FORMAT_IEEE_FLOAT && bits == 32) {
            float* dst_float = reinterpret_cast<float*>(dst);
            std::copy(src, src + frames * channels, dst_float);
            return frames;
        } else if (bits == 16) {
            int16_t* dst_16 = reinterpret_cast<int16_t*>(dst);
            for (UINT32 i = 0; i < frames * channels; i++) {
                float sample = (std::max)(-1.0f, (std::min)(1.0f, src[i]));
                dst_16[i] = static_cast<int16_t>(sample * 32767.0f);
            }
            return frames;
        } else {
            return 0; // Unsupported output format
        }
    }
};

// Forward declarations
bool play_wav_file(const std::string& filename);
#ifdef _WIN32
bool play_wav_via_wasapi(const uint8_t* wav_data, size_t data_size, const WavHeader& wav_header);
#endif

// Simple sine wave for testing
void generate_test_tone(float* buffer, int frames, float sample_rate, float frequency) {
    static float phase = 0;
    const float pi = 3.14159265358979323846f;
    float phase_increment = 2.0f * pi * frequency / sample_rate;
    
    for (int i = 0; i < frames; ++i) {
        float sample = 0.3f * std::sin(phase);
        buffer[i * 2] = sample;      // Left
        buffer[i * 2 + 1] = sample;  // Right
        phase += phase_increment;
        if (phase > 2.0f * pi) phase -= 2.0f * pi;
    }
}

#ifdef _WIN32
// Enhanced WASAPI playback with resampling support
bool play_wav_via_wasapi(const uint8_t* wav_data, size_t data_size, const WavHeader& wav_header) {
    HRESULT hr;
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    WAVEFORMATEX* deviceFormat = nullptr;
    
    std::cout << "Initializing WASAPI audio output..." << std::endl;
    
    // Initialize COM
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to initialize COM" << std::endl;
        return false;
    }
    
    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                         __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to create device enumerator" << std::endl;
        CoUninitialize();
        return false;
    }
    
    // Get default audio device
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to get default audio endpoint" << std::endl;
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }
    
    // Activate audio client
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to activate audio client" << std::endl;
        device->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }
    
    // Get device mix format
    hr = audioClient->GetMixFormat(&deviceFormat);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to get mix format" << std::endl;
        audioClient->Release();
        device->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }
    
    std::cout << "Device format: " << deviceFormat->nSamplesPerSec << "Hz, "
              << deviceFormat->nChannels << "ch, "
              << deviceFormat->wBitsPerSample << "-bit";
    if (deviceFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        std::cout << " (float)";
    }
    std::cout << std::endl;
    
    std::cout << "File format: " << wav_header.sample_rate << "Hz, "
              << wav_header.num_channels << "ch, "
              << wav_header.bits_per_sample << "-bit PCM" << std::endl;
    
    // Initialize audio client
    REFERENCE_TIME bufferDuration = 10000000; // 1 second
    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, bufferDuration, 0, deviceFormat, nullptr);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to initialize audio client" << std::endl;
        CoTaskMemFree(deviceFormat);
        audioClient->Release();
        device->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }
    
    // Get buffer size
    UINT32 bufferFrameCount;
    hr = audioClient->GetBufferSize(&bufferFrameCount);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to get buffer size" << std::endl;
        CoTaskMemFree(deviceFormat);
        audioClient->Release();
        device->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }
    
    // Get render client
    hr = audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient);
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to get render client" << std::endl;
        CoTaskMemFree(deviceFormat);
        audioClient->Release();
        device->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }

    std::cout << "WASAPI initialized successfully!" << std::endl;

    // Create format converter
    AudioFormatConverter converter;
    
    // Calculate frames to process
    UINT32 bytesPerFrame = wav_header.num_channels * (wav_header.bits_per_sample / 8);
    UINT32 totalFrames = data_size / bytesPerFrame;
    UINT32 currentFrame = 0;
    
    std::cout << "Total frames: " << totalFrames << std::endl;
    
    // Start playback
    hr = audioClient->Start();
    if (!check_hresult_ok(hr)) {
        std::cerr << "Failed to start audio client" << std::endl;
        renderClient->Release();
        CoTaskMemFree(deviceFormat);
        audioClient->Release();
        device->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }
    
    std::cout << "Starting playback..." << std::endl;
    
    // Playback loop
    while (currentFrame < totalFrames) {
        UINT32 framesAvailable;
        hr = audioClient->GetCurrentPadding(&framesAvailable);
        if (!check_hresult_ok(hr)) break;
        
        framesAvailable = bufferFrameCount - framesAvailable;
        if (framesAvailable == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Get buffer from WASAPI
        BYTE* wasapiBuffer;
        hr = renderClient->GetBuffer(framesAvailable, &wasapiBuffer);
        if (!check_hresult_ok(hr)) break;

        // Store data size locally for easier access
        const size_t wav_data_size = data_size;

        // Ensure we don't exceed the audio data
        if (currentFrame >= totalFrames) {
            // Audio playback complete
            break;
        }

        UINT32 remainingFrames = totalFrames - currentFrame;
        UINT32 framesToProcess = (std::min<UINT32>)(framesAvailable, remainingFrames);

        // Validate we have enough data
        if ((currentFrame + framesToProcess) * bytesPerFrame > wav_data_size) {
            framesToProcess = (wav_data_size / bytesPerFrame) - currentFrame;
            if (framesToProcess > remainingFrames) {
                framesToProcess = remainingFrames;
            }
        }

        // Ensure framesToProcess is valid
        if (framesToProcess == 0) {
            // Fill remaining buffer with silence
            memset(wasapiBuffer, 0, framesAvailable * deviceFormat->nBlockAlign);
            renderClient->ReleaseBuffer(framesAvailable, 0);
            break;
        }

        // Convert and process audio
        const uint8_t* srcData = wav_data + (currentFrame * bytesPerFrame);

        // Skip complex converter and use simple conversion directly

        // Simple conversion: treat as 16-bit PCM
        const int16_t* src16 = reinterpret_cast<const int16_t*>(srcData);
        float* dst32 = reinterpret_cast<float*>(wasapiBuffer);
        UINT32 src_channels = wav_header.num_channels;
        UINT32 dst_channels = deviceFormat->nChannels;

        for (UINT32 i = 0; i < framesToProcess; i++) {
            if (src_channels == 1 && dst_channels == 2) {
                // Mono to stereo
                float sample = static_cast<float>(src16[i]) / 32768.0f;
                dst32[i * 2] = sample;
                dst32[i * 2 + 1] = sample;
            } else if (src_channels == 2 && dst_channels == 2) {
                // Stereo to stereo
                dst32[i * 2] = static_cast<float>(src16[i * 2]) / 32768.0f;
                dst32[i * 2 + 1] = static_cast<float>(src16[i * 2 + 1]) / 32768.0f;
            } else {
                // Default: copy first channel
                UINT32 src_idx = i * (src_channels > 0 ? src_channels : 1);
                float sample = static_cast<float>(src16[src_idx]) / 32768.0f;
                for (UINT32 ch = 0; ch < dst_channels; ch++) {
                    dst32[i * dst_channels + ch] = sample;
                }
            }
        }

        // Release buffer
        hr = renderClient->ReleaseBuffer(framesToProcess, 0);
        if (!check_hresult_ok(hr)) break;

        // Remove debug output

        if (framesToProcess == 0) {
            std::cerr << "Warning: framesToProcess = 0, breaking loop" << std::endl;
            break;
        }

        currentFrame += framesToProcess;

        // Show progress more frequently for long files
        if (totalFrames > 10000000) {
            // For long files (>3 min), update every 24000 frames (0.5 sec)
            if (currentFrame % 24000 == 0) {
                int progress = (currentFrame * 100) / totalFrames;
                std::cout << "\rProgress: " << progress << "% ("
                          << (currentFrame / deviceFormat->nSamplesPerSec) << "s/"
                          << (totalFrames / deviceFormat->nSamplesPerSec) << "s)" << std::flush;
            }
        } else {
            // For normal files, update every 12000 frames (0.25 sec)
            if (currentFrame % 12000 == 0) {
                int progress = (currentFrame * 100) / totalFrames;
                std::cout << "\rProgress: " << progress << "%" << std::flush;
            }
        }
    }
    
    std::cout << "\rProgress: 100%" << std::endl;
    
    // Wait for playback to finish
    std::cout << "\nPlayback complete!" << std::endl;
    
    // Cleanup
    audioClient->Stop();
    renderClient->Release();
    CoTaskMemFree(deviceFormat);
    audioClient->Release();
    device->Release();
    deviceEnumerator->Release();
    CoUninitialize();
    
    return true;
}
#endif // _WIN32

bool play_wav_file(const std::string& filename) {
    std::cout << "Loading WAV file: " << filename << std::endl;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    // Read RIFF header
    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) {
        std::cerr << "Not a valid RIFF file" << std::endl;
        return false;
    }
    
    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);
    
    // Read WAVE header
    char wave[4];
    file.read(wave, 4);
    if (std::strncmp(wave, "WAVE", 4) != 0) {
        std::cerr << "Not a valid WAVE file" << std::endl;
        return false;
    }
    
    // Find fmt chunk
    WavHeader wav_header = {};
    bool found_fmt = false;
    bool found_data = false;
    std::vector<uint8_t> audio_data;
    
    while (file.good() && !found_data) {
        char chunk_id[4];
        uint32_t chunk_size;
        
        file.read(chunk_id, 4);
        file.read(reinterpret_cast<char*>(&chunk_size), 4);
        
        if (std::strncmp(chunk_id, "fmt ", 4) == 0) {
            // Read format chunk
            uint16_t audio_format;
            file.read(reinterpret_cast<char*>(&audio_format), 2);
            file.read(reinterpret_cast<char*>(&wav_header.num_channels), 2);
            file.read(reinterpret_cast<char*>(&wav_header.sample_rate), 4);
            file.read(reinterpret_cast<char*>(&wav_header.byte_rate), 4);
            file.read(reinterpret_cast<char*>(&wav_header.block_align), 2);
            file.read(reinterpret_cast<char*>(&wav_header.bits_per_sample), 2);
            
            wav_header.audio_format = audio_format;
            found_fmt = true;
            
            // Skip any extra format bytes
            if (chunk_size > 16) {
                file.seekg(chunk_size - 16, std::ios::cur);
            }
        } else if (std::strncmp(chunk_id, "data", 4) == 0) {
            // Read audio data
            audio_data.resize(chunk_size);
            file.read(reinterpret_cast<char*>(audio_data.data()), chunk_size);
            wav_header.data_size = chunk_size;
            found_data = true;
        } else {
            // Skip unknown chunks
            file.seekg(chunk_size, std::ios::cur);
        }
    }
    
    if (!found_fmt || !found_data) {
        std::cerr << "Missing required chunks in WAV file" << std::endl;
        return false;
    }
    
    std::cout << "WAV file loaded successfully:" << std::endl;
    std::cout << "  Sample Rate: " << wav_header.sample_rate << " Hz" << std::endl;
    std::cout << "  Channels: " << wav_header.num_channels << std::endl;
    std::cout << "  Bits: " << wav_header.bits_per_sample << "-bit" << std::endl;
    std::cout << "  Duration: " << (wav_header.data_size / wav_header.byte_rate) << " seconds" << std::endl;
    
#ifdef _WIN32
    return play_wav_via_wasapi(audio_data.data(), audio_data.size(), wav_header);
#else
    std::cerr << "Audio playback not supported on this platform" << std::endl;
    return false;
#endif
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   Music Player with Resampling v2.0" << std::endl;
    std::cout << "   Enhanced Audio Format Support" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Supported formats:" << std::endl;
        std::cerr << "  - Sample rates: Any (with automatic resampling)" << std::endl;
        std::cerr << "  - Bit depths: 16, 24, 32-bit" << std::endl;
        std::cerr << "  - Channels: 1-8 (with automatic channel conversion)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "The player will automatically convert to your audio device's format." << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    
    if (!play_wav_file(filename)) {
        std::cerr << "Failed to play file: " << filename << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "Playback completed successfully!" << std::endl;
    return 0;
}
#endif // _WIN32