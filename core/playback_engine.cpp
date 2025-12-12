#include "playback_engine.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace mp {
namespace core {

PlaybackEngine::PlaybackEngine()
    : audio_output_(nullptr)
    , current_decoder_(0)
    , next_decoder_(-1)
    , state_(PlaybackState::Stopped)
    , volume_(1.0f)
    , gapless_enabled_(true)
    , initialized_(false) {
}

PlaybackEngine::~PlaybackEngine() {
    shutdown();
}

Result PlaybackEngine::initialize(IAudioOutput* audio_output) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return Result::AlreadyInitialized;
    }
    
    if (!audio_output) {
        return Result::InvalidParameter;
    }
    
    audio_output_ = audio_output;
    current_decoder_ = 0;
    next_decoder_ = -1;
    initialized_ = true;
    
    return Result::Success;
}

void PlaybackEngine::shutdown() {
    stop();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    close_decoder(0);
    close_decoder(1);
    
    audio_output_ = nullptr;
    initialized_ = false;
}

Result PlaybackEngine::load_track(const std::string& file_path, IDecoder* decoder) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    // Close current decoder
    close_decoder(current_decoder_);
    
    // Open new track
    DecoderInstance& inst = decoders_[current_decoder_];
    inst.decoder = decoder;
    inst.track_info.file_path = file_path;
    
    Result result = decoder->open_stream(file_path.c_str(), &inst.handle);
    if (result != Result::Success) {
        std::cerr << "Failed to open track: " << file_path << std::endl;
        return result;
    }
    
    // Get stream info
    result = decoder->get_stream_info(inst.handle, &inst.stream_info);
    if (result != Result::Success) {
        decoder->close_stream(inst.handle);
        inst.handle.internal = nullptr;
        return result;
    }
    
    inst.track_info.total_samples = inst.stream_info.total_samples;
    inst.current_position = 0;
    inst.active = false;
    inst.eos = false;
    
    // TODO: Parse encoder delay/padding from metadata
    // For now, set to 0
    inst.track_info.encoder_delay = 0;
    inst.track_info.encoder_padding = 0;
    
    std::cout << "Loaded track: " << file_path << std::endl;
    std::cout << "  Sample rate: " << inst.stream_info.sample_rate << " Hz" << std::endl;
    std::cout << "  Channels: " << inst.stream_info.channels << std::endl;
    std::cout << "  Duration: " << (inst.stream_info.duration_ms / 1000) << " seconds" << std::endl;
    
    return Result::Success;
}

Result PlaybackEngine::prepare_next_track(const std::string& file_path, IDecoder* decoder) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    // Determine next decoder index (alternate between 0 and 1)
    int next_idx = (current_decoder_ == 0) ? 1 : 0;
    
    // Close previous next decoder if any
    close_decoder(next_idx);
    
    // Open next track
    DecoderInstance& inst = decoders_[next_idx];
    inst.decoder = decoder;
    inst.track_info.file_path = file_path;
    
    Result result = decoder->open_stream(file_path.c_str(), &inst.handle);
    if (result != Result::Success) {
        std::cerr << "Failed to prepare next track: " << file_path << std::endl;
        return result;
    }
    
    // Get stream info
    result = decoder->get_stream_info(inst.handle, &inst.stream_info);
    if (result != Result::Success) {
        decoder->close_stream(inst.handle);
        inst.handle.internal = nullptr;
        return result;
    }
    
    inst.track_info.total_samples = inst.stream_info.total_samples;
    inst.current_position = 0;
    inst.active = false;
    inst.eos = false;
    
    next_decoder_ = next_idx;
    
    std::cout << "Prepared next track: " << file_path << std::endl;
    
    return Result::Success;
}

