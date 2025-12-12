#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace mp {
namespace compat {

// Log levels
enum class LogLevel {
    Off = 0,
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4
};

// Logger interface
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, const std::string& message) = 0;
};

// Console logger implementation
class ConsoleLogger : public ILogger {
public:
    void log(LogLevel level, const std::string& message) override {
        std::string levelStr = logLevelToString(level);
        std::string timestamp = getCurrentTimestamp();
        
        std::cout << "[" << timestamp << "][" << levelStr << "] " << message << std::endl;
    }
    
private:
    std::string logLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Error: return "ERROR";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Info: return "INFO";
            case LogLevel::Debug: return "DEBUG";
            default: return "UNKNOWN";
        }
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
};

// File logger implementation
class FileLogger : public ILogger {
public:
    FileLogger(const std::string& filename) : filename_(filename) {
        file_.open(filename_, std::ios::app);
    }
    
    ~FileLogger() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void log(LogLevel level, const std::string& message) override {
        if (!file_.is_open()) {
            return;
        }
        
        std::string levelStr = logLevelToString(level);
        std::string timestamp = getCurrentTimestamp();
        
        std::lock_guard<std::mutex> lock(mutex_);
        file_ << "[" << timestamp << "][" << levelStr << "] " << message << std::endl;
        file_.flush();
    }
    
private:
    std::string logLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Error: return "ERROR";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Info: return "INFO";
            case LogLevel::Debug: return "DEBUG";
            default: return "UNKNOWN";
        }
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    std::string filename_;
    std::ofstream file_;
    std::mutex mutex_;
};

// Logger manager
class LogManager {
public:
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }
    
    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        logLevel_ = level;
    }
    
    void addLogger(std::unique_ptr<ILogger> logger) {
        std::lock_guard<std::mutex> lock(mutex_);
        loggers_.push_back(std::move(logger));
    }
    
    void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (level > logLevel_) {
            return;
        }
        
        for (auto& logger : loggers_) {
            logger->log(level, message);
        }
    }
    
    void logError(const std::string& message) {
        log(LogLevel::Error, message);
    }
    
    void logWarning(const std::string& message) {
        log(LogLevel::Warning, message);
    }
    
    void logInfo(const std::string& message) {
        log(LogLevel::Info, message);
    }
    
    void logDebug(const std::string& message) {
        log(LogLevel::Debug, message);
    }
    
private:
    LogManager() : logLevel_(LogLevel::Info) {
        // Add console logger by default
        addLogger(std::make_unique<ConsoleLogger>());
    }
    
    LogLevel logLevel_;
    std::vector<std::unique_ptr<ILogger>> loggers_;
    std::mutex mutex_;
};

// Convenience macros
#define COMPAT_LOG_ERROR(msg) ::mp::compat::LogManager::getInstance().logError(msg)
#define COMPAT_LOG_WARN(msg) ::mp::compat::LogManager::getInstance().logWarning(msg)
#define COMPAT_LOG_INFO(msg) ::mp::compat::LogManager::getInstance().logInfo(msg)
#define COMPAT_LOG_DEBUG(msg) ::mp::compat::LogManager::getInstance().logDebug(msg)

} // namespace compat
} // namespace mp
