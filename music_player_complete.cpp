/**
 * @file music_player_complete.cpp
 * @brief 完整的音乐播放器实现，包含播放控制、UI交互和正常退出
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
#include <map>
#include <string>
#include <algorithm>
#include <cmath>

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

// 播放状态枚举
enum PlaybackState {
    STOPPED,
    PLAYING,
    PAUSED
};

class MusicPlayer {
private:
    snd_pcm_t *pcm_handle_;
    PlaybackState state_;
    std::vector<int16_t> audio_buffer_;
    size_t current_pos_;
    size_t total_frames_;
    std::string current_file_;
    std::atomic<bool> quit_flag_;
    std::thread playback_thread_;
    std::thread input_thread_;

    // 音频参数
    unsigned int sample_rate_;
    unsigned int channels_;

public:
    MusicPlayer()
        : pcm_handle_(nullptr)
        , state_(STOPPED)
        , current_pos_(0)
        , total_frames_(0)
        , quit_flag_(false)
        , sample_rate_(44100)
        , channels_(2) {
    }

    ~MusicPlayer() {
        stop();
        cleanup();
    }

    bool initialize() {
        std::cout << "========================================" << std::endl;
        std::cout << "   Professional Music Player v1.0.0" << std::endl;
        std::cout << "   Cross-Platform Audio Player" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "Initializing audio system..." << std::endl;

        // 设置信号处理
        struct sigaction sa;
        sa.sa_handler = [](int sig) {
            if (sig == SIGINT) {
                std::cout << "\n[INFO] Received SIGINT, preparing to quit..." << std::endl;
            }
        };
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sigaction(SIGINT, &sa, nullptr);

        std::cout << "[OK] Audio system initialized" << std::endl;
        return true;
    }

    bool load_file(const std::string& filename) {
        std::cout << "\n[LOAD] Loading file: " << filename << std::endl;

        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "[ERROR] Cannot open file: " << filename << std::endl;
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (strncmp(header.riff, "RIFF", 4) != 0 ||
            strncmp(header.wave, "WAVE", 4) != 0) {
            std::cerr << "[ERROR] Not a valid WAV file" << std::endl;
            return false;
        }

        // 显示文件信息
        std::cout << "[INFO] File format:" << std::endl;
        std::cout << "  - Sample Rate: " << header.sample_rate << " Hz" << std::endl;
        std::cout << "  - Channels: " << header.channels << std::endl;
        std::cout << "  - Bits: " << header.bits << " bit" << std::endl;
        std::cout << "  - Duration: " << (header.data_size / (header.sample_rate * header.channels * header.bits / 8)) << " seconds" << std::endl;

        // 读取音频数据
        audio_buffer_.resize(header.data_size / 2);
        file.read(reinterpret_cast<char*>(audio_buffer_.data()), header.data_size);

        sample_rate_ = header.sample_rate;
        channels_ = header.channels;
        current_file_ = filename;

        std::cout << "[OK] Loaded " << audio_buffer_.size() << " samples" << std::endl;

        // 初始化ALSA
        if (!init_alsa()) {
            return false;
        }

        total_frames_ = audio_buffer_.size() / channels_;
        current_pos_ = 0;

        return true;
    }

    bool init_alsa() {
        if (pcm_handle_) {
            snd_pcm_close(pcm_handle_);
            pcm_handle_ = nullptr;
        }

        int err;

        // 打开PCM设备
        if ((err = snd_pcm_open(&pcm_handle_, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            std::cerr << "[ERROR] Cannot open audio device: " << snd_strerror(err) << std::endl;
            return false;
        }

        // 设置硬件参数
        snd_pcm_hw_params_t *hw_params;
        if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
            std::cerr << "[ERROR] Cannot allocate hw params: " << snd_strerror(err) << std::endl;
            return false;
        }

        if ((err = snd_pcm_hw_params_any(pcm_handle_, hw_params)) < 0) {
            std::cerr << "[ERROR] Cannot init hw params: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params_set_access(pcm_handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
            std::cerr << "[ERROR] Cannot set access: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params_set_format(pcm_handle_, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
            std::cerr << "[ERROR] Cannot set format: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params_set_channels(pcm_handle_, hw_params, channels_)) < 0) {
            std::cerr << "[ERROR] Cannot set channels: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        unsigned int rate = sample_rate_;
        if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle_, hw_params, &rate, 0)) < 0) {
            std::cerr << "[ERROR] Cannot set rate: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        // 设置缓冲区大小
        snd_pcm_uframes_t buffer_size = 1024;
        if ((err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle_, hw_params, &buffer_size)) < 0) {
            std::cerr << "[ERROR] Cannot set buffer size: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        if ((err = snd_pcm_hw_params(pcm_handle_, hw_params)) < 0) {
            std::cerr << "[ERROR] Cannot apply hw params: " << snd_strerror(err) << std::endl;
            snd_pcm_hw_params_free(hw_params);
            return false;
        }

        snd_pcm_hw_params_free(hw_params);

        // 准备PCM
        if ((err = snd_pcm_prepare(pcm_handle_)) < 0) {
            std::cerr << "[ERROR] Cannot prepare PCM: " << snd_strerror(err) << std::endl;
            return false;
        }

        std::cout << "[OK] ALSA initialized (Rate: " << rate << " Hz)" << std::endl;
        return true;
    }

    void play() {
        if (audio_buffer_.empty()) {
            std::cout << "[WARN] No file loaded. Use 'load <file>' first." << std::endl;
            return;
        }

        if (state_ == PLAYING) {
            std::cout << "[INFO] Already playing" << std::endl;
            return;
        }

        if (state_ == PAUSED) {
            state_ = PLAYING;
            std::cout << "[INFO] Resumed playback" << std::endl;
            return;
        }

        state_ = PLAYING;
        std::cout << "[INFO] Starting playback..." << std::endl;

        // 启动播放线程
        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }
        playback_thread_ = std::thread(&MusicPlayer::playback_worker, this);
    }

    void pause() {
        if (state_ == PLAYING) {
            state_ = PAUSED;
            std::cout << "[INFO] Playback paused" << std::endl;
        } else if (state_ == STOPPED) {
            std::cout << "[WARN] Nothing to pause" << std::endl;
        } else {
            std::cout << "[INFO] Already paused" << std::endl;
        }
    }

    void stop() {
        if (state_ != STOPPED) {
            state_ = STOPPED;
            current_pos_ = 0;
            std::cout << "[INFO] Playback stopped" << std::endl;

            if (playback_thread_.joinable()) {
                playback_thread_.join();
            }

            if (pcm_handle_) {
                snd_pcm_drain(pcm_handle_);
                snd_pcm_prepare(pcm_handle_);
            }
        }
    }

    void seek(double percent) {
        if (audio_buffer_.empty()) {
            std::cout << "[WARN] No file loaded" << std::endl;
            return;
        }

        percent = std::max(0.0, std::min(100.0, percent));
        size_t new_pos = static_cast<size_t>((percent / 100.0) * total_frames_);

        current_pos_ = new_pos;
        int seconds = current_pos_ / sample_rate_;
        std::cout << "[INFO] Seeked to " << seconds << "s (" << percent << "%)" << std::endl;
    }

    void show_status() {
        std::cout << "\n=== PLAYER STATUS ===" << std::endl;
        std::cout << "File: " << (current_file_.empty() ? "None" : current_file_) << std::endl;
        std::cout << "State: " << (state_ == PLAYING ? "PLAYING" :
                                state_ == PAUSED ? "PAUSED" : "STOPPED") << std::endl;

        if (!audio_buffer_.empty()) {
            int current_seconds = current_pos_ / sample_rate_;
            int total_seconds = total_frames_ / sample_rate_;
            int current_minutes = current_seconds / 60;
            current_seconds %= 60;
            int total_minutes = total_seconds / 60;
            total_seconds %= 60;

            double progress = (double)current_pos_ / total_frames_ * 100.0;

            std::cout << "Position: " << current_minutes << ":"
                      << (current_seconds < 10 ? "0" : "") << current_seconds
                      << " / " << total_minutes << ":"
                      << (total_seconds < 10 ? "0" : "") << total_seconds
                      << " (" << progress << "%)" << std::endl;
        }
        std::cout << "===================" << std::endl;
    }

    void show_help() {
        std::cout << "\n=== COMMANDS ===" << std::endl;
        std::cout << "load <file>    - Load WAV file" << std::endl;
        std::cout << "play           - Start/Resume playback" << std::endl;
        std::cout << "pause          - Pause playback" << std::endl;
        std::cout << "stop           - Stop playback" << std::endl;
        std::cout << "seek <percent> - Seek to position (0-100)" << std::endl;
        std::cout << "status         - Show current status" << std::endl;
        std::cout << "help           - Show this help" << std::endl;
        std::cout << "quit/exit      - Exit player" << std::endl;
        std::cout << "===============" << std::endl;
    }

    void run_interactive() {
        std::cout << "\nEnter 'help' for commands or 'quit' to exit" << std::endl;
        std::cout << "> " << std::flush;

        // 启动输入线程
        input_thread_ = std::thread(&MusicPlayer::input_worker, this);

        // 主线程等待退出
        while (!quit_flag_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // 检查播放是否完成
            if (state_ == PLAYING && current_pos_ >= total_frames_) {
                state_ = STOPPED;
                current_pos_ = 0;
                std::cout << "\n[INFO] Playback completed" << std::endl;
                std::cout << "> " << std::flush;
            }
        }

        // 清理
        stop();
        if (input_thread_.joinable()) {
            input_thread_.join();
        }
    }

    void quit() {
        quit_flag_ = true;
        stop();
        std::cout << "\n[INFO] Exiting player..." << std::endl;
    }

private:
    void playback_worker() {
        const size_t chunk_size = 1024;

        while (state_ != STOPPED && current_pos_ < total_frames_ && !quit_flag_) {
            if (state_ == PLAYING) {
                size_t frames_to_write = std::min(chunk_size, total_frames_ - current_pos_);

                int result = snd_pcm_writei(pcm_handle_,
                                           audio_buffer_.data() + current_pos_ * channels_,
                                           frames_to_write);

                if (result == -EPIPE) {
                    snd_pcm_prepare(pcm_handle_);
                } else if (result < 0) {
                    std::cerr << "[ERROR] Write error: " << snd_strerror(result) << std::endl;
                    break;
                } else {
                    current_pos_ += result;
                }
            } else {
                // 暂停状态
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    void input_worker() {
        std::string command;
        while (!quit_flag_ && std::getline(std::cin, command)) {
            // 转换为小写
            std::transform(command.begin(), command.end(), command.begin(), ::tolower);

            if (command.substr(0, 4) == "load" && command.length() > 5) {
                stop();
                std::string filename = command.substr(5);
                // 去除引号（如果有）
                if (filename.front() == '"' && filename.back() == '"') {
                    filename = filename.substr(1, filename.length() - 2);
                }
                load_file(filename);
            } else if (command == "play") {
                play();
            } else if (command == "pause") {
                pause();
            } else if (command == "stop") {
                stop();
            } else if (command.substr(0, 4) == "seek" && command.length() > 5) {
                try {
                    double percent = std::stod(command.substr(5));
                    seek(percent);
                } catch (...) {
                    std::cout << "[ERROR] Invalid seek position" << std::endl;
                }
            } else if (command == "status") {
                show_status();
            } else if (command == "help") {
                show_help();
            } else if (command == "quit" || command == "exit") {
                quit();
                break;
            } else if (command.empty()) {
                // 忽略空输入
            } else {
                std::cout << "[ERROR] Unknown command. Type 'help' for available commands." << std::endl;
            }

            if (!quit_flag_) {
                std::cout << "> " << std::flush;
            }
        }
    }

    void cleanup() {
        if (pcm_handle_) {
            snd_pcm_close(pcm_handle_);
            pcm_handle_ = nullptr;
        }

        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }

        if (input_thread_.joinable()) {
            input_thread_.join();
        }
    }
};

int main(int argc, char* argv[]) {
    MusicPlayer player;

    if (!player.initialize()) {
        std::cerr << "Failed to initialize player" << std::endl;
        return 1;
    }

    // 如果提供了文件名参数，自动加载
    if (argc > 1) {
        if (player.load_file(argv[1])) {
            std::cout << "\nFile loaded. Use commands to control playback." << std::endl;
        }
    }

    // 运行交互式界面
    player.run_interactive();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}