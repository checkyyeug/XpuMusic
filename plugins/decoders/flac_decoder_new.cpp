#include "flac_decoder.h"
#include <cstring>
#include <algorithm>

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

FLACDecoder::FLACDecoder() {
    decoder_ = nullptr;
    is_open_ = FLAC__false;
    output_buffer_.reserve(64 * 1024);  // 64KB output buffer
    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    end_of_stream_ = FLAC__false;

    total_samples_ = 0;
    sample_rate_ = 0;
    channels_ = 0;
    bits_per_sample_ = 0;
    duration_ = 0.0;
    current_sample_ = 0;
}

FLACDecoder::~FLACDecoder() {
    cleanup();
}

bool FLACDecoder::initialize() {
    if (!initialize_decoder()) {
        return false;
    }

    set_state(PluginState::Initialized);
    return true;
}

void FLACDecoder::shutdown() {
    cleanup();
    set_state(PluginState::Uninitialized);
}

PluginState FLACDecoder::get_state() const {
    return IPlugin::get_state();
}

void FLACDecoder::set_state(PluginState state) {
    IPlugin::set_state(state);
}

PluginInfo FLACDecoder::get_info() const {
    PluginInfo info;
    info.name = "FLAC Decoder";
    info.version = "1.0.0";
    info.author = "XpuMusic Team";
    info.description = "FLAC audio decoder using libFLAC";
    info.type = PluginType::AudioDecoder;
    info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
    info.supported_formats = {"flac", "oga"};
    return info;
}

std::string FLACDecoder::get_last_error() const {
    return last_error_;
}

bool FLACDecoder::can_decode(const std::string& file_path) {
    std::string ext = file_path.substr(file_path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == "flac" || ext == "oga";
}

std::vector<std::string> FLACDecoder::get_supported_extensions() {
    return {"flac", "oga"};
}

bool FLACDecoder::open(const std::string& file_path) {
    if (is_open_) {
        close();
    }

    file_path_ = file_path;

    // Initialize decoder if needed
    if (!decoder_ && !initialize_decoder()) {
        return false;
    }

    // Open file
    FLAC__StreamDecoderInitStatus init_status = FLAC__stream_decoder_init_file(
        decoder_,
        file_path.c_str(),
        true,  // seekable
        metadata_callback,
        error_callback,
        this
    );

    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        set_error("Failed to initialize FLAC decoder");
        return false;
    }

    // Set callbacks
    FLAC__bool ok = FLAC__stream_decoder_set_write_callback(decoder_, write_callback);
    ok &= FLAC__stream_decoder_set_seek_callback(decoder_, seek_callback);
    ok &= FLAC__stream_decoder_set_tell_callback(decoder_, tell_callback);
    ok &= FLAC__stream_decoder_set_length_callback(decoder_, length_callback);
    ok &= FLAC__stream_decoder_set_eof_callback(decoder_, eof_callback);

    if (!ok) {
        set_error("Failed to set FLAC callbacks");
        FLAC__stream_decoder_finish(decoder_);
        return false;
    }

    // Process until end of metadata
    FLAC__bool md_ok = FLAC__stream_decoder_process_until_end_of_metadata(
        decoder_,
        nullptr
    );

    if (!md_ok) {
        set_error("Failed to process FLAC metadata");
        FLAC__stream_decoder_finish(decoder_);
        return false;
    }

    // Check if we have stream info
    if (!FLAC__stream_decoder_get_channels(decoder_) ||
        !FLAC__stream_decoder_get_sample_rate(decoder_)) {
        set_error("Invalid FLAC stream");
        FLAC__stream_decoder_finish(decoder_);
        return false;
    }

    // Get stream info
    channels_ = FLAC__stream_decoder_get_channels(decoder_);
    sample_rate_ = FLAC__stream_decoder_get_sample_rate(decoder_);
    bits_per_sample_ = FLAC__stream_decoder_get_bits_per_sample(decoder_);
    total_samples_ = FLAC__stream_decoder_get_total_samples(decoder_);

    // Set audio format
    format_.sample_rate = sample_rate_;
    format_.channels = channels_;
    format_.bits_per_sample = 32;  // Convert to float
    format_.is_float = true;

    // Calculate duration
    calculate_duration();

    // Reset decode state
    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    end_of_stream_ = FLAC__false;
    current_sample_ = 0;

    is_open_ = true;
    set_state(PluginState::Active);

    return true;
}

