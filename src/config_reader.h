/**
 * @file config_reader.h
 * @brief Simple JSON configuration reader for XpuMusic
 * @date 2025-12-13
 */

#pragma once

#include <string>
#include <map>
#include <fstream>

class ConfigReader {
public:
    ConfigReader() = default;
    ~ConfigReader() = default;

    // Load configuration from JSON file
    bool load(const std::string& config_path);

    // Get configuration values
    bool get_bool(const std::string& section, const std::string& key, bool default_value = false) const;
    int get_int(const std::string& section, const std::string& key, int default_value = 0) const;
    double get_double(const std::string& section, const std::string& key, double default_value = 0.0) const;
    std::string get_string(const std::string& section, const std::string& key, const std::string& default_value = "") const;

    // Get nested configuration values (for JSON like "section.subsection.key")
    bool get_bool_nested(const std::string& nested_key, bool default_value = false) const;

    // Check if a key exists
    bool has_key(const std::string& section, const std::string& key) const;

private:
    // Simple JSON parser
    std::map<std::string, std::map<std::string, std::string>> config_data_;

    // Parse JSON line
    void parse_line(const std::string& line, std::string& current_section);

    // Extract value from JSON value string
    std::string extract_value(const std::string& raw_value) const;
};