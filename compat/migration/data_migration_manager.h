#pragma once

#include "mp_types.h"
#include <string>
#include <vector>
#include <memory>

namespace mp {
namespace compat {

// Migration task status
enum class MigrationStatus {
    NotStarted,
    InProgress,
    Completed,
    Failed,
    Cancelled
};

// Migration task result
struct MigrationResult {
    MigrationStatus status;
    std::string message;
    int items_processed;
    int items_succeeded;
    int items_failed;
    
    MigrationResult()
        : status(MigrationStatus::NotStarted)
        , items_processed(0)
        , items_succeeded(0)
        , items_failed(0) {}
};

// Data Migration Manager
// Handles migration of playlists, configurations, and library data
class DataMigrationManager {
public:
    DataMigrationManager();
    ~DataMigrationManager();
    
    // Initialize migration manager
    Result initialize(const std::string& foobar_path);
    
    // Shutdown migration manager
    void shutdown();
    
    // Migrate playlists (FPL to M3U)
    MigrationResult migrate_playlists(const std::string& output_dir);
    
    // Migrate configuration
    MigrationResult migrate_configuration(const std::string& output_file);
    
    // Migrate library database
    MigrationResult migrate_library(const std::string& output_dir);
    
    // Get list of available playlists
    std::vector<std::string> get_available_playlists() const;
    
    // Get foobar2000 configuration path
    std::string get_config_path() const;
    
    // Get foobar2000 library database path
    std::string get_library_path() const;
    
private:
    std::string foobar_path_;
    bool initialized_;
};

} // namespace compat
} // namespace mp
