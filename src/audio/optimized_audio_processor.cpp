/**
 * @file optimized_audio_processor.cpp
 * @brief Implementation of optimized audio processing with SIMD
 * @date 2025-12-13
 */

#include "optimized_audio_processor.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <iomanip>

#ifdef _WIN32
// Prevent Windows min/max macro conflicts
#define NOMINMAX
#include <windows.h>
#undef min
#undef max
#endif

namespace audio {

// Static member definitions
SIMDOperations::CPUFeatures SIMDOperations::cpu_features_{};
bool SIMDOperations::features_detected_ = false;

// SIMDOperations implementation
SIMDOperations::CPUFeatures SIMDOperations::detect_cpu_features() {
    if (features_detected_) {
        return cpu_features_;
    }

    CPUFeatures features = {};

#ifdef _WIN32
    int cpu_info[4];
    __cpuid(cpu_info, 0);

    // Check for SSE2
    __cpuid(cpu_info, 1);
    features.has_sse2 = (cpu_info[3] & (1 << 26)) != 0;

    // Check for SSE4.1
    features.has_sse4_1 = (cpu_info[2] & (1 << 19)) != 0;

    // Check for AVX
    features.has_avx = (cpu_info[2] & (1 << 28)) != 0;

    // Check for AVX2 and FMA3 (requires extended CPUID)
    __cpuidex(cpu_info, 7, 0);
    features.has_avx2 = (cpu_info[1] & (1 << 5)) != 0;

    __cpuid(cpu_info, 1);
    features.has_fma3 = (cpu_info[2] & (1 << 12)) != 0;
#else
    // Linux/macOS implementation using CPUID
    uint32_t eax, ebx, ecx, edx;
    __cpuid(0, eax, ebx, ecx, edx);

    __cpuid(1, eax, ebx, ecx, edx);
    features.has_sse2 = (edx & (1 << 26)) != 0;
    features.has_sse4_1 = (ecx & (1 << 19)) != 0;
    features.has_avx = (ecx & (1 << 28)) != 0;
    features.has_fma3 = (ecx & (1 << 12)) != 0;

    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    features.has_avx2 = (ebx & (1 << 5)) != 0;
#endif

    cpu_features_ = features;
    features_detected_ = true;

    std::cout << "CPU Features detected:" << std::endl;
    std::cout << "  SSE2: " << (features.has_sse2 ? "Yes" : "No") << std::endl;
    std::cout << "  SSE4.1: " << (features.has_sse4_1 ? "Yes" : "No") << std::endl;
    std::cout << "  AVX: " << (features.has_avx ? "Yes" : "No") << std::endl;
    std::cout << "  AVX2: " << (features.has_avx2 ? "Yes" : "No") << std::endl;
    std::cout << "  FMA3: " << (features.has_fma3 ? "Yes" : "No") << std::endl;

    return features;
}

void SIMDOperations::convert_int16_to_float_sse2(const int16_t* src, float* dst, size_t samples) {
    if (!cpu_features_.has_sse2) {
        // Fallback to scalar implementation
        for (size_t i = 0; i < samples; i++) {
            dst[i] = src[i] * (1.0f / 32768.0f);
        }
        return;
    }

    const float scale = 1.0f / 32768.0f;
    const __m128i zero = _mm_setzero_si128();
    const __m128 scale_vec = _mm_set1_ps(scale);

    size_t simd_samples = (samples / 8) * 8;

    for (size_t i = 0; i < simd_samples; i += 8) {
        // Load 8 int16 samples
        __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));

        // Convert int16 to int32
        __m128i input_lo = _mm_srai_epi32(_mm_unpacklo_epi16(zero, input), 16);
        __m128i input_hi = _mm_srai_epi32(_mm_unpackhi_epi16(zero, input), 16);

        // Convert int32 to float
        __m128 float_lo = _mm_cvtepi32_ps(input_lo);
        __m128 float_hi = _mm_cvtepi32_ps(input_hi);

        // Apply scale
        float_lo = _mm_mul_ps(float_lo, scale_vec);
        float_hi = _mm_mul_ps(float_hi, scale_vec);

        // Store results
        _mm_storeu_ps(dst + i, float_lo);
        _mm_storeu_ps(dst + i + 4, float_hi);
    }

    // Handle remaining samples
    for (size_t i = simd_samples; i < samples; i++) {
        dst[i] = src[i] * scale;
    }
}

