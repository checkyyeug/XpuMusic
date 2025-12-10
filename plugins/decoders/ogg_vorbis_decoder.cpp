#include "ogg_vorbis_decoder.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>

namespace xpumusic::plugins {

// Import plugin SDK types for convenience
using xpumusic::IPlugin;
using xpumusic::IAudioDecoder;
using xpumusic::PluginInfo;
using xpumusic::PluginState;
using xpumusic::AudioFormat;
using xpumusic::AudioBuffer;
using xpumusic::MetadataItem;
using xpumusic::PluginType;

OggVorbisDecoder::OggVorbisDecoder() {
    vf_ = {};
    vi_ = nullptr;
    vc_ = nullptr;
    is_open_ = false;
    output_buffer_.reserve(64 * 1024);  // 64KB output buffer
    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    total_samples_ = 0;
    duration_ = 0.0;
    current_sample_ = 0;
}

OggVorbisDecoder::~OggVorbisDecoder() {
    cleanup();
}

bool OggVorbisDecoder::initialize() {
    set_state(PluginState::Initialized);
    return true;
}

void OggVorbisDecoder::shutdown() {
    cleanup();
    set_state(PluginState::Uninitialized);
}

PluginState OggVorbisDecoder::get_state() const {
    return IPlugin::get_state();
}

void OggVorbisDecoder::set_state(PluginState state) {
    IPlugin::set_state(state);
}

PluginInfo OggVorbisDecoder::get_info() const {
    PluginInfo info;
    info.name = "OGG/Vorbis Decoder";
    info.version = "1.0.0";
    info.author = "XpuMusic Team";
    info.description = "OGG/Vorbis audio decoder using libvorbis";
    info.type = PluginType::AudioDecoder;
    info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
    info.supported_formats = {"ogg", "oga", "vorbis"};
    return info;
}

std::string OggVorbisDecoder::get_last_error() const {
    return last_error_;
}

bool OggVorbisDecoder::can_decode(const std::string& file_path) {
    std::string ext = file_path.substr(file_path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == "ogg" || ext == "oga" || ext == "vorbis";
}

std::vector<std::string> OggVorbisDecoder::get_supported_extensions() {
    return {"ogg", "oga", "vorbis"};
}

bool OggVorbisDecoder::open(const std::string& file_path) {
    if (is_open_) {
        close();
    }

    file_path_ = file_path;

    // Try to open the file
    FILE* file = fopen(file_path.c_str(), "rb");
    if (!file) {
        set_error("Failed to open file: " + file_path);
        return false;
    }

    // Initialize OggVorbis_File structure
    if (ov_open(file, &vf_, nullptr, 0) < 0) {
        fclose(file);
        set_error("File is not a valid Ogg/Vorbis bitstream");
        return false;
    }

    // Get stream information
    vi_ = ov_info(&vf_, -1);
    if (!vi_) {
        ov_clear(&vf_);
        set_error("Failed to get Ogg/Vorbis stream info");
        return false;
    }

    // Get comment information
    vc_ = ov_comment(&vf_, -1);

    // Set audio format
    format_.sample_rate = vi_->rate;
    format_.channels = vi_->channels;
    format_.bits_per_sample = 32;  // Convert to float
    format_.is_float = true;

    // Calculate total samples and duration
    ogg_int64_t pcm_total = ov_pcm_total(&vf_, -1);
    if (pcm_total == OV_EINVAL) {
        total_samples_ = 0;
        duration_ = 0.0;
    } else {
        total_samples_ = pcm_total;
        calculate_duration();
    }

    // Parse comments
    if (vc_) {
        parse_comments();
    }

    // Reset decode state
    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    current_sample_ = 0;

    is_open_ = true;
    set_state(PluginState::Active);

    return true;
}

int OggVorbisDecoder::decode(AudioBuffer& buffer, int max_frames) {
    if (!is_open_ || get_state() != PluginState::Active) {
        return 0;
    }

    // Ensure output buffer is large enough
    if (output_buffer_.size() < max_frames * format_.channels) {
        output_buffer_.resize(max_frames * format_.channels * 2);
    }

    float* output_ptr = output_buffer_.data() + output_buffer_pos_;
    int frames_decoded = 0;
    int samples_needed = max_frames * format_.channels;

    // Decode until we have enough frames or reach end of stream
    while (frames_decoded < max_frames && !feof(reinterpret_cast<FILE*>(vf_.datasource))) {
        int current_section;
        long samples_read = ov_read_float(&vf_, &output_ptr,
                                        samples_needed - frames_decoded * format_.channels,
                                        &current_section);

        if (samples_read > 0) {
            // Convert from samples per channel to total samples
            long total_samples = samples_read / format_.channels;
            frames_decoded += total_samples;
            output_ptr += samples_read;
            current_sample_ += total_samples;
        } else if (samples_read == 0) {
            // End of stream
            break;
        } else {
            // Error
            if (samples_read == OV_HOLE) {
                // Hole in data - skip and continue
                continue;
            } else if (samples_read == OV_EBADLINK) {
                set_error("Corrupt bitstream section");
                break;
            } else {
                set_error("Unknown Ogg/Vorbis error");
                break;
            }
        }
    }

    // Resize buffer to match decoded frames
    if (frames_decoded > 0) {
        buffer.resize(format_.channels, frames_decoded);

        // Copy data from output buffer
        float* buffer_ptr = buffer.data();
        memcpy(buffer_ptr,
               output_buffer_.data(),
               frames_decoded * format_.channels * sizeof(float));
    }

    return frames_decoded;
}

void OggVorbisDecoder::close() {
    if (is_open_) {
        cleanup();
    }
}

AudioFormat OggVorbisDecoder::get_format() const {
    return format_;
}

bool OggVorbisDecoder::seek(double seconds) {
    if (!is_open_) {
        return false;
    }

    // Calculate target PCM position
    ogg_int64_t target_pcm = static_cast<ogg_int64_t>(seconds * format_.sample_rate);

    // Seek to the position
    int result = ov_pcm_seek(&vf_, target_pcm);
    if (result != 0) {
        if (result == OV_ENOSEEK) {
            set_error("Bitstream is not seekable");
        } else if (result == OV_EINVAL) {
            set_error("Invalid seek position");
        } else if (result == OV_EREAD) {
            set_error("Read error while seeking");
        } else if (result == OV_EFAULT) {
            set_error("Internal error while seeking");
        } else if (result == OV_EOF) {
            set_error("Attempted to seek past end of file");
        } else {
            set_error("Unknown seek error");
        }
        return false;
    }

    // Reset decode state
    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    current_sample_ = target_pcm;

    return true;
}

double OggVorbisDecoder::get_duration() const {
    return duration_;
}

OggVorbisDecoder::VorbisComment OggVorbisDecoder::get_comments() const {
    return comments_;
}

nlohmann::json OggVorbisDecoder::get_metadata() const {
    nlohmann::json metadata;

    if (!comments_.title.empty()) metadata["title"] = comments_.title;
    if (!comments_.artist.empty()) metadata["artist"] = comments_.artist;
    if (!comments_.album.empty()) metadata["album"] = comments_.album;
    if (!comments_.date.empty()) metadata["date"] = comments_.date;
    if (!comments_.comment.empty()) metadata["comment"] = comments_.comment;
    if (!comments_.genre.empty()) metadata["genre"] = comments_.genre;
    if (!comments_.track.empty()) metadata["track"] = comments_.track;
    if (!comments_.albumartist.empty()) metadata["albumartist"] = comments_.albumartist;
    if (!comments_.composer.empty()) metadata["composer"] = comments_.composer;
    if (!comments_.performer.empty()) metadata["performer"] = comments_.performer;
    if (!comments_.copyright.empty()) metadata["copyright"] = comments_.copyright;
    if (!comments_.license.empty()) metadata["license"] = comments_.license;
    if (!comments_.location.empty()) metadata["location"] = comments_.location;
    if (!comments_.contact.empty()) metadata["contact"] = comments_.contact;
    if (!comments_.isrc.empty()) metadata["isrc"] = comments_.isrc;

    metadata["duration"] = duration_;
    metadata["sample_rate"] = format_.sample_rate;
    metadata["channels"] = format_.channels;
    metadata["bits_per_sample"] = format_.bits_per_sample;
    metadata["total_samples"] = total_samples_;
    metadata["lossless"] = false;
    metadata["encoder"] = "Vorbis";

    if (vi_) {
        metadata["nominal_bitrate"] = vi_->bitrate_nominal;
        metadata["minimum_bitrate"] = vi_->bitrate_lower;
        metadata["maximum_bitrate"] = vi_->bitrate_upper;
        metadata["bitrate_window"] = vi_->bitrate_window;
        metadata["version"] = vi_->version;
    }

    return metadata;
}

// Private methods implementation
void OggVorbisDecoder::cleanup() {
    if (is_open_) {
        ov_clear(&vf_);
        is_open_ = false;
    }

    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    vi_ = nullptr;
    vc_ = nullptr;
    current_sample_ = 0;
}

void OggVorbisDecoder::set_error(const std::string& error) {
    last_error_ = error;
    set_state(PluginState::Error);
}

void OggVorbisDecoder::calculate_duration() {
    if (format_.sample_rate > 0 && total_samples_ > 0) {
        duration_ = static_cast<double>(total_samples_) / format_.sample_rate;
    } else {
        duration_ = 0.0;
    }
}

void OggVorbisDecoder::parse_comments() {
    if (!vc_) return;

    for (int i = 0; i < vc_->comments; ++i) {
        std::string comment(vc_->user_comments[i], vc_->comment_lengths[i]);

        // Parse field=value pairs
        size_t equal_pos = comment.find('=');
        if (equal_pos != std::string::npos) {
            std::string field = comment.substr(0, equal_pos);
            std::string value = comment.substr(equal_pos + 1);

            // Convert field to uppercase for case-insensitive comparison
            std::transform(field.begin(), field.end(), field.begin(), ::toupper);

            // Parse common fields
            if (field == "TITLE") {
                comments_.title = value;
            } else if (field == "ARTIST") {
                comments_.artist = value;
            } else if (field == "ALBUM") {
                comments_.album = value;
            } else if (field == "DATE") {
                comments_.date = value;
            } else if (field == "COMMENT") {
                comments_.comment = value;
            } else if (field == "GENRE") {
                comments_.genre = value;
            } else if (field == "TRACKNUMBER") {
                comments_.track = value;
            } else if (field == "ALBUMARTIST") {
                comments_.albumartist = value;
            } else if (field == "COMPOSER") {
                comments_.composer = value;
            } else if (field == "PERFORMER") {
                comments_.performer = value;
            } else if (field == "COPYRIGHT") {
                comments_.copyright = value;
            } else if (field == "LICENSE") {
                comments_.license = value;
            } else if (field == "LOCATION") {
                comments_.location = value;
            } else if (field == "CONTACT") {
                comments_.contact = value;
            } else if (field == "ISRC") {
                comments_.isrc = value;
            }
        }
    }
}

// File operation callbacks for ov_open_callbacks
size_t OggVorbisDecoder::read_func(void* ptr, size_t size, size_t nmemb, void* datasource) {
    FILE* file = static_cast<FILE*>(datasource);
    return fread(ptr, size, nmemb, file);
}

int OggVorbisDecoder::seek_func(void* datasource, ogg_int64_t offset, int whence) {
    FILE* file = static_cast<FILE*>(datasource);
    return fseek(file, static_cast<long>(offset), whence);
}

long OggVorbisDecoder::tell_func(void* datasource) {
    FILE* file = static_cast<FILE*>(datasource);
    return ftell(file);
}

int OggVorbisDecoder::close_func(void* datasource) {
    FILE* file = static_cast<FILE*>(datasource);
    return fclose(file);
}

} // namespace xpumusic::plugins