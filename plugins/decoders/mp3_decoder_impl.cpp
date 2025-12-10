#include "mp3_decoder_impl.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <locale>
#include <codecvt>

#define MINIMP3_FLOAT_OUTPUT  // We want float output for DSP processing
#include <minimp3.h>

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

MP3Decoder::MP3Decoder() {
    memset(&mp3d_, 0, sizeof(mp3d_));
    memset(&mp3info_, 0, sizeof(mp3info_));

    input_buffer_.reserve(64 * 1024);  // 64KB input buffer
    input_buffer_size_ = 0;
    input_buffer_pos_ = 0;
}

MP3Decoder::~MP3Decoder() {
    cleanup();
}

bool MP3Decoder::initialize() {
    // Initialize minimp3
    mp3dec_init(&mp3d_);

    set_state(PluginState::Initialized);
    return true;
}


PluginState MP3Decoder::get_state() const {
    return IPlugin::get_state();
}

void MP3Decoder::set_state(PluginState state) {
    IPlugin::set_state(state);
}

PluginInfo MP3Decoder::get_info() const {
    PluginInfo info;
    info.name = "MP3 Decoder";
    info.version = "1.0.0";
    info.author = "XpuMusic Team";
    info.description = "MP3 audio decoder using minimp3 library";
    info.type = PluginType::AudioDecoder;
    info.api_version = XPUMUSIC_PLUGIN_API_VERSION;
    info.supported_formats = {"mp3", "mp2", "mp1"};
    return info;
}

std::string MP3Decoder::get_last_error() const {
    return last_error_;
}

bool MP3Decoder::can_decode(const std::string& file_path) {
    std::string ext = file_path.substr(file_path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == "mp3" || ext == "mp2" || ext == "mp1";
}

std::vector<std::string> MP3Decoder::get_supported_extensions() {
    return {"mp3", "mp2", "mp1"};
}

bool MP3Decoder::open(const std::string& file_path) {
    if (is_open_) {
        close();
    }

    file_ = fopen(file_path.c_str(), "rb");
    if (!file_) {
        set_error("Failed to open file: " + file_path);
        return false;
    }

    file_path_ = file_path;

    // Parse ID3v2 tag
    parse_id3v2_tag(file_);

    // Get file info
    mp3dec_file_info_t info;
    if (mp3dec_load(&mp3d_, file_path.c_str(), nullptr, nullptr, &info) == 0) {
        mp3info_ = info;

        // Set audio format
        format_.sample_rate = info.sample_rate;
        format_.channels = info.channels;
        format_.bits_per_sample = 32;
        format_.is_float = true;

        // Calculate duration
        duration_ = static_cast<double>(info.total_samples) / info.sample_rate;
        total_samples_ = info.total_samples;

        is_open_ = true;
        set_state(PluginState::Active);

        return true;
    } else {
        set_error("Failed to parse MP3 file");
        fclose(file_);
        file_ = nullptr;
        return false;
    }
}

int MP3Decoder::decode(AudioBuffer& buffer, int max_frames) {
    if (!is_open_ || get_state() != PluginState::Active) {
        return 0;
    }

    // Ensure output buffer is large enough
    int output_frames = std::min(max_frames, 4096);

    // Use a temporary internal buffer if needed
    static std::vector<float> temp_buffer;
    temp_buffer.resize(output_frames * format_.channels);

    // Initialize the AudioBuffer structure
    buffer.data = temp_buffer.data();
    buffer.frames = 0;
    buffer.channels = format_.channels;

    // Decode audio data
    int samples_decoded = 0;
    float* output_ptr = temp_buffer.data();

    while (samples_decoded < output_frames * format_.channels) {
        // Read input data
        if (!refill_input_buffer()) {
            break;  // End of file
        }

        // Decode one frame using float API
        mp3d_frame_t frame_info;
        int samples = mp3dec_decode_frame_float(
            &mp3d_,
            input_buffer_.data() + input_buffer_pos_,
            input_buffer_size_ - input_buffer_pos_,
            output_ptr + samples_decoded,
            &frame_info
        );

        if (samples > 0) {
            samples_decoded += samples;
        } else if (samples == 0) {
            // Need more input data
            // Skip frame_bytes since it's not available in our stub
            input_buffer_pos_ += 144; // Approximate frame size for MP3
            if (input_buffer_pos_ >= input_buffer_size_) {
                input_buffer_size_ = 0;
                input_buffer_pos_ = 0;
            }
        } else {
            // Error
            set_error("MP3 decode error");
            break;
        }
    }

    // Update buffer frame count
    buffer.frames = samples_decoded / format_.channels;

    // Update current position
    current_sample_ += buffer.frames;

    // Return number of frames decoded
    return buffer.frames;
}

void MP3Decoder::close() {
    if (is_open_) {
        cleanup();
    }
}

AudioFormat MP3Decoder::get_format() const {
    return format_;
}

bool MP3Decoder::seek(double seconds) {
    if (!is_open_) {
        return false;
    }

    // Calculate target sample position
    uint64_t target_sample = static_cast<uint64_t>(seconds * format_.sample_rate);

    // Simple implementation: reopen file and decode to target position
    // In real application, should use more efficient seek method
    std::string temp_path = file_path_;
    close();

    if (!open(temp_path)) {
        return false;
    }

    // Skip audio data to target position
    uint64_t samples_to_skip = target_sample;
    std::vector<float> temp_buffer(4096);

    while (samples_to_skip > 0) {
        AudioBuffer buffer;
        buffer.data = temp_buffer.data();
        buffer.channels = format_.channels;

        int frames = decode(buffer, 1024);
        if (frames <= 0) {
            break;
        }

        int samples_decoded = frames * format_.channels;
        if (samples_decoded >= samples_to_skip) {
            // Reached target position, need to re-decode this frame
            current_sample_ = target_sample;
            return true;
        }

        samples_to_skip -= samples_decoded;
    }

    current_sample_ = target_sample;
    return true;
}

MP3Decoder::ID3Tag MP3Decoder::get_id3_tag() const {
    return id3_tag_;
}


// Private methods implementation
bool MP3Decoder::parse_id3v1_tag(FILE* file) {
    // ID3v1 tag is at the end of file, 128 bytes
    if (fseek(file, -128, SEEK_END) != 0) {
        return false;
    }

    char tag[128];
    if (fread(tag, 1, 128, file) != 128) {
        return false;
    }

    // Check ID3v1 identifier
    if (strncmp(tag, "TAG", 3) != 0) {
        return false;
    }

    // Parse fields
    id3_tag_.title = std::string(tag + 3, 30);
    id3_tag_.artist = std::string(tag + 33, 30);
    id3_tag_.album = std::string(tag + 63, 30);
    id3_tag_.year = std::string(tag + 93, 4);
    id3_tag_.comment = std::string(tag + 97, 30);

    // Check track number (in the last two bytes of comment)
    if (tag[125] == 0 && tag[126] != 0) {
        id3_tag_.track = static_cast<unsigned char>(tag[126]);
    }

    // Remove spaces
    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(' '));
        s.erase(s.find_last_not_of(' ') + 1);
    };

    trim(id3_tag_.title);
    trim(id3_tag_.artist);
    trim(id3_tag_.album);
    trim(id3_tag_.year);
    trim(id3_tag_.comment);

    metadata_loaded_ = true;
    return true;
}

