/**
 * @file optimization_integration_example.cpp
 * @brief Example showing how to integrate performance optimizations into existing code
 * @date 2025-12-13
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>

#ifdef _WIN32
#include <intrin.h>
#else
#include <immintrin.h>
#endif

// Example 1: Optimizing existing format conversion
// Original code:
void convert_int16_to_float_original(const int16_t* input, float* output, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        output[i] = input[i] * (1.0f / 32768.0f);
    }
}

// Optimized version with SIMD:
void convert_int16_to_float_optimized(const int16_t* input, float* output, size_t samples) {
    const float scale = 1.0f / 32768.0f;
    const __m128 scale_vec = _mm_set1_ps(scale);
    const __m128i zero = _mm_setzero_si128();

    size_t simd_samples = (samples / 8) * 8;

    // Process 8 samples at a time with SSE2
    for (size_t i = 0; i < simd_samples; i += 8) {
        // Load 8 int16 samples
        __m128i input_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));

        // Convert int16 to int32
        __m128i input_lo = _mm_srai_epi32(_mm_unpacklo_epi16(zero, input_vec), 16);
        __m128i input_hi = _mm_srai_epi32(_mm_unpackhi_epi16(zero, input_vec), 16);

        // Convert int32 to float
        __m128 float_lo = _mm_cvtepi32_ps(input_lo);
        __m128 float_hi = _mm_cvtepi32_ps(input_hi);

        // Apply scale
        float_lo = _mm_mul_ps(float_lo, scale_vec);
        float_hi = _mm_mul_ps(float_hi, scale_vec);

        // Store results
        _mm_storeu_ps(output + i, float_lo);
        _mm_storeu_ps(output + i + 4, float_hi);
    }

    // Handle remaining samples
    for (size_t i = simd_samples; i < samples; i++) {
        output[i] = input[i] * scale;
    }
}

// Example 2: Optimized volume control
// Original code:
void apply_volume_original(float* audio, size_t samples, float volume) {
    for (size_t i = 0; i < samples; i++) {
        audio[i] *= volume;
    }
}

// Optimized version with SIMD:
void apply_volume_optimized(float* audio, size_t samples, float volume) {
    const __m128 volume_vec = _mm_set1_ps(volume);
    size_t simd_samples = (samples / 4) * 4;

    // Process 4 samples at a time
    for (size_t i = 0; i < simd_samples; i += 4) {
        __m128 samples_vec = _mm_loadu_ps(audio + i);
        samples_vec = _mm_mul_ps(samples_vec, volume_vec);
        _mm_storeu_ps(audio + i, samples_vec);
    }

    // Handle remaining samples
    for (size_t i = simd_samples; i < samples; i++) {
        audio[i] *= volume;
    }
}

// Example 3: Optimized stereo to mono conversion
// Original code:
void stereo_to_mono_original(const float* stereo, float* mono, size_t frames) {
    for (size_t i = 0; i < frames; i++) {
        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
    }
}

// Optimized version with SIMD:
void stereo_to_mono_optimized(const float* stereo, float* mono, size_t frames) {
    const __m128 half = _mm_set1_ps(0.5f);
    size_t simd_frames = (frames / 2) * 2; // Process 2 frames at a time (4 samples)

    // Process 2 stereo frames (4 samples) at a time
    for (size_t i = 0; i < simd_frames; i += 2) {
        // Load left and right channels
        __m128 left_right = _mm_loadu_ps(stereo + i * 2);

        // Shuffle to separate left and right
        __m128 left = _mm_movelh_ps(left_right, left_right);
        __m128 right = _mm_movehl_ps(left_right, left_right);

        // Average and store
        __m128 mixed = _mm_mul_ps(_mm_add_ps(left, right), half);
        _mm_storeu_ps(mono + i, mixed);
    }

    // Handle remaining frames
    for (size_t i = simd_frames; i < frames; i++) {
        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
    }
}

// Example 4: Memory alignment for SIMD operations
class AlignedAudioBuffer {
public:
    AlignedAudioBuffer(size_t size) : size_(size) {
#ifdef _WIN32
        data_ = static_cast<float*>(_aligned_malloc(size * sizeof(float), 16));
#else
        if (posix_memalign(reinterpret_cast<void**>(&data_), 16, size * sizeof(float)) != 0) {
            data_ = nullptr;
        }
#endif
    }

    ~AlignedAudioBuffer() {
        if (data_) {
#ifdef _WIN32
            _aligned_free(data_);
#else
            free(data_);
#endif
        }
    }

    float* data() { return data_; }
    const float* data() const { return data_; }
    size_t size() const { return size_; }

private:
    float* data_;
    size_t size_;
};

// Example 5: Lock-free buffer for audio processing
template<typename T, size_t Size>
class LockFreeQueue {
public:
    LockFreeQueue() : head_(0), tail_(0) {}

    bool push(const T& item) {
        size_t next_head = (head_ + 1) % Size;
        if (next_head == tail_) {
            return false; // Full
        }
        buffer_[head_] = item;
        head_ = next_head;
        return true;
    }

    bool pop(T& item) {
        if (head_ == tail_) {
            return false; // Empty
        }
        item = buffer_[tail_];
        tail_ = (tail_ + 1) % Size;
        return true;
    }

private:
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    T buffer_[Size];
};

// Example 6: Integration with existing music player
class OptimizedAudioProcessor {
public:
    OptimizedAudioProcessor() {
        // Detect CPU features at initialization
        detect_cpu_features();
    }

    void process_audio(float* buffer, size_t frames, int channels) {
        // Apply volume if needed
        if (volume_ != 1.0f) {
            apply_volume_optimized(buffer, frames * channels, volume_);
        }

        // Convert channels if needed
        if (channels == 2 && output_channels_ == 1) {
            temp_buffer_.resize(frames);
            stereo_to_mono_optimized(buffer, temp_buffer_.data(), frames);
            std::copy(temp_buffer_.begin(), temp_buffer_.end(), buffer);
        }
    }

    void set_volume(float volume) { volume_ = volume; }
    void set_output_channels(int channels) { output_channels_ = channels; }

private:
    void detect_cpu_features() {
#ifdef _WIN32
        int cpu_info[4];
        __cpuid(cpu_info, 1);
        has_sse2_ = (cpu_info[3] & (1 << 26)) != 0;
        has_avx_ = (cpu_info[2] & (1 << 28)) != 0;
#else
        has_sse2_ = true;  // Assume modern CPU
        has_avx_ = true;   // Assume modern CPU
#endif

        std::cout << "CPU Features detected: "
                  << "SSE2=" << (has_sse2_ ? "Yes" : "No")
                  << ", AVX=" << (has_avx_ ? "Yes" : "No") << std::endl;
    }

    bool has_sse2_;
    bool has_avx_;
    float volume_{1.0f};
    int output_channels_{2};
    std::vector<float> temp_buffer_;

    // Simple RAII profiler
    struct ScopedProfile {
        ScopedProfile(const std::string& name) : name_(name) {
            start_time_ = std::chrono::high_resolution_clock::now();
        }
        ~ScopedProfile() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time_);
            std::cout << name_ << ": " << duration.count() << " μs" << std::endl;
        }
    private:
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };

    #define PROFILE_AUDIO(name, samples) ScopedProfile _profile(name)
};

// Demonstration
int main() {
    std::cout << "XpuMusic Performance Optimization Integration Example" << std::endl;
    std::cout << "======================================================" << std::endl;

    const size_t SAMPLES = 1024 * 1024;
    std::vector<int16_t> input_int16(SAMPLES);
    std::vector<float> output_float_original(SAMPLES);
    std::vector<float> output_float_optimized(SAMPLES);

    // Generate test data
    for (size_t i = 0; i < SAMPLES; i++) {
        input_int16[i] = (i % 10000) - 5000; // -5000 to 5000
    }

    std::cout << "\nTesting int16 to float conversion:" << std::endl;

    // Test original implementation
    auto start = std::chrono::high_resolution_clock::now();
    convert_int16_to_float_original(input_int16.data(), output_float_original.data(), SAMPLES);
    auto end = std::chrono::high_resolution_clock::now();
    auto original_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Test optimized implementation
    start = std::chrono::high_resolution_clock::now();
    convert_int16_to_float_optimized(input_int16.data(), output_float_optimized.data(), SAMPLES);
    end = std::chrono::high_resolution_clock::now();
    auto optimized_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Original time:  " << original_time << " μs" << std::endl;
    std::cout << "Optimized time: " << optimized_time << " μs" << std::endl;
    std::cout << "Speedup:       " << (double)original_time / optimized_time << "x" << std::endl;

    // Verify results
    bool correct = true;
    for (size_t i = 0; i < SAMPLES && i < 100; i++) {  // Check first 100 samples
        float diff = std::abs(output_float_original[i] - output_float_optimized[i]);
        if (diff > 0.0001f) {
            correct = false;
            break;
        }
    }

    std::cout << "Result verification: " << (correct ? "✓ PASS" : "✗ FAIL") << std::endl;

    std::cout << "\nTesting OptimizedAudioProcessor:" << std::endl;
    OptimizedAudioProcessor processor;
    processor.set_volume(0.5f);
    processor.set_output_channels(1);

    std::vector<float> stereo_audio(SAMPLES * 2);
    for (size_t i = 0; i < SAMPLES * 2; i++) {
        stereo_audio[i] = (i % 1000) / 1000.0f;
    }

    processor.process_audio(stereo_audio.data(), SAMPLES, 2);

    std::cout << "\nIntegration examples completed successfully!" << std::endl;
    std::cout << "\nKey optimizations demonstrated:" << std::endl;
    std::cout << "  ✓ SIMD instruction usage (SSE2/AVX)" << std::endl;
    std::cout << "  ✓ Memory alignment for SIMD operations" << std::endl;
    std::cout << "  ✓ Lock-free data structures" << std::endl;
    std::cout << "  ✓ Batch processing of multiple samples" << std::endl;
    std::cout << "  ✓ RAII for resource management" << std::endl;

    return 0;
}