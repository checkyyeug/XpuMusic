/**
 * @file test_plugin_config.cpp
 * @brief Test plugin configuration and parameter management
 * @date 2025-12-13
 */

#include <iostream>
#include <iomanip>
#include "src/foobar_plugin_manager.h"
#include "compat/sdk_implementations/foobar_sdk_wrapper.h"

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void test_basic_config() {
    print_separator("Testing Basic Configuration");

    FoobarPluginManager plugin_manager;

    // Initialize with custom config file
    if (!plugin_manager.initialize("test_plugins_config.json")) {
        std::cerr << "Failed to initialize plugin manager with config" << std::endl;
        return;
    }

    std::cout << "Plugin manager initialized successfully!" << std::endl;
}

void test_parameter_management() {
    print_separator("Testing Parameter Management");

    FoobarPluginManager plugin_manager;
    plugin_manager.initialize("test_plugins_config.json");

    // Test setting different parameter types
    std::cout << "\n1. Setting boolean parameter:" << std::endl;
    plugin_manager.set_plugin_parameter("test_plugin", "enable_feature", true);
    auto bool_result = plugin_manager.get_plugin_parameter("test_plugin", "enable_feature", false);
    bool bool_val = std::get<bool>(bool_result);
    std::cout << "   enable_feature = " << (bool_val ? "true" : "false") << std::endl;

    std::cout << "\n2. Setting integer parameter:" << std::endl;
    plugin_manager.set_plugin_parameter("test_plugin", "max_threads", 8);
    auto int_result = plugin_manager.get_plugin_parameter("test_plugin", "max_threads", 4);
    int int_val = std::get<int>(int_result);
    std::cout << "   max_threads = " << int_val << std::endl;

    std::cout << "\n3. Setting double parameter:" << std::endl;
    plugin_manager.set_plugin_parameter("test_plugin", "quality_factor", 0.95);
    auto double_result = plugin_manager.get_plugin_parameter("test_plugin", "quality_factor", 0.5);
    double double_val = std::get<double>(double_result);
    std::cout << "   quality_factor = " << std::fixed << std::setprecision(2) << double_val << std::endl;

    std::cout << "\n4. Setting string parameter:" << std::endl;
    plugin_manager.set_plugin_parameter("test_plugin", "output_format", std::string("pcm"));
    auto str_result = plugin_manager.get_plugin_parameter("test_plugin", "output_format", std::string("default"));
    std::string str_val = std::get<std::string>(str_result);
    std::cout << "   output_format = " << str_val << std::endl;
}

void test_plugin_enable_disable() {
    print_separator("Testing Plugin Enable/Disable");

    FoobarPluginManager plugin_manager;
    plugin_manager.initialize("test_plugins_config.json");

    // Test multiple plugins
    std::vector<std::string> plugins = {"mp3_decoder", "flac_decoder", "ogg_decoder", "wav_decoder"};

    std::cout << "\nConfiguring plugins:" << std::endl;
    for (const auto& plugin : plugins) {
        plugin_manager.set_plugin_enabled(plugin, true);
        std::cout << "  " << plugin << ": enabled" << std::endl;
    }

    // Disable some plugins
    plugin_manager.set_plugin_enabled("ogg_decoder", false);
    std::cout << "\nAfter disabling ogg_decoder:" << std::endl;
    for (const auto& plugin : plugins) {
        bool enabled = plugin_manager.is_plugin_enabled(plugin);
        std::cout << "  " << plugin << ": " << (enabled ? "enabled" : "disabled") << std::endl;
    }

    // Re-enable
    plugin_manager.set_plugin_enabled("ogg_decoder", true);
    std::cout << "\nRe-enabled ogg_decoder: " <<
        (plugin_manager.is_plugin_enabled("ogg_decoder") ? "enabled" : "disabled") << std::endl;
}

