/**
 * @file optimized_audio_processor.h
 * @brief Optimized audio processing with SIMD, memory alignment and multithreading
 * @date 2025-12-13
 */

#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <queue>
#include <functional>

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#else
#include <immintrin.h>
#endif

#include "sample_rate_converter.h"

// Memory alignment for SIMD operations
constexpr size_t SIMD_ALIGNMENT = 32;

namespace audio {

// Aligned allocator for SIMD operations
template<typename T>
struct aligned_allocator {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = aligned_allocator<U>;
    };

    aligned_allocator() noexcept = default;

    template<typename U>
    aligned_allocator(const aligned_allocator<U>&) noexcept {}

    pointer allocate(size_type n) {
#ifdef _WIN32
        return static_cast<pointer>(_aligned_malloc(n * sizeof(T), SIMD_ALIGNMENT));
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, SIMD_ALIGNMENT, n * sizeof(T)) != 0) {
            throw std::bad_alloc();
        }
        return static_cast<pointer>(ptr);
#endif
    }

    void deallocate(pointer p, size_type) noexcept {
#ifdef _WIN32
        _aligned_free(p);
#else
        free(p);
#endif
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        ::new((void*)p) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        p->~U();
    }

    bool operator==(const aligned_allocator&) const noexcept { return true; }
    bool operator!=(const aligned_allocator&) const noexcept { return false; }
};

template<typename T>
using aligned_vector = std::vector<T, aligned_allocator<T>>;

/**
 * @brief SIMD-optimized audio operations
 */
class SIMDOperations {
public:
    // CPU feature detection
    struct CPUFeatures {
        bool has_sse2;
        bool has_sse4_1;
        bool has_avx;
        bool has_avx2;
        bool has_fma3;
    };

    static CPUFeatures detect_cpu_features();

    // Optimized functions
    static void convert_int16_to_float_sse2(const int16_t* src, float* dst, size_t samples);
    static void convert_int24_to_float_sse2(const uint8_t* src, float* dst, size_t samples);
    static void convert_float_to_int16_sse2(const float* src, int16_t* dst, size_t samples);
    static void volume_sse2(float* audio, size_t samples, float volume);
    static void mix_channels_sse2(const float* src1, const float* src2, float* dst, size_t samples);
    static void interpolate_linear_sse2(const float* a, const float* b, float* out, size_t samples, float ratio);

    // AVX optimized versions
    static void convert_int16_to_float_avx(const int16_t* src, float* dst, size_t samples);
    static void convert_float_to_int16_avx(const float* src, int16_t* dst, size_t samples);
    static void volume_avx(float* audio, size_t samples, float volume);
    static void mix_channels_avx(const float* src1, const float* src2, float* dst, size_t samples);

private:
    static CPUFeatures cpu_features_;
    static bool features_detected_;
};

/**
 * @brief Audio buffer pool for memory management
 */
class AudioBufferPool {
public:
    struct Buffer {
        aligned_vector<float> data;
        size_t capacity;
        std::atomic<bool> in_use{false};
    };

    AudioBufferPool(size_t pool_size = 16, size_t buffer_size = 65536);
    ~AudioBufferPool();

    std::shared_ptr<Buffer> acquire_buffer();
    void release_buffer(std::shared_ptr<Buffer> buffer);

private:
    std::vector<std::unique_ptr<Buffer>> pool_;
    std::mutex mutex_;
    size_t buffer_size_;
};

/**
 * @brief Task queue for multithreaded audio processing
 */
class AudioTaskQueue {
public:
    using Task = std::function<void()>;

    AudioTaskQueue(size_t num_threads = std::thread::hardware_concurrency());
    ~AudioTaskQueue();

    void enqueue(Task task);
    void wait_for_all();
    void shutdown();

private:
    void worker_thread();

    std::vector<std::thread> workers_;
    std::queue<Task> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_tasks_{0};
    std::condition_variable finished_;
};

