/**
 * @file performance_benchmark.cpp
 * @brief Audio processing performance benchmark
 * @date 2025-12-13
 */

#include "audio/optimized_audio_processor.h"
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace audio;
using namespace std::chrono;

// Test data size
constexpr size_t TEST_SAMPLES = 1024 * 1024;  // 1M samples
constexpr size_t TEST_FRAMES = TEST_SAMPLES / 2; // Stereo
constexpr int TEST_ITERATIONS = 100;

// Generate test audio data
void generate_test_audio(std::vector<int16_t>& audio, size_t samples) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int16_t> dist(-32768, 32767);

    audio.resize(samples);
    for (size_t i = 0; i < samples; i++) {
        audio[i] = dist(gen);
    }
}

void generate_test_audio_float(std::vector<float>& audio, size_t samples) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    audio.resize(samples);
    for (size_t i = 0; i < samples; i++) {
        audio[i] = dist(gen);
    }
}

// Benchmark functions
void benchmark_int16_to_float_scalar(const int16_t* input, float* output, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        output[i] = input[i] * (1.0f / 32768.0f);
    }
}

void benchmark_int16_to_float_simd(const int16_t* input, float* output, size_t samples) {
    auto& cpu = SIMDOperations::detect_cpu_features();
    if (cpu.has_avx) {
        SIMDOperations::convert_int16_to_float_avx(input, output, samples);
    } else {
        SIMDOperations::convert_int16_to_float_sse2(input, output, samples);
    }
}

void benchmark_float_to_int16_scalar(const float* input, int16_t* output, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        float sample = std::max(-1.0f, std::min(1.0f, input[i]));
        output[i] = static_cast<int16_t>(sample * 32767.0f);
    }
}

void benchmark_float_to_int16_simd(const float* input, int16_t* output, size_t samples) {
    auto& cpu = SIMDOperations::detect_cpu_features();
    if (cpu.has_avx) {
        SIMDOperations::convert_float_to_int16_avx(input, output, samples);
    } else {
        SIMDOperations::convert_float_to_int16_sse2(input, output, samples);
    }
}

void benchmark_volume_scalar(float* audio, size_t samples, float volume) {
    for (size_t i = 0; i < samples; i++) {
        audio[i] *= volume;
    }
}

void benchmark_volume_simd(float* audio, size_t samples, float volume) {
    auto& cpu = SIMDOperations::detect_cpu_features();
    if (cpu.has_avx) {
        SIMDOperations::volume_avx(audio, samples, volume);
    } else if (cpu.has_sse2) {
        SIMDOperations::volume_sse2(audio, samples, volume);
    }
}

void benchmark_mix_channels_scalar(const float* src1, const float* src2, float* dst, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        dst[i] = (src1[i] + src2[i]) * 0.5f;
    }
}

void benchmark_mix_channels_simd(const float* src1, const float* src2, float* dst, size_t samples) {
    auto& cpu = SIMDOperations::detect_cpu_features();
    if (cpu.has_avx) {
        SIMDOperations::mix_channels_avx(src1, src2, dst, samples);
    } else {
        SIMDOperations::mix_channels_sse2(src1, src2, dst, samples);
    }
}

void benchmark_stereo_to_mono_scalar(const float* stereo, float* mono, size_t frames) {
    for (size_t i = 0; i < frames; i++) {
        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
    }
}

void benchmark_stereo_to_mono_simd(const float* stereo, float* mono, size_t frames) {
    auto& cpu = SIMDOperations::detect_cpu_features();
    if (cpu.has_avx) {
        size_t samples = frames;
        size_t simd_samples = (samples / 4) * 4;

        for (size_t i = 0; i < simd_samples; i += 4) {
            __m256 left = _mm256_loadu_ps(stereo + i * 2);
            __m256 right = _mm256_loadu_ps(stereo + i * 2 + 4);
            __m256 mixed = _mm256_mul_ps(_mm256_add_ps(left, right), _mm256_set1_ps(0.5f));
            _mm256_storeu_ps(mono + i, mixed);
        }

        for (size_t i = simd_samples; i < samples; i++) {
            mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
        }
    } else {
        benchmark_stereo_to_mono_scalar(stereo, mono, frames);
    }
}

