/**
 * @file audio_chunk_impl.cpp
 * @brief audio_chunk_impl 瀹炵幇
 * @date 2025-12-09
 */

#include "audio_chunk_impl.h"
#include <algorithm>
#include <cmath>

namespace foobar2000_sdk {

audio_chunk_impl::audio_chunk_impl() {
    // 榛樿鏍煎紡: 44.1kHz, 绔嬩綋澹?    buffer_.set_format(44100, 2);
}

void audio_chunk_impl::set_data(const audio_sample* p_data, size_t p_sample_count, 
                                uint32_t p_channels, uint32_t p_sample_rate) {
    if (!p_data || p_sample_count == 0 || p_channels == 0) {
        reset();
        return;
    }
    
    // 璁剧疆鏍煎紡
    buffer_.set_format(p_sample_rate, p_channels);
    
    // 璋冩暣缂撳啿鍖哄ぇ灏?    size_t total_samples = p_sample_count * p_channels;
    buffer_.resize(total_samples);
    
    // 娣辨嫹璐濇暟鎹?    if (p_data != buffer_.data.data()) {
        std::memcpy(buffer_.data.data(), p_data, total_samples * sizeof(audio_sample));
    }
    
    buffer_.sample_count = static_cast<uint32_t>(p_sample_count);
}

void audio_chunk_impl::set_data(const audio_sample* p_data, size_t p_sample_count,
                                uint32_t p_channels, uint32_t p_sample_rate,
                                void (* p_data_free)(audio_sample*)) {
    // 瀵逛簬鎺ョ妯″紡锛屾垜浠洿鎺ユ嫹璐濇暟鎹紙绠€鍖栵級
    set_data(p_data, p_sample_count, p_channels, p_sample_rate);
    
    // 璋冪敤閲婃斁鍥炶皟锛堝鏋滄彁渚涳級
    if (p_data_free) {
        p_data_free(const_cast<audio_sample*>(p_data));
    }
}

void audio_chunk_impl::data_append(const audio_sample* p_data, size_t p_sample_count) {
    if (!p_data || p_sample_count == 0) {
        return;
    }
    
    // 楠岃瘉鏍煎紡鍖归厤
    size_t old_size = buffer_.data.size();
    size_t append_samples = p_sample_count * buffer_.channels;
    
    // 璋冩暣澶у皬浠ュ寘鍚柊鏁版嵁
    buffer_.data.resize(old_size + append_samples);
    
    // 杩藉姞鏁版嵁
    std::memcpy(buffer_.data.data() + old_size, p_data, 
                append_samples * sizeof(audio_sample));
    
    buffer_.sample_count += static_cast<uint32_t>(p_sample_count);
}

void audio_chunk_impl::data_pad(size_t p_sample_count) {
    if (p_sample_count == 0) {
        return;
    }
    
    size_t old_size = buffer_.data.size();
    size_t pad_samples = p_sample_count * buffer_.channels;
    
    // 鐢ㄩ潤闊冲～鍏咃紙0.0f锛?    buffer_.data.resize(old_size + pad_samples, 0.0f);
    
    buffer_.sample_count += static_cast<uint32_t>(p_sample_count);
}

audio_sample* audio_chunk_impl::get_channel_data(uint32_t p_channel) {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return nullptr;
    }
    
    // 杩斿洖璇ラ€氶亾鐨勪氦閿欐暟鎹寚閽?    // 娉ㄦ剰锛氳繖鏄?LRLRLR... 甯冨眬
    return &(buffer_.data[p_channel]);
}

const audio_sample* audio_chunk_impl::get_channel_data(uint32_t p_channel) const {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return nullptr;
    }
    
    return &(buffer_.data[p_channel]);
}

void audio_chunk_impl::set_channels(uint32_t p_channels, bool p_preserve_data) {
    if (p_channels == buffer_.channels) {
        return;  // 鏃犲彉鍖?    }
    
    if (!p_preserve_data || buffer_.data.empty()) {
        // 绠€鍗曡缃€氶亾鏁帮紙涓嶄繚鐣欐暟鎹級
        buffer_.channels = p_channels;
        return;
    }
    
    // 淇濈暀鏁版嵁妯″紡
    if (p_channels < buffer_.channels) {
        // 鍒犻櫎閫氶亾锛氶噸鏂版帓鍒楁暟鎹互绉婚櫎鐗瑰畾閫氶亾
        uint32_t channels_to_remove = buffer_.channels - p_channels;
        
        // 绠€鍖栵細鍙繚鐣欏墠 p_channels 涓€氶亾
        std::vector<audio_sample> new_data;
        new_data.reserve(buffer_.sample_count * p_channels);
        
        for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
            for (uint32_t ch = 0; ch < p_channels; ++ch) {
                new_data.push_back(buffer_.data[i * buffer_.channels + ch]);
            }
        }
        
        buffer_.data = std::move(new_data);
        buffer_.channels = p_channels;
    } else if (p_channels > buffer_.channels) {
        // 娣诲姞閫氶亾锛氱敤闈欓煶濉厖鏂伴€氶亾
        std::vector<audio_sample> new_data;
        new_data.reserve(buffer_.sample_count * p_channels);
        
        for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
            // 澶嶅埗鐜版湁閫氶亾
            for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
                new_data.push_back(buffer_.data[i * buffer_.channels + ch]);
            }
            // 鏂伴€氶亾濉厖闈欓煶
            for (uint32_t ch = buffer_.channels; ch < p_channels; ++ch) {
                new_data.push_back(0.0f);
            }
        }
        
        buffer_.data = std::move(new_data);
        buffer_.channels = p_channels;
    }
}