int FLACDecoder::decode(AudioBuffer& buffer, int max_frames) {
    if (!is_open_ || get_state() != PluginState::Active || end_of_stream_) {
        return 0;
    }

    // Try to get more data from FLAC decoder if buffer is low
    while (output_buffer_size_ < max_frames * format_.channels * 2 && !end_of_stream_) {
        FLAC__bool ok = FLAC__stream_decoder_process_single(
            decoder_,
            nullptr
        );

        if (!ok) {
            // Check if we reached end of stream
            FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder_);
            if (state == FLAC__STREAM_DECODER_END_OF_STREAM) {
                end_of_stream_ = FLAC__true;
            }
            break;
        }
    }

    // Calculate how many frames we can return
    int available_samples = (output_buffer_size_ - output_buffer_pos_) / format_.channels;
    int frames_to_return = std::min(available_samples, max_frames);

    if (frames_to_return > 0) {
        // Resize buffer to match output size
        buffer.resize(format_.channels, frames_to_return);

        // Copy data from output buffer
        float* output_ptr = buffer.data();
        memcpy(output_ptr,
               output_buffer_.data() + output_buffer_pos_,
               frames_to_return * format_.channels * sizeof(float));

        // Update position
        output_buffer_pos_ += frames_to_return * format_.channels;
        current_sample_ += frames_to_return;

        // Compact output buffer if we've used a significant portion
        if (output_buffer_pos_ > output_buffer_.size() / 2) {
            size_t remaining = output_buffer_size_ - output_buffer_pos_;
            if (remaining > 0) {
                std::memmove(output_buffer_.data(),
                           output_buffer_.data() + output_buffer_pos_,
                           remaining * sizeof(float));
            }
            output_buffer_size_ = remaining;
            output_buffer_pos_ = 0;
        }

        return frames_to_return;
    }

    return 0;
}

void FLACDecoder::close() {
    if (is_open_ && decoder_) {
        FLAC__stream_decoder_finish(decoder_);
        is_open_ = FLAC__false;
    }

    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    end_of_stream_ = FLAC__false;
}

AudioFormat FLACDecoder::get_format() const {
    return format_;
}

bool FLACDecoder::seek(double seconds) {
    if (!is_open_ || !decoder_) {
        return false;
    }

    // Calculate target sample
    FLAC__uint64 target_sample = static_cast<FLAC__uint64>(seconds * sample_rate_);

    // Seek in FLAC stream
    FLAC__bool ok = FLAC__stream_decoder_seek_absolute(
        decoder_,
        target_sample
    );

    if (!ok) {
        set_error("Failed to seek in FLAC stream");
        return false;
    }

    // Reset decode state
    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    end_of_stream_ = FLAC__false;
    current_sample_ = target_sample;

    return true;
}

double FLACDecoder::get_duration() const {
    return duration_;
}

FLACDecoder::FLACMetadata FLACDecoder::get_metadata() const {
    return metadata_;
}

nlohmann::json FLACDecoder::get_metadata() const {
    nlohmann::json metadata;

    if (!metadata_.title.empty()) metadata["title"] = metadata_.title;
    if (!metadata_.artist.empty()) metadata["artist"] = metadata_.artist;
    if (!metadata_.album.empty()) metadata["album"] = metadata_.album;
    if (!metadata_.date.empty()) metadata["date"] = metadata_.date;
    if (!metadata_.comment.empty()) metadata["comment"] = metadata_.comment;
    if (!metadata_.genre.empty()) metadata["genre"] = metadata_.genre;
    if (metadata_.track_number > 0) metadata["track"] = metadata_.track_number;
    if (metadata_.total_tracks > 0) metadata["total_tracks"] = metadata_.total_tracks;

    metadata["duration"] = duration_;
    metadata["sample_rate"] = sample_rate_;
    metadata["channels"] = channels_;
    metadata["bits_per_sample"] = bits_per_sample_;
    metadata["total_samples"] = total_samples_;
    metadata["lossless"] = true;

    return metadata;
}

// Private methods
bool FLACDecoder::initialize_decoder() {
    decoder_ = FLAC__stream_decoder_new();
    if (!decoder_) {
        set_error("Failed to create FLAC decoder");
        return false;
    }

    // Set decoder options
    FLAC__stream_decoder_set_md5_checking(decoder_, false);
    FLAC__stream_decoder_set_metadata_respond(decoder_,
        FLAC__METADATA_TYPE_VORBIS_COMMENT |
        FLAC__METADATA_TYPE_PICTURE);

    return true;
}

void FLACDecoder::cleanup() {
    if (decoder_) {
        FLAC__stream_decoder_delete(decoder_);
        decoder_ = nullptr;
    }

    is_open_ = FLAC__false;
    output_buffer_size_ = 0;
    output_buffer_pos_ = 0;
    end_of_stream_ = FLAC__false;
}

void FLACDecoder::set_error(const std::string& error) {
    last_error_ = error;
    set_state(PluginState::Error);
}

void FLACDecoder::calculate_duration() {
    if (sample_rate_ > 0 && total_samples_ > 0) {
        duration_ = static_cast<double>(total_samples_) / sample_rate_;
    } else {
        duration_ = 0.0;
    }
}

