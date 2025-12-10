#include "data_migration_manager.h"
#include "../logging.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

namespace mp {
namespace compat {

DataMigrationManager::DataMigrationManager()
    : initialized_(false) {
}

DataMigrationManager::~DataMigrationManager() {
    shutdown();
}

Result DataMigrationManager::initialize(const std::string& foobar_path) {
    if (initialized_) {
        return Result::Error;
    }
    
    foobar_path_ = foobar_path;
    initialized_ = true;
    
    COMPAT_LOG_INFO("[DataMigration] Initialized with foobar2000 path: " + foobar_path);
    
    return Result::Success;
}

void DataMigrationManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
    COMPAT_LOG_INFO("[DataMigration] Shutdown complete");
}

MigrationResult DataMigrationManager::migrate_playlists(const std::string& output_dir) {
    MigrationResult result;
    result.status = MigrationStatus::NotStarted;
    result.message = "Playlist migration not yet implemented";
    
    COMPAT_LOG_INFO("[DataMigration] Playlist migration to: " + output_dir);
    COMPAT_LOG_WARN("[DataMigration] Note: Implementation pending");
    
    return result;
}

MigrationResult DataMigrationManager::migrate_configuration(const std::string& output_file) {
    MigrationResult result;
    result.status = MigrationStatus::NotStarted;
    result.message = "Configuration migration not yet implemented";
    
    COMPAT_LOG_INFO("[DataMigration] Configuration migration to: " + output_file);
    COMPAT_LOG_WARN("[DataMigration] Note: Implementation pending");
    
    return result;
}

MigrationResult DataMigrationManager::migrate_library(const std::string& output_dir) {
    MigrationResult result;
    result.status = MigrationStatus::NotStarted;
    result.message = "Library migration not yet implemented";
    
    COMPAT_LOG_INFO("[DataMigration] Library migration to: " + output_dir);
    COMPAT_LOG_WARN("[DataMigration] Note: Implementation pending");
    
    return result;
}

std::vector<std::string> DataMigrationManager::get_available_playlists() const {
    // TODO: Scan foobar2000 playlists directory
    return std::vector<std::string>();
}

std::string DataMigrationManager::get_config_path() const {
#ifdef _WIN32
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return std::string(appDataPath) + "\\foobar2000";
    }
#endif
    return foobar_path_;
}

std::string DataMigrationManager::get_library_path() const {
    std::string config_path = get_config_path();
    return config_path + "\\library.fpl";
}

} // namespace compat
} // namespace mp
