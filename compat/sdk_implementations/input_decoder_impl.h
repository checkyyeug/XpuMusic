/**
 * @file input_decoder_impl.h
 * @brief Input decoder implementation
 * @date 2025-12-10
 */

#pragma once

#include "service_base_impl.h"
#include "../xpumusic_sdk/foobar2000.h"
#include <string>

namespace xpumusic_sdk {

/**
 * @brief Concrete input decoder implementation
 */
class InputDecoderImpl : public input_decoder {
private:
    GUID class_guid_;
    std::string format_name_;
    std::vector<std::string> extensions_;

public:
    InputDecoderImpl(const char* format_name, const char* extension, const GUID& guid);
    virtual ~InputDecoderImpl() = default;

    // service_get_class_guid implementation
    virtual const GUID* service_get_class_guid() {
        return &class_guid_;
    }

    // service_base interface
    int service_add_ref() override;
    int service_release() override;
    const char* service_get_name();

    // input_decoder interface
    bool initialize(const char* path, abort_callback& abort) override;
    void get_info(file_info& info, abort_callback& abort) override;
    bool decode_run(audio_chunk& chunk, abort_callback& abort) override;
    bool seek(double seconds, abort_callback& abort) override;
    bool can_seek() override;

    // Helper methods
    void add_extension(const char* ext);
    const std::string& get_format_name() const { return format_name_; }
};

} // namespace xpumusic_sdk