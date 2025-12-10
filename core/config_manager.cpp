#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Simple JSON parser/serializer (minimal implementation)
// For production, consider using nlohmann/json or RapidJSON

namespace mp {
namespace core {

// Helper functions for JSON parsing
namespace {

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    auto end = str.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::string escape_json_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string unescape_json_string(const std::string& str) {
    std::string result;
    bool escape = false;
    for (char c : str) {
        if (escape) {
            switch (c) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += c; break;
            }
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else {
            result += c;
        }
    }
    return result;
}

} // anonymous namespace

//=============================================================================
// ConfigValue implementation
//=============================================================================

ConfigValue::ConfigValue() 
    : type_(ConfigType::String), num_value_(0.0), bool_value_(false) {
}

ConfigValue::ConfigValue(const std::string& value)
    : type_(ConfigType::String), str_value_(value), num_value_(0.0), bool_value_(false) {
}

ConfigValue::ConfigValue(int value)
    : type_(ConfigType::Integer), num_value_(static_cast<double>(value)), bool_value_(false) {
    str_value_ = std::to_string(value);
}

ConfigValue::ConfigValue(double value)
    : type_(ConfigType::Float), num_value_(value), bool_value_(false) {
    str_value_ = std::to_string(value);
}

ConfigValue::ConfigValue(bool value)
    : type_(ConfigType::Boolean), num_value_(0.0), bool_value_(value) {
    str_value_ = value ? "true" : "false";
}

std::string ConfigValue::as_string(const std::string& default_val) const {
    return str_value_.empty() ? default_val : str_value_;
}

int ConfigValue::as_int(int default_val) const {
    if (type_ == ConfigType::Integer || type_ == ConfigType::Float) {
        return static_cast<int>(num_value_);
    }
    try {
        return std::stoi(str_value_);
    } catch (...) {
        return default_val;
    }
}

double ConfigValue::as_float(double default_val) const {
    if (type_ == ConfigType::Integer || type_ == ConfigType::Float) {
        return num_value_;
    }
    try {
        return std::stod(str_value_);
    } catch (...) {
        return default_val;
    }
}

bool ConfigValue::as_bool(bool default_val) const {
    if (type_ == ConfigType::Boolean) {
        return bool_value_;
    }
    std::string lower = str_value_;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "1" || lower == "yes") return true;
    if (lower == "false" || lower == "0" || lower == "no") return false;
    return default_val;
}

void ConfigValue::set_string(const std::string& value) {
    type_ = ConfigType::String;
    str_value_ = value;
}

void ConfigValue::set_int(int value) {
    type_ = ConfigType::Integer;
    num_value_ = static_cast<double>(value);
    str_value_ = std::to_string(value);
}

void ConfigValue::set_float(double value) {
    type_ = ConfigType::Float;
    num_value_ = value;
    str_value_ = std::to_string(value);
}

void ConfigValue::set_bool(bool value) {
    type_ = ConfigType::Boolean;
    bool_value_ = value;
    str_value_ = value ? "true" : "false";
}

//=============================================================================
// ConfigSection implementation
//=============================================================================

ConfigSection::ConfigSection(const std::string& name)
    : name_(name) {
}

ConfigSection::~ConfigSection() {
}

ConfigValue ConfigSection::get_value(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : ConfigValue();
}

void ConfigSection::set_value(const std::string& key, const ConfigValue& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[key] = value;
}

std::string ConfigSection::get_string(const std::string& key, const std::string& default_val) const {
    return get_value(key).as_string(default_val);
}

int ConfigSection::get_int(const std::string& key, int default_val) const {
    return get_value(key).as_int(default_val);
}

double ConfigSection::get_float(const std::string& key, double default_val) const {
    return get_value(key).as_float(default_val);
}

bool ConfigSection::get_bool(const std::string& key, bool default_val) const {
    return get_value(key).as_bool(default_val);
}

void ConfigSection::set_string(const std::string& key, const std::string& value) {
    set_value(key, ConfigValue(value));
}

void ConfigSection::set_int(const std::string& key, int value) {
    set_value(key, ConfigValue(value));
}

void ConfigSection::set_float(const std::string& key, double value) {
    set_value(key, ConfigValue(value));
}

void ConfigSection::set_bool(const std::string& key, bool value) {
    set_value(key, ConfigValue(value));
}

bool ConfigSection::has_key(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return values_.find(key) != values_.end();
}

void ConfigSection::remove_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_.erase(key);
}

