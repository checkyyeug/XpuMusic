#include "compat/foobar_sdk/foobar2000_sdk.h"
#include <iostream>

// Test that the interfaces compile and can be inherited
class TestAudioChunk : public foobar2000_sdk::audio_chunk {
public:
    uint32_t get_sample_rate() const override { return 44100; }
    void set_sample_rate(uint32_t rate) override {}
    uint32_t get_channels() const override { return 2; }
    void set_channels(uint32_t ch) override {}
    uint32_t get_channel_config() const override { return 0; }
    void set_channel_config(uint32_t config) override {}
    size_t get_sample_count() const override { return 0; }
    void set_sample_count(size_t count) override {}
    const foobar2000_sdk::audio_sample* get_data() const override { return nullptr; }
    foobar2000_sdk::audio_sample* get_data() override { return nullptr; }
    void set_data_size(size_t samples_per_channel) override {}
    double get_duration() const override { return 0.0; }
    void reset() override {}
    void set_data(const foobar2000_sdk::audio_sample* p_data, size_t p_samples, uint32_t p_channels, uint32_t p_sample_rate) override {}
    size_t get_data_size() const override { return 0; }
    size_t get_data_bytes() const override { return 0; }
    foobar2000_sdk::audio_sample* get_channel_data(uint32_t p_channel) override { return nullptr; }
    const foobar2000_sdk::audio_sample* get_channel_data(uint32_t p_channel) const override { return nullptr; }
    void scale(const foobar2000_sdk::audio_sample& p_scale) override {}
    void copy(const foobar2000_sdk::audio_chunk& p_source) override {}
    bool is_valid() const override { return true; }
    bool is_empty() const override { return false; }
};

class TestFileInfo : public foobar2000_sdk::file_info {
public:
    void reset() override {}
    bool is_valid() const override { return true; }
    const char* meta_get(const char* name, size_t index) const override { return nullptr; }
    size_t meta_get_count(const char* name) const override { return 0; }
    bool meta_set(const char* name, const char* value) override { return true; }
    bool meta_add(const char* name, const char* value) override { return true; }
    bool meta_remove(const char* name) override { return true; }
    void meta_remove_index(const char* name, size_t index) override {}
    std::vector<std::string> meta_enumerate() const override { return {}; }
    double get_length() const override { return 0.0; }
    void set_length(double length) override {}
    uint32_t get_sample_rate() const override { return 44100; }
    void set_sample_rate(uint32_t rate) override {}
    uint32_t get_channels() const override { return 2; }
    void set_channels(uint32_t channels) override {}
    uint32_t get_bitrate() const override { return 320; }
    void set_bitrate(uint32_t bitrate) override {}
    const char* get_codec() const override { return "mp3"; }
    void set_codec(const char* codec) override {}
    void copy(const foobar2000_sdk::file_info& other) override {}
    void merge(const foobar2000_sdk::file_info& other) override {}
    const foobar2000_sdk::file_stats& get_stats() const override { return stats_; }
    foobar2000_sdk::file_stats& get_stats() override { return stats_; }
    void set_stats(const foobar2000_sdk::file_stats& stats) override { stats_ = stats; }
    const foobar2000_sdk::audio_info& get_audio_info() const override { return audio_info_; }
    foobar2000_sdk::audio_info& get_audio_info() override { return audio_info_; }
    void set_audio_info(const foobar2000_sdk::audio_info& info) override { audio_info_ = info; }
private:
    foobar2000_sdk::file_stats stats_;
    foobar2000_sdk::audio_info audio_info_;
};

// Simple playable location implementation for the test
class TestPlayableLocation : public foobar2000_sdk::playable_location {
public:
    const char* get_path() const override { return path_.c_str(); }
    void set_path(const char* path) override { path_ = path ? path : ""; }
    uint32_t get_subsong_index() const override { return 0; }
    void set_subsong_index(uint32_t index) override {}
    bool is_empty() const override { return path_.empty(); }
private:
    std::string path_ = "/test/path";
};

class TestMetadbHandle : public foobar2000_sdk::metadb_handle {
public:
    const foobar2000_sdk::playable_location& get_location() const override { return location_; }
    const foobar2000_sdk::file_info& get_info() const override { return info_; }
    foobar2000_sdk::file_info& get_info() override { return info_; }
    void set_info(const foobar2000_sdk::file_info& info) override {}
    const foobar2000_sdk::file_stats& get_file_stats() const override { return stats_; }
    uint64_t get_location_hash() const override { return 0; }
    bool is_same(const foobar2000_sdk::metadb_handle& other) const override { return true; }
    bool is_valid() const override { return true; }
    void reload(foobar2000_sdk::abort_callback& p_abort) override {}
    std::string get_path() const override { return "/test/path"; }
    std::string get_filename() const override { return "test.mp3"; }
    std::string get_directory() const override { return "/test"; }
    void ref_add_ref() override {}
    void ref_release() override {}
private:
    TestPlayableLocation location_;
    TestFileInfo info_;
    foobar2000_sdk::file_stats stats_;
};

int main() {
    std::cout << "Testing foobar2000 SDK interface compilation..." << std::endl;
    
    TestAudioChunk chunk;
    TestFileInfo info;
    TestMetadbHandle handle;
    
    std::cout << "All interfaces compiled successfully!" << std::endl;
    
    return 0;
}