Result PlaybackEngine::play() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Result::NotInitialized;
    }

    if (state_ == PlaybackState::Playing) {
        return Result::Success;  // Already playing
    }

    // Check if we have a loaded track
    if (!decoders_[current_decoder_].handle.internal) {
        return Result::InvalidState;
    }

    // Ensure decoder is marked as active
    decoders_[current_decoder_].active = true;
    decoders_[current_decoder_].eos = false;

    // Configure and start audio output with system-preferred format for compatibility
    if (state_ == PlaybackState::Stopped) {
        AudioOutputConfig config;
        config.device_id = nullptr;  // Use default device
        config.sample_rate = 48000;  // Use system preferred rate per analyst report
        config.channels = 2;         // Force stereo for compatibility
        config.format = SampleFormat::Float32;  // Use float for processing
        config.buffer_frames = 1024;  // Smaller buffer for lower latency
        config.callback = audio_callback;
        config.user_data = this;

        std::cout << "Configuring audio output: " << config.sample_rate
                  << " Hz, " << config.channels << " channels" << std::endl;

        Result result = audio_output_->open(config);
        if (result != Result::Success) {
            std::cerr << "Failed to open audio output: " << static_cast<int>(result) << std::endl;
            return result;
        }
    }

    state_ = PlaybackState::Playing;

    Result result = audio_output_->start();
    if (result != Result::Success) {
        state_ = PlaybackState::Stopped;
        decoders_[current_decoder_].active = false;
        std::cerr << "Failed to start audio output: " << static_cast<int>(result) << std::endl;
        return result;
    }

    std::cout << "鉁?Playback started successfully" << std::endl;
    return Result::Success;
}

Result PlaybackEngine::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (state_ != PlaybackState::Playing) {
        return Result::Success;
    }
    
    audio_output_->stop();
    state_ = PlaybackState::Paused;
    
    std::cout << "Playback paused" << std::endl;
    return Result::Success;
}

Result PlaybackEngine::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    if (state_ == PlaybackState::Stopped) {
        return Result::Success;
    }
    
    audio_output_->stop();
    audio_output_->close();
    
    decoders_[current_decoder_].active = false;
    decoders_[current_decoder_].current_position = 0;
    
    state_ = PlaybackState::Stopped;
    
    std::cout << "Playback stopped" << std::endl;
    return Result::Success;
}

Result PlaybackEngine::seek(uint64_t position_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return Result::NotInitialized;
    }
    
    DecoderInstance& inst = decoders_[current_decoder_];
    if (!inst.handle.internal) {
        return Result::InvalidState;
    }
    
    uint64_t actual_position = 0;
    Result result = inst.decoder->seek(inst.handle, position_ms, &actual_position);
    if (result != Result::Success) {
        return result;
    }
    
    // Update position
    inst.current_position = (actual_position * inst.stream_info.sample_rate) / 1000;
    inst.eos = false;
    
    return Result::Success;
}

uint64_t PlaybackEngine::get_position() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const DecoderInstance& inst = decoders_[current_decoder_];
    if (inst.stream_info.sample_rate == 0) {
        return 0;
    }
    
    return (inst.current_position * 1000) / inst.stream_info.sample_rate;
}

uint64_t PlaybackEngine::get_duration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return decoders_[current_decoder_].stream_info.duration_ms;
}

Result PlaybackEngine::transition_to_next() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (next_decoder_ < 0) {
        return Result::InvalidState;  // No next track prepared
    }
    
    std::cout << "Transitioning to next track (gapless)" << std::endl;
    
    // Close current decoder
    close_decoder(current_decoder_);
    
    // Switch to next decoder
    current_decoder_ = next_decoder_;
    next_decoder_ = -1;
    
    decoders_[current_decoder_].active = true;
    
    return Result::Success;
}

void PlaybackEngine::set_volume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    if (audio_output_) {
        audio_output_->set_volume(volume_);
    }
}

void PlaybackEngine::audio_callback(void* buffer, size_t frames, void* user_data) {
    PlaybackEngine* engine = static_cast<PlaybackEngine*>(user_data);
    engine->fill_buffer(static_cast<float*>(buffer), frames);
}

