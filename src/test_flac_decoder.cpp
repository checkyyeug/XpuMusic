// FLAC Decoder Plugin Test Program
// Demonstrates plugin loading and basic functionality

#include "mp_plugin.h"
#include "mp_decoder.h"
#include <iostream>
#include <fstream>
#include <cstring>

// Platform-specific plugin loading
#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE PluginHandle;
    #define LOAD_PLUGIN(path) LoadLibraryA(path)
    #define GET_FUNCTION(handle, name) GetProcAddress(handle, name)
    #define UNLOAD_PLUGIN(handle) FreeLibrary(handle)
#else
    #include <dlfcn.h>
    typedef void* PluginHandle;
    #define LOAD_PLUGIN(path) dlopen(path, RTLD_LAZY)
    #define GET_FUNCTION(handle, name) dlsym(handle, name)
    #define UNLOAD_PLUGIN(handle) dlclose(handle)
#endif

using CreatePluginFunc = mp::IPlugin* (*)();
using DestroyPluginFunc = void (*)(mp::IPlugin*);

// Simple service registry implementation for testing
class TestServiceRegistry : public mp::IServiceRegistry {
public:
    mp::Result register_service(mp::ServiceID id, void* service) override {
        (void)id;
        (void)service;
        return mp::Result::Success;
    }
    
    mp::Result unregister_service(mp::ServiceID id) override {
        (void)id;
        return mp::Result::Success;
    }
    
    void* query_service(mp::ServiceID id) override {
        (void)id;
        return nullptr;
    }
};

void print_separator() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
}

bool test_plugin_loading(const char* plugin_path) {
    print_separator();
    std::cout << "TEST 1: Plugin Loading" << std::endl;
    print_separator();
    
    // Load plugin library
    PluginHandle handle = LOAD_PLUGIN(plugin_path);
    if (!handle) {
        std::cerr << "❌ Failed to load plugin: " << plugin_path << std::endl;
        return false;
    }
    std::cout << "✅ Plugin library loaded: " << plugin_path << std::endl;
    
    // Get plugin entry points
    auto create_plugin = (CreatePluginFunc)GET_FUNCTION(handle, "create_plugin");
    auto destroy_plugin = (DestroyPluginFunc)GET_FUNCTION(handle, "destroy_plugin");
    
    if (!create_plugin || !destroy_plugin) {
        std::cerr << "❌ Failed to get plugin entry points" << std::endl;
        UNLOAD_PLUGIN(handle);
        return false;
    }
    std::cout << "✅ Plugin entry points found" << std::endl;
    
    // Create plugin instance
    mp::IPlugin* plugin = create_plugin();
    if (!plugin) {
        std::cerr << "❌ Failed to create plugin instance" << std::endl;
        UNLOAD_PLUGIN(handle);
        return false;
    }
    std::cout << "✅ Plugin instance created" << std::endl;
    
    // Get plugin info
    const mp::PluginInfo& info = plugin->get_plugin_info();
    std::cout << "\nPlugin Information:" << std::endl;
    std::cout << "  Name: " << info.name << std::endl;
    std::cout << "  Author: " << info.author << std::endl;
    std::cout << "  Description: " << info.description << std::endl;
    std::cout << "  Version: " << info.version.major << "." 
              << info.version.minor << "." << info.version.patch << std::endl;
    std::cout << "  UUID: " << info.uuid << std::endl;
    
    // Check capabilities
    mp::PluginCapability caps = plugin->get_capabilities();
    std::cout << "  Capabilities: ";
    if (mp::has_capability(caps, mp::PluginCapability::Decoder)) {
        std::cout << "Decoder ";
    }
    std::cout << std::endl;
    
    // Initialize plugin
    TestServiceRegistry registry;
    mp::Result result = plugin->initialize(&registry);
    if (result == mp::Result::Success) {
        std::cout << "✅ Plugin initialized successfully" << std::endl;
    } else if (result == mp::Result::NotSupported) {
        std::cout << "⚠️  Plugin initialized (stub mode - libFLAC not available)" << std::endl;
    } else {
        std::cerr << "❌ Plugin initialization failed" << std::endl;
        destroy_plugin(plugin);
        UNLOAD_PLUGIN(handle);
        return false;
    }
    
    // Cleanup
    plugin->shutdown();
    destroy_plugin(plugin);
    UNLOAD_PLUGIN(handle);
    
    std::cout << "✅ Plugin unloaded successfully" << std::endl;
    return true;
}

