/**
 * @file plugin_config.cpp
 * @brief Plugin configuration and parameter management implementation
 * @date 2025-12-13
 */

#include "plugin_config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

// ConfigSection implementation
void ConfigSection::add_param(const ConfigParam& param) {
    params_.emplace(param.key, param);
    // Set default value if not already set
    if (values_.find(param.key) == values_.end()) {
        values_[param.key] = param.default_value;
    }
}

bool ConfigSection::set_value(const std::string& key, const ConfigValue& value) {
    auto it = params_.find(key);
    if (it == params_.end()) {
        return false;  // Parameter not found
    }

    // Validate value range
    if (std::holds_alternative<int>(value)) {
        int val = std::get<int>(value);
        if (std::holds_alternative<int>(it->second.min_value)) {
            val = std::max(val, std::get<int>(it->second.min_value));
        }
        if (std::holds_alternative<int>(it->second.max_value)) {
            val = std::min(val, std::get<int>(it->second.max_value));
        }
        values_[key] = val;
    } else if (std::holds_alternative<double>(value)) {
        double val = std::get<double>(value);
        if (std::holds_alternative<double>(it->second.min_value)) {
            val = std::max(val, std::get<double>(it->second.min_value));
        }
        if (std::holds_alternative<double>(it->second.max_value)) {
            val = std::min(val, std::get<double>(it->second.max_value));
        }
        values_[key] = val;
    } else {
        values_[key] = value;
    }

    return true;
}

ConfigValue ConfigSection::get_value(const std::string& key) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    // Return empty string as default
    return std::string("");
}

ConfigValue ConfigSection::get_value_or_default(const std::string& key) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }

    auto param_it = params_.find(key);
    if (param_it != params_.end()) {
        return param_it->second.default_value;
    }

    return std::string("");
}

const ConfigParam* ConfigSection::get_param(const std::string& key) const {
    auto it = params_.find(key);
    if (it != params_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ConfigParam> ConfigSection::get_all_params() const {
    std::vector<ConfigParam> result;
    for (const auto& pair : params_) {
        result.push_back(pair.second);
    }
    return result;
}

bool ConfigSection::validate_values() const {
    for (const auto& pair : params_) {
        const std::string& key = pair.first;
        const ConfigParam& param = pair.second;

        auto it = values_.find(key);
        if (it == values_.end()) {
            return false;  // Required parameter missing
        }

        // Type checking
        if (param.default_value.index() != it->second.index()) {
            return false;  // Type mismatch
        }
    }
    return true;
}

void ConfigSection::reset_to_defaults() {
    for (const auto& pair : params_) {
        values_[pair.first] = pair.second.default_value;
    }
}

std::string ConfigSection::export_to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"enabled\": " << (enabled_ ? "true" : "false") << ",\n";
    oss << "  \"priority\": " << priority_ << ",\n";
    oss << "  \"parameters\": {\n";

    bool first = true;
    for (const auto& pair : values_) {
        if (!first) oss << ",\n";
        first = false;

        const std::string& key = pair.first;
        const ConfigValue& value = pair.second;

        oss << "    \"" << key << "\": ";
        if (std::holds_alternative<bool>(value)) {
            oss << (std::get<bool>(value) ? "true" : "false");
        } else if (std::holds_alternative<int>(value)) {
            oss << std::get<int>(value);
        } else if (std::holds_alternative<double>(value)) {
            oss << std::fixed << std::setprecision(6) << std::get<double>(value);
        } else if (std::holds_alternative<std::string>(value)) {
            // Simple JSON escaping for strings
            std::string str = std::get<std::string>(value);
            oss << "\"";
            for (char c : str) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c; break;
                }
            }
            oss << "\"";
        }
    }

    oss << "\n  }\n}";
    return oss.str();
}

