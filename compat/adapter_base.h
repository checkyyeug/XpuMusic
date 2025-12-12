/**
 * @file adapter_base.h
 * @brief Base class for XpuMusic adapters
 * @date 2025-12-10
 */

#pragma once

#include <string>

namespace mp {
namespace compat {

/**
 * @brief Base class for XpuMusic compatibility adapters
 */
class AdapterBase {
public:
    AdapterBase() = default;
    virtual ~AdapterBase() = default;

    /**
     * @brief Initialize the adapter
     * @return true if successful
     */
    virtual bool initialize() { return true; }

    /**
     * @brief Shutdown the adapter
     */
    virtual void shutdown() {}

    /**
     * @brief Get adapter name
     * @return Adapter name
     */
    virtual const char* get_name() const = 0;

    /**
     * @brief Check if adapter is enabled
     * @return true if enabled
     */
    virtual bool is_enabled() const { return enabled_; }

    /**
     * @brief Set adapter enabled state
     * @param enabled Enabled state
     */
    virtual void set_enabled(bool enabled) { enabled_ = enabled; }

protected:
    bool enabled_ = true;
};

} // namespace compat
} // namespace mp