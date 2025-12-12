#pragma once

#include "../xpumusic_sdk/foobar2000_sdk_complete.h"
#include <memory>

namespace foobar2000_sdk {

// File info implementation
class file_info_impl : public file_info {
private:
    audio_info info_;
    file_stats stats_;
    std::unordered_map<std::string, field_value> meta_;

public:
    file_info_impl() = default;

    void reset() override {
        info_.reset();
        stats_ = file_stats{};
        meta_.clear();
    }

    void copy(const file_info& source) override {
        info_ = source.get_info();
        stats_ = source.get_file_stats();
        // Note: In a real implementation, we would copy metadata
    }

    const char* meta_get(const char* name, int index) const override {
        auto it = meta_.find(name);
        if (it != meta_.end() && index < static_cast<int>(it->second.values.size())) {
            return it->second.values[index].c_str();
        }
        return nullptr;
    }

    int meta_get_count(const char* name) const override {
        auto it = meta_.find(name);
        if (it != meta_.end()) {
            return static_cast<int>(it->second.values.size());
        }
        return 0;
    }

    bool meta_exists(const char* name) const override {
        return meta_.find(name) != meta_.end();
    }

    void meta_set(const char* name, const char* value) override {
        if (name && value) {
            meta_[name].values.clear();
            meta_[name].values.push_back(value);
            meta_[name].cache_valid = false;
        }
    }

    void meta_add_value(const char* name, const char* value) override {
        if (name && value) {
            meta_[name].add_value(value);
        }
    }

    void meta_remove_field(const char* name) override {
        meta_.erase(name);
    }

    const audio_info& get_info() const override {
        return info_;
    }

    void set_info(const audio_info& info) override {
        info_ = info;
    }

    const file_stats& get_file_stats() const override {
        return stats_;
    }

    void set_file_stats(const file_stats& stats) override {
        stats_ = stats;
    }
};

// Metadb handle implementation
class metadb_handle_impl : public metadb_handle {
public:
    metadb_handle_impl() = default;

    metadb_handle_impl(const playable_location& loc) {
        set_location(loc);
    }

protected:
    file_info* create_file_info() override {
        return new file_info_impl();
    }
};

// Factory function
inline std::unique_ptr<metadb_handle> create_metadb_handle() {
    return std::make_unique<metadb_handle_impl>();
}

inline std::unique_ptr<metadb_handle> create_metadb_handle(const playable_location& loc) {
    return std::make_unique<metadb_handle_impl>(loc);
}

} // namespace foobar2000_sdk