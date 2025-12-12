/**
 * @file optimized_format_converter.cpp
 * @brief Implementation of optimized format conversion and streaming processing
 * @date 2025-12-13
 */

#include "optimized_audio_processor.h"
#include "sample_rate_converter.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

namespace audio {

// Simple forward declaration for resampler interface
// Using ISampleRateConverter interface from sample_rate_converter.h

// OptimizedFormatConverter implementation
OptimizedFormatConverter::OptimizedFormatConverter()
    : resampler_(nullptr) {
    // Detect CPU features on first use
    SIMDOperations::detect_cpu_features();
}

OptimizedFormatConverter::~OptimizedFormatConverter() = default;

bool OptimizedFormatConverter::initialize(const Format& input, const Format& output) {
    input_format_ = input;
    output_format_ = output;

    // Create resampler if needed
    if (input.sample_rate != output.sample_rate) {
        resampler_ = std::make_unique<LinearSampleRateConverter>();
        if (!resampler_->initialize(input.sample_rate, output.sample_rate, input.channels)) {
            return false;
        }
    }

    // Allocate temporary buffers
    size_t temp_size = std::max({
        static_cast<size_t>(input.channels * input.sample_rate),  // 1 second
        static_cast<size_t>(output.channels * output.sample_rate) // 1 second
    });
    temp_buffer_.reserve(temp_size);

    if (input.channels != output.channels) {
        channel_buffer_.reserve(temp_size);
    }

    return true;
}

size_t OptimizedFormatConverter::convert_chunk(const void* input, size_t input_frames,
                                              void* output, size_t max_output_frames) {
    PROFILE_AUDIO("format_convert_chunk", input_frames);

    // Step 1: Convert to float if needed
    if (input_format_.bits_per_sample != 32 || !input_format_.is_float) {
        temp_buffer_.resize(input_frames * input_format_.channels);
        convert_to_float(input, temp_buffer_.data(), input_frames);
    }

    // Step 2: Resample if needed
    float* processing_buffer = (input_format_.bits_per_sample == 32 && input_format_.is_float)
                              ? static_cast<float*>(const_cast<void*>(input))
                              : temp_buffer_.data();
    size_t processing_frames = input_frames;

    if (input_format_.sample_rate != output_format_.sample_rate) {
        // Calculate output buffer size
        double ratio = static_cast<double>(output_format_.sample_rate) / input_format_.sample_rate;
        size_t max_frames = static_cast<size_t>(input_frames * ratio * 1.2 + 1);
        max_frames = std::min(max_frames, max_output_frames);

        aligned_vector<float> resampled_buffer;
        resampled_buffer.resize(max_frames * output_format_.channels);

        int output_frames = resampler_->convert(processing_buffer, input_frames,
                                              resampled_buffer.data(), max_frames);

        if (output_frames <= 0) {
            return 0;
        }

        // Swap buffers
        temp_buffer_ = std::move(resampled_buffer);
        processing_buffer = temp_buffer_.data();
        processing_frames = output_frames;
    }

    // Step 3: Convert channels if needed
    if (input_format_.channels != output_format_.channels) {
        channel_buffer_.resize(processing_frames * output_format_.channels);
        convert_channels(processing_buffer, channel_buffer_.data(), processing_frames);
        processing_buffer = channel_buffer_.data();
    }

    // Step 4: Convert from float if needed
    if (output_format_.bits_per_sample != 32 || !output_format_.is_float) {
        convert_from_float(processing_buffer, output, processing_frames);
        return processing_frames;
    } else {
        // Direct copy for float output
        std::memcpy(output, processing_buffer, processing_frames * output_format_.channels * sizeof(float));
        return processing_frames;
    }
}

size_t OptimizedFormatConverter::convert(const void* input, size_t input_frames,
                                        void* output, size_t max_output_frames) {
    PROFILE_AUDIO("format_convert", input_frames);
    return convert_chunk(input, input_frames, output, max_output_frames);
}

void OptimizedFormatConverter::reset() {
    if (resampler_) {
        resampler_->reset();
    }
}

void OptimizedFormatConverter::convert_to_float(const void* input, float* output, size_t frames) {
    PROFILE_AUDIO("convert_to_float", frames * input_format_.channels);

    auto& cpu = SIMDOperations::detect_cpu_features();

    if (input_format_.bits_per_sample == 16) {
        const int16_t* src = static_cast<const int16_t*>(input);
        if (cpu.has_avx) {
            SIMDOperations::convert_int16_to_float_avx(src, output, frames * input_format_.channels);
        } else {
            SIMDOperations::convert_int16_to_float_sse2(src, output, frames * input_format_.channels);
        }
    } else if (input_format_.bits_per_sample == 24) {
        const uint8_t* src = static_cast<const uint8_t*>(input);
        SIMDOperations::convert_int24_to_float_sse2(src, output, frames * input_format_.channels);
    } else if (input_format_.bits_per_sample == 32 && !input_format_.is_float) {
        const int32_t* src = static_cast<const int32_t*>(input);
        const float scale = 1.0f / 2147483648.0f;
        const __m128 scale_vec = _mm_set1_ps(scale);
        size_t samples = frames * input_format_.channels;
        size_t simd_samples = (samples / 4) * 4;

        for (size_t i = 0; i < simd_samples; i += 4) {
            __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
            __m128i zeros = _mm_setzero_si128();
            __m128i lo = _mm_unpacklo_epi32(input, zeros);
            __m128i hi = _mm_unpackhi_epi32(input, zeros);
            __m128 float_lo = _mm_cvtepi32_ps(lo);
            __m128 float_hi = _mm_cvtepi32_ps(hi);
            float_lo = _mm_mul_ps(float_lo, scale_vec);
            float_hi = _mm_mul_ps(float_hi, scale_vec);
            _mm_storeu_ps(output + i, float_lo);
            _mm_storeu_ps(output + i + 4, float_hi);
        }

        for (size_t i = simd_samples; i < samples; i++) {
            output[i] = src[i] * scale;
        }
    } else if (input_format_.bits_per_sample == 32 && input_format_.is_float) {
        // Already float, just copy
        std::memcpy(output, input, frames * input_format_.channels * sizeof(float));
    }
}

void OptimizedFormatConverter::convert_from_float(const float* input, void* output, size_t frames) {
    PROFILE_AUDIO("convert_from_float", frames * output_format_.channels);

    auto& cpu = SIMDOperations::detect_cpu_features();

    if (output_format_.bits_per_sample == 16) {
        int16_t* dst = static_cast<int16_t*>(output);
        if (cpu.has_avx) {
            SIMDOperations::convert_float_to_int16_avx(input, dst, frames * output_format_.channels);
        } else {
            SIMDOperations::convert_float_to_int16_sse2(input, dst, frames * output_format_.channels);
        }
    } else if (output_format_.bits_per_sample == 32 && output_format_.is_float) {
        // Already float, just copy
        std::memcpy(output, input, frames * output_format_.channels * sizeof(float));
    }
}

void OptimizedFormatConverter::convert_channels(const float* input, float* output, size_t frames) {
    PROFILE_AUDIO("convert_channels", frames * std::max(input_format_.channels, output_format_.channels));

    if (input_format_.channels == output_format_.channels) {
        std::copy(input, input + frames * input_format_.channels, output);
        return;
    }

    if (input_format_.channels == 1 && output_format_.channels == 2) {
        // Mono to stereo
        for (size_t i = 0; i < frames; i++) {
            float sample = input[i];
            output[i * 2] = sample;
            output[i * 2 + 1] = sample;
        }
    } else if (input_format_.channels == 2 && output_format_.channels == 1) {
        // Stereo to mono
        auto& cpu = SIMDOperations::detect_cpu_features();
        size_t samples = frames;
        size_t simd_samples = (samples / 4) * 4;

        if (cpu.has_avx) {
            for (size_t i = 0; i < simd_samples; i += 4) {
                __m256 left = _mm256_loadu_ps(input + i * 2);
                __m256 right = _mm256_loadu_ps(input + i * 2 + 4);
                __m256 mixed = _mm256_mul_ps(_mm256_add_ps(left, right), _mm256_set1_ps(0.5f));
                _mm256_storeu_ps(output + i, mixed);
            }
        } else {
            const __m128 half = _mm_set1_ps(0.5f);
            for (size_t i = 0; i < simd_samples; i += 4) {
                __m128 left = _mm_loadu_ps(input + i * 2);
                __m128 right = _mm_loadu_ps(input + i * 2 + 4);
                __m128 mixed = _mm_mul_ps(_mm_add_ps(left, right), half);
                _mm_storeu_ps(output + i, mixed);
            }
        }

        for (size_t i = simd_samples; i < samples; i++) {
            output[i] = (input[i * 2] + input[i * 2 + 1]) * 0.5f;
        }
    } else {
        // Unsupported conversion - copy available channels
        size_t copy_channels = std::min(input_format_.channels, output_format_.channels);
        for (size_t i = 0; i < frames; i++) {
            for (size_t ch = 0; ch < output_format_.channels; ch++) {
                output[i * output_format_.channels + ch] = (ch < copy_channels)
                    ? input[i * input_format_.channels + ch]
                    : 0.0f;
            }
        }
    }
}

// StreamingAudioProcessor implementation
StreamingAudioProcessor::StreamingAudioProcessor(const ProcessingSettings& settings)
    : settings_(settings) {
    buffer_pool_ = std::make_unique<AudioBufferPool>(
        settings_.buffer_count,
        settings_.chunk_size * 8 // Assume max 8 channels
    );

    if (settings_.enable_multithreading) {
        task_queue_ = std::make_unique<AudioTaskQueue>(settings_.thread_count);
    }

    converter_ = std::make_unique<OptimizedFormatConverter>();
}

StreamingAudioProcessor::~StreamingAudioProcessor() {
    stop_processing();
}

bool StreamingAudioProcessor::start_processing(const std::string& filename) {
    if (processing_) {
        stop_processing();
    }

    // For now, just set processing flag
    // In a real implementation, would open the audio file
    processing_ = true;

    // Start processing thread
    processing_thread_ = std::thread(&StreamingAudioProcessor::processing_loop, this);

    return true;
}

void StreamingAudioProcessor::stop_processing() {
    if (!processing_) {
        return;
    }

    processing_ = false;
    input_cv_.notify_all();
    output_cv_.notify_all();

    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }

