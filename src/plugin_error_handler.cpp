/**
 * @file plugin_error_handler.cpp
 * @brief Enhanced error handling for plugin system implementation
 * @date 2025-12-13
 */

#include "plugin_error_handler.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>

PluginErrorHandler::PluginErrorHandler()
    : verbose_(false), max_log_entries_(1000) {
}

PluginErrorHandler::~PluginErrorHandler() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

bool PluginErrorHandler::initialize(const std::string& log_file_path,
                                   bool verbose, int max_entries) {
    log_file_path_ = log_file_path;
    verbose_ = verbose;
    max_log_entries_ = max_entries;

    // Open log file
    log_file_.open(log_file_path_, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "Warning: Could not open log file: " << log_file_path_ << std::endl;
        return false;
    }

    // Write initialization message
    log_info("Plugin error handler initialized", "System");
    return true;
}

void PluginErrorHandler::log_error(ErrorSeverity severity, PluginErrorCode code,
                                   const std::string& message, const std::string& plugin_name,
                                   const std::string& file_path, const std::string& details) {
    PluginError error(severity, code, message, plugin_name, file_path, details);

    std::lock_guard<std::mutex> lock(log_mutex_);

    // Add to memory log
    error_log_.push_back(error);

    // Limit log size
    if (error_log_.size() > static_cast<size_t>(max_log_entries_)) {
        error_log_.erase(error_log_.begin());
    }

    // Write to file
    write_to_file(error);

    // Console output if verbose
    if (verbose_ || severity >= ErrorSeverity::Error) {
        std::string prefix;
        switch (severity) {
            case ErrorSeverity::Info:    prefix = "[INFO]"; break;
            case ErrorSeverity::Warning: prefix = "[WARN]"; break;
            case ErrorSeverity::Error:   prefix = "[ERROR]"; break;
            case ErrorSeverity::Critical:prefix = "[CRITICAL]"; break;
        }

        std::cerr << prefix << " " << format_error(error) << std::endl;
    }
}

void PluginErrorHandler::log_info(const std::string& message, const std::string& plugin) {
    log_error(ErrorSeverity::Info, PluginErrorCode::Success, message, plugin);
}

void PluginErrorHandler::log_warning(const std::string& message, const std::string& plugin) {
    log_error(ErrorSeverity::Warning, PluginErrorCode::Success, message, plugin);
}

void PluginErrorHandler::log_error(const std::string& message, const std::string& plugin) {
    log_error(ErrorSeverity::Error, PluginErrorCode::Success, message, plugin);
}

void PluginErrorHandler::log_critical(const std::string& message, const std::string& plugin) {
    log_error(ErrorSeverity::Critical, PluginErrorCode::Success, message, plugin);
}

std::string PluginErrorHandler::get_error_code_string(PluginErrorCode code) {
    switch (code) {
        case PluginErrorCode::Success: return "Success";
        case PluginErrorCode::FileNotFound: return "File Not Found";
        case PluginErrorCode::FileAccessDenied: return "Access Denied";
        case PluginErrorCode::InvalidFileFormat: return "Invalid File Format";
        case PluginErrorCode::FileCorrupted: return "File Corrupted";
        case PluginErrorCode::LibraryLoadFailed: return "Library Load Failed";
        case PluginErrorCode::EntryPointNotFound: return "Entry Point Not Found";
        case PluginErrorCode::InitializationFailed: return "Initialization Failed";
        case PluginErrorCode::VersionMismatch: return "Version Mismatch";
        case PluginErrorCode::PluginCrashed: return "Plugin Crashed";
        case PluginErrorCode::OutOfMemory: return "Out of Memory";
        case PluginErrorCode::InvalidParameter: return "Invalid Parameter";
        case PluginErrorCode::InsufficientPermissions: return "Insufficient Permissions";
        case PluginErrorCode::DiskSpaceLow: return "Disk Space Low";
        case PluginErrorCode::NetworkError: return "Network Error";
        default: return "Unknown Error";
    }
}

std::vector<PluginError> PluginErrorHandler::get_recent_errors(int count) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(log_mutex_));

    std::vector<PluginError> recent;
    int start = std::max(0, static_cast<int>(error_log_.size()) - count);

    for (int i = start; i < static_cast<int>(error_log_.size()); i++) {
        recent.push_back(error_log_[i]);
    }

    return recent;
}

