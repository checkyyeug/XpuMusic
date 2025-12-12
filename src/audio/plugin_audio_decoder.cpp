/**
 * @file plugin_audio_decoder.cpp
 * @brief PluginAudioDecoder implementation
 * @date 2025-12-10
 */

#include "plugin_audio_decoder.h"
#include "cubic_resampler.h"
#include "../../compat/xpumusic_compat_manager.h"
#include "../../compat/sdk_implementations/input_decoder_impl.h"
#include "../../compat/xpumusic_sdk/input_decoder.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>

// External service query function
extern "C" FOOBAR2000_EXPORT bool service_query(const xpumusic_sdk::GUID&, void**);

// Static factory members
std::unique_ptr<PluginAudioDecoder> PluginAudioDecoderFactory::shared_decoder_;

PluginAudioDecoder::PluginAudioDecoder(mp::compat::XpuMusicPluginLoader* plugin_loader,
                                     int target_rate)
    : plugin_loader_(plugin_loader)
    , target_sample_rate_(target_rate) {

    audio_info_ = AudioInfo();

    // Create service bridge if needed
    if (!plugin_loader_) {
        auto compat_manager = std::make_unique<mp::compat::XpuMusicCompatManager>();
        plugin_loader_ = new mp::compat::XpuMusicPluginLoader(compat_manager.get());
        // service_bridge_ = std::make_unique<mp::compat::ServiceRegistryBridge>(
        //     /*service_registry=*/nullptr); // We'll need to integrate with actual registry
    }
}

PluginAudioDecoder::~PluginAudioDecoder() {
    close_file();

    // Clean up if we own the loader
    if (plugin_loader_) {
        delete plugin_loader_;
    }
}

bool PluginAudioDecoder::initialize(const char* plugin_directory) {
    // Initialize plugin system
    if (!initialize_plugin_system()) {
        std::cerr << "Failed to initialize plugin system" << std::endl;
        return false;
    }

    // Load plugins from directory if provided
    if (plugin_directory) {
        int loaded = load_plugins_from_directory(plugin_directory);
        std::cout << "Loaded " << loaded << " plugins from " << plugin_directory << std::endl;
    }

    // Register known decoders
    register_known_decoders();

    return true;
}

bool PluginAudioDecoder::initialize_plugin_system() {
    // Initialize foobar2000 compatibility
    mp::compat::CompatConfig config;
    config.enable_plugin_compat = true;
    config.adapter_logging_level = 2; // Warnings

    // Note: This would need the actual XpuMusicCompatManager initialization
    // For now, we'll assume it's initialized externally

    return true;
}

void PluginAudioDecoder::register_known_decoders() {
    // Register built-in decoders
    using namespace foobar2000;

    // Register built-in decoders with simplified approach
    // For now, we'll use a basic decoder registration system
    
    // WAV decoder - use built-in decoder
    if (!register_builtin_decoder("wav", "WAV Decoder")) {
        std::cerr << "Failed to register WAV decoder" << std::endl;
    }
    
    // FLAC decoder - use built-in decoder
    if (!register_builtin_decoder("flac", "FLAC Decoder")) {
        std::cerr << "Failed to register FLAC decoder" << std::endl;
    }
    
    // MP3 decoder - use built-in decoder
    if (!register_builtin_decoder("mp3", "MP3 Decoder")) {
        std::cerr << "Failed to register MP3 decoder" << std::endl;
    }
    
    // OGG decoder - use built-in decoder
    if (!register_builtin_decoder("ogg", "OGG Vorbis Decoder")) {
        std::cerr << "Failed to register OGG decoder" << std::endl;
    }
}

