/**
 * @file test_audio_chunk.cpp
 * @brief audio_chunk 实现的测试
 * @date 2025-12-09
 */

#include "audio_chunk_impl.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace foobar2000_sdk;

#define TEST(name) void test_##name(); \
                   tests.push_back({#name, test_##name}); \
                   void test_##name()

struct TestCase {
    const char* name;
    void (*func)();
};

std::vector<TestCase> tests;
int passed = 0;
int failed = 0;

void run_tests() {
    std::cout << "Running audio_chunk Tests..." << std::endl;
    std::cout << "=============================" << std::endl;
    
    for (const auto& test : tests) {
        std::cout << "Testing: " << test.name << "... ";
        try {
            test.func();
            std::cout << "PASSED" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << std::endl;
            failed++;
        } catch (...) {
            std::cout << "FAILED: Unknown error" << std::endl;
            failed++;
        }
    }
    
    std::cout << "=============================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
}

void assert_true(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void assert_equals(int expected, int actual, const char* message) {
    if (expected != actual) {
        std::ostringstream oss;
        oss << message << " (expected: " << expected << ", actual: " << actual << ")";
        throw std::runtime_error(oss.str());
    }
}

void assert_float_equals(float expected, float actual, float epsilon, const char* message) {
    if (std::abs(expected - actual) > epsilon) {
        std::ostringstream oss;
        oss << message << " (expected: " << expected << ", actual: " << actual << ")";
        throw std::runtime_error(oss.str());
    }
}

//==============================================================================
// 测试用例
//==============================================================================

TEST(chunk_create) {
    auto chunk = audio_chunk_create();
    assert_true(chunk != nullptr, "Chunk should be created");
    
    assert_equals(0, chunk->get_sample_count(), "Initial sample count should be 0");
    assert_equals(2, chunk->get_channels(), "Default channels should be 2");
    assert_equals(44100, chunk->get_sample_rate(), "Default sample rate should be 44100");
}

TEST(chunk_set_data) {
    auto chunk = audio_chunk_create();
    
    // 创建测试数据（1 秒 440Hz 正弦波）
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = sample_rate;  // 1 秒
    
    std::vector<audio_sample> data(sample_count * channels);
    float frequency = 440.0f;
    
    for (uint32_t i = 0; i < sample_count; ++i) {
        float sample = std::sin(2.0f * M_PI * frequency * i / sample_rate);
        for (uint32_t ch = 0; ch < channels; ++ch) {
            data[i * channels + ch] = sample * 0.5f;  // -0.5 to 0.5
        }
    }
    
    // 设置数据
    chunk->set_data(data.data(), sample_count, channels, sample_rate);
    
    assert_equals(sample_count, chunk->get_sample_count(), "Sample count should match");
    assert_equals(channels, chunk->get_channels(), "Channels should match");
    assert_equals(sample_rate, chunk->get_sample_rate(), "Sample rate should match");
    
    // 验证数据
    const audio_sample* retrieved = chunk->get_data();
    assert_true(retrieved != nullptr, "Data should not be null");
    
    // 检查前几个样本
    assert_float_equals(data[0], retrieved[0], 1e-6f, "First sample should match");
    assert_float_equals(data[1], retrieved[1], 1e-6f, "Second sample should match");
}

TEST(chunk_append_data) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    std::vector<audio_sample> data1(sample_count * channels, 0.5f);
    std::vector<audio_sample> data2(sample_count * channels, -0.5f);
    
    // 设置初始数据
    chunk->set_data(data1.data(), sample_count, channels, sample_rate);
    assert_equals(sample_count, chunk->get_sample_count(), "Initial sample count should match");
    
    // 追加数据
    chunk->data_append(data2.data(), sample_count);
    assert_equals(sample_count * 2, chunk->get_sample_count(), "Sample count should double after append");
    
    // 验证数据
    const audio_sample* retrieved = chunk->get_data();
    assert_float_equals(0.5f, retrieved[0], 1e-6f, "First part should be 0.5");
    assert_float_equals(-0.5f, retrieved[sample_count * channels], 1e-6f, "Second part should be -0.5");
}

TEST(chunk_data_pad) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t initial_samples = 100;
    const uint32_t pad_samples = 50;
    
    std::vector<audio_sample> data(initial_samples * channels, 0.5f);
    chunk->set_data(data.data(), initial_samples, channels, sample_rate);
    
    // 填充静音
    chunk->data_pad(pad_samples);
    assert_equals(initial_samples + pad_samples, chunk->get_sample_count(), "Sample count should include padding");
    
    // 验证填充部分是静音（0.0f）
    const audio_sample* retrieved = chunk->get_data();
    assert_float_equals(0.0f, retrieved[initial_samples * channels], 1e-6f, "Padding should be silence");
}

TEST(chunk_get_channel_data) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    // 创建测试数据（L 通道 = 0.5, R 通道 = -0.5）
    std::vector<audio_sample> data(sample_count * channels);
    for (uint32_t i = 0; i < sample_count; ++i) {
        data[i * channels + 0] = 0.5f;   // Left
        data[i * channels + 1] = -0.5f;  // Right
    }
    
    chunk->set_data(data.data(), sample_count, channels, sample_rate);
    
    // 测试通道数据访问（注意：foobar2000 使用交错格式 LRLRLR...）
    audio_sample* left_data = chunk->get_channel_data(0);
    audio_sample* right_data = chunk->get_channel_data(1);
    
    assert_true(left_data != nullptr, "Left channel data should not be null");
    assert_true(right_data != nullptr, "Right channel data should not be null");
    
    // 在交错格式中，left_data 指向 data[0]，right_data 指向 data[1]
    // 每个通道的数据不是连续的！
    assert_float_equals(0.5f, left_data[0], 1e-6f, "Left channel first sample");
    assert_float_equals(-0.5f, right_data[0], 1e-6f, "Right channel first sample");
}