void test_config_persistence() {
    print_separator("Testing Configuration Persistence");

    std::string config_file = "persistence_test_config.json";

    // Phase 1: Save configuration
    {
        std::cout << "\nPhase 1: Creating and saving configuration..." << std::endl;
        FoobarPluginManager manager1;
        manager1.initialize(config_file);

        manager1.set_plugin_parameter("mp3_decoder", "quality", 5);
        manager1.set_plugin_parameter("flac_decoder", "verify", false);
        manager1.set_plugin_enabled("mp3_decoder", true);
        manager1.set_plugin_enabled("flac_decoder", false);

        if (!manager1.save_configuration()) {
            std::cerr << "Failed to save configuration!" << std::endl;
            return;
        }
        std::cout << "Configuration saved to: " << config_file << std::endl;
    }

    // Phase 2: Load configuration
    {
        std::cout << "\nPhase 2: Loading configuration..." << std::endl;
        FoobarPluginManager manager2;
        manager2.initialize(config_file);

        // Verify loaded values
        auto mp3_result = manager2.get_plugin_parameter("mp3_decoder", "quality", 0);
        auto flac_result = manager2.get_plugin_parameter("flac_decoder", "verify", true);
        int mp3_quality = std::get<int>(mp3_result);
        bool flac_verify = std::get<bool>(flac_result);
        bool mp3_enabled = manager2.is_plugin_enabled("mp3_decoder");
        bool flac_enabled = manager2.is_plugin_enabled("flac_decoder");

        std::cout << "\nLoaded configuration:" << std::endl;
        std::cout << "  mp3_decoder.quality = " << mp3_quality << std::endl;
        std::cout << "  mp3_decoder.enabled = " << (mp3_enabled ? "true" : "false") << std::endl;
        std::cout << "  flac_decoder.verify = " << (flac_verify ? "true" : "false") << std::endl;
        std::cout << "  flac_decoder.enabled = " << (flac_enabled ? "true" : "false") << std::endl;

        // Verify values match what we saved
        bool success = (mp3_quality == 5) && (flac_verify == false) &&
                       mp3_enabled && !flac_enabled;
        std::cout << "\nPersistence test " << (success ? "PASSED" : "FAILED") << std::endl;
    }
}

void test_config_validation() {
    print_separator("Testing Configuration Validation");

    FoobarPluginManager plugin_manager;
    plugin_manager.initialize("validation_test_config.json");

    // Get config manager directly to test validation
    auto* config_mgr = plugin_manager.get_config_manager();

    std::cout << "\nCreating plugin section with parameters..." << std::endl;
    auto* section = config_mgr->create_section("validation_test_plugin");

    // Add parameters with constraints
    ConfigParam param1("volume", "Volume", "Output volume level", 100);
    param1.min_value = 0;
    param1.max_value = 200;
    section->add_param(param1);

    ConfigParam param2("sample_rate", "Sample Rate", "Audio sample rate", 44100);
    std::vector<std::string> sample_rates = {"22050", "44100", "48000", "96000"};
    param2.options = sample_rates;
    section->add_param(param2);

    std::cout << "Added parameters with constraints." << std::endl;

    // Test validation
    bool valid = config_mgr->validate_all_configs();
    std::cout << "\nConfiguration validation: " << (valid ? "PASSED" : "FAILED") << std::endl;

    // Test parameter values
    std::cout << "\nCurrent parameter values:" << std::endl;
    auto volume_result = section->get_value("volume");
    auto sample_rate_result = section->get_value("sample_rate");
    int volume = std::get<int>(volume_result);
    int sample_rate = std::get<int>(sample_rate_result);
    std::cout << "  volume = " << volume << " (0-200)" << std::endl;
    std::cout << "  sample_rate = " << sample_rate << " Hz" << std::endl;
}