bool PluginAudioDecoder::open_file(const char* path) {
    if (!path) return false;

    // Close any existing file
    close_file();

    // Find appropriate decoder
    current_decoder_ = find_decoder_for_file(path);
    if (!current_decoder_.is_valid()) {
        std::cerr << "No decoder found for file: " << path << std::endl;
        return false;
    }

    // Try to initialize the decoder
    xpumusic_sdk::abort_callback_dummy abort;
    if (!current_decoder_->initialize(path, abort)) {
        std::cerr << "Failed to initialize decoder for file: " << path << std::endl;
        current_decoder_.release();
        return false;
    }

    // Store file path
    current_file_path_ = path;

    // Get audio information
    // TODO: Implement file_info retrieval
    // For now, set defaults
    audio_info_.sample_rate = 44100;
    audio_info_.channels = 2;
    audio_info_.bits_per_sample = 16;
    audio_info_.total_samples = 0;
    audio_info_.duration_seconds = 0.0;
    audio_info_.format_name = get_decoder_name();

    // Setup resampler if needed
    if (target_sample_rate_ > 0 && target_sample_rate_ != audio_info_.sample_rate) {
        resampler_ = audio::CubicSampleRateConverterFactory::create();
        if (!resampler_->initialize(audio_info_.sample_rate, target_sample_rate_, audio_info_.channels)) {
            std::cerr << "Failed to initialize resampler" << std::endl;
            resampler_.reset();
        }
    }

    // Extract metadata (simplified)
    metadata_.clear();
    // In a real implementation, we would extract metadata from the file

    return true;
}

void PluginAudioDecoder::close_file() {
    if (current_decoder_.is_valid()) {
        current_decoder_.release();
    }

    current_file_path_.clear();
    audio_info_ = AudioInfo();
    metadata_.clear();
    resampler_.reset();
    conversion_buffer_.clear();
}

int PluginAudioDecoder::decode_frames(float* output, int max_frames) {
    if (!current_decoder_.is_valid() || !output || max_frames <= 0) {
        return 0;
    }

    int frames_decoded = 0;

    // If using resampler
    if (resampler_) {
        // Calculate how many input frames we need
        int ratio = static_cast<int>(target_sample_rate_) / audio_info_.sample_rate;
        int input_frames_needed = max_frames * audio_info_.sample_rate / target_sample_rate_ + 1;

        // Ensure conversion buffer is large enough
        conversion_buffer_.resize(input_frames_needed * audio_info_.channels);

        // Create audio chunk for decoding with resampling
        auto audio_chunk = std::make_unique<xpumusic_sdk::audio_chunk_impl>();
        if (audio_chunk->set_data(conversion_buffer_.data(), input_frames_needed,
                                  audio_info_.channels, audio_info_.sample_rate)) {
            frames_decoded = current_decoder_->decode_run(*audio_chunk, output, max_frames, abort);
            
            // Resample if needed
            if (resampler_ && frames_decoded > 0) {
                frames_decoded = resampler_->convert_to_target(output, frames_decoded, target_sample_rate_);
            }
        }
    } else {
        // Decode directly without resampling
        auto audio_chunk = std::make_unique<xpumusic_sdk::audio_chunk_impl>();
        if (audio_chunk->set_data(output, max_frames, audio_info_.channels, audio_info_.sample_rate)) {
            frames_decoded = current_decoder_->decode_run(*audio_chunk, output, max_frames, abort);
        }
    }

    return frames_decoded;
}

bool PluginAudioDecoder::seek(int64_t position) {
    if (!current_decoder_.is_valid()) {
        return false;
    }

    xpumusic_sdk::abort_callback_dummy abort;
    double seconds = static_cast<double>(position) / audio_info_.sample_rate;
    return current_decoder_->seek(seconds, abort);
}

std::string PluginAudioDecoder::get_metadata(const char* key) const {
    if (!key) return "";

    auto it = std::find_if(metadata_.begin(), metadata_.end(),
        [key](const Metadata& m) { return m.key == key; });

    return (it != metadata_.end()) ? it->value : "";
}

bool PluginAudioDecoder::can_decode(const char* path) const {
    if (!path) return false;

    auto decoder = find_decoder_for_file(path);
    return decoder.is_valid();
}

std::vector<std::string> PluginAudioDecoder::get_supported_extensions() const {
    std::vector<std::string> extensions;

    // Common audio formats
    extensions.push_back("wav");
    extensions.push_back("flac");
    extensions.push_back("mp3");
    extensions.push_back("ogg");
    extensions.push_back("m4a");
    extensions.push_back("aac");
    extensions.push_back("wma");
    extensions.push_back("ape");
    extensions.push_back("m4a");
    extensions.push_back("mp4");

    return extensions;
}