bool ConfigSection::import_from_json(const std::string& json) {
    // Simplified JSON parsing - in production, use a proper JSON library
    if (json.find("\"enabled\"") != std::string::npos) {
        size_t pos = json.find("\"enabled\":");
        if (pos != std::string::npos) {
            pos = json.find(":", pos) + 1;
            enabled_ = json.substr(pos, 4).find("true") != std::string::npos;
        }
    }

    if (json.find("\"priority\"") != std::string::npos) {
        size_t pos = json.find("\"priority\":");
        if (pos != std::string::npos) {
            pos = json.find(":", pos) + 1;
            priority_ = std::stoi(json.substr(pos));
        }
    }

    // Parse parameters
    // This is a simplified implementation
    for (const auto& param_pair : params_) {
        const std::string& key = param_pair.first;
        //const ConfigParam& param = param_pair.second; // Not used in current implementation

        // Look for key in JSON
        std::string search_key = "\"" + key + "\":";
        size_t pos = json.find(search_key);
        if (pos != std::string::npos) {
            pos = json.find(":", pos) + 1;
            size_t end_pos = json.find(",", pos);
            if (end_pos == std::string::npos) {
                end_pos = json.find("}", pos);
            }

            std::string value_str = json.substr(pos, end_pos - pos);
            // Remove whitespace and quotes
            value_str.erase(0, value_str.find_first_not_of(" \t\n\r"));
            value_str.erase(value_str.find_last_not_of(" \t\n\r") + 1);

            if (value_str.front() == '"' && value_str.back() == '"') {
                value_str = value_str.substr(1, value_str.length() - 2);
                set_value(key, value_str);
            } else if (value_str == "true" || value_str == "false") {
                set_value(key, value_str == "true");
            } else if (value_str.find('.') != std::string::npos) {
                set_value(key, std::stod(value_str));
            } else {
                set_value(key, std::stoi(value_str));
            }
        }
    }

    return true;
}

// PluginConfigManager implementation
PluginConfigManager::PluginConfigManager()
    : auto_save_(true) {
}

PluginConfigManager::~PluginConfigManager() {
    if (auto_save_ && !config_file_path_.empty()) {
        save_config();
    }
}

bool PluginConfigManager::initialize(const std::string& config_file, bool auto_save) {
    config_file_path_ = config_file;
    auto_save_ = auto_save;

    // Try to load existing config
    if (std::ifstream(config_file).good()) {
        return load_config();
    }

    // Create default configuration
    generate_defaults();
    return save_config();
}

bool PluginConfigManager::save_config() {
    if (config_file_path_.empty()) {
        return false;
    }

    std::ofstream file(config_file_path_);
    if (!file.is_open()) {
        return false;
    }

    file << export_full_config();
    file.close();
    return true;
}

bool PluginConfigManager::load_config() {
    if (config_file_path_.empty()) {
        return false;
    }

    std::ifstream file(config_file_path_);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return import_full_config(buffer.str());
}