bool MP3Decoder::parse_id3v2_tag(FILE* file) {
    // Read ID3v2 header
    char header[10];
    if (fread(header, 1, 10, file) != 10) {
        return false;
    }

    // Check ID3v2 identifier
    if (strncmp(header, "ID3", 3) != 0) {
        return false;
    }

    // Parse version and size
    int version = (header[3] << 8) | header[4];
    int flags = header[5];

    // ID3v2 size uses synchsafe integer
    int size = (header[6] & 0x7F) << 21 |
               (header[7] & 0x7F) << 14 |
               (header[8] & 0x7F) << 7 |
               (header[9] & 0x7F);

    // Read tag frames
    std::vector<char> tag_data(size);
    if (fread(tag_data.data(), 1, size, file) != size) {
        return false;
    }

    // Parse frames (simplified version)
    int pos = 0;
    while (pos + 10 < size) {
        // Frame ID (4 bytes)
        std::string frame_id(tag_data.data() + pos, 4);
        pos += 4;

        // Frame size (4 bytes, synchsafe integer)
        int frame_size = 0;
        for (int i = 0; i < 4; ++i) {
            frame_size = (frame_size << 7) | (tag_data[pos + i] & 0x7F);
        }
        pos += 4;

        // Frame flags (2 bytes)
        pos += 2;

        // Frame data
        if (pos + frame_size <= size) {
            std::string data(tag_data.data() + pos, frame_size);

            // Decode text (ID3v2.2 uses ISO-8859-1, ID3v2.3/4 may use UTF-16)
            if (frame_id[0] == 'T') {  // Text information frame
                std::string value = parse_id3_text(
                    reinterpret_cast<const uint8_t*>(data.data()),
                    data.size()
                );

                if (frame_id == "TIT2" || frame_id == "TT2") {
                    id3_tag_.title = value;
                } else if (frame_id == "TPE1" || frame_id == "TP1") {
                    id3_tag_.artist = value;
                } else if (frame_id == "TALB" || frame_id == "TAL") {
                    id3_tag_.album = value;
                } else if (frame_id == "TYER" || frame_id == "TYE") {
                    id3_tag_.year = value;
                } else if (frame_id == "TRCK" || frame_id == "TRK") {
                    id3_tag_.track = parse_id3_int(
                        reinterpret_cast<const uint8_t*>(data.data()),
                        data.size()
                    );
                }
            }
        }

        pos += frame_size;

        // Align to next frame (if needed)
        if (flags & 0x01) {  // Unsynchronisation flag
            // Skip alignment bytes
            while (pos < size && tag_data[pos] == 0) {
                pos++;
            }
        }
    }

    metadata_loaded_ = true;
    return true;
}