    // Clear queues
    {
        std::lock_guard<std::mutex> lock(input_mutex_);
        while (!input_queue_.empty()) {
            input_queue_.pop();
        }
    }

    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        while (!output_queue_.empty()) {
            output_queue_.pop();
        }
    }
}

bool StreamingAudioProcessor::get_processed_chunk(aligned_vector<float>& chunk, size_t& frames) {
    std::unique_lock<std::mutex> lock(output_mutex_);
    output_cv_.wait(lock, [this] { return !output_queue_.empty() || !processing_; });

    if (output_queue_.empty() || !processing_) {
        return false;
    }

    chunk = std::move(output_queue_.front());
    output_queue_.pop();

    frames = chunk.size() / 8; // Assume 8 channels for now
    return true;
}

bool StreamingAudioProcessor::seek(size_t position) {
    // Implementation would seek in the audio file
    // For now, just return true
    return true;
}

OptimizedFormatConverter::Format StreamingAudioProcessor::get_format() const {
    // Return default format for now
    return {44100, 2, 16, false};
}

void StreamingAudioProcessor::processing_loop() {
    while (processing_) {
        if (load_chunk()) {
            std::lock_guard<std::mutex> lock(input_mutex_);
            if (!input_queue_.empty()) {
                auto chunk = std::move(input_queue_.front());
                input_queue_.pop();

                if (settings_.enable_multithreading) {
                    // Process in background thread
                    task_queue_->enqueue([this, chunk = std::move(chunk)]() mutable {
                        process_chunk(chunk);
                        {
                            std::lock_guard<std::mutex> lock(output_mutex_);
                            output_queue_.push(std::move(chunk));
                        }
                        output_cv_.notify_one();
                    });
                } else {
                    // Process in current thread
                    process_chunk(chunk);
                    {
                        std::lock_guard<std::mutex> lock(output_mutex_);
                        output_queue_.push(std::move(chunk));
                    }
                    output_cv_.notify_one();
                }
            }
        } else {
            // No more data to process
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

bool StreamingAudioProcessor::load_chunk() {
    // In a real implementation, would load from audio file
    // For now, generate test data
    auto buffer = buffer_pool_->acquire_buffer();
    if (!buffer) {
        return false;
    }

    aligned_vector<float> chunk;
    chunk.resize(settings_.chunk_size * 2); // Stereo

    // Generate sine wave
    static float phase = 0;
    const float pi = 3.14159265358979323846f;
    float phase_increment = 2.0f * pi * 440.0f / 44100.0f;

    for (size_t i = 0; i < settings_.chunk_size; i++) {
        float sample = 0.3f * std::sin(phase);
        chunk[i * 2] = sample;
        chunk[i * 2 + 1] = sample;
        phase += phase_increment;
        if (phase > 2.0f * pi) phase -= 2.0f * pi;
    }

    std::lock_guard<std::mutex> lock(input_mutex_);
    input_queue_.push(std::move(chunk));
    input_cv_.notify_one();

    return true;
}

void StreamingAudioProcessor::process_chunk(aligned_vector<float>& chunk) {
    PROFILE_AUDIO("process_chunk", chunk.size());

    // Apply volume (SIMD optimized)
    auto& cpu = SIMDOperations::detect_cpu_features();
    float volume = 0.8f;

    if (cpu.has_avx) {
        SIMDOperations::volume_avx(chunk.data(), chunk.size(), volume);
    } else if (cpu.has_sse2) {
        SIMDOperations::volume_sse2(chunk.data(), chunk.size(), volume);
    } else {
        for (size_t i = 0; i < chunk.size(); i++) {
            chunk[i] *= volume;
        }
    }
}

} // namespace audio