void SIMDOperations::convert_int24_to_float_sse2(const uint8_t* src, float* dst, size_t samples) {
    // 24-bit audio is less common, use scalar implementation for now
    // Could be optimized with custom SSE implementation if needed
    for (size_t i = 0; i < samples; i++) {
        int32_t sample = (src[i*3] << 8) | (src[i*3+1] << 16) | (src[i*3+2] << 24);
        dst[i] = sample * (1.0f / 2147483648.0f);
    }
}

void SIMDOperations::convert_float_to_int16_sse2(const float* src, int16_t* dst, size_t samples) {
    if (!cpu_features_.has_sse2) {
        // Fallback
        for (size_t i = 0; i < samples; i++) {
            float sample = std::max(-1.0f, std::min(1.0f, src[i]));
            dst[i] = static_cast<int16_t>(sample * 32767.0f);
        }
        return;
    }

    const float scale = 32767.0f;
    const __m128 scale_vec = _mm_set1_ps(scale);
    const __m128 max_val = _mm_set1_ps(32767.0f);
    const __m128 min_val = _mm_set1_ps(-32768.0f);

    size_t simd_samples = (samples / 4) * 4;

    for (size_t i = 0; i < simd_samples; i += 4) {
        // Load 4 float samples
        __m128 input = _mm_loadu_ps(src + i);

        // Clamp to range
        input = _mm_max_ps(min_val, _mm_min_ps(max_val, input));

        // Scale
        input = _mm_mul_ps(input, scale_vec);

        // Convert to int32
        __m128i int32_val = _mm_cvtps_epi32(input);

        // Pack to int16
        __m128i int16_val = _mm_packs_epi32(int32_val, int32_val);

        // Store result
        _mm_storel_epi64(reinterpret_cast<__m128i*>(dst + i), int16_val);
    }

    // Handle remaining samples
    for (size_t i = simd_samples; i < samples; i++) {
        float sample = std::max(-1.0f, std::min(1.0f, src[i]));
        dst[i] = static_cast<int16_t>(sample * 32767.0f);
    }
}

void SIMDOperations::volume_sse2(float* audio, size_t samples, float volume) {
    if (!cpu_features_.has_sse2 || volume == 1.0f) {
        if (volume != 1.0f) {
            for (size_t i = 0; i < samples; i++) {
                audio[i] *= volume;
            }
        }
        return;
    }

    const __m128 volume_vec = _mm_set1_ps(volume);
    size_t simd_samples = (samples / 4) * 4;

    for (size_t i = 0; i < simd_samples; i += 4) {
        __m128 samples_vec = _mm_loadu_ps(audio + i);
        samples_vec = _mm_mul_ps(samples_vec, volume_vec);
        _mm_storeu_ps(audio + i, samples_vec);
    }

    for (size_t i = simd_samples; i < samples; i++) {
        audio[i] *= volume;
    }
}

void SIMDOperations::mix_channels_sse2(const float* src1, const float* src2, float* dst, size_t samples) {
    if (!cpu_features_.has_sse2) {
        for (size_t i = 0; i < samples; i++) {
            dst[i] = src1[i] + src2[i];
        }
        return;
    }

    const __m128 half = _mm_set1_ps(0.5f);
    size_t simd_samples = (samples / 4) * 4;

    for (size_t i = 0; i < simd_samples; i += 4) {
        __m128 s1 = _mm_loadu_ps(src1 + i);
        __m128 s2 = _mm_loadu_ps(src2 + i);
        __m128 mixed = _mm_mul_ps(_mm_add_ps(s1, s2), half);
        _mm_storeu_ps(dst + i, mixed);
    }

    for (size_t i = simd_samples; i < samples; i++) {
        dst[i] = (src1[i] + src2[i]) * 0.5f;
    }
}

void SIMDOperations::interpolate_linear_sse2(const float* a, const float* b, float* out,
                                            size_t samples, float ratio) {
    if (!cpu_features_.has_sse2) {
        for (size_t i = 0; i < samples; i++) {
            float t = ratio * static_cast<float>(i) / static_cast<float>(samples);
            out[i] = a[i] * (1.0f - t) + b[i] * t;
        }
        return;
    }

    const __m128 one = _mm_set1_ps(1.0f);
    size_t simd_samples = (samples / 4) * 4;
    const float inv_samples = 1.0f / samples;

    for (size_t i = 0; i < simd_samples; i += 4) {
        __m128 indices = _mm_set_ps(i + 3, i + 2, i + 1, i);
        __m128 t = _mm_mul_ps(_mm_set1_ps(ratio * inv_samples), indices);
        __m128 one_minus_t = _mm_sub_ps(one, t);

        __m128 a_vec = _mm_loadu_ps(a + i);
        __m128 b_vec = _mm_loadu_ps(b + i);

        __m128 result = _mm_add_ps(_mm_mul_ps(a_vec, one_minus_t), _mm_mul_ps(b_vec, t));
        _mm_storeu_ps(out + i, result);
    }

    for (size_t i = simd_samples; i < samples; i++) {
        float t = ratio * i / samples;
        out[i] = a[i] * (1.0f - t) + b[i] * t;
    }
}

