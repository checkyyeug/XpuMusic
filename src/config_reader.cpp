/**
 * @file config_reader.cpp
 * @brief Simple JSON configuration reader implementation
 * @date 2025-12-13
 */

#include "config_reader.h"
#include <iostream>
#include <sstream>

bool ConfigReader::load(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << config_path << std::endl;
        return false;
    }

    config_data_.clear();
    std::string line;
    std::string current_section;

    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '/' || line[0] == '{' || line[0] == '}' || line[0] == ']') {
            continue;
        }

        parse_line(line, current_section);
    }

    return true;
}

void ConfigReader::parse_line(const std::string& line, std::string& current_section) {
    // Check for section header
    if (line[0] == '"') {
        size_t end = line.find('"', 1);
        if (end != std::string::npos && line.size() > end + 2 && line[end + 1] == ':') {
            current_section = line.substr(1, end - 1);
            // Remove quotes from section name
            if (current_section.front() == '"' && current_section.back() == '"') {
                current_section = current_section.substr(1, current_section.length() - 2);
            }
            return;
        }
    }

    // Parse key-value pair
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos && !current_section.empty()) {
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);

        // Clean up key
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        if (key.front() == '"' && key.back() == '"') {
            key = key.substr(1, key.length() - 2);
        }

        // Clean up value
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t,") + 1);

        // Store the pair
        config_data_[current_section][key] = extract_value(value);
    }
}

std::string ConfigReader::extract_value(const std::string& raw_value) const {
    if (raw_value.empty()) return "";

    std::string value = raw_value;

    // Remove quotes if present
    if (value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
    }

    return value;
}

bool ConfigReader::get_bool(const std::string& section, const std::string& key, bool default_value) const {
    if (!has_key(section, key)) return default_value;

    const std::string& value = config_data_.at(section).at(key);
    return (value == "true" || value == "1");
}

int ConfigReader::get_int(const std::string& section, const std::string& key, int default_value) const {
    if (!has_key(section, key)) return default_value;

    try {
        return std::stoi(config_data_.at(section).at(key));
    } catch (...) {
        return default_value;
    }
}

double ConfigReader::get_double(const std::string& section, const std::string& key, double default_value) const {
    if (!has_key(section, key)) return default_value;

    try {
        return std::stod(config_data_.at(section).at(key));
    } catch (...) {
        return default_value;
    }
}

std::string ConfigReader::get_string(const std::string& section, const std::string& key, const std::string& default_value) const {
    if (!has_key(section, key)) return default_value;

    return config_data_.at(section).at(key);
}

bool ConfigReader::has_key(const std::string& section, const std::string& key) const {
    auto section_it = config_data_.find(section);
    if (section_it == config_data_.end()) return false;

    auto key_it = section_it->second.find(key);
    return key_it != section_it->second.end();
}

bool ConfigReader::get_bool_nested(const std::string& nested_key, bool default_value) const {
    // Parse nested key like "plugins.foobar2000.enable_compatibility"
    size_t first_dot = nested_key.find('.');
    if (first_dot == std::string::npos) return default_value;

    size_t second_dot = nested_key.find('.', first_dot + 1);
    if (second_dot == std::string::npos) return default_value;

    std::string section = nested_key.substr(0, first_dot);
    std::string subsection = nested_key.substr(first_dot + 1, second_dot - first_dot - 1);
    std::string key = nested_key.substr(second_dot + 1);

    // Check if section exists
    auto section_it = config_data_.find(section);
    if (section_it == config_data_.end()) return default_value;

    // Check for subsection key (like "foobar2000": { "enable_compatibility": ... })
    auto subsection_it = section_it->second.find(subsection);
    if (subsection_it != section_it->second.end()) {
        // The value might contain the nested JSON
        std::string subsection_value = subsection_it->second;

        // Simple check for enable_compatibility in the subsection value
        if (subsection_value.find("\"enable_compatibility\": true") != std::string::npos) {
            return true;
        } else if (subsection_value.find("\"enable_compatibility\": false") != std::string::npos) {
            return false;
        }
    }

    // Try direct key lookup
    return get_bool(section, key, default_value);
}