void test_config_export_import() {
    print_separator("Testing Configuration Export/Import");

    FoobarPluginManager plugin_manager;
    plugin_manager.initialize("export_test_config.json");

    // Configure some plugins
    plugin_manager.set_plugin_parameter("mp3_decoder", "quality", 3);
    plugin_manager.set_plugin_parameter("mp3_decoder", "buffer_size", 65536);
    plugin_manager.set_plugin_enabled("mp3_decoder", true);

    plugin_manager.set_plugin_parameter("flac_decoder", "verify", true);
    plugin_manager.set_plugin_enabled("flac_decoder", false);

    // Export configuration
    std::cout << "\nExporting configuration..." << std::endl;
    std::string exported = plugin_manager.get_config_manager()->export_full_config();

    // Save exported config to file for inspection
    std::ofstream export_file("exported_config.json");
    if (export_file.is_open()) {
        export_file << exported;
        export_file.close();
        std::cout << "Configuration exported to: exported_config.json" << std::endl;
    }

    // Create new manager and import
    std::cout << "\nImporting configuration to new manager..." << std::endl;
    FoobarPluginManager manager2;
    manager2.initialize("import_test_config.json");

    bool imported = manager2.get_config_manager()->import_full_config(exported);
    std::cout << "Import result: " << (imported ? "SUCCESS" : "FAILED") << std::endl;

    // Verify imported values
    if (imported) {
        auto quality_result = manager2.get_plugin_parameter("mp3_decoder", "quality", 0);
        auto verify_result = manager2.get_plugin_parameter("flac_decoder", "verify", true);
        int quality = std::get<int>(quality_result);
        bool verify = std::get<bool>(verify_result);
        bool mp3_enabled = manager2.is_plugin_enabled("mp3_decoder");
        bool flac_enabled = manager2.is_plugin_enabled("flac_decoder");

        std::cout << "\nImported values:" << std::endl;
        std::cout << "  mp3_decoder.quality = " << quality << std::endl;
        std::cout << "  mp3_decoder.enabled = " << (mp3_enabled ? "true" : "false") << std::endl;
        std::cout << "  flac_decoder.verify = " << (verify ? "true" : "false") << std::endl;
        std::cout << "  flac_decoder.enabled = " << (flac_enabled ? "true" : "false") << std::endl;
    }
}

void show_statistics() {
    print_separator("Plugin Statistics");

    FoobarPluginManager plugin_manager;
    plugin_manager.initialize("");

    auto* config_mgr = plugin_manager.get_config_manager();
    auto sections = config_mgr->get_all_sections();

    std::cout << "\nConfiguration Statistics:" << std::endl;
    std::cout << "  Total plugin sections: " << sections.size() << std::endl;

    int total_params = 0;
    int enabled_plugins = 0;
    int disabled_plugins = 0;

    for (auto* section : sections) {
        auto params = section->get_all_params();
        total_params += params.size();

        if (section->is_enabled()) {
            enabled_plugins++;
        } else {
            disabled_plugins++;
        }
    }

    std::cout << "  Total parameters: " << total_params << std::endl;
    std::cout << "  Enabled plugins: " << enabled_plugins << std::endl;
    std::cout << "  Disabled plugins: " << disabled_plugins << std::endl;

    // Show default configurations
    std::cout << "\nDefault plugin configurations:" << std::endl;
    auto enabled = config_mgr->get_enabled_plugins();
    auto disabled = config_mgr->get_disabled_plugins();

    if (!enabled.empty()) {
        std::cout << "  Enabled by default: ";
        for (size_t i = 0; i < enabled.size(); i++) {
            std::cout << enabled[i];
            if (i < enabled.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }

    if (!disabled.empty()) {
        std::cout << "  Disabled by default: ";
        for (size_t i = 0; i < disabled.size(); i++) {
            std::cout << disabled[i];
            if (i < disabled.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Plugin Configuration Test    " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_basic_config();
        test_parameter_management();
        test_plugin_enable_disable();
        test_config_persistence();
        test_config_validation();
        test_config_export_import();
        show_statistics();

        std::cout << "\n========================================" << std::endl;
        std::cout << "   All tests completed!          " << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }

    // Cleanup generated files
    remove("test_plugins_config.json");
    remove("persistence_test_config.json");
    remove("validation_test_config.json");
    remove("export_test_config.json");
    remove("import_test_config.json");
    remove("exported_config.json");

    return 0;
}