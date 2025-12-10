/**
 * @file metadb_handle_interface.h
 * @brief Metadb handle interface definition for foobar2000 compatibility
 * @date 2025-12-10
 */

#pragma once

#include <string>

namespace foobar2000_sdk {

// Forward declaration
class file_info_impl;

/**
 * @brief Metadb handle interface
 */
class metadb_handle_interface {
public:
    virtual ~metadb_handle_interface() = default;

    virtual const char* get_path() const = 0;
    virtual const char* get_location() const = 0;
    virtual bool is_remote() const = 0;
    virtual void get_info(file_info_impl& p_info) const = 0;
    virtual void set_info(const file_info_impl& p_info) = 0;
};

} // namespace foobar2000_sdk