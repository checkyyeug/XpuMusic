/**
 * @file input_decoder_impl.cpp
 * @brief Input decoder implementation
 * @date 2025-12-10
 */

#include "input_decoder_impl.h"
#include <algorithm>
#include <cstring>
#include <filesystem>

namespace foobar2000 {

InputDecoderImpl::InputDecoderImpl(const char* format_name, const char* extension, const GUID& guid) {
    set_service_name(format_name);
    format_name_ = format_name ? format_name : "Unknown";
    class_guid_ = guid;

    if (extension) {
        add_extension(extension);
    }
}

bool InputDecoderImpl::can_decode(const char* path) {
    if (!path) return false;

    std::filesystem::path file_path(path);
    std::string ext = file_path.extension().string();

    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& supported_ext : extensions_) {
        if (ext == supported_ext) {
            return true;
        }
    }

    return false;
}

int InputDecoderImpl::open(const char* path) {
    // In a real implementation, this would open the file and initialize decoder
    // For now, just check if we can decode it
    return can_decode(path) ? 0 : -1;
}

int InputDecoderImpl::decode(void* buffer, int bytes) {
    // Stub implementation
    if (!buffer || bytes <= 0) return -1;
    memset(buffer, 0, bytes);
    return bytes;
}

void InputDecoderImpl::seek(int64_t position) {
    // Stub implementation
}

int64_t InputDecoderImpl::get_length() {
    // Stub implementation
    return 0;
}

int InputDecoderImpl::get_sample_rate() {
    // Stub implementation
    return 44100;
}

int InputDecoderImpl::get_channels() {
    // Stub implementation
    return 2;
}

int InputDecoderImpl::get_bits_per_sample() {
    // Stub implementation
    return 16;
}

void InputDecoderImpl::add_extension(const char* ext) {
    if (ext) {
        std::string extension = ext;
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        // Remove dot if present
        if (!extension.empty() && extension[0] == '.') {
            extension = extension.substr(1);
        }

        // Check if already exists
        for (const auto& existing : extensions_) {
            if (existing == extension) {
                return;
            }
        }

        extensions_.push_back(extension);
    }
}

} // namespace foobar2000