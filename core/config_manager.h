#pragma once

#include "mp_types.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <functional>

namespace mp {
namespace core {

// Forward declarations
class ConfigValue;

// Configuration value types
enum class ConfigType {
    String,
    Integer,
    Float,
    Boolean,
    Array,
    Object
};

// Configuration value wrapper
class ConfigValue {
public:
    ConfigValue();
    explicit ConfigValue(const std::string& value);
    explicit ConfigValue(int value);
    explicit ConfigValue(double value);
    explicit ConfigValue(bool value);
    
    ConfigType get_type() const { return type_; }
    
    // Type getters
    std::string as_string(const std::string& default_val = "") const;
    int as_int(int default_val = 0) const;
    double as_float(double default_val = 0.0) const;
    bool as_bool(bool default_val = false) const;
    
    // Type setters
    void set_string(const std::string& value);
    void set_int(int value);
    void set_float(double value);
    void set_bool(bool value);
    
    // Check if value exists
    bool is_valid() const { return type_ != ConfigType::Object || !str_value_.empty(); }
    
private:
    ConfigType type_;
    std::string str_value_;
    double num_value_;
    bool bool_value_;
};

// Configuration section (group of related settings)
class ConfigSection {
public:
    ConfigSection(const std::string& name);
    ~ConfigSection();
    
    // Get section name
    const std::string& get_name() const { return name_; }
    
    // Value access
    ConfigValue get_value(const std::string& key) const;
    void set_value(const std::string& key, const ConfigValue& value);
    
    // Convenience methods
    std::string get_string(const std::string& key, const std::string& default_val = "") const;
    int get_int(const std::string& key, int default_val = 0) const;
    double get_float(const std::string& key, double default_val = 0.0) const;
    bool get_bool(const std::string& key, bool default_val = false) const;
    
    void set_string(const std::string& key, const std::string& value);
    void set_int(const std::string& key, int value);
    void set_float(const std::string& key, double value);
    void set_bool(const std::string& key, bool value);
    
    // Check if key exists
    bool has_key(const std::string& key) const;
    
    // Remove key
    void remove_key(const std::string& key);
    
    // Get all keys
    std::vector<std::string> get_keys() const;
    
    // Clear all values
    void clear();
    
    // Internal access for serialization
    const std::map<std::string, ConfigValue>& get_values() const { return values_; }
    
private:
    std::string name_;
    std::map<std::string, ConfigValue> values_;
    mutable std::mutex mutex_;
};

// Configuration change callback
using ConfigChangeCallback = std::function<void(const std::string& section, const std::string& key)>;

// Configuration manager - handles application configuration
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Initialize with config file path
    Result initialize(const std::string& config_path);
    
    // Shutdown and save configuration
    void shutdown();
    
    // Load configuration from file
    Result load();
    
    // Save configuration to file
    Result save();
    
    // Get or create a configuration section
    ConfigSection* get_section(const std::string& section_name);
    
    // Check if section exists
    bool has_section(const std::string& section_name) const;
    
    // Remove section
    void remove_section(const std::string& section_name);
    
    // Get all section names
    std::vector<std::string> get_sections() const;
    
    // Convenience methods for global settings
    std::string get_string(const std::string& section, const std::string& key, 
                          const std::string& default_val = "") const;
    int get_int(const std::string& section, const std::string& key, int default_val = 0) const;
    double get_float(const std::string& section, const std::string& key, double default_val = 0.0) const;
    bool get_bool(const std::string& section, const std::string& key, bool default_val = false) const;
    
    void set_string(const std::string& section, const std::string& key, const std::string& value);
    void set_int(const std::string& section, const std::string& key, int value);
    void set_float(const std::string& section, const std::string& key, double value);
    void set_bool(const std::string& section, const std::string& key, bool value);
    
    // Register change notification callback
    void register_change_callback(ConfigChangeCallback callback);
    
    // Auto-save control
    void set_auto_save(bool enabled) { auto_save_ = enabled; }
    bool get_auto_save() const { return auto_save_; }
    
    // Get config file path
    const std::string& get_config_path() const { return config_path_; }
    
    // Schema version for migration
    int get_schema_version() const { return schema_version_; }
    void set_schema_version(int version) { schema_version_ = version; }
    
private:
    void notify_change(const std::string& section, const std::string& key);
    Result parse_json(const std::string& json_content);
    std::string serialize_to_json() const;
    
    std::string config_path_;
    std::map<std::string, std::unique_ptr<ConfigSection>> sections_;
    mutable std::mutex mutex_;
    std::vector<ConfigChangeCallback> change_callbacks_;
    bool auto_save_;
    bool initialized_;
    int schema_version_;
};

}} // namespace mp::core