// Run benchmark and print results
template<typename Func>
double run_benchmark(Func func, const std::string& name, size_t samples, int iterations) {
    // Warm up
    for (int i = 0; i < 10; i++) {
        func();
    }

    // Measure
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        func();
    }
    auto end = high_resolution_clock::now();

    double total_time = duration_cast<microseconds>(end - start).count() / 1000.0;
    double avg_time = total_time / iterations;
    double samples_per_second = (samples * iterations) / (total_time / 1000.0);

    std::cout << std::setw(30) << name << ": "
              << std::setw(10) << std::fixed << std::setprecision(3) << avg_time << " ms avg, "
              << std::setw(15) << std::scientific << samples_per_second << " samples/sec"
              << std::endl;

    return avg_time;
}

int main() {
    std::cout << "Audio Processing Performance Benchmark" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Test samples: " << TEST_SAMPLES << " (" << TEST_FRAMES << " stereo frames)" << std::endl;
    std::cout << "Iterations per test: " << TEST_ITERATIONS << std::endl;
    std::cout << std::endl;

    // Detect CPU features
    SIMDOperations::detect_cpu_features();
    std::cout << std::endl;

    // Prepare test data
    std::vector<int16_t> input_int16;
    std::vector<float> input_float;
    std::vector<float> input_float2;
    std::vector<float> output_float;
    std::vector<int16_t> output_int16;
    std::vector<float> mixed_output;
    std::vector<float> mono_output;

    generate_test_audio(input_int16, TEST_SAMPLES);
    generate_test_audio_float(input_float, TEST_SAMPLES);
    generate_test_audio_float(input_float2, TEST_SAMPLES);
    output_float.resize(TEST_SAMPLES);
    output_int16.resize(TEST_SAMPLES);
    mixed_output.resize(TEST_SAMPLES);
    mono_output.resize(TEST_FRAMES);

    std::cout << "Format Conversion Benchmarks:" << std::endl;
    std::cout << "------------------------------" << std::endl;

    // int16 to float
    double int16_to_float_scalar_time = run_benchmark(
        [&]() { benchmark_int16_to_float_scalar(input_int16.data(), output_float.data(), TEST_SAMPLES); },
        "int16_to_float (scalar)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    double int16_to_float_simd_time = run_benchmark(
        [&]() { benchmark_int16_to_float_simd(input_int16.data(), output_float.data(), TEST_SAMPLES); },
        "int16_to_float (SIMD)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    std::cout << std::setw(30) << "Speedup: " << std::fixed << std::setprecision(2)
              << (int16_to_float_scalar_time / int16_to_float_simd_time) << "x" << std::endl;
    std::cout << std::endl;

    // float to int16
    double float_to_int16_scalar_time = run_benchmark(
        [&]() { benchmark_float_to_int16_scalar(input_float.data(), output_int16.data(), TEST_SAMPLES); },
        "float_to_int16 (scalar)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    double float_to_int16_simd_time = run_benchmark(
        [&]() { benchmark_float_to_int16_simd(input_float.data(), output_int16.data(), TEST_SAMPLES); },
        "float_to_int16 (SIMD)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    std::cout << std::setw(30) << "Speedup: " << std::fixed << std::setprecision(2)
              << (float_to_int16_scalar_time / float_to_int16_simd_time) << "x" << std::endl;
    std::cout << std::endl;

    std::cout << "Audio Processing Benchmarks:" << std::endl;
    std::cout << "----------------------------" << std::endl;

    // Volume control
    double volume_scalar_time = run_benchmark(
        [&]() { benchmark_volume_scalar(output_float.data(), TEST_SAMPLES, 0.8f); },
        "Volume (scalar)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    double volume_simd_time = run_benchmark(
        [&]() { benchmark_volume_simd(output_float.data(), TEST_SAMPLES, 0.8f); },
        "Volume (SIMD)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    std::cout << std::setw(30) << "Speedup: " << std::fixed << std::setprecision(2)
              << (volume_scalar_time / volume_simd_time) << "x" << std::endl;
    std::cout << std::endl;

    // Channel mixing
    double mix_scalar_time = run_benchmark(
        [&]() { benchmark_mix_channels_scalar(input_float.data(), input_float2.data(), mixed_output.data(), TEST_SAMPLES); },
        "Mix channels (scalar)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    double mix_simd_time = run_benchmark(
        [&]() { benchmark_mix_channels_simd(input_float.data(), input_float2.data(), mixed_output.data(), TEST_SAMPLES); },
        "Mix channels (SIMD)",
        TEST_SAMPLES,
        TEST_ITERATIONS
    );

    std::cout << std::setw(30) << "Speedup: " << std::fixed << std::setprecision(2)
              << (mix_scalar_time / mix_simd_time) << "x" << std::endl;
    std::cout << std::endl;

    // Stereo to mono
    double stereo_to_mono_scalar_time = run_benchmark(
        [&]() { benchmark_stereo_to_mono_scalar(input_float.data(), mono_output.data(), TEST_FRAMES); },
        "Stereo to mono (scalar)",
        TEST_FRAMES,
        TEST_ITERATIONS
    );

    double stereo_to_mono_simd_time = run_benchmark(
        [&]() { benchmark_stereo_to_mono_simd(input_float.data(), mono_output.data(), TEST_FRAMES); },
        "Stereo to mono (SIMD)",
        TEST_FRAMES,
        TEST_ITERATIONS
    );

    std::cout << std::setw(30) << "Speedup: " << std::fixed << std::setprecision(2)
              << (stereo_to_mono_scalar_time / stereo_to_mono_simd_time) << "x" << std::endl;
    std::cout << std::endl;

    // Summary
    std::cout << "Benchmark Summary:" << std::endl;
    std::cout << "=================" << std::endl;

    double avg_speedup = (int16_to_float_scalar_time / int16_to_float_simd_time +
                         float_to_int16_scalar_time / float_to_int16_simd_time +
                         volume_scalar_time / volume_simd_time +
                         mix_scalar_time / mix_simd_time +
                         stereo_to_mono_scalar_time / stereo_to_mono_simd_time) / 5.0;

    std::cout << "Average SIMD speedup: " << std::fixed << std::setprecision(2) << avg_speedup << "x" << std::endl;

    if (avg_speedup > 2.0) {
        std::cout << "Significant performance improvement achieved!" << std::endl;
    } else if (avg_speedup > 1.5) {
        std::cout << "Moderate performance improvement achieved." << std::endl;
    } else {
        std::cout << "Limited performance improvement. Check SIMD availability." << std::endl;
    }

    // Test audio buffer pool
    std::cout << "\nAudio Buffer Pool Test:" << std::endl;
    std::cout << "-----------------------" << std::endl;

    {
        AudioProfiler::instance().clear();
        PROFILE_AUDIO("buffer_pool_test", 1000000);

        AudioBufferPool pool(8, 65536);
        std::vector<std::shared_ptr<AudioBufferPool::Buffer>> buffers;

        auto start = high_resolution_clock::now();

        // Acquire and release buffers
        for (int i = 0; i < 1000; i++) {
            auto buffer = pool.acquire_buffer();
            if (buffer) {
                // Simulate buffer usage
                std::fill(buffer->data.begin(), buffer->data.begin() + 1024, 0.5f);
                buffers.push_back(buffer);
            }

            // Randomly release some buffers
            if (i % 3 == 0 && !buffers.empty()) {
                buffers.erase(buffers.begin());
            }
        }

        auto end = high_resolution_clock::now();
        double pool_time = duration_cast<microseconds>(end - start).count() / 1000.0;

        std::cout << "Buffer pool operations: " << pool_time << " ms total" << std::endl;
    }

    // Print profiler report
    AudioProfiler::instance().print_report();

    std::cout << "\nBenchmark completed successfully!" << std::endl;

    return 0;
}