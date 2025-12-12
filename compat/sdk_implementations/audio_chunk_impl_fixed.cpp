/**
 * @file audio_chunk_impl_fixed.cpp
 * @brief Fixed audio_chunk implementation using foobar2000 SDK
 * @date 2025-12-12
 */

#include "pch.h"
#include <foobar2000/SDK/audio_chunk.h>
#include <foobar2000/SDK/file_info.h>

using namespace foobar2000;

// Fixed audio_chunk implementation
class audio_chunk_impl_fixed : public audio_chunk {
private:
    pfc::array_t<audio_sample> buffer_;
    t_uint32 sample_rate_;
    t_uint32 channel_count_;
    t_uint32 channel_config_;

public:
    audio_chunk_impl_fixed() {
        sample_rate_ = 44100;
        channel_count_ = 2;
        channel_config_ = audio_chunk::channel_config_stereo;
    }

    void set_data(const audio_sample* p_data, t_size p_sample_count,
                  t_uint32 p_channels, t_uint32 p_sample_rate) {
        if (!p_data || p_sample_count == 0 || p_channels == 0) {
            reset();
            return;
        }

        sample_rate_ = p_sample_rate;
        channel_count_ = p_channels;

        // Set channel config based on channel count
        if (p_channels == 1) {
            channel_config_ = audio_chunk::channel_config_mono;
        } else if (p_channels == 2) {
            channel_config_ = audio_chunk::channel_config_stereo;
        } else {
            channel_config_ = audio_chunk::channel_config_front_left |
                              audio_chunk::channel_config_front_right;
        }

        // Resize buffer
        t_size total_samples = p_sample_count * p_channels;
        buffer_.set_size(total_samples);

        // Copy data
        buffer_.copy_from_ptr(p_data, total_samples);
    }

    void reset() {
        buffer_.set_size(0);
        sample_rate_ = 44100;
        channel_count_ = 2;
        channel_config_ = audio_chunk::channel_config_stereo;
    }

    // audio_chunk interface
    audio_sample* get_data() override {
        return buffer_.get_ptr();
    }

    const audio_sample* get_data() const override {
        return buffer_.get_ptr();
    }

    t_size get_sample_count() const override {
        return channel_count_ > 0 ? buffer_.get_size() / channel_count_ : 0;
    }

    t_uint32 get_sample_rate() const override {
        return sample_rate_;
    }

    t_uint32 get_channels() const override {
        return channel_count_;
    }

    t_uint32 get_channel_config() const override {
        return channel_config_;
    }

    void set_sample_rate(t_uint32 val) override {
        sample_rate_ = val;
    }

    void set_channels(t_uint32 val) override {
        if (val != channel_count_) {
            channel_count_ = val;
            buffer_.set_size(0); // Clear buffer on channel change
        }
    }

    void set_channel_config(t_uint32 val) override {
        channel_config_ = val;
    }

    // scale is optional - just return
    void scale(const audio_sample& p_scale) override {
        if (buffer_.is_empty()) return;

        audio_sample* data = buffer_.get_ptr();
        t_size count = buffer_.get_size();

        for (t_size i = 0; i < count; ++i) {
            data[i] *= p_scale;
        }
    }

    // get_channel_data is optional
    audio_sample* get_channel_data(t_uint32 p_index) override {
        if (p_index >= channel_count_ || buffer_.is_empty()) {
            return nullptr;
        }

        return buffer_.get_ptr() + p_index * get_sample_count();
    }
};

// Service class for creating audio chunks
class audio_chunk_service : public service_base {
public:
    static audio_chunk_service& get_instance() {
        static audio_chunk_service instance;
        return instance;
    }

    // Factory method
    static audio_chunk::ptr g_create() {
        return new service_impl_t<audio_chunk_impl_fixed>();
    }
};

// Export the factory function
extern "C" {
    __declspec(dllexport) audio_chunk* create_audio_chunk() {
        return new audio_chunk_impl_fixed();
    }
}