void audio_chunk_impl::set_sample_rate(uint32_t p_sample_rate, t_resample_mode p_mode) {
    if (p_sample_rate == buffer_.sample_rate || buffer_.data.empty()) {
        buffer_.sample_rate = p_sample_rate;
        return;
    }
    
    // 娉ㄦ剰锛氱湡姝ｇ殑 resample 闇€瑕?DSP
    // 鐩墠鍙洿鏂伴噰鏍风巼鏍囪
    (void)p_mode;  // 鏆傛椂蹇界暐妯″紡
    buffer_.sample_rate = p_sample_rate;
}

void audio_chunk_impl::convert(t_sample_point_format p_target_format) {
    if (buffer_.data.empty()) {
        return;
    }
    
    // audio_sample 濮嬬粓鏄?float锛堣繖鏄?foobar2000 鐨勫唴閮ㄦ牸寮忥級
    // 杩欎釜鍑芥暟鐢ㄤ簬涓庢暣鏁版牸寮忕殑澶栭儴杞崲
    // 瀵逛簬鍐呴儴瀹炵幇锛屾垜浠彲浠ュ拷鐣ヨ浆鎹紙搴旇宸茬粡姝ｇ‘锛?    (void)p_target_format;
}

void audio_chunk_impl::scale(const audio_sample& p_gain) {
    if (buffer_.data.empty() || p_gain == 1.0f) {
        return;
    }
    
    // 搴旂敤绾挎€у鐩婂埌鎵€鏈夋牱鏈?    for (auto& sample : buffer_.data) {
        sample *= p_gain;
    }
}

void audio_chunk_impl::scale(const audio_sample* p_gain) {
    if (buffer_.data.empty() || !p_gain) {
        return;
    }
    
    // 姣忎釜閫氶亾搴旂敤涓嶅悓澧炵泭
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            buffer_.data[i * buffer_.channels + ch] *= p_gain[ch];
        }
    }
}

void audio_chunk_impl::calculate_peak(audio_sample* p_peak) const {
    if (buffer_.data.empty() || !p_peak) {
        return;
    }
    
    // 鍒濆鍖栧嘲鍊兼暟缁?    for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
        p_peak[ch] = 0.0f;
    }
    
    // 璁＄畻姣忎釜閫氶亾鐨勫嘲鍊肩數骞筹紙缁濆鍊兼渶澶у€硷級
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            audio_sample sample = std::abs(buffer_.data[i * buffer_.channels + ch]);
            if (sample > p_peak[ch]) {
                p_peak[ch] = sample;
            }
        }
    }
}

void audio_chunk_impl::remove_channel(uint32_t p_channel) {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return;
    }
    
    // 鍒犻櫎鐗瑰畾閫氶亾
    std::vector<audio_sample> new_data;
    new_data.reserve(buffer_.sample_count * (buffer_.channels - 1));
    
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            if (ch != p_channel) {
                new_data.push_back(buffer_.data[i * buffer_.channels + ch]);
            }
        }
    }
    
    buffer_.data = std::move(new_data);
    buffer_.channels--;
}

void audio_chunk_impl::copy_channel_to(uint32_t p_channel, audio_chunk_interface& p_target) const {
    if (p_channel >= buffer_.channels || buffer_.data.empty()) {
        return;
    }
    
    // 鍒涘缓鏂扮殑 chunk 鐢ㄤ簬鐩爣
    audio_chunk_impl* target_impl = dynamic_cast<audio_chunk_impl*>(&p_target);
    if (!target_impl) {
        return;
    }
    
    // 璁剧疆鐩爣鏍煎紡
    target_impl->set_sample_rate(buffer_.sample_rate);
    target_impl->set_channels(1, false);  // 鍗曢€氶亾
    
    // 鍒嗛厤绌洪棿骞跺鍒舵暟鎹?    target_impl->buffer_.data.resize(buffer_.sample_count);
    target_impl->buffer_.sample_count = buffer_.sample_count;
    
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        target_impl->buffer_.data[i] = buffer_.data[i * buffer_.channels + p_channel];
    }
}