TEST(chunk_set_channels) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    std::vector<audio_sample> data(sample_count * channels, 0.5f);
    chunk->set_data(data.data(), sample_count, channels, sample_rate);
    
    // 移除一个通道（保留数据）
    chunk->set_channels(1, true);
    assert_equals(1, chunk->get_channels(), "Should have 1 channel now");
    assert_equals(sample_count, chunk->get_sample_count(), "Sample count should remain");
}

TEST(chunk_scale_uniform) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    std::vector<audio_sample> data(sample_count * channels, 0.5f);
    chunk->set_data(data.data(), sample_count, channels, sample_rate);
    
    // 应用 0.5 增益（Volume -6dB）
    chunk->scale(0.5f);
    
    // 验证所有样本被缩放
    const audio_sample* retrieved = chunk->get_data();
    assert_float_equals(0.25f, retrieved[0], 1e-6f, "Sample should be scaled to 0.25");
}

TEST(chunk_scale_per_channel) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    std::vector<audio_sample> data(sample_count * channels, 0.5f);
    chunk->set_data(data.data(), sample_count, channels, sample_rate);
    
    // 不同通道应用不同增益
    audio_sample gains[2] = {0.5f, 2.0f};
    chunk->scale(gains);
    
    // 验证
    const audio_sample* retrieved = chunk->get_data();
    assert_float_equals(0.25f, retrieved[0], 1e-6f, "Left should be scaled to 0.25");
    assert_float_equals(1.0f, retrieved[1], 1e-6f, "Right should be scaled to 1.0");
}

TEST(chunk_calculate_peak) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    // 创建不同峰值的数据
    std::vector<audio_sample> data(sample_count * channels);
    audio_sample left_peak = 0.0f;
    audio_sample right_peak = 0.0f;
    
    for (uint32_t i = 0; i < sample_count; ++i) {
        audio_sample left = std::sin(2.0f * M_PI * i / sample_count) * 0.8f;
        audio_sample right = std::cos(2.0f * M_PI * i / sample_count) * 0.6f;
        
        data[i * channels + 0] = left;
        data[i * channels + 1] = right;
        
        left_peak = std::max(left_peak, std::abs(left));
        right_peak = std::max(right_peak, std::abs(right));
    }
    
    chunk->set_data(data.data(), sample_count, channels, sample_rate);
    
    // 计算峰值
    std::vector<audio_sample> peaks(channels);
    chunk->calculate_peak(peaks.data());
    
    assert_float_equals(left_peak, peaks[0], 1e-6f, "Left peak should match");
    assert_float_equals(right_peak, peaks[1], 1e-6f, "Right peak should match");
}

TEST(chunk_copy) {
    auto chunk1 = audio_chunk_create();
    auto chunk2 = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    std::vector<audio_sample> data(sample_count * channels, 0.5f);
    chunk1->set_data(data.data(), sample_count, channels, sample_rate);
    
    // 复制到 chunk2
    chunk2->copy(*chunk1);
    
    assert_equals(sample_count, chunk2->get_sample_count(), "Copied chunk should have same samples");
    assert_equals(channels, chunk2->get_channels(), "Copied chunk should have same channels");
    
    // 验证数据相等
    assert_true(chunk2->audio_data_equals(*chunk1), "Chunks should be equal");
}

TEST(chunk_reset) {
    auto chunk = audio_chunk_create();
    
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 2;
    const uint32_t sample_count = 100;
    
    std::vector<audio_sample> data(sample_count * channels, 0.5f);
    chunk->set_data(data.data(), sample_count, channels, sample_rate);
    
    assert_equals(sample_count, chunk->get_sample_count(), "Should have data before reset");
    
    // 重置
    chunk->reset();
    
    assert_equals(0, chunk->get_sample_count(), "Should have no data after reset");
    assert_equals(2, chunk->get_channels(), "Should have default channels after reset");
    assert_equals(44100, chunk->get_sample_rate(), "Should have default rate after reset");
}

//==============================================================================
// 主函数
//==============================================================================

int main() {
    try {
        run_tests();
        
        if (failed > 0) {
            std::cerr << "\n❌ audio_chunk tests FAILED" << std::endl;
            return 1;
        } else {
            std::cout << "\n✅ All audio_chunk tests PASSED" << std::endl;
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