std::vector<std::string> ConfigSection::get_keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> keys;
    for (const auto& pair : values_) {
        keys.push_back(pair.first);
    }
    return keys;
}

void ConfigSection::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    values_.clear();
}

//=============================================================================
// ConfigManager implementation
//=============================================================================

ConfigManager::ConfigManager()
    : auto_save_(false), initialized_(false), schema_version_(1) {
}

ConfigManager::~ConfigManager() {
    shutdown();
}

Result ConfigManager::initialize(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return Result::AlreadyInitialized;
    }
    
    config_path_ = config_path;
    initialized_ = true;
    
    // Try to load existing config, but don't fail if it doesn't exist
    load();
    
    return Result::Success;
}

void ConfigManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    if (auto_save_) {
        save();
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    sections_.clear();
    change_callbacks_.clear();
    initialized_ = false;
}

Result ConfigManager::load() {
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        // File doesn't exist, create default config
        return Result::Success;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    return parse_json(content);
}

Result ConfigManager::save() {
    std::string json = serialize_to_json();
    
    std::ofstream file(config_path_);
    if (!file.is_open()) {
        return Result::FileError;
    }
    
    file << json;
    file.close();
    
    return Result::Success;
}

ConfigSection* ConfigManager::get_section(const std::string& section_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sections_.find(section_name);
    if (it != sections_.end()) {
        return it->second.get();
    }
    
    // Create new section
    auto section = std::make_unique<ConfigSection>(section_name);
    auto* section_ptr = section.get();
    sections_[section_name] = std::move(section);
    
    return section_ptr;
}

bool ConfigManager::has_section(const std::string& section_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sections_.find(section_name) != sections_.end();
}

void ConfigManager::remove_section(const std::string& section_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    sections_.erase(section_name);
}

std::vector<std::string> ConfigManager::get_sections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> section_names;
    for (const auto& pair : sections_) {
        section_names.push_back(pair.first);
    }
    return section_names;
}

std::string ConfigManager::get_string(const std::string& section, const std::string& key,
                                     const std::string& default_val) const {
    auto* sec = const_cast<ConfigManager*>(this)->get_section(section);
    return sec ? sec->get_string(key, default_val) : default_val;
}

int ConfigManager::get_int(const std::string& section, const std::string& key, int default_val) const {
    auto* sec = const_cast<ConfigManager*>(this)->get_section(section);
    return sec ? sec->get_int(key, default_val) : default_val;
}

double ConfigManager::get_float(const std::string& section, const std::string& key, double default_val) const {
    auto* sec = const_cast<ConfigManager*>(this)->get_section(section);
    return sec ? sec->get_float(key, default_val) : default_val;
}

bool ConfigManager::get_bool(const std::string& section, const std::string& key, bool default_val) const {
    auto* sec = const_cast<ConfigManager*>(this)->get_section(section);
    return sec ? sec->get_bool(key, default_val) : default_val;
}

void ConfigManager::set_string(const std::string& section, const std::string& key, const std::string& value) {
    auto* sec = get_section(section);
    if (sec) {
        sec->set_string(key, value);
        notify_change(section, key);
        if (auto_save_) {
            save();
        }
    }
}

void ConfigManager::set_int(const std::string& section, const std::string& key, int value) {
    auto* sec = get_section(section);
    if (sec) {
        sec->set_int(key, value);
        notify_change(section, key);
        if (auto_save_) {
            save();
        }
    }
}

void ConfigManager::set_float(const std::string& section, const std::string& key, double value) {
    auto* sec = get_section(section);
    if (sec) {
        sec->set_float(key, value);
        notify_change(section, key);
        if (auto_save_) {
            save();
        }
    }
}

void ConfigManager::set_bool(const std::string& section, const std::string& key, bool value) {
    auto* sec = get_section(section);
    if (sec) {
        sec->set_bool(key, value);
        notify_change(section, key);
        if (auto_save_) {
            save();
        }
    }
}

void ConfigManager::register_change_callback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    change_callbacks_.push_back(callback);
}

void ConfigManager::notify_change(const std::string& section, const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& callback : change_callbacks_) {
        callback(section, key);
    }
}

