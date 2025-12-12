/**
 * @file audio_output_coreaudio.cpp
 * @brief CoreAudio audio output implementation for macOS
 * @date 2025-12-10
 */

#ifdef AUDIO_BACKEND_COREAUDIO

#include "audio_output.h"
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFormat.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdexcept>
#include <iostream>

namespace audio {

/**
 * @brief CoreAudio audio output implementation for macOS
 */
class AudioOutputCoreAudio final : public IAudioOutput {
private:
    AudioQueueRef audio_queue_;
    AudioQueueBufferRef buffers_[3];
    bool is_open_;
    bool is_playing_;
    int buffer_size_;
    int latency_;
    unsigned int sample_rate_;
    unsigned int channels_;
    int current_buffer_;

    // Audio queue callback
    static void audio_queue_output_callback(void* inUserData,
                                            AudioQueueRef inAQ,
                                            AudioQueueBufferRef inBuffer) {
        AudioOutputCoreAudio* This = static_cast<AudioOutputCoreAudio*>(inUserData);

        // For simplicity, just silence the buffer
        // In a real implementation, we would fill with actual audio data
        memset(inBuffer->mAudioData, 0, inBuffer->mAudioDataByteSize);

        // Re-enqueue the buffer
        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, nullptr);
    }

public:
    AudioOutputCoreAudio()
        : audio_queue_(nullptr)
        , is_open_(false)
        , is_playing_(false)
        , buffer_size_(0)
        , latency_(0)
        , sample_rate_(44100)
        , channels_(2)
        , current_buffer_(0) {
    }

    ~AudioOutputCoreAudio() override {
        close();
    }

    bool open(const AudioFormat& format) override {
        try {
            sample_rate_ = format.sample_rate;
            channels_ = format.channels;

            // Set up the audio format
            AudioStreamBasicDescription audio_format;
            audio_format.mSampleRate = sample_rate_;
            audio_format.mFormatID = kAudioFormatLinearPCM;
            audio_format.mFormatFlags = kAudioFormatFlagIsSignedInteger |
                                      kAudioFormatFlagIsPacked;
            audio_format.mBytesPerPacket = channels_ * sizeof(int16_t);
            audio_format.mFramesPerPacket = 1;
            audio_format.mBytesPerFrame = channels_ * sizeof(int16_t);
            audio_format.mChannelsPerFrame = channels_;
            audio_format.mBitsPerChannel = 16;

            // Create the audio queue
            OSStatus status = AudioQueueNewOutput(
                &audio_format,
                audio_queue_output_callback,
                this,
                CFRunLoopGetCurrent(),
                kCFRunLoopCommonModes,
                0,
                &audio_queue_
            );

            if (status != noErr) {
                std::cerr << "AudioQueueNewOutput failed: " << status << std::endl;
                return false;
            }

            // Set buffer size (about 10ms of audio)
            buffer_size_ = (sample_rate_ * channels_ * sizeof(int16_t)) / 100;
            if (buffer_size_ < 1024) buffer_size_ = 1024;

            // Allocate buffers
            for (int i = 0; i < 3; i++) {
                status = AudioQueueAllocateBuffer(
                    audio_queue_,
                    buffer_size_,
                    &buffers_[i]
                );

                if (status != noErr) {
                    std::cerr << "AudioQueueAllocateBuffer failed: " << status << std::endl;
                    close();
                    return false;
                }

                // Initialize buffer
                buffers_[i]->mAudioDataByteSize = buffer_size_;
                memset(buffers_[i]->mAudioData, 0, buffer_size_);
            }

            // Estimate latency (typical CoreAudio latency)
            latency_ = 20; // 20ms typical

            is_open_ = true;
            is_playing_ = false;
            current_buffer_ = 0;

            return true;

        } catch (const std::exception& e) {
            std::cerr << "Failed to open CoreAudio: " << e.what() << std::endl;
            close();
            return false;
        }
    }

    void close() override {
        if (audio_queue_) {
            if (is_playing_) {
                AudioQueueStop(audio_queue_, true);
            }
            AudioQueueDispose(audio_queue_, true);
            audio_queue_ = nullptr;
        }

        is_open_ = false;
        is_playing_ = false;
    }

    int write(const float* buffer, int frames) override {
        if (!is_open_) return 0;

        // Start playing if not already
        if (!is_playing_) {
            OSStatus status = AudioQueueStart(audio_queue_, nullptr);
            if (status != noErr) {
                std::cerr << "AudioQueueStart failed: " << status << std::endl;
                return 0;
            }
            is_playing_ = true;
        }

        // In a real implementation, we would write the float data to the buffer
        // For now, we just simulate the write
        int bytes_to_write = frames * channels_ * sizeof(int16_t);

        // Return success
        return frames;
    }

    int get_latency() const override {
        return latency_;
    }

    int get_buffer_size() const override {
        return buffer_size_;
    }

    bool is_ready() const override {
        return is_open_ && audio_queue_;
    }
};

// Factory function for CoreAudio backend
std::unique_ptr<IAudioOutput> create_coreaudio_audio_output() {
    return std::make_unique<AudioOutputCoreAudio>();
}

} // namespace audio

#else // AUDIO_BACKEND_COREAUDIO not defined

#include "audio_output.h"
#include <iostream>

namespace audio {

// Fallback to stub if CoreAudio not available
std::unique_ptr<IAudioOutput> create_coreaudio_audio_output() {
    std::cout << "CoreAudio backend not compiled, falling back to stub" << std::endl;
    return create_audio_output(); // This will create stub
}

#endif // AUDIO_BACKEND_COREAUDIO