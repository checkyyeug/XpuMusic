/**
 * @file plugin_audio_decoder.h
 * @brief Audio decoder that uses foobar2000 plugins
 * @date 2025-12-10
 */

#pragma once

#include "../../compat/plugin_loader/plugin_loader.h"
#include "../../compat/xpumusic_sdk/foobar2000.h"
#include "../../compat/xpumusic_sdk/input_decoder.h"
#include "sample_rate_converter.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

// Forward declarations
class WAVWriter;

/**
 * @class PluginAudioDecoder
 * @brief Audio decoder supporting multiple formats via foobar2000 plugins
 *
 * This class provides a unified interface for decoding audio files
 * using foobar2000 input decoder plugins. It supports:
 * - Multiple audio formats (MP3, FLAC, WAV, OGG, etc.)
 * - Automatic plugin selection based on file extension
 * - Sample rate conversion to target format
 * - Metadata extraction
 */
class PluginAudioDecoder {
public:
    /**
     * @brief Audio format information
     */
    struct AudioInfo {
        int sample_rate;
        int channels;
        int bits_per_sample;
        int64_t total_samples;
        double duration_seconds;
        std::string format_name;
        std::string encoder_info;

        AudioInfo() : sample_rate(0), channels(0), bits_per_sample(0),
                     total_samples(0), duration_seconds(0.0) {}
    };

    /**
     * @brief Metadata key-value pair
     */
    struct Metadata {
        std::string key;
        std::string value;
    };

private:
    // Plugin management
    mp::compat::XpuMusicPluginLoader* plugin_loader_;
    // std::unique_ptr<mp::compat::ServiceRegistryBridge> service_bridge_;

    // Current decoder instance
    xpumusic_sdk::service_ptr_t<xpumusic_sdk::input_decoder> current_decoder_;
    std::string current_file_path_;

    // Audio information
    AudioInfo audio_info_;
    std::vector<Metadata> metadata_;

    // Sample rate conversion (if needed)
    std::unique_ptr<audio::ISampleRateConverter> resampler_;
    int target_sample_rate_;

    // Internal buffer for converted data
    std::vector<float> conversion_buffer_;

    // Built-in decoder registration
    std::map<std::string, std::string> builtin_decoders_;

    // Plugin registration
    bool initialize_plugin_system();
    void register_known_decoders();
    bool register_builtin_decoder(const std::string& extension, const std::string& name);
    xpumusic_sdk::service_ptr_t<xpumusic_sdk::input_decoder> find_decoder_for_file(const char* path) const;

public:
    /**
     * @brief Constructor
     * @param plugin_loader Optional external plugin loader
     * @param target_rate Target sample rate for output (0 = no conversion)
     */
    PluginAudioDecoder(mp::compat::XpuMusicPluginLoader* plugin_loader = nullptr,
                      int target_rate = 0);

    /**
     * @brief Destructor
     */
    ~PluginAudioDecoder();

    /**
     * @brief Initialize the decoder system
     * @param plugin_directory Optional directory to load plugins from
     * @return true on success
     */
    bool initialize(const char* plugin_directory = nullptr);

    /**
     * @brief Open an audio file
     * @param path File path
     * @return true on success
     */
    bool open_file(const char* path);

    /**
     * @brief Close current file
     */
    void close_file();

    /**
     * @brief Decode audio frames
     * @param output Buffer to receive decoded samples (interleaved float)
     * @param max_frames Maximum frames to decode
     * @return Number of frames actually decoded
     */
    int decode_frames(float* output, int max_frames);

    /**
     * @brief Seek to a specific position
     * @param position Sample position to seek to
     * @return true on success
     */
    bool seek(int64_t position);

    /**
     * @brief Get audio format information
     * @return AudioInfo structure
     */
    const AudioInfo& get_audio_info() const { return audio_info_; }

    /**
     * @brief Get metadata
     * @return Vector of metadata entries
     */
    const std::vector<Metadata>& get_metadata() const { return metadata_; }

    /**
     * @brief Get specific metadata value
     * @param key Metadata key (e.g., "TITLE", "ARTIST")
     * @return Value or empty string if not found
     */
    std::string get_metadata(const char* key) const;

    /**
     * @brief Check if a file can be decoded
     * @param path File path
     * @return true if a decoder is available
     */
    bool can_decode(const char* path) const;

    /**
     * @brief Get supported file extensions
     * @return Vector of supported extensions (without dot)
     */
    std::vector<std::string> get_supported_extensions() const;

    /**
     * @brief Set target sample rate
     * @param rate Target rate (0 to disable conversion)
     */
    void set_target_sample_rate(int rate);

    /**
     * @brief Get current decoder name
     * @return Decoder name or empty string
     */
    std::string get_decoder_name() const;

    /**
     * @brief Load plugins from directory
     * @param directory Directory containing plugin files
     * @return Number of plugins loaded
     */
    int load_plugins_from_directory(const char* directory);

    /**
     * @brief Get plugin loading statistics
     * @return Number of loaded plugins and decoders
     */
    std::pair<size_t, size_t> get_plugin_stats() const;
};

/**
 * @class PluginAudioDecoderFactory
 * @brief Factory for creating PluginAudioDecoder instances
 */
class PluginAudioDecoderFactory {
private:
    static std::unique_ptr<PluginAudioDecoder> shared_decoder_;

public:
    /**
     * @brief Create a new decoder instance
     * @param plugin_directory Optional plugin directory
     * @param target_rate Optional target sample rate
     * @return Unique pointer to decoder
     */
    static std::unique_ptr<PluginAudioDecoder> create(
        const char* plugin_directory = nullptr,
        int target_rate = 0);

    /**
     * @brief Get shared decoder instance
     * @return Reference to shared decoder
     */
    static PluginAudioDecoder& get_shared();

    /**
     * @brief Check if plugin system is initialized
     * @return true if initialized
     */
    static bool is_initialized();
};