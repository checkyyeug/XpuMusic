/**
 * @file test_foobar_plugins.cpp
 * @brief Test foobar2000 plugin manager functionality
 * @date 2025-12-13
 */

#include <iostream>
#include "src/foobar_plugin_manager.h"
#include "compat/sdk_implementations/foobar_sdk_wrapper.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Foobar2000 Plugin Manager Test   " << std::endl;
    std::cout << "========================================" << std::endl;

    // Initialize plugin manager
    FoobarPluginManager plugin_manager;
    if (!plugin_manager.initialize("")) {
        std::cerr << "Failed to initialize plugin manager!" << std::endl;
        return 1;
    }

    // Test loading plugins from common directories
    std::vector<std::string> plugin_dirs = {
        "C:\\Program Files (x86)\\foobar2000\\components",
        "C:\\Program Files\\foobar2000\\components",
        "./plugins",
        "./foobar_plugins"
    };

    bool loaded_any = false;
    for (const auto& dir : plugin_dirs) {
        std::cout << "\nScanning plugin directory: " << dir << std::endl;
        if (plugin_manager.load_plugins_from_directory(dir)) {
            loaded_any = true;
        }
    }

    // Show loaded plugins
    const auto& plugins = plugin_manager.get_loaded_plugins();
    std::cout << "\n=== Loaded Plugins ===" << std::endl;
    if (plugins.empty()) {
        std::cout << "No plugins loaded." << std::endl;
    } else {
        for (size_t i = 0; i < plugins.size(); i++) {
            const auto& plugin = plugins[i];
            std::cout << "\nPlugin #" << (i + 1) << ":" << std::endl;
            std::cout << "  Name: " << plugin.name << std::endl;
            std::cout << "  Version: " << plugin.version << std::endl;
            std::cout << "  Description: " << plugin.description << std::endl;
            std::cout << "  Path: " << plugin.file_path << std::endl;
            std::cout << "  Supported extensions: ";
            for (size_t j = 0; j < plugin.supported_extensions.size(); j++) {
                std::cout << plugin.supported_extensions[j];
                if (j < plugin.supported_extensions.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
    }

    // Show supported file extensions
    auto extensions = plugin_manager.get_supported_extensions();
    std::cout << "\n=== Supported File Extensions ===" << std::endl;
    if (extensions.empty()) {
        std::cout << "No supported extensions." << std::endl;
    } else {
        for (size_t i = 0; i < extensions.size(); i++) {
            std::cout << extensions[i];
            if (i < extensions.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }

    // Test decoder lookup
    std::cout << "\n=== Testing Decoder Lookup ===" << std::endl;
    std::vector<std::string> test_files = {
        "test.mp3",
        "test.flac",
        "test.ogg",
        "test.wav",
        "unknown.xyz"
    };

    for (const auto& file : test_files) {
        auto decoder = plugin_manager.find_decoder(file);
        if (decoder) {
            std::cout << "✓ Found decoder for: " << file << std::endl;

            // Test opening and getting info
            if (decoder->open(file)) {
                xpumusic_sdk::audio_info info;
                if (decoder->get_audio_info(info)) {
                    std::cout << "  Sample Rate: " << info.m_sample_rate << " Hz" << std::endl;
                    std::cout << "  Channels: " << info.m_channels << std::endl;
                    std::cout << "  Bitrate: " << info.m_bitrate << " bps" << std::endl;
                    std::cout << "  Duration: " << info.m_length << " seconds" << std::endl;
                }
                decoder->close();
            }
        } else {
            std::cout << "✗ No decoder found for: " << file << std::endl;
        }
    }

    // Test error handling
    std::cout << "\n=== Testing Error Handling ===" << std::endl;

    // Test loading non-existent plugin
    std::cout << "\n1. Testing non-existent plugin:" << std::endl;
    if (!plugin_manager.load_plugin("non_existent_plugin.dll")) {
        std::cout << "✓ Correctly rejected non-existent plugin" << std::endl;
    }

    // Test loading invalid file
    std::cout << "\n2. Testing invalid file (test_foobar_plugins.exe):" << std::endl;
    if (!plugin_manager.load_plugin("test_foobar_plugins.exe")) {
        std::cout << "✓ Correctly rejected invalid file format" << std::endl;
    }

    // Create an empty file for testing
    std::ofstream empty_file("empty_test.dll");
    empty_file.close();
    std::cout << "\n3. Testing empty file:" << std::endl;
    if (!plugin_manager.load_plugin("empty_test.dll")) {
        std::cout << "✓ Correctly rejected empty file" << std::endl;
    }
    remove("empty_test.dll");

    // Get error handler and show statistics
    std::cout << "\n=== Error Statistics ===" << std::endl;
    auto error_handler = plugin_manager.get_error_handler();
    if (error_handler) {
        auto stats = error_handler->get_statistics();
        std::cout << "Total Errors: " << stats.total_errors << std::endl;
        std::cout << "Critical Errors: " << stats.critical_errors << std::endl;
        std::cout << "Plugin Load Failures: " << stats.plugin_load_failures << std::endl;
        std::cout << "Runtime Errors: " << stats.runtime_errors << std::endl;

        // Show recent errors
        std::cout << "\n=== Recent Errors (Last 5) ===" << std::endl;
        auto recent_errors = error_handler->get_recent_errors(5);
        for (const auto& error : recent_errors) {
            std::cout << "- " << error.message << std::endl;
        }
    }

    // Generate full error report
    std::cout << "\n=== Full Error Report ===" << std::endl;
    std::string report = plugin_manager.generate_error_report();
    std::cout << report << std::endl;

    // Write report to file
    std::ofstream report_file("plugin_error_report.txt");
    if (report_file.is_open()) {
        report_file << report;
        report_file.close();
        std::cout << "\nError report saved to: plugin_error_report.txt" << std::endl;
    }

    // Cleanup
    plugin_manager.shutdown();

    std::cout << "\nPlugin manager test completed!" << std::endl;
    return 0;
}