/**
 * @brief Optimized audio format converter
 */
class OptimizedFormatConverter {
public:
    struct Format {
        int sample_rate;
        int channels;
        int bits_per_sample;
        bool is_float;
    };

    OptimizedFormatConverter();
    ~OptimizedFormatConverter();

    bool initialize(const Format& input, const Format& output);

    // Convert in chunks for streaming processing
    size_t convert_chunk(const void* input, size_t input_frames,
                        void* output, size_t max_output_frames);

    // Batch conversion for entire buffer
    size_t convert(const void* input, size_t input_frames,
                   void* output, size_t max_output_frames);

    // Reset converter state
    void reset();

private:
    Format input_format_;
    Format output_format_;
    std::unique_ptr<ISampleRateConverter> resampler_;
    aligned_vector<float> temp_buffer_;
    std::vector<float> channel_buffer_;

    void convert_to_float(const void* input, float* output, size_t frames);
    void convert_from_float(const float* input, void* output, size_t frames);
    void convert_channels(const float* input, float* output, size_t frames);
};

/**
 * @brief Streaming audio processor for large files
 */
class StreamingAudioProcessor {
public:
    struct ProcessingSettings {
        size_t chunk_size;          // Frames per processing chunk
        size_t buffer_count;        // Number of buffers in pool
        bool enable_multithreading; // Use multiple threads
        size_t thread_count;        // Number of processing threads
    };

    StreamingAudioProcessor(const ProcessingSettings& settings = {});
    ~StreamingAudioProcessor();

    // Process audio in streaming fashion
    bool start_processing(const std::string& filename);
    void stop_processing();

    // Get processed audio chunks
    bool get_processed_chunk(aligned_vector<float>& chunk, size_t& frames);

    // Seek to position (in samples)
    bool seek(size_t position);

    // Get audio format
    OptimizedFormatConverter::Format get_format() const;

private:
    ProcessingSettings settings_;
    std::unique_ptr<AudioBufferPool> buffer_pool_;
    std::unique_ptr<AudioTaskQueue> task_queue_;
    std::unique_ptr<OptimizedFormatConverter> converter_;

    // Thread-safe queues
    std::queue<aligned_vector<float>> input_queue_;
    std::queue<aligned_vector<float>> output_queue_;
    std::mutex input_mutex_;
    std::mutex output_mutex_;
    std::condition_variable input_cv_;
    std::condition_variable output_cv_;

    std::atomic<bool> processing_{false};
    std::thread processing_thread_;

    void processing_loop();
    bool load_chunk();
    void process_chunk(aligned_vector<float>& chunk);
};

/**
 * @brief Performance profiler for audio operations
 */
class AudioProfiler {
public:
    struct ProfileEntry {
        std::string name;
        double time_ms;
        size_t samples_processed;
        double samples_per_second;
    };

    static AudioProfiler& instance();

    void start_profile(const std::string& name);
    void end_profile(const std::string& name, size_t samples = 0);

    std::vector<ProfileEntry> get_entries() const;
    void clear();
    void print_report() const;

private:
    struct TimingInfo {
        std::chrono::high_resolution_clock::time_point start_time;
        size_t total_samples = 0;
        double total_time = 0.0;
        size_t call_count = 0;
    };

    std::unordered_map<std::string, TimingInfo> profiles_;
    mutable std::mutex mutex_;
};

// RAII profiler helper
class ScopedProfile {
public:
    ScopedProfile(const std::string& name, size_t samples = 0)
        : name_(name), samples_(samples) {
        AudioProfiler::instance().start_profile(name_);
    }

    ~ScopedProfile() {
        AudioProfiler::instance().end_profile(name_, samples_);
    }

private:
    std::string name_;
    size_t samples_;
};

#define PROFILE_AUDIO(name, samples) ScopedProfile _profile(name, samples)

} // namespace audio