Result ConfigManager::parse_json(const std::string& json_content) {
    // Simple JSON parser for configuration format:
    // { "section_name": { "key": "value", ... }, ... }
    
    std::string content = trim(json_content);
    if (content.empty() || content.front() != '{' || content.back() != '}') {
        return Result::InvalidFormat;
    }
    
    // Remove outer braces
    content = content.substr(1, content.length() - 2);
    
    // Parse sections
    size_t pos = 0;
    while (pos < content.length()) {
        // Find section name
        size_t quote1 = content.find('"', pos);
        if (quote1 == std::string::npos) break;
        
        size_t quote2 = content.find('"', quote1 + 1);
        if (quote2 == std::string::npos) break;
        
        std::string section_name = unescape_json_string(content.substr(quote1 + 1, quote2 - quote1 - 1));
        
        // Find section object
        size_t brace1 = content.find('{', quote2);
        if (brace1 == std::string::npos) break;
        
        int brace_count = 1;
        size_t brace2 = brace1 + 1;
        while (brace2 < content.length() && brace_count > 0) {
            if (content[brace2] == '{') brace_count++;
            else if (content[brace2] == '}') brace_count--;
            brace2++;
        }
        
        std::string section_content = content.substr(brace1 + 1, brace2 - brace1 - 2);
        
        // Parse section key-value pairs
        auto* section = get_section(section_name);
        size_t kv_pos = 0;
        while (kv_pos < section_content.length()) {
            size_t kq1 = section_content.find('"', kv_pos);
            if (kq1 == std::string::npos) break;
            
            size_t kq2 = section_content.find('"', kq1 + 1);
            if (kq2 == std::string::npos) break;
            
            std::string key = unescape_json_string(section_content.substr(kq1 + 1, kq2 - kq1 - 1));
            
            size_t colon = section_content.find(':', kq2);
            if (colon == std::string::npos) break;
            
            size_t val_start = section_content.find_first_not_of(" \t\n\r", colon + 1);
            if (val_start == std::string::npos) break;
            
            size_t val_end;
            std::string value;
            
            if (section_content[val_start] == '"') {
                // String value
                size_t vq2 = section_content.find('"', val_start + 1);
                if (vq2 == std::string::npos) break;
                value = unescape_json_string(section_content.substr(val_start + 1, vq2 - val_start - 1));
                val_end = vq2 + 1;
                section->set_string(key, value);
            } else {
                // Number or boolean
                val_end = section_content.find_first_of(",}", val_start);
                if (val_end == std::string::npos) val_end = section_content.length();
                value = trim(section_content.substr(val_start, val_end - val_start));
                
                if (value == "true" || value == "false") {
                    section->set_bool(key, value == "true");
                } else if (value.find('.') != std::string::npos) {
                    section->set_float(key, std::stod(value));
                } else {
                    section->set_int(key, std::stoi(value));
                }
            }
            
            kv_pos = val_end;
        }
        
        pos = brace2;
    }
    
    return Result::Success;
}

std::string ConfigManager::serialize_to_json() const {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"_schema_version\": " << schema_version_ << ",\n";
    
    bool first_section = true;
    for (const auto& section_pair : sections_) {
        if (!first_section) {
            ss << ",\n";
        }
        first_section = false;
        
        const auto& section_name = section_pair.first;
        const auto& section = section_pair.second;
        
        ss << "  \"" << escape_json_string(section_name) << "\": {\n";
        
        const auto& values = section->get_values();
        bool first_value = true;
        for (const auto& value_pair : values) {
            if (!first_value) {
                ss << ",\n";
            }
            first_value = false;
            
            const auto& key = value_pair.first;
            const auto& value = value_pair.second;
            
            ss << "    \"" << escape_json_string(key) << "\": ";
            
            switch (value.get_type()) {
                case ConfigType::String:
                    ss << "\"" << escape_json_string(value.as_string()) << "\"";
                    break;
                case ConfigType::Integer:
                    ss << value.as_int();
                    break;
                case ConfigType::Float:
                    ss << value.as_float();
                    break;
                case ConfigType::Boolean:
                    ss << (value.as_bool() ? "true" : "false");
                    break;
                default:
                    ss << "null";
                    break;
            }
        }
        
        ss << "\n  }";
    }
    
    ss << "\n}\n";
    return ss.str();
}

}} // namespace mp::core
