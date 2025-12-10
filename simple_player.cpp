/**
 * @file simple_player.cpp
 * @brief 简化的WAV播放器，不会卡死
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <alsa/asoundlib.h>
#include <signal.h>

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

// 全局变量用于信号处理
volatile bool stop_playing = false;

void signal_handler(int sig) {
    if (sig == SIGINT) {
        std::cout << "\n收到停止信号，正在退出...\n";
        stop_playing = true;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "用法: " << argv[0] << " <wav文件>\n";
        return 1;
    }

    // 设置信号处理
    signal(SIGINT, signal_handler);

    // 打开WAV文件
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "错误：无法打开文件 " << argv[1] << std::endl;
        return 1;
    }

    // 读取WAV头
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (strncmp(header.riff, "RIFF", 4) != 0 ||
        strncmp(header.wave, "WAVE", 4) != 0) {
        std::cerr << "错误：不是有效的WAV文件\n";
        return 1;
    }

    std::cout << "=== 简易WAV播放器 ===\n";
    std::cout << "文件: " << argv[1] << "\n";
    std::cout << "采样率: " << header.sample_rate << " Hz\n";
    std::cout << "声道数: " << header.channels << "\n";
    std::cout << "位深: " << header.bits << " bit\n";
    std::cout << "数据大小: " << header.data_size << " bytes\n\n";

    // 读取音频数据
    std::vector<char> audio_data(header.data_size);
    file.read(audio_data.data(), header.data_size);

    // ALSA设置
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
    int err;

    // 打开PCM设备
    if ((err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "无法打开音频设备: " << snd_strerror(err) << std::endl;
        return 1;
    }

    // 分配硬件参数结构
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        std::cerr << "无法分配硬件参数结构: " << snd_strerror(err) << std::endl;
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // 初始化硬件参数
    if ((err = snd_pcm_hw_params_any(pcm_handle, hw_params)) < 0) {
        std::cerr << "无法初始化硬件参数: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // 设置访问类型
    if ((err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        std::cerr << "无法设置访问类型: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // 设置样本格式
    if ((err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        std::cerr << "无法设置样本格式: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // 设置通道数
    if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, header.channels)) < 0) {
        std::cerr << "无法设置通道数: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // 设置采样率
    unsigned int rate = header.sample_rate;
    if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0)) < 0) {
        std::cerr << "无法设置采样率: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // 设置缓冲区大小
    snd_pcm_uframes_t buffer_size = 1024;
    if ((err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params, &buffer_size)) < 0) {
        std::cerr << "无法设置缓冲区大小: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // 应用参数
    if ((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0) {
        std::cerr << "无法应用硬件参数: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    snd_pcm_hw_params_free(hw_params);

    // 准备PCM
    if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
        std::cerr << "无法准备音频设备: " << snd_strerror(err) << std::endl;
        snd_pcm_close(pcm_handle);
        return 1;
    }

    std::cout << "开始播放... (按 Ctrl+C 停止)\n\n";

    // 播放音频
    size_t total_frames = header.data_size / (header.channels * header.bits / 8);
    size_t frames_written = 0;
    const size_t chunk_size = 1024;

    while (frames_written < total_frames && !stop_playing) {
        size_t frames_to_write = std::min(chunk_size, total_frames - frames_written);
        size_t bytes_to_write = frames_to_write * header.channels * header.bits / 8;

        err = snd_pcm_writei(pcm_handle,
                            audio_data.data() + frames_written * header.channels * header.bits / 8,
                            frames_to_write);

        if (err == -EPIPE) {
            std::cerr << "缓冲区下溢\n";
            snd_pcm_prepare(pcm_handle);
        } else if (err < 0) {
            std::cerr << "写入错误: " << snd_strerror(err) << std::endl;
            break;
        } else {
            frames_written += err;
        }

        // 显示进度
        if (frames_written % (total_frames / 10) == 0) {
            int progress = frames_written * 100 / total_frames;
            std::cout << "\r播放进度: " << progress << "%" << std::flush;
        }
    }

    std::cout << "\n\n播放完成！\n";

    // 清理
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    return 0;
}