void PlaybackEngine::fill_buffer(float* buffer, size_t frames) {
    // This is called from audio thread - must be real-time safe

    // Initialize buffer to silence first - always stereo for safety
    std::memset(buffer, 0, frames * 2 * sizeof(float));

    if (state_ != PlaybackState::Playing) {
        return;  // Just return silence
    }

    // Safety check: ensure decoder is valid
    if (current_decoder_ < 0 || current_decoder_ >= 2) {
        state_ = PlaybackState::Stopped;
        return;
    }

    const DecoderInstance& decoder = decoders_[current_decoder_];

    // Safety check: ensure decoder is properly initialized
    if (!decoder.handle.internal || !decoder.decoder) {
        state_ = PlaybackState::Stopped;
        return;
    }

    // Safety check: ensure stream info is valid
    if (decoder.stream_info.channels == 0 || decoder.stream_info.sample_rate == 0) {
        state_ = PlaybackState::Stopped;
        return;
    }

    // Simple decode without gapless for now (avoid complexity that could cause crashes)
    size_t decoded = decode_samples(current_decoder_, buffer, frames);

    if (decoded < frames) {
        // End of track - fill remaining with silence
        size_t silence_samples = (frames - decoded) * decoder.stream_info.channels;
        float* silence_start = buffer + (decoded * decoder.stream_info.channels);
        std::memset(silence_start, 0, silence_samples * sizeof(float));

        // Stop playback gracefully if we got no samples
        if (decoded == 0) {
            state_ = PlaybackState::Stopped;
        }
    }
}

size_t PlaybackEngine::decode_samples(int decoder_idx, float* buffer, size_t frames) {
    DecoderInstance& inst = decoders_[decoder_idx];

    if (!inst.active || inst.eos || !inst.handle.internal) {
        return 0;
    }

    // Use a temporary buffer for decoder output
    std::vector<int32_t> temp_buffer(frames * inst.stream_info.channels);
    size_t temp_buffer_size = temp_buffer.size() * sizeof(int32_t);
    size_t samples_decoded = 0;

    // Decode to temporary buffer first
    Result result = inst.decoder->decode_block(inst.handle, temp_buffer.data(), temp_buffer_size, &samples_decoded);

    if (result != Result::Success || samples_decoded == 0) {
        inst.eos = true;
        return 0;
    }

    // Convert int32 to float (normalized to [-1.0, 1.0])
    for (size_t i = 0; i < samples_decoded * inst.stream_info.channels; ++i) {
        buffer[i] = static_cast<float>(temp_buffer[i]) / 2147483648.0f;  // 2^31
    }

    // Update position
    inst.current_position += samples_decoded;

    // Check if we've reached the end
    uint64_t effective_end = inst.track_info.total_samples - inst.track_info.encoder_padding;
    if (inst.current_position >= effective_end) {
        inst.eos = true;
    }

    return samples_decoded;
}

void PlaybackEngine::switch_decoder() {
    // Must be called with mutex locked
    close_decoder(current_decoder_);
    current_decoder_ = next_decoder_;
    next_decoder_ = -1;
    
    std::cout << "Switched to next track" << std::endl;
}

bool PlaybackEngine::is_approaching_end() const {
    const DecoderInstance& inst = decoders_[current_decoder_];
    
    if (inst.stream_info.sample_rate == 0 || inst.track_info.total_samples == 0) {
        return false;
    }
    
    uint64_t remaining_samples = inst.track_info.total_samples - inst.current_position;
    uint64_t remaining_ms = (remaining_samples * 1000) / inst.stream_info.sample_rate;
    
    return remaining_ms < PREBUFFER_THRESHOLD_MS;
}

void PlaybackEngine::close_decoder(int decoder_idx) {
    DecoderInstance& inst = decoders_[decoder_idx];
    
    if (inst.handle.internal && inst.decoder) {
        inst.decoder->close_stream(inst.handle);
    }
    
    inst.decoder = nullptr;
    inst.handle.internal = nullptr;
    inst.active = false;
    inst.eos = false;
    inst.current_position = 0;
    std::memset(&inst.stream_info, 0, sizeof(inst.stream_info));
}

}} // namespace mp::core
