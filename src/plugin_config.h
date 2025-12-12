/**
 * @file plugin_config.h
 * @brief Plugin configuration and parameter management system
 * @date 2025-12-13
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <memory>
#include <functional>

// Configuration value types
using ConfigValue = std::variant<bool, int, double, std::string>;

/**
 * @brief Plugin configuration parameter definition
 */
struct ConfigParam {
    std::string key;                    // Parameter key
    std::string name;                    // Display name
    std::string description;            // Description
    ConfigValue default_value;          // Default value
    ConfigValue min_value;              // Minimum value (for numeric)
    ConfigValue max_value;              // Maximum value (for numeric)
    std::vector<std::string> options;   // Available options (for enum)

    ConfigParam(const std::string& k, const std::string& n, const std::string& desc,
              const ConfigValue& def)
        : key(k), name(n), description(desc), default_value(def) {}
};

/**
 * @brief Plugin configuration section
 */
class ConfigSection {
private:
    std::string plugin_name_;
    std::map<std::string, ConfigParam> params_;
    std::map<std::string, ConfigValue> values_;
    bool enabled_;
    int priority_;  // Plugin load priority

public:
    ConfigSection(const std::string& plugin_name)
        : plugin_name_(plugin_name), enabled_(true), priority_(100) {}

    // Add configuration parameter
    void add_param(const ConfigParam& param);

    // Get/set parameter values
    bool set_value(const std::string& key, const ConfigValue& value);
    ConfigValue get_value(const std::string& key) const;
    ConfigValue get_value_or_default(const std::string& key) const;

    // Get parameter definition
    const ConfigParam* get_param(const std::string& key) const;
    std::vector<ConfigParam> get_all_params() const;

    // Enable/disable plugin
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

    // Priority management
    void set_priority(int priority) { priority_ = priority; }
    int get_priority() const { return priority_; }

    // Plugin name
    const std::string& get_plugin_name() const { return plugin_name_; }

    // Validation
    bool validate_values() const;

    // Reset to defaults
    void reset_to_defaults();

    // Export/Import
    std::string export_to_json() const;
    bool import_from_json(const std::string& json);
};

/**
 * @brief Global plugin configuration manager
 */
class PluginConfigManager {
private:
    std::map<std::string, std::unique_ptr<ConfigSection>> sections_;
    std::string config_file_path_;
    bool auto_save_;
    std::vector<std::function<void(const std::string&, const std::string&, const ConfigValue&)>> change_listeners_;

public:
    PluginConfigManager();
    ~PluginConfigManager();

    // Initialize with config file
    bool initialize(const std::string& config_file = "plugins_config.json", bool auto_save = true);

    // Save/Load configuration
    bool save_config();
    bool load_config();

    // Get or create plugin section
    ConfigSection* get_section(const std::string& plugin_name);
    const ConfigSection* get_section(const std::string& plugin_name) const;
    ConfigSection* create_section(const std::string& plugin_name);

    // Remove plugin section
    void remove_section(const std::string& plugin_name);

    // Global settings
    void set_auto_save(bool auto_save) { auto_save_ = auto_save; }
    bool get_auto_save() const { return auto_save_; }

    // Get all plugin sections
    std::vector<ConfigSection*> get_all_sections();

    // Change notifications
    void add_change_listener(std::function<void(const std::string&, const std::string&, const ConfigValue&)> listener);
    void notify_change(const std::string& plugin_name, const std::string& param_key, const ConfigValue& value);

    // Configuration validation
    bool validate_all_configs() const;

    // Generate default configuration
    void generate_defaults();

    // Export/Import full configuration
    std::string export_full_config() const;
    bool import_full_config(const std::string& json);

    // Plugin enable/disable management
    std::vector<std::string> get_enabled_plugins() const;
    std::vector<std::string> get_disabled_plugins() const;

private:
    // Helper functions
    std::string escape_json_string(const std::string& str) const;
    std::string unescape_json_string(const std::string& str) const;
    std::string value_to_json(const ConfigValue& value) const;
    ConfigValue json_to_value(const std::string& json, const std::string& type) const;
};