// FLAC callbacks
FLAC__StreamDecoderWriteStatus FLACDecoder::write_callback(
    const FLAC__StreamDecoder* decoder,
    const FLAC__int32* const* buffer,
    unsigned samples,
    unsigned bytes_per_sample,
    unsigned frame_size,
    void* client_data) {

    FLACDecoder* self = static_cast<FLACDecoder*>(client_data);

    // Calculate total samples (frames * channels)
    unsigned total_samples = samples * self->channels_;

    // Ensure output buffer has enough space
    if (self->output_buffer_pos_ + total_samples > self->output_buffer_.size()) {
        self->output_buffer_.resize(self->output_buffer_.size() * 2);
    }

    // Convert FLAC int32 samples to float
    float* output_ptr = self->output_buffer_.data() + self->output_buffer_pos_;
    const float scale = 1.0f / static_cast<float>(1U << (self->bits_per_sample_ - 1));

    for (unsigned i = 0; i < total_samples; ++i) {
        *output_ptr++ = static_cast<float>(buffer[0][i]) * scale;
    }

    self->output_buffer_size_ += total_samples;
    self->output_buffer_pos_ += total_samples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus FLACDecoder::seek_callback(
    const FLAC__StreamDecoder* decoder,
    FLAC__uint64 absolute_byte_offset,
    void* client_data) {

    // Not used for file-based decoding
    return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
}

FLAC__StreamDecoderTellStatus FLACDecoder::tell_callback(
    const FLAC__StreamDecoder* decoder,
    FLAC__uint64* absolute_byte_offset,
    void* client_data) {

    // Not used for file-based decoding
    return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
}

FLAC__StreamDecoderLengthStatus FLACDecoder::length_callback(
    const FLAC__StreamDecoder* decoder,
    FLAC__uint64* stream_length,
    void* client_data) {

    // Not used for file-based decoding
    return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
}

FLAC__bool FLACDecoder::eof_callback(
    const FLAC__StreamDecoder* decoder,
    void* client_data) {

    FLACDecoder* self = static_cast<FLACDecoder*>(client_data);
    return self->end_of_stream_;
}

void FLACDecoder::metadata_callback(
    const FLAC__StreamDecoder* decoder,
    const FLAC__StreamMetadata* metadata,
    void* client_data) {

    FLACDecoder* self = static_cast<FLACDecoder*>(client_data);

    switch (metadata->type) {
        case FLAC__METADATA_TYPE_STREAMINFO:
            // Stream info is processed during open
            break;

        case FLAC__METADATA_TYPE_VORBIS_COMMENT:
            self->process_vorbis_comment(metadata);
            break;

        case FLAC__METADATA_TYPE_PICTURE:
            self->process_picture(metadata);
            break;

        default:
            // Ignore other metadata types
            break;
    }
}

void FLACDecoder::error_callback(
    const FLAC__StreamDecoder* decoder,
    FLAC__StreamDecoderErrorStatus status,
    void* client_data) {

    FLACDecoder* self = static_cast<FLACDecoder*>(client_data);

    const char* error_string = FLAC__StreamDecoderErrorStatusString[status];
    self->set_error(std::string("FLAC decode error: ") + error_string);
}

void FLACDecoder::process_vorbis_comment(const FLAC__StreamMetadata* metadata) {
    FLAC__StreamMetadata_VorbisComment_Entry entry;

    FLAC__metadata_get_vorbis_comment(metadata)->num_comments = 0;

    for (FLAC__uint32 i = 0; i < FLAC__metadata_get_vorbis_comment(metadata)->num_comments; ++i) {
        entry = FLAC__metadata_get_vorbis_comment(metadata)->comments[i];

        // Parse field/value pairs (FIELD=value)
        const char* entry_str = reinterpret_cast<const char*>(entry.entry);
        const char* equal = strchr(entry_str, '=');

        if (equal) {
            std::string field(entry_str, equal - entry_str);
            std::string value(equal + 1);

            // Common fields
            if (field == "TITLE") {
                metadata_.title = value;
            } else if (field == "ARTIST") {
                metadata_.artist = value;
            } else if (field == "ALBUM") {
                metadata_.album = value;
            } else if (field == "DATE") {
                metadata_.date = value;
            } else if (field == "COMMENT") {
                metadata_.comment = value;
            } else if (field == "GENRE") {
                metadata_.genre = value;
            } else if (field == "TRACKNUMBER") {
                metadata_.track_number = std::stoi(value);
            } else if (field == "TRACKTOTAL") {
                metadata_.total_tracks = std::stoi(value);
            }
        }
    }
}

void FLACDecoder::process_picture(const FLAC__StreamMetadata* metadata) {
    // TODO: Process embedded pictures (album art)
    // This would extract and store album art if needed
}

} // namespace xpumusic::plugins