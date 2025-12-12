#pragma once

#include "mp_types.h"
#include <string>
#include <mutex>

namespace mp {
namespace compat {

// Adapter type enumeration
enum class AdapterType {
    InputDecoder,
    DSPChain,
    UIExtension,
    Metadb
};

// Adapter status
enum class AdapterStatus {
    NotInitialized,
    Ready,
    Error,
    Disabled
};

// Base class for all foobar2000 compatibility adapters
class AdapterBase {
public:
    AdapterBase(AdapterType type, const std::string& name)
        : type_(type)
        , name_(name)
        , status_(AdapterStatus::NotInitialized) {}
    
    virtual ~AdapterBase() = default;
    
    // Initialize adapter
    virtual Result initialize() = 0;
    
    // Shutdown adapter
    virtual void shutdown() = 0;
    
    // Get adapter type
    AdapterType get_type() const { return type_; }
    
    // Get adapter name
    const std::string& get_name() const { return name_; }
    
    // Get adapter status
    AdapterStatus get_status() const { return status_; }
    
    // Check if adapter is ready
    bool is_ready() const { return status_ == AdapterStatus::Ready; }
    
protected:
    // Set adapter status
    void set_status(AdapterStatus status) { status_ = status; }
    
    AdapterType type_;
    std::string name_;
    AdapterStatus status_;
    mutable std::mutex mutex_;
};

// Adapter statistics
struct AdapterStats {
    uint64_t calls_total = 0;
    uint64_t calls_success = 0;
    uint64_t calls_failed = 0;
    uint64_t bytes_processed = 0;
    double total_time_ms = 0.0;
    double avg_time_ms = 0.0;
    
    void record_call(bool success, double time_ms, uint64_t bytes = 0) {
        calls_total++;
        if (success) {
            calls_success++;
        } else {
            calls_failed++;
        }
        bytes_processed += bytes;
        total_time_ms += time_ms;
        avg_time_ms = total_time_ms / calls_total;
    }
    
    void reset() {
        calls_total = 0;
        calls_success = 0;
        calls_failed = 0;
        bytes_processed = 0;
        total_time_ms = 0.0;
        avg_time_ms = 0.0;
    }
};

} // namespace compat
} // namespace mp