// AVX optimized versions
void SIMDOperations::convert_int16_to_float_avx(const int16_t* src, float* dst, size_t samples) {
    if (!cpu_features_.has_avx) {
        convert_int16_to_float_sse2(src, dst, samples);
        return;
    }

    const float scale = 1.0f / 32768.0f;
    const __m256 scale_vec = _mm256_set1_ps(scale);
    const __m256i zero = _mm256_setzero_si256();

    size_t simd_samples = (samples / 16) * 16;

    for (size_t i = 0; i < simd_samples; i += 16) {
        // Load 16 int16 samples
        __m128i input_low = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
        __m128i input_high = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i + 8));

        // Convert to int32 and then to float using AVX
        __m256i int32_low = _mm256_cvtepi16_epi32(input_low);
        __m256i int32_high = _mm256_cvtepi16_epi32(input_high);

        __m256 float_low = _mm256_cvtepi32_ps(int32_low);
        __m256 float_high = _mm256_cvtepi32_ps(int32_high);

        // Apply scale
        float_low = _mm256_mul_ps(float_low, scale_vec);
        float_high = _mm256_mul_ps(float_high, scale_vec);

        // Store results
        _mm256_storeu_ps(dst + i, float_low);
        _mm256_storeu_ps(dst + i + 8, float_high);
    }

    // Handle remaining samples with SSE2
    convert_int16_to_float_sse2(src + simd_samples, dst + simd_samples, samples - simd_samples);
}

void SIMDOperations::convert_float_to_int16_avx(const float* src, int16_t* dst, size_t samples) {
    if (!cpu_features_.has_avx) {
        convert_float_to_int16_sse2(src, dst, samples);
        return;
    }

    const float scale = 32767.0f;
    const __m256 scale_vec = _mm256_set1_ps(scale);
    const __m256 max_val = _mm256_set1_ps(32767.0f);
    const __m256 min_val = _mm256_set1_ps(-32768.0f);

    size_t simd_samples = (samples / 8) * 8;

    for (size_t i = 0; i < simd_samples; i += 8) {
        // Load 8 float samples
        __m256 input = _mm256_loadu_ps(src + i);

        // Clamp to range
        input = _mm256_max_ps(min_val, _mm256_min_ps(max_val, input));

        // Scale
        input = _mm256_mul_ps(input, scale_vec);

        // Convert to int32
        __m256i int32 = _mm256_cvtps_epi32(input);

        // Pack to int16 (requires two steps)
        __m128i int32_low = _mm256_castsi256_si128(int32);
        __m128i int32_high = _mm256_extracti128_si256(int32, 1);
        __m128i int16 = _mm_packs_epi32(int32_low, int32_high);

        // Store result
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), int16);
    }

    // Handle remaining samples
    convert_float_to_int16_sse2(src + simd_samples, dst + simd_samples, samples - simd_samples);
}

void SIMDOperations::volume_avx(float* audio, size_t samples, float volume) {
    if (!cpu_features_.has_avx || volume == 1.0f) {
        volume_sse2(audio, samples, volume);
        return;
    }

    const __m256 volume_vec = _mm256_set1_ps(volume);
    size_t simd_samples = (samples / 8) * 8;

    for (size_t i = 0; i < simd_samples; i += 8) {
        __m256 samples_vec = _mm256_loadu_ps(audio + i);
        samples_vec = _mm256_mul_ps(samples_vec, volume_vec);
        _mm256_storeu_ps(audio + i, samples_vec);
    }

    for (size_t i = simd_samples; i < samples; i++) {
        audio[i] *= volume;
    }
}

