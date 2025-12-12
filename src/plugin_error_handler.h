/**
 * @file plugin_error_handler.h
 * @brief Enhanced error handling for plugin system
 * @date 2025-12-13
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <mutex>
#include <chrono>

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
    Info = 0,
    Warning = 1,
    Error = 2,
    Critical = 3
};

/**
 * @brief Error codes for plugin operations
 */
enum class PluginErrorCode {
    Success = 0,

    // File related errors
    FileNotFound = 1001,
    FileAccessDenied = 1002,
    InvalidFileFormat = 1003,
    FileCorrupted = 1004,

    // Loading errors
    LibraryLoadFailed = 2001,
    EntryPointNotFound = 2002,
    InitializationFailed = 2003,
    VersionMismatch = 2004,

    // Runtime errors
    PluginCrashed = 3001,
    OutOfMemory = 3002,
    InvalidParameter = 3003,

    // System errors
    InsufficientPermissions = 4001,
    DiskSpaceLow = 4002,
    NetworkError = 4003
};

/**
 * @brief Error information structure
 */
struct PluginError {
    ErrorSeverity severity;
    PluginErrorCode code;
    std::string message;
    std::string plugin_name;
    std::string file_path;
    std::string timestamp;
    std::string details;

    PluginError(ErrorSeverity sev, PluginErrorCode err, const std::string& msg,
                const std::string& plugin = "", const std::string& path = "",
                const std::string& det = "")
        : severity(sev), code(err), message(msg), plugin_name(plugin),
          file_path(path), details(det) {

        // Generate timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        // Use ctime_s for security (Windows) or ctime with buffer
#ifdef _WIN32
        char time_buf[26];
        ctime_s(time_buf, sizeof(time_buf), &time_t);
        timestamp = time_buf;
#else
        char time_buf[26];
        timestamp = ctime_r(&time_t, time_buf);
#endif

        // Remove trailing newline
        if (!timestamp.empty() && timestamp.back() == '\n') {
            timestamp.pop_back();
        }
    }
};

/**
 * @brief Enhanced error handler for plugin system
 */
class PluginErrorHandler {
private:
    std::vector<PluginError> error_log_;
    std::mutex log_mutex_;
    std::ofstream log_file_;
    std::string log_file_path_;
    bool verbose_;
    int max_log_entries_;

public:
    PluginErrorHandler();
    ~PluginErrorHandler();

    // Initialize error handler
    bool initialize(const std::string& log_file_path = "plugin_errors.log",
                   bool verbose = true, int max_entries = 1000);

    // Log an error
    void log_error(ErrorSeverity severity, PluginErrorCode code,
                   const std::string& message, const std::string& plugin_name = "",
                   const std::string& file_path = "", const std::string& details = "");

    // Convenience methods
    void log_info(const std::string& message, const std::string& plugin = "");
    void log_warning(const std::string& message, const std::string& plugin = "");
    void log_error(const std::string& message, const std::string& plugin = "");
    void log_critical(const std::string& message, const std::string& plugin = "");

    // Get error code string
    static std::string get_error_code_string(PluginErrorCode code);

    // Get recent errors
    std::vector<PluginError> get_recent_errors(int count = 10) const;

    // Get errors for specific plugin
    std::vector<PluginError> get_plugin_errors(const std::string& plugin_name) const;

    // Get errors by severity
    std::vector<PluginError> get_errors_by_severity(ErrorSeverity severity) const;

    // Clear error log
    void clear_log();

    // Get error statistics
    struct ErrorStats {
        int total_errors;
        int critical_errors;
        int plugin_load_failures;
        int runtime_errors;
    };
    ErrorStats get_statistics() const;

    // Generate error report
    std::string generate_error_report() const;

private:
    // Write error to file
    void write_to_file(const PluginError& error);

    // Format error message
    std::string format_error(const PluginError& error) const;
};