void PluginAudioDecoder::set_target_sample_rate(int rate) {
    if (rate != target_sample_rate_) {
        target_sample_rate_ = rate;

        // Reinitialize resampler if file is open
        if (current_decoder_.is_valid() && rate > 0 && rate != audio_info_.sample_rate) {
            resampler_ = audio::CubicSampleRateConverterFactory::create();
            if (!resampler_->initialize(audio_info_.sample_rate, rate, audio_info_.channels)) {
                resampler_.reset();
            }
        }
    }
}

std::string PluginAudioDecoder::get_decoder_name() const {
    if (!current_decoder_.is_valid()) {
        return "";
    }

    // In a real implementation, we would get the name from the decoder
    // For now, return based on file extension
    if (current_file_path_.empty()) {
        return "Unknown";
    }

    std::filesystem::path path(current_file_path_);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".mp3") return "MP3 Decoder";
    if (ext == ".flac") return "FLAC Decoder";
    if (ext == ".wav") return "WAV Decoder";
    if (ext == ".ogg") return "OGG Vorbis Decoder";
    if (ext == ".m4a" || ext == ".aac") return "AAC Decoder";

    return "Generic Decoder";
}

int PluginAudioDecoder::load_plugins_from_directory(const char* directory) {
    if (!directory || !plugin_loader_) {
        return 0;
    }

    // Load plugins using the plugin loader
    auto result = plugin_loader_->load_plugins_from_directory(directory);
    if (result != mp::Result::Success) {
        std::cerr << "Failed to load plugins from " << directory << std::endl;
        return 0;
    }

    return static_cast<int>(plugin_loader_->get_module_count());
}

std::pair<size_t, size_t> PluginAudioDecoder::get_plugin_stats() const {
    size_t plugin_count = 0;
    size_t decoder_count = 0;

    if (plugin_loader_) {
        plugin_count = plugin_loader_->get_module_count();
        const auto& services = plugin_loader_->get_services();

        // Count input decoder services
        for (const auto& service : services) {
            if (service.available && service.name.find("input") != std::string::npos) {
                decoder_count++;
            }
        }
    }

    return {plugin_count, decoder_count};
}

bool PluginAudioDecoder::register_builtin_decoder(const std::string& extension, const std::string& name) {
    // For now, just store the decoder info for basic format support
    // In a full implementation, this would register actual decoder services
    builtin_decoders_[extension] = name;
    return true;
}

xpumusic_sdk::service_ptr_t<xpumusic_sdk::input_decoder>
PluginAudioDecoder::find_decoder_for_file(const char* path) const {
    using namespace xpumusic_sdk;

    if (!path) return service_ptr_t<input_decoder>();

    // Get file extension
    std::filesystem::path file_path(path);
    std::string ext = file_path.extension().string();
    if (ext.empty()) return service_ptr_t<input_decoder>();

    // Remove dot and convert to lowercase
    if (ext[0] == '.') ext = ext.substr(1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    void* service_ptr = nullptr;

    // Check against built-in decoders first
    auto it = builtin_decoders_.find(ext);
    if (it != builtin_decoders_.end()) {
        // For now, create a basic decoder placeholder
        // In a full implementation, this would return actual decoder instances
        // For now, we'll return an invalid decoder to indicate format support
        // This allows the decoder to attempt to open the file
        return service_ptr_t<input_decoder>();
    }

    // TODO: Implement service lookup for plugins when available
    // For now, return empty decoder for unsupported formats
    return service_ptr_t<input_decoder>();
}

// Factory implementation
std::unique_ptr<PluginAudioDecoder> PluginAudioDecoderFactory::create(
    const char* plugin_directory, int target_rate) {

    auto decoder = std::make_unique<PluginAudioDecoder>(nullptr, target_rate);

    if (!decoder->initialize(plugin_directory)) {
        return nullptr;
    }

    return decoder;
}

PluginAudioDecoder& PluginAudioDecoderFactory::get_shared() {
    if (!shared_decoder_) {
        shared_decoder_ = create();
    }
    return *shared_decoder_;
}

bool PluginAudioDecoderFactory::is_initialized() {
    return shared_decoder_ != nullptr;
}