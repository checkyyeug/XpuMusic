/**
 * @file real_player.cpp
 * @brief 真正的音频播放器，使用ALSA输出
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <alsa/asoundlib.h>
#include <signal.h>
#include <atomic>
#include <thread>
#include <chrono>

// WAV文件头
struct WAVHeader {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits;
    char data[4];
    uint32_t data_size;
};

// 全局原子变量
std::atomic<bool> g_stop_flag(false);

void signal_handler(int sig) {
    if (sig == SIGINT) {
        g_stop_flag = true;
    }
}

class SimpleMusicPlayer {
private:
    snd_pcm_t *pcm_handle_;
    bool is_initialized_;
    bool is_playing_;
    std::vector<int16_t> audio_buffer_;
    size_t current_pos_;

public:
    SimpleMusicPlayer()
        : pcm_handle_(nullptr)
        , is_initialized_(false)
        , is_playing_(false)
        , current_pos_(0) {
    }

    ~SimpleMusicPlayer() {
        stop();
        cleanup();
    }

    bool initialize() {
        // 设置信号处理
        signal(SIGINT, signal_handler);

        std::cout << "Initializing Music Player Core Engine..." << std::endl;
        return true;
    }

    bool load_wav(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (strncmp(header.riff, "RIFF", 4) != 0 ||
            strncmp(header.wave, "WAVE", 4) != 0) {
            std::cerr << "Error: Not a valid WAV file" << std::endl;
            return false;
        }

        // 显示WAV信息
        std::cout << "WAV Info:" << std::endl;
        std::cout << "  Sample Rate: " << header.sample_rate << " Hz" << std::endl;
        std::cout << "  Channels: " << header.channels << std::endl;
        std::cout << "  Bits: " << header.bits << std::endl;
        std::cout << "  Data Size: " << header.data_size << " bytes" << std::endl;

        // 读取音频数据
        audio_buffer_.resize(header.data_size / 2);
        file.read(reinterpret_cast<char*>(audio_buffer_.data()), header.data_size);

        std::cout << "Loaded " << audio_buffer_.size() << " samples" << std::endl;

        // 初始化ALSA
        if (!init_alsa(header.sample_rate, header.channels)) {
            return false;
        }

        is_initialized_ = true;
        current_pos_ = 0;
        return true;
    }

    bool init_alsa(unsigned int sample_rate, unsigned int channels) {
        int err;

        // 打开PCM设备
        if ((err = snd_pcm_open(&pcm_handle_, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            std::cerr << "Error: Cannot open audio device: " << snd_strerror(err) << std::endl;
            return false;
        }

        // 设置硬件参数
        snd_pcm_hw_params_t *hw_params;
        if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
            std::cerr << "Error: Cannot allocate hardware parameter structure: " << snd_strerror(err) << std::endl;
            return false;
        }

        if ((err = snd_pcm_hw_params_any(pcm_handle_, hw_params)) < 0) {
            std::cerr << "Error: Cannot initialize hardware parameter structure: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params_set_access(pcm_handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
            std::cerr << "Error: Cannot set access type: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params_set_format(pcm_handle_, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
            std::cerr << "Error: Cannot set sample format: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params_set_channels(pcm_handle_, hw_params, channels)) < 0) {
            std::cerr << "Error: Cannot set channel count: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        unsigned int rate = sample_rate;
        if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle_, hw_params, &rate, 0)) < 0) {
            std::cerr << "Error: Cannot set sample rate: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        // 设置缓冲区大小
        snd_pcm_uframes_t buffer_size = 1024;
        if ((err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle_, hw_params, &buffer_size)) < 0) {
            std::cerr << "Error: Cannot set buffer size: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params(pcm_handle_, hw_params)) < 0) {
            std::cerr << "Error: Cannot apply hardware parameters: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        snd_pcm_hw_params_free(hw_params);

        // 准备PCM
        if ((err = snd_pcm_prepare(pcm_handle_)) < 0) {
            std::cerr << "Error: Cannot prepare audio interface: " << snd_strerror(err) << std::endl;
            return false;
        }

        return true;
    }

    bool play() {
        if (!is_initialized_) {
            std::cerr << "Error: Player not initialized" << std::endl;
            return false;
        }

        is_playing_ = true;
        std::cout << "\nPlaying... (Press Ctrl+C to stop)" << std::endl;

        const size_t chunk_size = 1024;
        int progress_counter = 0;

        while (is_playing_ && !g_stop_flag && current_pos_ < audio_buffer_.size()) {
            // 计算本次要写入的帧数
            size_t frames_to_write = std::min(chunk_size, audio_buffer_.size() - current_pos_);

            // 写入音频数据
            int result = snd_pcm_writei(pcm_handle_,
                                       audio_buffer_.data() + current_pos_,
                                       frames_to_write);

            if (result == -EPIPE) {
                // 缓冲区下溢
                snd_pcm_prepare(pcm_handle_);
                continue;
            } else if (result < 0) {
                std::cerr << "Write error: " << snd_strerror(result) << std::endl;
                break;
            }

            current_pos_ += result;

            // 进度显示
            if (++progress_counter >= 100) {
                progress_counter = 0;
                int progress = current_pos_ * 100 / audio_buffer_.size();
                std::cout << "\rProgress: " << progress << "%  " << std::flush;
            }
        }

        std::cout << "\n" << std::endl;
        stop();
        return true;
    }

    void stop() {
        is_playing_ = false;
        if (pcm_handle_) {
            snd_pcm_drain(pcm_handle_);
        }
    }

    void cleanup() {
        if (pcm_handle_) {
            snd_pcm_close(pcm_handle_);
            pcm_handle_ = nullptr;
        }
        is_initialized_ = false;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "   Professional Music Player v0.1.0" << std::endl;
    std::cout << "   Cross-Platform Audio Player" << std::endl;
    std::cout << "========================================" << std::endl;

    SimpleMusicPlayer player;

    // 初始化
    if (!player.initialize()) {
        std::cerr << "Failed to initialize player" << std::endl;
        return 1;
    }

    // 加载WAV文件
    if (!player.load_wav(argv[1])) {
        std::cerr << "Failed to load WAV file" << std::endl;
        return 1;
    }

    // 播放
    if (!player.play()) {
        std::cerr << "Playback failed" << std::endl;
        return 1;
    }

    if (g_stop_flag) {
        std::cout << "\nPlayback stopped by user" << std::endl;
    } else {
        std::cout << "Playback completed successfully!" << std::endl;
    }

    return 0;
}