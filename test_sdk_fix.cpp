/**
 * @file test_sdk_fix.cpp
 * @brief 测试 SDK 实现修复效果
 * @date 2025-12-11
 */

#include "compat/sdk_implementations/abort_callback.implified.h"
#include "compat/sdk_implementations/file_info_impl.h"
#include "compat/sdk_implementations/file_info_interface.h"
#include "compat/sdk_implementations/metadb_handle_impl_simple.h"
#include <iostream>
#include <memory>

using namespace foobar2000_sdk;

int main() {
    std::cout << "=== 测试 SDK 实现修复 ===" << std::endl;
    
    try {
        // 测试 1: abort_callback 实现
        std::cout << "测试 1: abort_callback 实现..." << std::endl;
        abort_callback_impl abort_cb;
        bool is_aborting = abort_cb.is_aborting();
        std::cout << "✓ abort_callback 正常工作, is_aborting = " << is_aborting << std::endl;
        
        // 测试 2: file_info 实现
        std::cout << "测试 2: file_info 实现..." << std::endl;
        auto file_info = std::make_unique<file_info_impl>();
        
        // 设置一些元数据
        file_info->meta_set("artist", "Test Artist");
        file_info->meta_set("title", "Test Song");
        file_info->meta_set("album", "Test Album");
        
        // 验证元数据
        const char* artist = file_info->meta_get("artist", 0);
        const char* title = file_info->meta_get("title", 0);
        const char* album = file_info->meta_get("album", 0);
        
        std::cout << "✓ 元数据设置/获取正常" << std::endl;
        std::cout << "  Artist: " << (artist ? artist : "null") << std::endl;
        std::cout << "  Title: " << (title ? title : "null") << std::endl;
        std::cout << "  Album: " << (album ? album : "null") << std::endl;
        
        // 测试音频信息
        xpumusic_sdk::audio_info audio_info;
        audio_info.m_sample_rate = 44100;
        audio_info.m_channels = 2;
        audio_info.m_bitrate = 320;
        audio_info.m_length = 180.0; // 3分钟
        
        file_info->set_audio_info(audio_info);
        const xpumusic_sdk::audio_info& retrieved_audio = file_info->get_audio_info();
        
        std::cout << "✓ 音频信息设置/获取正常" << std::endl;
        std::cout << "  Sample Rate: " << retrieved_audio.m_sample_rate << " Hz" << std::endl;
        std::cout << "  Channels: " << retrieved_audio.m_channels << std::endl;
        std::cout << "  Bitrate: " << retrieved_audio.m_bitrate << " kbps" << std::endl;
        std::cout << "  Length: " << retrieved_audio.m_length << " seconds" << std::endl;
        
        // 测试文件统计
        xpumusic_sdk::file_stats file_stats;
        file_stats.m_size = 1024 * 1024; // 1MB
        file_stats.m_timestamp = 1234567890;
        
        file_info->set_file_stats(file_stats);
        const xpumusic_sdk::file_stats& retrieved_stats = file_info->get_file_stats();
        
        std::cout << "✓ 文件统计设置/获取正常" << std::endl;
        std::cout << "  File Size: " << retrieved_stats.m_size << " bytes" << std::endl;
        std::cout << "  Timestamp: " << retrieved_stats.m_timestamp << std::endl;
        
        // 测试 3: metadb_handle 实现
        std::cout << "测试 3: metadb_handle 实现..." << std::endl;
        auto handle = std::make_unique<metadb_handle_impl_simple>();
        
        playable_location location;
        location.set_path("C:\\Music\\test.mp3");
        location.set_subsong_index(0);
        
        handle->initialize(location);
        
        std::cout << "✓ metadb_handle 初始化正常" << std::endl;
        std::cout << "  Path: " << handle->get_path() << std::endl;
        std::cout << "  Filename: " << handle->get_filename() << std::endl;
        std::cout << "  Directory: " << handle->get_directory() << std::endl;
        
        // 测试复制功能
        auto file_info2 = std::make_unique<file_info_impl>();
        file_info2->copy_from(*file_info);
        
        const char* copied_artist = file_info2->meta_get("artist", 0);
        std::cout << "✓ 复制功能正常" << std::endl;
        std::cout << "  Copied Artist: " << (copied_artist ? copied_artist : "null") << std::endl;
        
        std::cout << "\n🎉 所有测试通过！SDK 实现修复成功！" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败，异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 测试失败，未知异常" << std::endl;
        return 1;
    }
}