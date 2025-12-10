#include <iostream>
#include <alsa/asoundlib.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    std::cout << "=== ALSA 音频调试 ===\n";

    snd_pcm_t *pcm_handle;
    int err;

    // 尝试打开默认音频设备
    std::cout << "正在打开音频设备... ";
    if ((err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cout << "失败!\n错误: " << snd_strerror(err) << std::endl;
        return 1;
    }
    std::cout << "成功!\n";

    // 设置参数
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 2);

    unsigned int rate = 44100;
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);

    if ((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0) {
        std::cout << "设置参数失败: " << snd_strerror(err) << std::endl;
        return 1;
    }

    std::cout << "采样率: " << rate << " Hz\n";

    // 准备音频设备
    snd_pcm_prepare(pcm_handle);

    // 生成测试音调
    const int duration = 2; // 2秒
    const int sample_rate = 44100;
    const int frequency = 440; // A4音符
    const int frames = sample_rate * duration;

    int16_t buffer[frames * 2]; // 立体声

    for (int i = 0; i < frames; i++) {
        float t = (float)i / sample_rate;
        float value = sinf(2 * M_PI * frequency * t) * 16384; // 50%音量

        // 立体声，两个声道相同
        buffer[i*2] = (int16_t)value;
        buffer[i*2+1] = (int16_t)value;
    }

    std::cout << "播放 2秒 440Hz 测试音调...\n";

    // 播放音频
    int frames_written = snd_pcm_writei(pcm_handle, buffer, frames);

    if (frames_written < 0) {
        std::cout << "播放失败: " << snd_strerror(frames_written) << std::endl;
    } else {
        std::cout << "成功播放了 " << frames_written << " 帧\n";
    }

    // 清理
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    std::cout << "\n测试完成！\n";

    return 0;
}