#include "audio_chunk.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace fb2k {

// audio_chunk_impl 瀹炵幇
// IUnknown 瀹炵幇宸茬粡鍦ㄥご鏂囦欢涓畬鎴?

// 鍏蜂綋鐨勫伐鍏峰嚱鏁板疄鐜?
namespace audio_chunk_utils {

std::unique_ptr<audio_chunk> create_chunk(size_t samples, uint32_t channels, uint32_t sample_rate) {
    auto chunk = std::make_unique<audio_chunk_impl>(samples);
    chunk->set_data_size(samples);
    
    // 璁剧疆鏍煎紡
    float* data = chunk->get_data();
    if(data) {
        // 鍒濆鍖栦负闈欓煶
        std::memset(data, 0, samples * channels * sizeof(float));
        chunk->set_data(data, samples, channels, sample_rate);
    }
    
    return chunk;
}

std::unique_ptr<audio_chunk> create_silence(size_t samples, uint32_t channels, uint32_t sample_rate) {
    return create_chunk(samples, channels, sample_rate); // 鐩稿悓瀹炵幇
}

std::unique_ptr<audio_chunk> duplicate_chunk(const audio_chunk& source) {
    auto dest = std::make_unique<audio_chunk_impl>(source.get_sample_count());
    dest->copy(source);
    return dest;
}

std::unique_ptr<audio_chunk> concatenate_chunks(
    const audio_chunk& chunk1, const audio_chunk& chunk2) {
    
    if(chunk1.get_sample_rate() != chunk2.get_sample_rate() ||
       chunk1.get_channels() != chunk2.get_channels()) {
        return nullptr; // 鏍煎紡涓嶅尮閰?
    }
    
    size_t total_samples = chunk1.get_sample_count() + chunk2.get_sample_count();
    auto result = std::make_unique<audio_chunk_impl>(total_samples);
    
    float* dest_data = result->get_data();
    if(!dest_data) return nullptr;
    
    // 澶嶅埗绗竴涓潡
    const float* src1_data = chunk1.get_data();
    if(src1_data) {
        std::memcpy(dest_data, src1_data, 
                   chunk1.get_sample_count() * chunk1.get_channels() * sizeof(float));
    }
    
    // 澶嶅埗绗簩涓潡
    const float* src2_data = chunk2.get_data();
    if(src2_data) {
        std::memcpy(dest_data + chunk1.get_sample_count() * chunk1.get_channels(),
                   src2_data,
                   chunk2.get_sample_count() * chunk2.get_channels() * sizeof(float));
    }
    
    result->set_data(dest_data, total_samples, 
                    chunk1.get_channels(), chunk1.get_sample_rate());
    
    return result;
}

void apply_gain_to_chunk(audio_chunk& chunk, float gain) {
    chunk.apply_gain(gain);
}

float calculate_rms(const audio_chunk& chunk) {
    if(chunk.is_empty()) return 0.0f;
    
    const float* data = chunk.get_data();
    if(!data) return 0.0f;
    
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    double sum_squares = 0.0;
    
    for(size_t i = 0; i < total_samples; ++i) {
        double sample = data[i];
        sum_squares += sample * sample;
    }
    
    return static_cast<float>(std::sqrt(sum_squares / total_samples));
}

float calculate_peak(const audio_chunk& chunk) {
    if(chunk.is_empty()) return 0.0f;
    
    const float* data = chunk.get_data();
    if(!data) return 0.0f;
    
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    float peak = 0.0f;
    
    for(size_t i = 0; i < total_samples; ++i) {
        peak = std::max(peak, std::abs(data[i]));
    }
    
    return peak;
}

} // namespace audio_chunk_utils

// 楠岃瘉鍜屾祴璇曞嚱鏁?
namespace audio_chunk_validation {

bool validate_audio_chunk_basic(const audio_chunk& chunk) {
    // 鍩虹楠岃瘉
    if(chunk.is_empty()) {
        return true; // 绌哄潡鏄湁鏁堢殑
    }
    
    if(!chunk.is_valid()) {
        return false;
    }
    
    // 妫€鏌ユ暟鎹寚閽?
    const float* data = chunk.get_data();
    if(!data && chunk.get_sample_count() > 0) {
        return false; // 鏈夐噰鏍蜂絾娌℃湁鏁版嵁
    }
    
    // 妫€鏌ュ弬鏁颁竴鑷存€?
    size_t expected_size = chunk.get_sample_count() * chunk.get_channels();
    if(expected_size == 0) {
        return false; // 涓嶅簲璇ユ湁闆跺ぇ灏?
    }
    
    return true;
}

bool validate_audio_chunk_format(const audio_chunk& chunk) {
    uint32_t sample_rate = chunk.get_sample_rate();
    uint32_t channels = chunk.get_channels();
    uint32_t channel_config = chunk.get_channel_config();
    
    // 楠岃瘉閲囨牱鐜?
    if(sample_rate < 8000 || sample_rate > 192000) {
        return false;
    }
    
    // 楠岃瘉澹伴亾鏁?
    if(channels == 0 || channels > 8) {
        return false;
    }
    
    // 楠岃瘉澹伴亾閰嶇疆
    uint32_t expected_config = 0;
    switch(channels) {
        case 1: expected_config = 0x4; break;      // mono
        case 2: expected_config = 0x3; break;      // stereo
        case 3: expected_config = 0x103; break;    // 2.1
        case 6: expected_config = 0x37; break;     // 5.1
        case 8: expected_config = 0xFF; break;     // 7.1
        default: expected_config = (1u << channels) - 1; break;
    }
    
    if(channel_config != expected_config) {
        return false; // 澹伴亾閰嶇疆涓嶅尮閰?
    }
    
    return true;
}

bool validate_audio_chunk_data(const audio_chunk& chunk) {
    if(chunk.is_empty()) {
        return true;
    }
    
    const float* data = chunk.get_data();
    if(!data) return false;
    
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    // 妫€鏌ユ槸鍚︽湁鏃犳晥鍊硷紙NaN, Inf锛?
    for(size_t i = 0; i < total_samples; ++i) {
        float sample = data[i];
        if(!std::isfinite(sample)) {
            return false; // 鍙戠幇鏃犳晥娴偣鍊?
        }
    }
    
    return true;
}

void log_audio_chunk_info(const audio_chunk& chunk, const char* prefix = "") {
    printf("%sAudio Chunk Info:\n", prefix);
    printf("%s  Sample Count: %zu\n", prefix, chunk.get_sample_count());
    printf("%s  Sample Rate: %u Hz\n", prefix, chunk.get_sample_rate());
    printf("%s  Channels: %u\n", prefix, chunk.get_channels());
    printf("%s  Channel Config: 0x%X\n", prefix, chunk.get_channel_config());
    printf("%s  Duration: %.3f seconds\n", prefix, chunk.get_duration());
    printf("%s  Data Bytes: %zu\n", prefix, chunk.get_data_bytes());
    printf("%s  Is Valid: %s\n", prefix, chunk.is_valid() ? "Yes" : "No");
    printf("%s  Is Empty: %s\n", prefix, chunk.is_empty() ? "Yes" : "No");
    
    if(!chunk.is_empty()) {
        float rms = audio_chunk_utils::calculate_rms(chunk);
        float peak = audio_chunk_utils::calculate_peak(chunk);
        printf("%s  RMS Level: %.4f\n", prefix, rms);
        printf("%s  Peak Level: %.4f\n", prefix, peak);
    }
}

} // namespace audio_chunk_validation