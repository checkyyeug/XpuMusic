/**
 * @file music_player_config.cpp
 * @brief 音乐播放器主程序 - 使用配置系统
 * @date 2025-12-10
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <string>
#include <vector>

#include "config_manager.h"
#include "xpumusic_plugin_sdk.h"
#include "audio_output.h"

using namespace xpumusic;
using namespace std;

// 全局变量
std::atomic<bool> g_running(true);
std::unique_ptr<ConfigManager> g_config;
std::unique_ptr<IAudioOutput> g_audio_output;

// 信号处理
void signal_handler(int signal) {
    cout << "\nReceived signal " << signal << ", shutting down..." << endl;
    g_running = false;

    if (g_audio_output) {
        g_audio_output->stop();
        g_audio_output->cleanup();
    }
}

// 打印帮助信息
void print_help(const char* program_name) {
    cout << "Usage: " << program_name << " [options] [audio_file]" << endl;
    cout << "Options:" << endl;
    cout << "  -h, --help          Show this help message" << endl;
    cout << "  -c, --config <file> Specify config file path" << endl;
    cout << "  --list-devices      List available audio devices" << endl;
    cout << "  --device <name>     Use specific audio device" << endl;
    cout << "  --rate <rate>       Set sample rate (default: from config)" << endl;
    cout << "  --channels <num>    Set channel count (default: from config)" << endl;
    cout << "  --volume <0-1>      Set volume (default: from config)" << endl;
}

// 打印配置信息
void print_config() {
    const auto& config = g_config->get_config();

    cout << "=== Configuration ===" << endl;
    cout << "Audio:" << endl;
    cout << "  Output Device: " << config.audio.output_device << endl;
    cout << "  Sample Rate: " << config.audio.sample_rate << " Hz" << endl;
    cout << "  Channels: " << config.audio.channels << endl;
    cout << "  Buffer Size: " << config.audio.buffer_size << endl;
    cout << "  Volume: " << config.audio.volume << endl;
    cout << "  Mute: " << (config.audio.mute ? "Yes" : "No") << endl;

    cout << "Player:" << endl;
    cout << "  Show Console: " << (config.player.show_console_output ? "Yes" : "No") << endl;
    cout << "  Show Progress: " << (config.player.show_progress_bar ? "Yes" : "No") << endl;
    cout << "  Repeat: " << (config.player.repeat ? "Yes" : "No") << endl;
    cout << "  Shuffle: " << (config.player.shuffle ? "Yes" : "No") << endl;

    cout << "Resampler:" << endl;
    cout << "  Quality: " << config.resampler.quality << endl;
    cout << "  Adaptive: " << (config.resampler.enable_adaptive ? "Yes" : "No") << endl;
    cout << "  CPU Threshold: " << config.resampler.cpu_threshold << endl;

    cout << "Plugins:" << endl;
    cout << "  Auto Load: " << (config.plugins.auto_load_plugins ? "Yes" : "No") << endl;
    cout << "  Directories: ";
    for (const auto& dir : config.plugins.plugin_directories) {
        cout << dir << " ";
    }
    cout << endl;
    cout << "===================" << endl;
}

// 列出可用设备
void list_devices() {
    cout << "Available audio devices:" << endl;

    // 这里需要调用音频后端的设备枚举功能
    // 暂时显示默认设备
    cout << "  default - Default audio device" << endl;

#ifdef MP_PLATFORM_LINUX
    cout << "  pulse - PulseAudio server" << endl;
    cout << "  hw:0,0 - ALSA hardware device 0,0" << endl;
#endif
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 解析命令行参数
    string config_file;
    string audio_file;
    string device_name;
    int sample_rate = 0;
    int channels = 0;
    double volume = -1.0;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        }
        else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                cerr << "Error: " << arg << " requires a filename" << endl;
                return 1;
            }
        }
        else if (arg == "--list-devices") {
            list_devices();
            return 0;
        }
        else if (arg == "--device") {
            if (i + 1 < argc) {
                device_name = argv[++i];
            } else {
                cerr << "Error: " << arg << " requires a device name" << endl;
                return 1;
            }
        }
        else if (arg == "--rate") {
            if (i + 1 < argc) {
                sample_rate = stoi(argv[++i]);
            } else {
                cerr << "Error: " << arg << " requires a sample rate" << endl;
                return 1;
            }
        }
        else if (arg == "--channels") {
            if (i + 1 < argc) {
                channels = stoi(argv[++i]);
            } else {
                cerr << "Error: " << arg << " requires a channel count" << endl;
                return 1;
            }
        }
        else if (arg == "--volume") {
            if (i + 1 < argc) {
                volume = stod(argv[++i]);
            } else {
                cerr << "Error: " << arg << " requires a volume value" << endl;
                return 1;
            }
        }
        else if (arg[0] != '-') {
            if (audio_file.empty()) {
                audio_file = arg;
            } else {
                cerr << "Error: Multiple audio files specified" << endl;
                return 1;
            }
        }
    }

    // 初始化配置管理器
    g_config = make_unique<ConfigManager>();

    if (!g_config->initialize(config_file)) {
        cerr << "Error: Failed to initialize configuration" << endl;
        return 1;
    }

    // 打印配置信息（如果启用控制台输出）
    if (g_config->get_config().player.show_console_output) {
        print_config();
    }

    // 应用命令行参数覆盖
    if (!device_name.empty()) {
        g_config->audio().output_device = device_name;
    }
    if (sample_rate > 0) {
        g_config->audio().sample_rate = sample_rate;
    }
    if (channels > 0) {
        g_config->audio().channels = channels;
    }
    if (volume >= 0.0 && volume <= 1.0) {
        g_config->audio().volume = volume;
    }

    // 创建音频输出
    g_audio_output = create_audio_output();
    if (!g_audio_output) {
        cerr << "Error: Failed to create audio output" << endl;
        return 1;
    }

    // 初始化音频输出
    AudioFormat format;
    format.sample_rate = g_config->get_config().audio.sample_rate;
    format.channels = g_config->get_config().audio.channels;
    format.bits_per_sample = g_config->get_config().audio.bits_per_sample;
    format.is_float = g_config->get_config().audio.use_float;

    AudioConfig audio_config;
    audio_config.buffer_size = g_config->get_config().audio.buffer_size;
    audio_config.buffer_count = g_config->get_config().audio.buffer_count;
    audio_config.device_name = g_config->get_config().audio.output_device;
    audio_config.volume = g_config->get_config().audio.volume;
    audio_config.mute = g_config->get_config().audio.mute;

    if (!g_audio_output->initialize(format, audio_config)) {
        cerr << "Error: Failed to initialize audio output" << endl;
        return 1;
    }

    // 应用音量和静音设置
    g_audio_output->set_volume(g_config->get_config().audio.volume);
    g_audio_output->set_mute(g_config->get_config().audio.mute);

    // 如果没有指定音频文件，播放测试音
    if (audio_file.empty()) {
        cout << "Playing 440 Hz test tone..." << endl;

        // 生成440Hz正弦波
        const int duration = 3; // 3秒
        const int frames = format.sample_rate * duration;
        const int freq = 440;
        const double amplitude = 0.3 * g_config->get_config().audio.volume;

        vector<float> buffer(frames * format.channels);

        for (int i = 0; i < frames; ++i) {
            double t = static_cast<double>(i) / format.sample_rate;
            double sample = amplitude * sin(2.0 * M_PI * freq * t);

            for (int ch = 0; ch < format.channels; ++ch) {
                buffer[i * format.channels + ch] = static_cast<float>(sample);
            }
        }

        // 播放音频
        g_audio_output->start();

        int frames_written = 0;
        while (g_running && frames_written < frames) {
            int chunk = min(4096, frames - frames_written);
            if (g_audio_output->write(buffer.data() + frames_written * format.channels, chunk) != chunk) {
                cerr << "Error: Failed to write audio data" << endl;
                break;
            }
            frames_written += chunk;

            // 显示进度
            if (g_config->get_config().player.show_progress_bar) {
                int progress = (frames_written * 100) / frames;
                cout << "\rProgress: [";
                for (int i = 0; i < 50; ++i) {
                    cout << (i < progress / 2 ? "=" : " ");
                }
                cout << "] " << progress << "%" << flush;
            }

            this_thread::sleep_for(chrono::milliseconds(10));
        }

        if (g_config->get_config().player.show_progress_bar) {
            cout << endl;
        }
    } else {
        // TODO: 实现音频文件播放
        // 这里需要集成音频解码器
        cout << "Audio file playback not yet implemented" << endl;
    }

    // 清理
    g_audio_output->stop();
    g_audio_output->cleanup();
    g_audio_output.reset();

    cout << "Playback finished." << endl;
    return 0;
}