void SIMDOperations::mix_channels_avx(const float* src1, const float* src2, float* dst, size_t samples) {
    if (!cpu_features_.has_avx) {
        mix_channels_sse2(src1, src2, dst, samples);
        return;
    }

    const __m256 half = _mm256_set1_ps(0.5f);
    size_t simd_samples = (samples / 8) * 8;

    for (size_t i = 0; i < simd_samples; i += 8) {
        __m256 s1 = _mm256_loadu_ps(src1 + i);
        __m256 s2 = _mm256_loadu_ps(src2 + i);
        __m256 mixed = _mm256_mul_ps(_mm256_add_ps(s1, s2), half);
        _mm256_storeu_ps(dst + i, mixed);
    }

    for (size_t i = simd_samples; i < samples; i++) {
        dst[i] = (src1[i] + src2[i]) * 0.5f;
    }
}

// AudioBufferPool implementation
AudioBufferPool::AudioBufferPool(size_t pool_size, size_t buffer_size)
    : buffer_size_(buffer_size) {
    pool_.reserve(pool_size);
    for (size_t i = 0; i < pool_size; i++) {
        auto buffer = std::make_unique<Buffer>();
        buffer->data.reserve(buffer_size);
        buffer->capacity = buffer_size;
        pool_.push_back(std::move(buffer));
    }
}

AudioBufferPool::~AudioBufferPool() = default;

std::shared_ptr<AudioBufferPool::Buffer> AudioBufferPool::acquire_buffer() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& buffer : pool_) {
        if (!buffer->in_use.exchange(true)) {
            return std::shared_ptr<Buffer>(buffer.get(), [this](Buffer* buf) {
                buf->in_use = false;
            });
        }
    }

    // Pool exhausted, create new buffer
    auto buffer = std::make_unique<Buffer>();
    buffer->data.reserve(buffer_size_);
    buffer->capacity = buffer_size_;
    buffer->in_use = true;

    return std::shared_ptr<Buffer>(buffer.get(), [this, buf = buffer.get()](Buffer*) {
        buf->in_use = false;
    });
}

void AudioBufferPool::release_buffer(std::shared_ptr<Buffer> buffer) {
    // Buffer is automatically released when shared_ptr goes out of scope
}

// AudioTaskQueue implementation
AudioTaskQueue::AudioTaskQueue(size_t num_threads) {
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; i++) {
        workers_.emplace_back(&AudioTaskQueue::worker_thread, this);
    }
}

AudioTaskQueue::~AudioTaskQueue() {
    shutdown();
}

void AudioTaskQueue::enqueue(Task task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}

void AudioTaskQueue::wait_for_all() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    finished_.wait(lock, [this] { return tasks_.empty() && active_tasks_ == 0; });
}

void AudioTaskQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void AudioTaskQueue::worker_thread() {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
            active_tasks_++;
        }

        task();

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            active_tasks_--;
            if (active_tasks_ == 0 && tasks_.empty()) {
                finished_.notify_all();
            }
        }
    }
}

// AudioProfiler implementation
AudioProfiler& AudioProfiler::instance() {
    static AudioProfiler profiler;
    return profiler;
}

void AudioProfiler::start_profile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    profiles_[name].start_time = std::chrono::high_resolution_clock::now();
}

void AudioProfiler::end_profile(const std::string& name, size_t samples) {
    auto end_time = std::chrono::high_resolution_clock::now();

    std::lock_guard<std::mutex> lock(mutex_);
    auto& info = profiles_[name];

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - info.start_time);

    info.total_time += duration.count() / 1000.0; // Convert to milliseconds
    info.total_samples += samples;
    info.call_count++;
}

std::vector<AudioProfiler::ProfileEntry> AudioProfiler::get_entries() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ProfileEntry> entries;
    entries.reserve(profiles_.size());

    for (const auto& pair : profiles_) {
        ProfileEntry entry;
        entry.name = pair.first;
        entry.time_ms = pair.second.total_time;
        entry.samples_processed = pair.second.total_samples;

        if (entry.time_ms > 0) {
            entry.samples_per_second = (pair.second.total_samples / entry.time_ms) * 1000.0;
        } else {
            entry.samples_per_second = 0;
        }

        entries.push_back(entry);
    }

    return entries;
}

void AudioProfiler::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    profiles_.clear();
}

void AudioProfiler::print_report() const {
    auto entries = get_entries();

    std::cout << "\n=== Audio Performance Report ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(30) << "Operation"
              << std::setw(15) << "Time (ms)"
              << std::setw(15) << "Samples"
              << std::setw(20) << "Samples/sec" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    for (const auto& entry : entries) {
        std::cout << std::setw(30) << entry.name
                  << std::setw(15) << entry.time_ms
                  << std::setw(15) << entry.samples_processed
                  << std::setw(20) << entry.samples_per_second << std::endl;
    }
    std::cout << std::endl;
}

} // namespace audio