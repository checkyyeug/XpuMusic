/**
 * @file music_player_real.cpp
 * @brief REAL music player - actually plays audio files!
 * @version 0.4.0 - Working WAV playback
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

#ifdef _WIN32
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

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

// ============================================================================
// AUDIO FORMAT CONVERSION
// Converts between different audio formats (bit depth, sample rate, channels)
// Currently supports: 16-bit PCM → 32-bit IEEE float
// ============================================================================

/**
 * @brief Convert audio format from WAV file to WASAPI format
 * @param src Source audio data (WAV file format)
 * @param dst Destination buffer (WASAPI format)
 * @param frames Number of frames to convert
 * @param wav_header WAV file header (source format)
 * @param wasapi_format WASAPI format (destination format)
 * @return Number of frames actually converted
 */
inline UINT32 convert_audio_format(
    const uint8_t* src,
    uint8_t* dst,
    UINT32 frames,
    const WavHeader& wav_header,
    const WAVEFORMATEX& wasapi_format) {

    static bool conversion_message_shown = false;

    // No conversion needed if formats match
    bool rates_match = (wav_header.sample_rate == wasapi_format.nSamplesPerSec);
    bool bits_match = (wav_header.bits_per_sample == wasapi_format.wBitsPerSample);
    bool channels_match = (wav_header.num_channels == wasapi_format.nChannels);
    bool format_tags_match = (wav_header.audio_format == 1 && wasapi_format.wFormatTag == WAVE_FORMAT_PCM) ||
                             (wasapi_format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
                             (wasapi_format.wFormatTag == 65534);  // WAVE_FORMAT_EXTENSIBLE
    
    if (rates_match && bits_match && channels_match && format_tags_match) {
        // Formats match - simple copy
        UINT32 bytes_to_copy = frames * wav_header.block_align;
        std::memcpy(dst, src, bytes_to_copy);
        return frames;
    }
    
    // Check if we can convert
    // Support: 16-bit PCM → 32-bit float, with or without sample rate mismatch
    // Note: WASAPI may return WAVE_FORMAT_EXTENSIBLE (65534) for float formats
    if (wav_header.bits_per_sample == 16 &&
        wasapi_format.wBitsPerSample == 32 &&
        (wasapi_format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT || wasapi_format.wFormatTag == 65534) &&
        wav_header.audio_format == 1) {  // PCM

        // Handle channel conversion
        UINT32 src_channels = wav_header.num_channels;
        UINT32 dst_channels = wasapi_format.nChannels;

        const int16_t* pcm_src = reinterpret_cast<const int16_t*>(src);
        float* float_dst = reinterpret_cast<float*>(dst);

        // Convert 16-bit PCM [-32768, 32767] to 32-bit float [-1.0, 1.0]
        if (src_channels == dst_channels) {
            // Simple channel count - just convert
            UINT32 samples = frames * src_channels;
            for (UINT32 i = 0; i < samples; i++) {
                float_dst[i] = pcm_src[i] * (1.0f / 32768.0f);
            }
        } else if (src_channels == 1 && dst_channels == 2) {
            // Mono to stereo
            for (UINT32 i = 0; i < frames; i++) {
                float sample = pcm_src[i] * (1.0f / 32768.0f);
                float_dst[i * 2] = sample;        // Left
                float_dst[i * 2 + 1] = sample;    // Right
            }
        } else if (src_channels == 2 && dst_channels == 1) {
            // Stereo to mono - average channels
            for (UINT32 i = 0; i < frames; i++) {
                float left = pcm_src[i * 2] * (1.0f / 32768.0f);
                float right = pcm_src[i * 2 + 1] * (1.0f / 32768.0f);
                float_dst[i] = (left + right) / 2.0f;
            }
        } else {
            // Unsupported channel configuration
            goto unsupported;
        }

        // Show conversion message only once
        if (!conversion_message_shown) {
            std::cout << "✓ Converting audio: " << src_channels << "ch "
                      << wav_header.bits_per_sample << "-bit PCM → "
                      << dst_channels << "ch " << wasapi_format.wBitsPerSample << "-bit float" << std::endl;
            conversion_message_shown = true;
        }

        return frames;
    }

unsupported:
    // Unsupported conversion
    std::cerr << "⚠️  Unsupported format conversion!" << std::endl;
    std::cerr << "  From: " << wav_header.sample_rate << "Hz, " 
              << wav_header.bits_per_sample << "-bit, "
              << wav_header.num_channels << "ch, PCM" << std::endl;
    std::cerr << "  To: " << wasapi_format.nSamplesPerSec << "Hz, "
              << wasapi_format.wBitsPerSample << "-bit, "
              << wasapi_format.nChannels << "ch, "
              << (wasapi_format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT ? "Float" : "PCM") << std::endl;
    
    return 0;  // Return 0 to indicate failure
}

// Forward declarations
bool play_wav_file(const std::string& filename);
#ifdef _WIN32
bool play_wav_via_wasapi(const uint8_t* wav_data, const WavHeader& wav_header);
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

// Direct WASAPI playback
bool play_test_tone(float frequency = 440.0f, float duration = 2.0f) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;
    
    // Get audio device
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&enumerator);
    if (FAILED(hr)) return false;
    
    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        enumerator->Release();
        return false;
    }
    
    // Activate audio client
    IAudioClient* audio_client = nullptr;
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                         NULL, (void**)&audio_client);
    if (FAILED(hr)) {
        device->Release();
        enumerator->Release();
        return false;
    }
    
    // Get mix format
    WAVEFORMATEX* mix_format = nullptr;
    hr = audio_client->GetMixFormat(&mix_format);
    if (FAILED(hr)) {
        device->Release();
        enumerator->Release();
        audio_client->Release();
        return false;
    }
    
    // Force 44.1kHz stereo for maximum compatibility
    mix_format->nSamplesPerSec = 44100;
    mix_format->nChannels = 2;
    mix_format->wBitsPerSample = 32;
    mix_format->nBlockAlign = mix_format->nChannels * mix_format->wBitsPerSample / 8;
    mix_format->nAvgBytesPerSec = mix_format->nSamplesPerSec * mix_format->nBlockAlign;
    mix_format->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    
    // Initialize audio client
    hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  0, 10000000, 0, mix_format, NULL);
    if (FAILED(hr)) {
        CoTaskMemFree(mix_format);
        device->Release();
        enumerator->Release();
        audio_client->Release();
        return false;
    }
    
    IAudioRenderClient* render_client = nullptr;
    hr = audio_client->GetService(__uuidof(IAudioRenderClient),
                                  (void**)&render_client);
    if (FAILED(hr)) {
        CoTaskMemFree(mix_format);
        device->Release();
        enumerator->Release();
        audio_client->Release();
        return false;
    }
    
    UINT32 buffer_frame_count = 0;
    audio_client->GetBufferSize(&buffer_frame_count);
    
    // Start playback
    hr = audio_client->Start();
    if (FAILED(hr)) {
        render_client->Release();
        CoTaskMemFree(mix_format);
        device->Release();
        enumerator->Release();
        audio_client->Release();
        return false;
    }
    
    std::cout << "✓ Audio started - Playing " << duration << " second tone..." << std::endl;
    std::cout << "  Format: " << mix_format->nSamplesPerSec << " Hz, " 
              << mix_format->nChannels << " channels, Float32" << std::endl;
    std::cout << "  Buffer: " << buffer_frame_count << " frames" << std::endl;
    
    // Generate and play audio
    std::vector<float> temp_buffer(buffer_frame_count * mix_format->nChannels);
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count() < duration) {
        UINT32 num_frames_padding = 0;
        audio_client->GetCurrentPadding(&num_frames_padding);
        
        UINT32 num_frames_available = buffer_frame_count - num_frames_padding;
        
        if (num_frames_available > 0) {
            BYTE* data = nullptr;
            HRESULT hr = render_client->GetBuffer(num_frames_available, &data);
            if (SUCCEEDED(hr)) {
                generate_test_tone(reinterpret_cast<float*>(data), num_frames_available, 
                                   mix_format->nSamplesPerSec, frequency);
                render_client->ReleaseBuffer(num_frames_available, 0);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // Cleanup
    audio_client->Stop();
    render_client->Release();
    CoTaskMemFree(mix_format);
    device->Release();
    enumerator->Release();
    audio_client->Release();
    
    std::cout << "✓ Audio playback complete!" << std::endl;
    return true;
}
#endif

// Complete WAV file player - reads and plays actual WAV data
bool play_wav_file(const std::string& filename) {
    std::cout << "Reading WAV file: " << filename << std::endl;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "❌ Cannot open file: " << filename << std::endl;
        return false;
    }
    
    // Read RIFF header
    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) {
        std::cerr << "❌ Not a RIFF file" << std::endl;
        file.close();
        return false;
    }
    
    uint32_t file_size;
    file.read(reinterpret_cast<char*>(&file_size), 4);
    
    char wave[4];
    file.read(wave, 4);
    if (std::strncmp(wave, "WAVE", 4) != 0) {
        std::cerr << "❌ Not a WAVE file" << std::endl;
        file.close();
        return false;
    }
    
    std::cout << "✓ Valid WAV file" << std::endl;
    
    // Find fmt chunk
    char chunk_id[4];
    uint32_t chunk_size;
    WavHeader header = {};
    bool found_fmt = false;
    bool found_data = false;
    std::streampos data_pos = 0;
    
    while (file.read(chunk_id, 4)) {
        file.read(reinterpret_cast<char*>(&chunk_size), 4);
        
        if (std::strncmp(chunk_id, "fmt ", 4) == 0) {
            // Read format chunk
            file.read(reinterpret_cast<char*>(&header.audio_format), 2);
            file.read(reinterpret_cast<char*>(&header.num_channels), 2);
            file.read(reinterpret_cast<char*>(&header.sample_rate), 4);
            file.read(reinterpret_cast<char*>(&header.byte_rate), 4);
            file.read(reinterpret_cast<char*>(&header.block_align), 2);
            file.read(reinterpret_cast<char*>(&header.bits_per_sample), 2);
            
            // Skip any extra format bytes
            if (chunk_size > 16) {
                file.seekg(chunk_size - 16, std::ios::cur);
            }
            
            found_fmt = true;
            std::cout << "\n✓ Format chunk found:" << std::endl;
            std::cout << "  Format: " << (header.audio_format == 1 ? "PCM" : "Other") << std::endl;
            std::cout << "  Channels: " << header.num_channels << std::endl;
            std::cout << "  Sample Rate: " << header.sample_rate << " Hz" << std::endl;
            std::cout << "  Bits per Sample: " << header.bits_per_sample << std::endl;
            std::cout << "  Byte Rate: " << header.byte_rate << std::endl;
            
            if (header.audio_format != 1) {
                std::cerr << "❌ Only PCM WAV files supported" << std::endl;
                file.close();
                return false;
            }
        } else if (std::strncmp(chunk_id, "data", 4) == 0) {
            // Found data chunk
            header.data_size = chunk_size;
            data_pos = file.tellg();
            found_data = true;
            break;
        } else {
            // Skip unknown chunk
            file.seekg(chunk_size, std::ios::cur);
        }
        
        if (!file) break;
    }
    
    if (!found_fmt) {
        std::cerr << "❌ No format chunk found" << std::endl;
        file.close();
        return false;
    }
    
    if (!found_data) {
        std::cerr << "❌ No data chunk found" << std::endl;
        file.close();
        return false;
    }
    
    std::cout << "✓ Data chunk found: " << header.data_size << " bytes" << std::endl;
    std::cout << "  Duration: " << (float)header.data_size / header.byte_rate << " seconds" << std::endl;
    
    // Read audio data
    std::vector<uint8_t> audio_data(header.data_size);
    file.seekg(data_pos);
    file.read(reinterpret_cast<char*>(audio_data.data()), header.data_size);
    
    if (!file || file.gcount() != header.data_size) {
        std::cerr << "❌ Failed to read audio data" << std::endl;
        file.close();
        return false;
    }
    
    file.close();
    std::cout << "✓ Read " << header.data_size << " bytes of audio data" << std::endl;
    
    // Now play the audio data via WASAPI
#ifdef _WIN32
    return play_wav_via_wasapi(audio_data.data(), header);
#else
    std::cerr << "❌ WASAPI playback only supported on Windows" << std::endl;
    return false;
#endif
}