void audio_chunk_impl::copy_channel_from(uint32_t p_channel, const audio_chunk_interface& p_source) {
    if (buffer_.data.empty()) {
        return;
    }
    
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl || source_impl->buffer_.data.empty()) {
        return;
    }
    
    // 纭繚鏍煎紡鍖归厤
    if (buffer_.sample_rate != source_impl->buffer_.sample_rate ||
        buffer_.sample_count != source_impl->buffer_.sample_count) {
        return;
    }
    
    // 澶嶅埗閫氶亾鏁版嵁
    for (uint32_t i = 0; i < buffer_.sample_count; ++i) {
        if (p_channel < buffer_.channels) {
            buffer_.data[i * buffer_.channels + p_channel] = 
                source_impl->buffer_.data[i * source_impl->buffer_.channels];
        }
    }
}

void audio_chunk_impl::duplicate(const audio_chunk_interface& p_source) {
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl) {
        return;
    }
    
    *this = *source_impl;  // 娣辨嫹璐?}

void audio_chunk_impl::combine(const audio_chunk_interface& p_source, size_t p_count) {
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl || source_impl->buffer_.data.empty()) {
        return;
    }
    
    // 纭繚鏍煎紡鍏煎
    if (buffer_.sample_rate != source_impl->buffer_.sample_rate ||
        buffer_.channels != source_impl->buffer_.channels) {
        return;
    }
    
    // 闄愬埗娣烽煶鏍锋湰鏁?    size_t samples_to_combine = std::min(p_count, 
                                        static_cast<size_t>(source_impl->buffer_.sample_count));
    samples_to_combine = std::min(samples_to_combine, 
                                 static_cast<size_t>(buffer_.sample_count));
    
    // 绠€鍗曟贩闊筹紙鐩稿姞锛?    for (uint32_t i = 0; i < samples_to_combine; ++i) {
        for (uint32_t ch = 0; ch < buffer_.channels; ++ch) {
            size_t idx = i * buffer_.channels + ch;
            buffer_.data[idx] += source_impl->buffer_.data[idx];
        }
    }
}

void audio_chunk_impl::copy(const audio_chunk_interface& p_source) {
    duplicate(p_source);
}

void audio_chunk_impl::copy_meta(const audio_chunk_interface& p_source) {
    const audio_chunk_impl* source_impl = dynamic_cast<const audio_chunk_impl*>(&p_source);
    if (!source_impl) {
        return;
    }
    
    // 鍙鍒跺厓鏁版嵁锛屼笉澶嶅埗闊抽鏁版嵁
    buffer_.sample_rate = source_impl->buffer_.sample_rate;
    buffer_.channels = source_impl->buffer_.channels;
    buffer_.sample_count = source_impl->buffer_.sample_count;
}

void audio_chunk_impl::reset() {
    buffer_.data.clear();
    buffer_.sample_count = 0;
    buffer_.channels = 2;  // 榛樿绔嬩綋澹?    buffer_.sample_rate = 44100;  // 榛樿 44.1kHz
}

uint32_t audio_chunk_impl::find_peaks(uint32_t p_start) const {
    if (buffer_.data.empty() || buffer_.channels == 0) {
        return 0;
    }
    
    // 绠€鍖栧疄鐜帮細濡傛灉鏍锋湰鎺ヨ繎 0锛岃繑鍥炴湁鏁堜綅缃?    // 鐪熸鐨勫疄鐜伴渶瑕?scan backwards 鎵惧埌闈為浂鏍锋湰
    (void)p_start;
    
    return buffer_.sample_count > 0 ? buffer_.sample_count - 1 : 0;
}

void audio_chunk_impl::swap(audio_chunk_interface& p_other) {
    audio_chunk_impl* other_impl = dynamic_cast<audio_chunk_impl*>(&p_other);
    if (!other_impl) {
        return;
    }
    
    std::swap(buffer_, other_impl->buffer_);
}

bool audio_chunk_impl::audio_data_equals(const audio_chunk_interface& p_other) const {
    const audio_chunk_impl* other_impl = dynamic_cast<const audio_chunk_impl*>(&p_other);
    if (!other_impl) {
        return false;
    }
    
    // 妫€鏌ユ牸寮忓尮閰?    if (buffer_.sample_rate != other_impl->buffer_.sample_rate ||
        buffer_.channels != other_impl->buffer_.channels ||
        buffer_.sample_count != other_impl->buffer_.sample_count) {
        return false;
    }
    
    // 姣旇緝鎵€鏈夋牱鏈槸鍚︾浉绛夛紙鍏佽寰皬娴偣璇樊锛?    const float epsilon = 1e-6f;
    for (size_t i = 0; i < buffer_.data.size(); ++i) {
        if (std::abs(buffer_.data[i] - other_impl->buffer_.data[i]) > epsilon) {
            return false;
        }
    }
    
    return true;
}

// 杈呭姪鍑芥暟
std::unique_ptr<audio_chunk_impl> audio_chunk_create() {
    return std::make_unique<audio_chunk_impl>();
}

} // namespace foobar2000_sdk
