#pragma once

#include "mp_types.h"
#include "mp_decoder.h"
#include "mp_audio_output.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <string>

namespace mp {
namespace core {

// Playback state
enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
    Transitioning  // During gapless track change
};

// Track information for playback
struct TrackInfo {
    std::string file_path;
    uint64_t encoder_delay;     // Samples to skip at start
    uint64_t encoder_padding;   // Samples to skip at end
    uint64_t total_samples;
    
    TrackInfo() : encoder_delay(0), encoder_padding(0), total_samples(0) {}
};

// Decoder instance wrapper
struct DecoderInstance {
    IDecoder* decoder;
    DecoderHandle handle;
    AudioStreamInfo stream_info;
    TrackInfo track_info;
    uint64_t current_position;  // In samples
    bool active;
    bool eos;  // End of stream reached
    
    DecoderInstance() 
        : decoder(nullptr)
        , current_position(0)
        , active(false)
        , eos(false) {
        handle.internal = nullptr;
        std::memset(&stream_info, 0, sizeof(stream_info));
    }
};

// Playback engine with gapless support
class PlaybackEngine {
public:
    PlaybackEngine();
    ~PlaybackEngine();
    
    // Initialize playback engine
    Result initialize(IAudioOutput* audio_output);
    
    // Shutdown playback engine
    void shutdown();
    
    // Load track for playback (to primary decoder)
    Result load_track(const std::string& file_path, IDecoder* decoder);
    
    // Prepare next track for gapless transition
    Result prepare_next_track(const std::string& file_path, IDecoder* decoder);
    
    // Start playback
    Result play();
    
    // Pause playback
    Result pause();
    
    // Stop playback
    Result stop();
    
    // Seek to position (in milliseconds)
    Result seek(uint64_t position_ms);
    
    // Get current playback position (in milliseconds)
    uint64_t get_position() const;
    
    // Get track duration (in milliseconds)
    uint64_t get_duration() const;
    
    // Get playback state
    PlaybackState get_state() const { return state_; }
    
    // Get current track info
    const TrackInfo& get_current_track() const { return decoders_[current_decoder_].track_info; }
    
    // Check if next track is ready for gapless
    bool is_next_track_ready() const { return next_decoder_ >= 0; }
    
    // Trigger gapless transition to next track
    Result transition_to_next();
    
    // Set volume (0.0 to 1.0)
    void set_volume(float volume);
    
    // Get volume
    float get_volume() const { return volume_; }
    
    // Enable/disable gapless playback
    void set_gapless_enabled(bool enabled) { gapless_enabled_ = enabled; }
    bool is_gapless_enabled() const { return gapless_enabled_; }
    
private:
    // Audio callback function
    static void audio_callback(void* buffer, size_t frames, void* user_data);
    
    // Fill audio buffer (called from audio callback)
    void fill_buffer(float* buffer, size_t frames);
    
    // Decode samples from active decoder
    size_t decode_samples(int decoder_idx, float* buffer, size_t frames);
    
    // Switch to next decoder (gapless transition)
    void switch_decoder();
    
    // Check if approaching end of track
    bool is_approaching_end() const;
    
    // Close decoder instance
    void close_decoder(int decoder_idx);
    
    IAudioOutput* audio_output_;
    DecoderInstance decoders_[2];  // Dual decoder setup (A/B)
    int current_decoder_;          // Index of current active decoder (0 or 1)
    int next_decoder_;             // Index of next decoder (-1 if none)
    
    std::atomic<PlaybackState> state_;
    std::atomic<float> volume_;
    std::atomic<bool> gapless_enabled_;
    
    mutable std::mutex mutex_;
    
    // Pre-buffering threshold (in milliseconds)
    static constexpr uint64_t PREBUFFER_THRESHOLD_MS = 5000;  // 5 seconds
    
    // Crossfade duration for sample rate changes (in milliseconds)
    static constexpr uint64_t CROSSFADE_DURATION_MS = 50;
    
    bool initialized_;
};

}} // namespace mp::core