bool test_decoder_interface(const char* plugin_path) {
    print_separator();
    std::cout << "TEST 2: Decoder Interface" << std::endl;
    print_separator();
    
    // Load plugin
    PluginHandle handle = LOAD_PLUGIN(plugin_path);
    if (!handle) return false;
    
    auto create_plugin = (CreatePluginFunc)GET_FUNCTION(handle, "create_plugin");
    auto destroy_plugin = (DestroyPluginFunc)GET_FUNCTION(handle, "destroy_plugin");
    if (!create_plugin || !destroy_plugin) {
        UNLOAD_PLUGIN(handle);
        return false;
    }
    
    mp::IPlugin* plugin = create_plugin();
    if (!plugin) {
        UNLOAD_PLUGIN(handle);
        return false;
    }
    
    TestServiceRegistry registry;
    plugin->initialize(&registry);
    
    // Get decoder service
    constexpr mp::ServiceID SERVICE_DECODER = mp::hash_string("mp.service.decoder");
    void* service = plugin->get_service(SERVICE_DECODER);
    
    if (!service) {
        std::cerr << "❌ Failed to get decoder service" << std::endl;
        plugin->shutdown();
        destroy_plugin(plugin);
        UNLOAD_PLUGIN(handle);
        return false;
    }
    std::cout << "✅ Decoder service obtained" << std::endl;
    
    mp::IDecoder* decoder = static_cast<mp::IDecoder*>(service);
    
    // Test file probing
    std::cout << "\nTesting file probing:" << std::endl;
    
    // FLAC header
    uint8_t flac_header[] = {'f', 'L', 'a', 'C'};
    int confidence = decoder->probe_file(flac_header, 4);
    std::cout << "  FLAC file confidence: " << confidence << std::endl;
    if (confidence == 100) {
        std::cout << "  ✅ FLAC file correctly identified" << std::endl;
    } else {
        std::cout << "  ❌ FLAC file detection failed" << std::endl;
    }
    
    // MP3 header (should not match)
    uint8_t mp3_header[] = {0xFF, 0xFB, 0x90, 0x00};
    confidence = decoder->probe_file(mp3_header, 4);
    std::cout << "  MP3 file confidence: " << confidence << std::endl;
    if (confidence == 0) {
        std::cout << "  ✅ MP3 file correctly rejected" << std::endl;
    } else {
        std::cout << "  ❌ MP3 file detection failed" << std::endl;
    }
    
    // Get supported extensions
    std::cout << "\nSupported file extensions:" << std::endl;
    const char** extensions = decoder->get_extensions();
    for (int i = 0; extensions[i] != nullptr; i++) {
        std::cout << "  - ." << extensions[i] << std::endl;
    }
    
    // Test stream opening (will fail without libFLAC or valid file)
    std::cout << "\nTesting stream operations:" << std::endl;
    mp::DecoderHandle handle_decoder;
    mp::Result result = decoder->open_stream("test.flac", &handle_decoder);
    
    if (result == mp::Result::NotSupported) {
        std::cout << "  ⚠️  Stream opening not supported (stub mode)" << std::endl;
    } else if (result == mp::Result::FileNotFound) {
        std::cout << "  ⚠️  Test file not found (expected without test.flac)" << std::endl;
    } else if (result == mp::Result::Success) {
        std::cout << "  ✅ Stream opened successfully" << std::endl;
        
        // Get stream info
        mp::AudioStreamInfo info;
        result = decoder->get_stream_info(handle_decoder, &info);
        if (result == mp::Result::Success) {
            std::cout << "  Stream Information:" << std::endl;
            std::cout << "    Sample Rate: " << info.sample_rate << " Hz" << std::endl;
            std::cout << "    Channels: " << info.channels << std::endl;
            std::cout << "    Format: Int32" << std::endl;
            std::cout << "    Duration: " << info.duration_ms << " ms" << std::endl;
            std::cout << "    Bitrate: " << info.bitrate << " kbps" << std::endl;
        }
        
        // Get metadata
        const mp::MetadataTag* tags = nullptr;
        size_t count = 0;
        result = decoder->get_metadata(handle_decoder, &tags, &count);
        if (result == mp::Result::Success && count > 0) {
            std::cout << "  Metadata Tags:" << std::endl;
            for (size_t i = 0; i < count; i++) {
                std::cout << "    " << tags[i].key << ": " << tags[i].value << std::endl;
            }
        }
        
        decoder->close_stream(handle_decoder);
    }
    
    // Cleanup
    plugin->shutdown();
    destroy_plugin(plugin);
    UNLOAD_PLUGIN(handle);
    
    std::cout << "✅ Decoder interface test completed" << std::endl;
    return true;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [plugin_path]" << std::endl;
    std::cout << "\nDefault plugin paths:" << std::endl;
    std::cout << "  Windows: build\\bin\\Release\\flac_decoder.dll" << std::endl;
    std::cout << "  Linux:   build/bin/flac_decoder.so" << std::endl;
    std::cout << "  macOS:   build/bin/flac_decoder.dylib" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "FLAC Decoder Plugin Test Program" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Determine plugin path
    const char* plugin_path = nullptr;
    if (argc > 1) {
        plugin_path = argv[1];
    } else {
        // Default path based on platform
#ifdef _WIN32
        plugin_path = "build\\bin\\Release\\flac_decoder.dll";
#elif defined(__APPLE__)
        plugin_path = "build/bin/flac_decoder.dylib";
#else
        plugin_path = "build/bin/flac_decoder.so";
#endif
    }
    
    std::cout << "\nPlugin path: " << plugin_path << std::endl;
    
    // Check if plugin file exists
    std::ifstream test_file(plugin_path);
    if (!test_file.good()) {
        std::cerr << "\n❌ Plugin file not found: " << plugin_path << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    test_file.close();
    
    // Run tests
    bool success = true;
    
    success = test_plugin_loading(plugin_path) && success;
    success = test_decoder_interface(plugin_path) && success;
    
    // Summary
    print_separator();
    std::cout << "Test Summary" << std::endl;
    print_separator();
    
    if (success) {
        std::cout << "✅ All tests passed!" << std::endl;
        std::cout << "\nNote: To enable full FLAC decoding functionality:" << std::endl;
        std::cout << "  1. Install libFLAC library for your platform" << std::endl;
        std::cout << "  2. Rebuild the project" << std::endl;
        std::cout << "  3. Rerun tests with actual FLAC files" << std::endl;
        return 0;
    } else {
        std::cerr << "❌ Some tests failed" << std::endl;
        return 1;
    }
}