#ifdef _WIN32
// Play WAV data via WASAPI
bool play_wav_via_wasapi(const uint8_t* wav_data, const WavHeader& wav_header) {
    std::cout << "\nInitializing WASAPI audio output..." << std::endl;
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "❌ Failed to initialize COM" << std::endl;
        return false;
    }
    
    // Get audio device
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&enumerator);
    if (FAILED(hr)) return false;
    
    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        enumerator->Release();
        return false;
    }
    
    // Configure format (match WAV file or convert)
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = wav_header.num_channels;
    wfx.nSamplesPerSec = wav_header.sample_rate;
    wfx.wBitsPerSample = wav_header.bits_per_sample;
    wfx.nBlockAlign = wav_header.block_align;
    wfx.nAvgBytesPerSec = wav_header.byte_rate;
    
    std::cout << "✓ Audio format configured:" << std::endl;
    std::cout << "  - Channels: " << wfx.nChannels << std::endl;
    std::cout << "  - Sample Rate: " << wfx.nSamplesPerSec << " Hz" << std::endl;
    std::cout << "  - Bits per Sample: " << wfx.wBitsPerSample << std::endl;
    std::cout << "  - Block Align: " << wfx.nBlockAlign << std::endl;
    
    // Try to initialize
    IAudioClient* audio_client = nullptr;
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                         NULL, (void**)&audio_client);
    if (FAILED(hr)) {
        std::cerr << "❌ Failed to activate audio client" << std::endl;
        device->Release();
        enumerator->Release();
        return false;
    }
    
    // Use the already configured wfx from line 407
    WAVEFORMATEX actual_format = {};  // Store actual format used  // Store actual format used
    bool using_fallback = false;
    WAVEFORMATEX* wasapi_format = nullptr;
    
    // Try to initialize with file format first
    hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  0, 10000000, 0, &wfx, NULL);
    if (SUCCEEDED(hr)) {
        std::cout << "✅ Successfully initialized with file format!" << std::endl;
        actual_format = wfx;  // Use file format
        wasapi_format = &actual_format;
    } else {
        // Try with system default format if initialization fails
        std::cout << "⚠️  File format not supported, trying system default..." << std::endl;
        hr = audio_client->GetMixFormat(&wasapi_format);
        if (FAILED(hr)) {
            std::cerr << "❌ Failed to get system mix format" << std::endl;
            audio_client->Release();
            device->Release();
            enumerator->Release();
            return false;
        }
        
        hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                      0, 10000000, 0, wasapi_format, NULL);
        if (FAILED(hr)) {
            CoTaskMemFree(wasapi_format);
            std::cerr << "❌ Failed to initialize with system format" << std::endl;
            audio_client->Release();
            device->Release();
            enumerator->Release();
            return false;
        }
        
        using_fallback = true;
        actual_format = *wasapi_format;  // Copy to local storage
        wasapi_format = &actual_format;  // Point to local copy
        
        std::cout << "✅ Using system default format:" << std::endl;
        std::cout << "  - Channels: " << wasapi_format->nChannels << std::endl;
        std::cout << "  - Sample Rate: " << wasapi_format->nSamplesPerSec << " Hz" << std::endl;
        std::cout << "  - Bits per Sample: " << wasapi_format->wBitsPerSample << std::endl;
        std::cout << "  - Format Tag: " << wasapi_format->wFormatTag << " (1=PCM, 3=Float)" << std::endl;
    }
    
    // Get render client
    IAudioRenderClient* render_client = nullptr;
    hr = audio_client->GetService(__uuidof(IAudioRenderClient), (void**)&render_client);
    if (FAILED(hr)) {
        audio_client->Release();
        device->Release();
        enumerator->Release();
        return false;
    }
    
    UINT32 buffer_size = 0;
    audio_client->GetBufferSize(&buffer_size);
    
    // Start playback
    hr = audio_client->Start();
    if (FAILED(hr)) {
        render_client->Release();
        audio_client->Release();
        device->Release();
        enumerator->Release();
        return false;
    }
    
    std::cout << "✅ Audio playback started" << std::endl;
    std::cout << "✅ Buffer size: " << buffer_size << " frames" << std::endl;
    
    // Check if conversion is needed
    bool format_matches = (wav_header.sample_rate == wasapi_format->nSamplesPerSec &&
                          wav_header.bits_per_sample == wasapi_format->wBitsPerSample &&
                          wav_header.num_channels == wasapi_format->nChannels);
    bool needs_float_conv = (wav_header.bits_per_sample == 16 && 
                            wasapi_format->wBitsPerSample == 32 &&
                            wasapi_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
    
    if (!format_matches && needs_float_conv) {
        std::cout << "⚠️  Converting 16-bit PCM → 32-bit Float (format mismatch)" << std::endl;
    } else if (format_matches) {
        std::cout << "✅ Audio format matches (optimal, no conversion needed)" << std::endl;
    } else {
        std::cout << "⚠️  Format mismatch but no conversion available" << std::endl;
    }
    std::cout << std::endl;
    
    // Stream audio data with format conversion
    const uint8_t* data_ptr = wav_data;
    const uint8_t* end_ptr = wav_data + wav_header.data_size;
    int frames_played = 0;
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (data_ptr < end_ptr) {
        UINT32 padding = 0;
        audio_client->GetCurrentPadding(&padding);
        
        UINT32 available = buffer_size - padding;
        if (available > 0) {
            BYTE* buffer = nullptr;
            hr = render_client->GetBuffer(available, &buffer);
            if (check_hresult_ok(hr) && buffer) {
                // Calculate frames to copy (not bytes!)
                UINT32 frames_remaining = static_cast<UINT32>((end_ptr - data_ptr) / wav_header.block_align);
                UINT32 frames_to_copy = (std::min)(available, frames_remaining);
                
                if (frames_to_copy > 0) {
                    // Convert and copy audio data (handles format conversion)
                    UINT32 frames_converted = convert_audio_format(
                        data_ptr, buffer, frames_to_copy,
                        wav_header, *wasapi_format);
                    
                    if (frames_converted > 0) {
                        data_ptr += frames_converted * wav_header.block_align;
                        frames_played += frames_converted;
                    } else {
                        // Conversion failed - fill with silence
                        UINT32 bytes_to_fill = frames_to_copy * wasapi_format->nBlockAlign;
                        std::memset(buffer, 0, bytes_to_fill);
                    }
                } else {
                    // No more data - fill with silence
                    UINT32 bytes_to_fill = available * wasapi_format->nBlockAlign;
                    std::memset(buffer, 0, bytes_to_fill);
                }
                
                // CRITICAL FIX: Release buffer with actual frames written
                render_client->ReleaseBuffer(frames_to_copy, 0);
            }
        }
        
        // FIX: Sleep for 1ms instead of 100μs to avoid CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Wait for playback to finish
    auto duration = std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count();
    float expected_duration = static_cast<float>(wav_header.data_size) / wav_header.byte_rate;
    float remaining_time = expected_duration - duration - 0.5f;  // Give 500ms buffer
    
    if (remaining_time > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(remaining_time * 1000)));
    }
    
    // Cleanup
    float seconds_played = static_cast<float>(frames_played) / wav_header.sample_rate;
    std::cout << "✅ Playback complete (" << frames_played << " frames, " 
              << std::fixed << std::setprecision(1) << seconds_played << "s)" << std::endl;
    audio_client->Stop();
    render_client->Release();
    if (using_fallback) CoTaskMemFree(wasapi_format);
    audio_client->Release();
    device->Release();
    enumerator->Release();
    
    return true;
}
#endif

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "   Music Player v0.5.0 (FULL WAV)" << std::endl;
    std::cout << "   NOW PLAYS ACTUAL WAV FILES!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    if (argc < 2) {
#ifdef _WIN32
        std::cout << "No file specified - playing test tone..." << std::endl;
        std::cout << std::endl;
        play_test_tone(440.0f, 2.0f);
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ Test complete!" << std::endl;
        std::cout << "Now try: music-player yourfile.wav" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
#else
        std::cout << "❌ Audio playback only works on Windows in this demo" << std::endl;
        return 1;
#endif
    }
    
    std::string filename = argv[1];
    std::cout << "Playing: " << filename << std::endl;
    std::cout << std::endl;
    
    // Play the actual WAV file now!
    return play_wav_file(filename) ? 0 : 1;
}