ConfigSection* PluginConfigManager::get_section(const std::string& plugin_name) {
    auto it = sections_.find(plugin_name);
    if (it != sections_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ConfigSection* PluginConfigManager::get_section(const std::string& plugin_name) const {
    auto it = sections_.find(plugin_name);
    if (it != sections_.end()) {
        return it->second.get();
    }
    return nullptr;
}

ConfigSection* PluginConfigManager::create_section(const std::string& plugin_name) {
    auto section = std::make_unique<ConfigSection>(plugin_name);
    ConfigSection* ptr = section.get();
    sections_[plugin_name] = std::move(section);
    return ptr;
}

void PluginConfigManager::remove_section(const std::string& plugin_name) {
    sections_.erase(plugin_name);
}

std::vector<ConfigSection*> PluginConfigManager::get_all_sections() {
    std::vector<ConfigSection*> result;
    for (auto& pair : sections_) {
        result.push_back(pair.second.get());
    }
    return result;
}

void PluginConfigManager::add_change_listener(std::function<void(const std::string&, const std::string&, const ConfigValue&)> listener) {
    change_listeners_.push_back(listener);
}

void PluginConfigManager::notify_change(const std::string& plugin_name, const std::string& param_key, const ConfigValue& value) {
    for (auto& listener : change_listeners_) {
        listener(plugin_name, param_key, value);
    }

    if (auto_save_) {
        save_config();
    }
}

bool PluginConfigManager::validate_all_configs() const {
    for (const auto& pair : sections_) {
        if (!pair.second->validate_values()) {
            return false;
        }
    }
    return true;
}

void PluginConfigManager::generate_defaults() {
    // Create default configuration for common plugin types
    auto mp3_section = create_section("mp3_decoder");
    mp3_section->add_param(ConfigParam("quality", "Decoding Quality", "MP3 decoding quality level", 3));
    mp3_section->add_param(ConfigParam("enable_replaygain", "Enable ReplayGain", "Process ReplayGain tags", true));
    mp3_section->add_param(ConfigParam("buffer_size", "Buffer Size", "Audio buffer size in bytes", 65536));
    mp3_section->set_value("buffer_size", 65536);
    mp3_section->set_priority(100);

    auto flac_section = create_section("flac_decoder");
    flac_section->add_param(ConfigParam("verify", "Verify Integrity", "Verify FLAC integrity during playback", true));
    flac_section->add_param(ConfigParam("enable_md5", "Enable MD5", "Calculate and verify MD5 checksums", false));
    flac_section->set_priority(90);
}

std::string PluginConfigManager::export_full_config() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"version\": \"1.0\",\n";
    oss << "  \"plugins\": {\n";

    bool first_plugin = true;
    for (const auto& pair : sections_) {
        if (!first_plugin) oss << ",\n";
        first_plugin = false;

        const std::string& plugin_name = pair.first;
        const ConfigSection& section = *pair.second;

        oss << "    \"" << escape_json_string(plugin_name) << "\": " << section.export_to_json();
    }

    oss << "\n  }\n}";
    return oss.str();
}

bool PluginConfigManager::import_full_config(const std::string& json) {
    // Simplified JSON parsing
    // In production, use a proper JSON library like nlohmann/json

    size_t plugins_start = json.find("\"plugins\":");
    if (plugins_start == std::string::npos) {
        return false;
    }

    // Extract plugins object
    size_t open_brace = json.find("{", plugins_start);
    size_t close_brace = json.rfind("}");
    if (open_brace == std::string::npos || close_brace == std::string::npos) {
        return false;
    }

    // Parse each plugin section
    // This is a simplified implementation
    for (const auto& pair : sections_) {
        const std::string& plugin_name = pair.first;
        std::string search_key = "\"" + plugin_name + "\":";

        size_t section_start = json.find(search_key);
        if (section_start != std::string::npos && section_start < close_brace) {
            section_start = json.find("{", section_start);
            if (section_start != std::string::npos) {
                // Find matching closing brace
                int brace_count = 1;
                size_t section_end = section_start + 1;
                while (section_end < json.size() && brace_count > 0) {
                    if (json[section_end] == '{') brace_count++;
                    else if (json[section_end] == '}') brace_count--;
                    section_end++;
                }

                std::string section_json = json.substr(section_start, section_end - section_start);
                pair.second->import_from_json(section_json);
            }
        }
    }

    return true;
}

std::vector<std::string> PluginConfigManager::get_enabled_plugins() const {
    std::vector<std::string> enabled;
    for (const auto& pair : sections_) {
        if (pair.second->is_enabled()) {
            enabled.push_back(pair.first);
        }
    }
    return enabled;
}

std::vector<std::string> PluginConfigManager::get_disabled_plugins() const {
    std::vector<std::string> disabled;
    for (const auto& pair : sections_) {
        if (!pair.second->is_enabled()) {
            disabled.push_back(pair.first);
        }
    }
    return disabled;
}

std::string PluginConfigManager::escape_json_string(const std::string& str) const {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

std::string PluginConfigManager::unescape_json_string(const std::string& str) const {
    std::string unescaped;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"': unescaped += '"'; i++; break;
                case '\\': unescaped += '\\'; i++; break;
                case 'n': unescaped += '\n'; i++; break;
                case 'r': unescaped += '\r'; i++; break;
                case 't': unescaped += '\t'; i++; break;
                default: unescaped += str[i]; break;
            }
        } else {
            unescaped += str[i];
        }
    }
    return unescaped;
}

std::string PluginConfigManager::value_to_json(const ConfigValue& value) const {
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        return std::to_string(std::get<double>(value));
    } else if (std::holds_alternative<std::string>(value)) {
        return "\"" + escape_json_string(std::get<std::string>(value)) + "\"";
    }
    return "null";
}

ConfigValue PluginConfigManager::json_to_value(const std::string& json, const std::string& type) const {
    if (type == "bool") {
        return json == "true";
    } else if (type == "int") {
        return std::stoi(json);
    } else if (type == "double") {
        return std::stod(json);
    } else {
        return json;
    }
}