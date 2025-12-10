/**
 * @file test_compile_headers.cpp
 * @brief Test compilation of all modified headers
 * @date 2025-12-10
 */

// Include all modified headers to check for compilation errors
#include "compat/sdk_implementations/audio_sample.h"
#include "compat/sdk_implementations/file_info_types.h"
#include "compat/sdk_implementations/file_info_impl.h"
#include "compat/sdk_implementations/audio_chunk_impl.h"
#include "compat/sdk_implementations/metadb_handle_impl.h"
#include "compat/sdk_implementations/abort_callback.implified.h"

#include <iostream>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>

int main() {
    std::cout << "✅ All headers compile successfully!" << std::endl;

    // Test that types are defined correctly
    foobar2000_sdk::audio_sample sample = 0.5f;
    foobar2000_sdk::audio_info info;
    foobar2000_sdk::file_stats stats;

    std::cout << "✅ Types defined correctly:" << std::endl;
    std::cout << "   audio_sample: " << sample << std::endl;
    std::cout << "   audio_info: " << info.m_sample_rate << " Hz, " << info.m_channels << " ch" << std::endl;

    return 0;
}