std::string MP3Decoder::parse_id3_text(const uint8_t* data, size_t size, bool unicode) {
    if (size == 0) return "";

    if (unicode && size >= 3 && data[0] == 0xFF && data[1] == 0xFE) {
        // UTF-16LE with BOM - skip BOM and handle text
        // For simplicity, just extract ASCII characters
        std::string result;
        for (size_t i = 2; i + 1 < size; i += 2) {
            if (data[i+1] == 0 && data[i] >= 32 && data[i] < 127) {
                // ASCII character
                result += static_cast<char>(data[i]);
            } else if (data[i+1] != 0 || data[i] != 0) {
                // Non-ASCII - add placeholder
                result += '?';
            }
        }
        return result;
    } else {
        // ISO-8859-1 or UTF-16 without BOM, treat as ISO-8859-1
        return std::string(reinterpret_cast<const char*>(data), size);
    }
}

int MP3Decoder::parse_id3_int(const uint8_t* data, size_t size) {
    std::string str = parse_id3_text(data, size);
    try {
        return std::stoi(str);
    } catch (...) {
        return 0;
    }
}

bool MP3Decoder::refill_input_buffer() {
    if (input_buffer_pos_ >= input_buffer_size_) {
        // Read new data
        input_buffer_size_ = fread(
            input_buffer_.data(),
            1,
            input_buffer_.capacity(),
            file_
        );
        input_buffer_pos_ = 0;

        return input_buffer_size_ > 0;
    }
    return input_buffer_pos_ < input_buffer_size_;
}

void MP3Decoder::cleanup() {
    if (file_) {
        fclose(file_);
        file_ = nullptr;
    }

    is_open_ = false;
    input_buffer_size_ = 0;
    input_buffer_pos_ = 0;
    current_sample_ = 0;
    memset(&mp3info_, 0, sizeof(mp3info_));
}

void MP3Decoder::set_error(const std::string& error) {
    last_error_ = error;
    set_state(PluginState::Error);
}

void MP3Decoder::finalize() {
    cleanup();
    set_state(PluginState::Uninitialized);
}

bool MP3Decoder::seek(int64_t sample_pos) {
    if (!is_open_) {
        return false;
    }

    // Convert sample position to seconds
    double seconds = static_cast<double>(sample_pos) / format_.sample_rate;
    return seek(seconds);
}

int64_t MP3Decoder::get_length() const {
    return static_cast<int64_t>(total_samples_);
}

double MP3Decoder::get_duration() const {
    return duration_;
}

std::vector<MetadataItem> MP3Decoder::get_metadata() {
    std::vector<MetadataItem> items;

    if (!id3_tag_.title.empty()) {
        items.emplace_back("TITLE", id3_tag_.title);
    }

    if (!id3_tag_.artist.empty()) {
        items.emplace_back("ARTIST", id3_tag_.artist);
    }

    if (!id3_tag_.album.empty()) {
        items.emplace_back("ALBUM", id3_tag_.album);
    }

    // Add more fields as needed

    return items;
}

std::string MP3Decoder::get_metadata_value(const std::string& key) {
    if (key == "TITLE" || key == "title") return id3_tag_.title;
    if (key == "ARTIST" || key == "artist") return id3_tag_.artist;
    if (key == "ALBUM" || key == "album") return id3_tag_.album;
    if (key == "YEAR" || key == "year") return id3_tag_.year;
    if (key == "COMMENT" || key == "comment") return id3_tag_.comment;
    if (key == "GENRE" || key == "genre") return id3_tag_.genre;
    if (key == "TRACK" || key == "track") return std::to_string(id3_tag_.track);

    return "";
}

int64_t MP3Decoder::get_position() const {
    return current_sample_;
}

bool MP3Decoder::is_eof() const {
    // Simplified check - in a real implementation, would check actual stream state
    return feof(file_) != 0;
}

} // namespace xpumusic::plugins