std::vector<PluginError> PluginErrorHandler::get_plugin_errors(const std::string& plugin_name) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(log_mutex_));

    std::vector<PluginError> plugin_errors;
    for (const auto& error : error_log_) {
        if (error.plugin_name == plugin_name) {
            plugin_errors.push_back(error);
        }
    }

    return plugin_errors;
}

std::vector<PluginError> PluginErrorHandler::get_errors_by_severity(ErrorSeverity severity) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(log_mutex_));

    std::vector<PluginError> filtered_errors;
    for (const auto& error : error_log_) {
        if (error.severity == severity) {
            filtered_errors.push_back(error);
        }
    }

    return filtered_errors;
}

void PluginErrorHandler::clear_log() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    error_log_.clear();
    log_info("Error log cleared", "System");
}

PluginErrorHandler::ErrorStats PluginErrorHandler::get_statistics() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(log_mutex_));

    ErrorStats stats = {0, 0, 0, 0};

    for (const auto& error : error_log_) {
        stats.total_errors++;

        if (error.severity == ErrorSeverity::Critical) {
            stats.critical_errors++;
        }

        if (error.code == PluginErrorCode::LibraryLoadFailed ||
            error.code == PluginErrorCode::EntryPointNotFound ||
            error.code == PluginErrorCode::InitializationFailed) {
            stats.plugin_load_failures++;
        }

        if (error.code == PluginErrorCode::PluginCrashed ||
            error.code == PluginErrorCode::OutOfMemory ||
            error.code == PluginErrorCode::InvalidParameter) {
            stats.runtime_errors++;
        }
    }

    return stats;
}

std::string PluginErrorHandler::generate_error_report() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(log_mutex_));

    std::stringstream report;
    report << "=== Plugin Error Report ===\n";
    report << "Generated: " << PluginError(ErrorSeverity::Info, PluginErrorCode::Success, "", "").timestamp << "\n\n";

    ErrorStats stats = get_statistics();
    report << "Statistics:\n";
    report << "  Total Errors: " << stats.total_errors << "\n";
    report << "  Critical Errors: " << stats.critical_errors << "\n";
    report << "  Plugin Load Failures: " << stats.plugin_load_failures << "\n";
    report << "  Runtime Errors: " << stats.runtime_errors << "\n\n";

    // Recent critical errors
    auto critical_errors = get_errors_by_severity(ErrorSeverity::Critical);
    if (!critical_errors.empty()) {
        report << "Recent Critical Errors:\n";
        int count = 0;
        for (auto it = critical_errors.rbegin(); it != critical_errors.rend() && count < 5; ++it, ++count) {
            report << "  - " << format_error(*it) << "\n";
        }
        report << "\n";
    }

    // Plugins with most errors
    std::map<std::string, int> plugin_error_counts;
    for (const auto& error : error_log_) {
        if (!error.plugin_name.empty()) {
            plugin_error_counts[error.plugin_name]++;
        }
    }

    if (!plugin_error_counts.empty()) {
        report << "Error Count by Plugin:\n";
        for (const auto& pair : plugin_error_counts) {
            report << "  " << pair.first << ": " << pair.second << " errors\n";
        }
    }

    return report.str();
}

void PluginErrorHandler::write_to_file(const PluginError& error) {
    if (log_file_.is_open()) {
        log_file_ << format_error(error) << std::endl;
        log_file_.flush();
    }
}

std::string PluginErrorHandler::format_error(const PluginError& error) const {
    std::stringstream ss;
    ss << "[" << error.timestamp << "] ";

    switch (error.severity) {
        case ErrorSeverity::Info:    ss << "INFO"; break;
        case ErrorSeverity::Warning: ss << "WARN"; break;
        case ErrorSeverity::Error:   ss << "ERROR"; break;
        case ErrorSeverity::Critical:ss << "CRITICAL"; break;
    }

    ss << " [" << get_error_code_string(error.code) << "]";

    if (!error.plugin_name.empty()) {
        ss << " [" << error.plugin_name << "]";
    }

    ss << ": " << error.message;

    if (!error.details.empty()) {
        ss << " (" << error.details << ")";
    }

    return ss.str();
}