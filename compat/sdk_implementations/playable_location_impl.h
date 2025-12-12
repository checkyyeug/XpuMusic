/**
 * @file playable_location_impl.h
 * @brief Concrete implementation of playable_location class
 * @date 2025-12-11
 */

#pragma once

#include "../xpumusic_sdk/foobar2000_sdk.h"
#include <string>
#include <memory>

namespace xpumusic_sdk {

/**
 * @class playable_location_impl
 * @brief Concrete implementation of playable_location
 * 
 * This class provides a concrete implementation of the playable_location
 * abstract base class, representing a playable media file location.
 */
class playable_location_impl : public playable_location {
private:
    std::string path_;           // File path
    uint32_t subsong_index_;     // Subsong index (for multi-track files)
    bool is_empty_;             // Whether this location is empty/invalid

public:
    /**
     * @brief Default constructor
     */
    playable_location_impl();

    /**
     * @brief Constructor with path and subsong index
     * @param path File path
     * @param subsong_index Subsong index (default 0)
     */
    playable_location_impl(const std::string& path, uint32_t subsong_index = 0);

    /**
     * @brief Copy constructor
     * @param other Another playable_location_impl
     */
    playable_location_impl(const playable_location_impl& other);

    /**
     * @brief Move constructor
     * @param other Another playable_location_impl
     */
    playable_location_impl(playable_location_impl&& other) noexcept;

    /**
     * @brief Destructor
     */
    ~playable_location_impl() override = default;

    //============================================================================
    // playable_location interface implementation
    //============================================================================

    /**
     * @brief Get the file path
     * @return File path string
     */
    const char* get_path() const override;

    /**
     * @brief Set the file path
     * @param path New file path
     */
    void set_path(const char* path) override;

    /**
     * @brief Get subsong index
     * @return Subsong index
     */
    uint32_t get_subsong_index() const override;

    /**
     * @brief Set subsong index
     * @param index New subsong index
     */
    void set_subsong_index(uint32_t index) override;

    /**
     * @brief Check if location is empty
     * @return true if empty/invalid
     */
    bool is_empty() const override;

    //============================================================================
    // Additional functionality
    //============================================================================

    /**
     * @brief Get subsong as string (for compatibility with foobar2000 API)
     * @return Subsong string representation
     */
    const char* get_subsong() const;

    /**
     * @brief Get the full location string (path + subsong)
     * @return Full location string
     */
    std::string get_full_location() const;

    /**
     * @brief Reset to empty state
     */
    void reset();

    /**
     * @brief Assignment operator
     * @param other Another playable_location_impl
     * @return Reference to this
     */
    playable_location_impl& operator=(const playable_location_impl& other);

    /**
     * @brief Move assignment operator
     * @param other Another playable_location_impl
     * @return Reference to this
     */
    playable_location_impl& operator=(playable_location_impl&& other) noexcept;

    /**
     * @brief Equality operator
     * @param other Another playable_location_impl
     * @return true if equal
     */
    bool operator==(const playable_location_impl& other) const;

    /**
     * @brief Inequality operator
     * @param other Another playable_location_impl
     * @return true if not equal
     */
    bool operator!=(const playable_location_impl& other) const;
};

/**
 * @brief Create a playable_location from path and optional subsong
 * @param path File path
 * @param subsong_index Subsong index (default 0)
 * @return Unique pointer to playable_location_impl
 */
std::unique_ptr<playable_location_impl> create_playable_location(
    const std::string& path, 
    uint32_t subsong_index = 0);

} // namespace xpumusic_sdk