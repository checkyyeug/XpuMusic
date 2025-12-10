/**
 * @file data_migration_manager.h
 * @brief Data migration manager for foobar2000 compatibility
 * @date 2025-12-10
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace mp {
namespace compat {

/**
 * @brief Manages data migration from foobar2000 installations
 */
class DataMigrationManager {
public:
    DataMigrationManager() = default;
    virtual ~DataMigrationManager() = default;

    /**
     * @brief Initialize the migration manager
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown the migration manager
     */
    void shutdown();

    /**
     * @brief Check if migration is needed
     * @return true if migration is needed
     */
    bool is_migration_needed() const;

    /**
     * @brief Perform migration
     * @return true if successful
     */
    bool perform_migration();

    /**
     * @brief Get migration status
     * @return 0 = not needed, 1 = in progress, 2 = completed, -1 = error
     */
    int get_migration_status() const { return migration_status_; }

private:
    int migration_status_ = 0;  // 0 = not needed, 1 = in progress, 2 = completed, -1 = error
    bool initialized_ = false;
};

} // namespace compat
} // namespace mp