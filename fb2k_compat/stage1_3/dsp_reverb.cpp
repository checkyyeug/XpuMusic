#include "dsp_reverb.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <random>

namespace fb2k {

// 鎴块棿娣峰搷寮曟搸瀹炵幇
room_reverb_engine::room_reverb_engine(const reverb_parameters& params, uint32_t sample_rate)
    : reverb_engine(params, sample_rate) {
    
    calculate_delays();
    initialize_filters();
    
    // 鍒涘缓鏃╂湡鍙嶅皠
    std::vector<size_t> early_delays = {static_cast<size_t>(0.001f * sample_rate),
                                       static_cast<size_t>(0.002f * sample_rate),
                                       static_cast<size_t>(0.003f * sample_rate),
                                       static_cast<size_t>(0.005f * sample_rate)};
    
    std::vector<float> early_gains = {0.8f, 0.6f, 0.4f, 0.2f};
    
    early_reflections_ = std::make_unique<early_reflections>(early_delays, early_gains);
    
    // 鍒涘缓璋冨埗鍣?
    modulation_ = std::make_unique<modulator>(params.modulation_rate, params.modulation_depth, sample_rate);
}

void room_reverb_engine::calculate_delays() {
    float room_size_factor = params_.room_size;
    float sample_rate = static_cast<float>(sample_rate_);
    
    // 姊崇姸婊ゆ尝鍣ㄥ欢杩燂紙鍩轰簬鎴块棿澶у皬锛?
    comb_delays_.resize(8);
    comb_feedbacks_.resize(8);
    
    // 鍩虹寤惰繜鏃堕棿锛堟绉掕浆鎹负閲囨牱锛?
    float base_delay_ms = 20.0f + 80.0f * room_size_factor; // 20-100ms
    
    // 璐ㄦ暟寤惰繜鏃堕棿锛堥伩鍏嶈皭鎸級
    static const size_t prime_delays[8] = {1553, 1613, 1759, 1831, 1933, 2011, 2087, 2153};
    
    for(size_t i = 0; i < 8; ++i) {
        // 鍩轰簬鎴块棿澶у皬鐨勫欢杩熷彉鍖?
        float size_factor = 0.8f + 0.4f * (i / 7.0f) * room_size_factor;
        comb_delays_[i] = static_cast<size_t>(prime_delays[i] * size_factor);
        comb_feedbacks_[i] = 0.84f - 0.2f * room_size_factor; // 鍙嶉闅忔埧闂村澶ц€屽噺灏?
    }
    
    // 鍏ㄩ€氭护娉㈠櫒寤惰繜
    allpass_delays_.resize(4);
    for(size_t i = 0; i < 4; ++i) {
        allpass_delays_[i] = static_cast<size_t>((100.0f + 50.0f * i) * sample_rate / 1000.0f);
    }
}

void room_reverb_engine::initialize_filters() {
    // 鍒涘缓姊崇姸婊ゆ尝鍣?
    comb_filters_.clear();
    comb_filters_.reserve(comb_delays_.size());
    
    for(size_t i = 0; i < comb_delays_.size(); ++i) {
        float feedback = comb_feedbacks_[i];
        float damping = params_.damping;
        
        comb_filters_.push_back(std::make_unique<comb_filter>(comb_delays_[i], feedback, damping));
    }
    
    // 鍒涘缓鍏ㄩ€氭护娉㈠櫒
    allpass_filters_.clear();
    allpass_filters_.reserve(allpass_delays_.size());
    
    for(size_t i = 0; i < allpass_delays_.size(); ++i) {
        allpass_filters_.push_back(std::make_unique<allpass_filter>(allpass_delays_[i], 0.5f));
    }
}

void room_reverb_engine::process(audio_chunk& chunk) {
    if(chunk.is_empty()) return;
    
    // 淇濆瓨鍘熷鏁版嵁鐢ㄤ簬骞蹭俊鍙锋贩鍚?
    audio_chunk original_chunk;
    original_chunk.copy(chunk);
    
    // 澶勭悊鏃╂湡鍙嶅皠
    process_early_reflections(chunk);
    
    // 澶勭悊娣峰搷灏鹃煶
    process_reverb_tail(chunk);
    
    // 搴旂敤璋冨埗
    apply_modulation(chunk);
    
    // 娣峰悎骞叉箍淇″彿
    float wet_level = params_.wet_level;
    float dry_level = params_.dry_level;
    float width = params_.width;
    
    float* data = chunk.get_data();
    const float* original_data = original_chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    for(size_t i = 0; i < total_samples; ++i) {
        float wet = data[i];
        float dry = original_data[i];
        
        // 搴旂敤绔嬩綋澹板搴︼紙绠€鍖栫増锛?
        if(chunk.get_channels() == 2) {
            size_t channel = i % 2;
            float width_factor = (channel == 0) ? (1.0f + width) / 2.0f : (1.0f - width) / 2.0f;
            wet *= width_factor;
        }
        
        // 娣峰悎骞叉箍淇″彿
        data[i] = dry * dry_level + wet * wet_level;
    }
}

void room_reverb_engine::reset() {
    if(early_reflections_) {
        early_reflections_->reset();
    }
    
    for(auto& filter : comb_filters_) {
        if(filter) filter->reset();
    }
    
    for(auto& filter : allpass_filters_) {
        if(filter) filter->reset();
    }
    
    if(modulation_) {
        modulation_->reset();
    }
}

void room_reverb_engine::process_early_reflections(audio_chunk& chunk) {
    if(!early_reflections_) return;
    
    float* data = chunk.get_data();
    size_t samples = chunk.get_sample_count();
    uint32_t channels = chunk.get_channels();
    
    // 澶勭悊姣忎釜澹伴亾
    for(uint32_t ch = 0; ch < channels; ++ch) {
        for(size_t i = 0; i < samples; ++i) {
            size_t idx = i * channels + ch;
            float reflection = early_reflections_->process(data[idx], ch);
            data[idx] += reflection * 0.3f; // 娣峰悎鏃╂湡鍙嶅皠
        }
    }
}

void room_reverb_engine::process_reverb_tail(audio_chunk& chunk) {
    float* data = chunk.get_data();
    size_t samples = chunk.get_sample_count();
    uint32_t channels = chunk.get_channels();
    
    // 澶勭悊姣忎釜澹伴亾
    for(uint32_t ch = 0; ch < channels; ++ch) {
        for(size_t i = 0; i < samples; ++i) {
            size_t idx = i * channels + ch;
            float input = data[idx];
            float output = 0.0f;
            
            // 閫氳繃鎵€鏈夋⒊鐘舵护娉㈠櫒
            for(auto& comb : comb_filters_) {
                if(comb) {
                    output += comb->process(input);
                }
            }
            
            // 褰掍竴鍖?
            if(!comb_filters_.empty()) {
                output /= comb_filters_.size();
            }
            
            // 閫氳繃鍏ㄩ€氭护娉㈠櫒
            for(auto& allpass : allpass_filters_) {
                if(allpass) {
                    output = allpass->process(output);
                }
            }
            
            data[idx] = output;
        }
    }
}

void room_reverb_engine::apply_modulation(audio_chunk& chunk) {
    if(!modulation_ || !params_.enable_modulation) return;
    
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    for(size_t i = 0; i < total_samples; ++i) {
        float modulation = modulation_->process();
        data[i] *= (1.0f + modulation * params_.modulation_depth);
    }
}

// 澶у巺娣峰搷寮曟搸
hall_reverb_engine::hall_reverb_engine(const reverb_parameters& params, uint32_t sample_rate)
    : reverb_engine(params, sample_rate) {
    
    calculate_hall_delays();
    initialize_hall_filters();
    
    // 鍒涘缓璋冨埗鍣?
    modulation_ = std::make_unique<modulator>(params.modulation_rate, params.modulation_depth, sample_rate);
}

void hall_reverb_engine::calculate_hall_delays() {
    float room_size_factor = params_.room_size;
    float sample_rate = static_cast<float>(sample_rate_);
    
    // 澶у巺闇€瑕佹洿闀跨殑寤惰繜鏃堕棿
    comb_delays_.resize(12); // 鏇村鐨勬⒊鐘舵护娉㈠櫒
    comb_feedbacks_.resize(12);
    
    // 鏇村ぇ鐨勫欢杩熸椂闂磋寖鍥?
    static const size_t hall_delays[12] = {
        1777, 1847, 1913, 1993, 2053, 2111,
        2179, 2237, 2293, 2357, 2411, 2473
    };
    
    for(size_t i = 0; i < 12; ++i) {
        float size_factor = 0.9f + 0.2f * (i / 11.0f) * room_size_factor;
        comb_delays_[i] = static_cast<size_t>(hall_delays[i] * size_factor);
        comb_feedbacks_[i] = 0.88f - 0.15f * room_size_factor;
    }
    
    // 鏇村鐨勫叏閫氭护娉㈠櫒
    allpass_delays_.resize(6);
    for(size_t i = 0; i < 6; ++i) {
        allpass_delays_[i] = static_cast<size_t>((150.0f + 75.0f * i) * sample_rate / 1000.0f);
    }
}

void hall_reverb_engine::initialize_hall_filters() {
    // 鍒涘缓鏇村鐨勬⒊鐘舵护娉㈠櫒
    comb_filters_.clear();
    comb_filters_.reserve(comb_delays_.size());
    
    for(size_t i = 0; i < comb_delays_.size(); ++i) {
        float feedback = comb_feedbacks_[i];
        float damping = params_.damping;
        
        comb_filters_.push_back(std::make_unique<comb_filter>(comb_delays_[i], feedback, damping));
    }
    
    // 鍒涘缓鏇村鐨勫叏閫氭护娉㈠櫒
    allpass_filters_.clear();
    allpass_filters_.reserve(allpass_delays_.size());
    
    for(size_t i = 0; i < allpass_delays_.size(); ++i) {
        allpass_filters_.push_back(std::make_unique<allpass_filter>(allpass_delays_[i], 0.6f));
    }
}

void hall_reverb_engine::process(audio_chunk& chunk) {
    if(chunk.is_empty()) return;
    
    // 淇濆瓨鍘熷鏁版嵁
    audio_chunk original_chunk;
    original_chunk.copy(chunk);
    
    // 澶勭悊閫昏緫涓庢埧闂存贩鍝嶇被浼硷紝浣嗕娇鐢ㄦ洿澶氱殑婊ゆ尝鍣?
    float* data = chunk.get_data();
    size_t samples = chunk.get_sample_count();
    uint32_t channels = chunk.get_channels();
    
    // 澶勭悊姣忎釜澹伴亾
    for(uint32_t ch = 0; ch < channels; ++ch) {
        for(size_t i = 0; i < samples; ++i) {
            size_t idx = i * channels + ch;
            float input = data[idx];
            float output = 0.0f;
            
            // 閫氳繃鎵€鏈夋⒊鐘舵护娉㈠櫒
            for(auto& comb : comb_filters_) {
                if(comb) {
                    output += comb->process(input);
                }
            }
            
            // 褰掍竴鍖?
            if(!comb_filters_.empty()) {
                output /= comb_filters_.size();
            }
            
            // 閫氳繃鎵€鏈夊叏閫氭护娉㈠櫒
            for(auto& allpass : allpass_filters_) {
                if(allpass) {
                    output = allpass->process(output);
                }
            }
            
            data[idx] = output;
        }
    }
    
    // 搴旂敤璋冨埗鍜屾贩鍚堬紙涓庢埧闂存贩鍝嶇被浼硷級
    // ...锛堢渷鐣ラ噸澶嶄唬鐮侊級
}

void hall_reverb_engine::reset() {
    // 閲嶇疆鎵€鏈夋护娉㈠櫒
    for(auto& comb : comb_filters_) {
        if(comb) comb->reset();
    }
    
    for(auto& allpass : allpass_filters_) {
        if(allpass) allpass->reset();
    }
    
    if(modulation_) {
        modulation_->reset();
    }
}

// 鏉垮紡娣峰搷寮曟搸
plate_reverb_engine::plate_reverb_engine(const reverb_parameters& params, uint32_t sample_rate)
    : reverb_engine(params, sample_rate) {
    
    initialize_diffusion_network();
    
    // 鍒涘缓楂樺瘑搴﹁皟鍒?
    modulation_ = std::make_unique<modulator>(params.modulation_rate * 2.0f, params.modulation_depth, sample_rate);
}

void plate_reverb_engine::initialize_diffusion_network() {
    // 鍒涘缓鎵╂暎鐭╅樀
    diffusion_matrix_ = {
        {0.5f, 0.3f, 0.1f, 0.1f},
        {0.1f, 0.5f, 0.3f, 0.1f},
        {0.1f, 0.1f, 0.5f, 0.3f},
        {0.3f, 0.1f, 0.1f, 0.5f}
    };
    
    diffusion_state_.resize(4, 0.0f);
    
    // 鍒涘缓鍏ㄩ€氭护娉㈠櫒缃戠粶
    allpass_filters_.clear();
    
    // 楂樺瘑搴﹀叏閫氭护娉㈠櫒
    static const size_t plate_delays[16] = {
        149, 163, 181, 197, 211, 227, 241, 257,
        271, 283, 293, 307, 317, 331, 347, 359
    };
    
    float sample_rate_factor = static_cast<float>(sample_rate_) / 44100.0f;
    
    for(size_t i = 0; i < 16; ++i) {
        size_t delay = static_cast<size_t>(plate_delays[i] * sample_rate_factor);
        allpass_filters_.push_back(std::make_unique<allpass_filter>(delay, 0.7f));
    }
}

void plate_reverb_engine::process(audio_chunk& chunk) {
    if(chunk.is_empty()) return;
    
    // 淇濆瓨鍘熷鏁版嵁
    audio_chunk original_chunk;
    original_chunk.copy(chunk);
    
    // 閫氳繃鎵╂暎缃戠粶
    process_diffusion_network(chunk);
    
    // 閫氳繃鍏ㄩ€氭护娉㈠櫒缃戠粶
    float* data = chunk.get_data();
    size_t samples = chunk.get_sample_count();
    uint32_t channels = chunk.get_channels();
    
    for(uint32_t ch = 0; ch < channels; ++ch) {
        for(size_t i = 0; i < samples; ++i) {
            size_t idx = i * channels + ch;
            float input = data[idx];
            float output = input;
            
            // 閫氳繃鎵€鏈夊叏閫氭护娉㈠櫒
            for(auto& allpass : allpass_filters_) {
                if(allpass) {
                    output = allpass->process(output);
                }
            }
            
            data[idx] = output;
        }
    }
    
    // 搴旂敤璋冨埗
    apply_modulation(chunk);
    
    // 娣峰悎骞叉箍淇″彿
    float wet_level = params_.wet_level;
    float dry_level = params_.dry_level;
    
    float* final_data = chunk.get_data();
    const float* original_data = original_chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    for(size_t i = 0; i < total_samples; ++i) {
        float wet = final_data[i];
        float dry = original_data[i];
        final_data[i] = dry * dry_level + wet * wet_level;
    }
}

void plate_reverb_engine::reset() {
    for(auto& allpass : allpass_filters_) {
        if(allpass) allpass->reset();
    }
    
    std::fill(diffusion_state_.begin(), diffusion_state_.end(), 0.0f);
    
    if(modulation_) {
        modulation_->reset();
    }
}

void plate_reverb_engine::process_diffusion_network(audio_chunk& chunk) {
    float* data = chunk.get_data();
    size_t samples = chunk.get_sample_count();
    uint32_t channels = chunk.get_channels();
    
    // 绠€鍖栫殑鎵╂暎缃戠粶澶勭悊
    for(size_t i = 0; i < samples; ++i) {
        for(uint32_t ch = 0; ch < channels && ch < 4; ++ch) {
            size_t idx = i * channels + ch;
            float input = data[idx];
            
            // 搴旂敤鎵╂暎鐭╅樀
            float output = 0.0f;
            for(size_t j = 0; j < 4 && j < channels; ++j) {
                if(i < samples) {
                    size_t src_idx = i * channels + j;
                    output += diffusion_matrix_[ch][j] * data[src_idx];
                }
            }
            
            // 鐘舵€佹洿鏂?
            diffusion_state_[ch] = output;
            data[idx] = diffusion_state_[ch];
        }
    }
}

// DSP娣峰搷鏁堟灉鍣ㄥ疄鐜?
dsp_reverb_advanced::dsp_reverb_advanced() 
    : dsp_effect_advanced(create_default_reverb_params()) {
    create_reverb_engine();
    create_modulation();
    create_filters();
}

dsp_reverb_advanced::dsp_reverb_advanced(const dsp_effect_params& params)
    : dsp_effect_advanced(params) {
    create_reverb_engine();
    create_modulation();
    create_filters();
}

dsp_reverb_advanced::~dsp_reverb_advanced() = default;

bool dsp_reverb_advanced::instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                                     uint32_t channels) {
    if(sample_rate < 8000 || sample_rate > 192000) {
        return false;
    }
    
    if(channels < 1 || channels > 8) {
        return false;
    }
    
    // 鏇存柊娣峰搷寮曟搸
    if(engine_) {
        engine_->set_params(params_);
    }
    
    // 鍒涘缓缂撳啿
    wet_buffer_ = audio_chunk_utils::create_chunk(4096, channels, sample_rate);
    dry_buffer_ = audio_chunk_utils::create_chunk(4096, channels, sample_rate);
    
    return true;
}

void dsp_reverb_advanced::run(audio_chunk& chunk, abort_callback& abort) {
    if(is_bypassed() || !is_enabled() || chunk.is_empty() || abort.is_aborting()) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    process_chunk_internal(chunk, abort);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    update_cpu_usage(static_cast<float>(duration.count()) / 1000.0f); // 杞崲涓烘绉?
}

void dsp_reverb_advanced::reset() {
    if(engine_) {
        engine_->reset();
    }
    
    if(modulation_) {
        modulation_->reset();
    }
    
    if(input_filter_) {
        input_filter_->reset();
    }
    
    if(output_filter_) {
        output_filter_->reset();
    }
}

void dsp_reverb_advanced::set_room_size(float size) {
    params_.room_size = std::max(0.0f, std::min(1.0f, size));
    if(engine_) {
        engine_->set_params(params_);
    }
}

void dsp_reverb_advanced::set_damping(float damping) {
    params_.damping = std::max(0.0f, std::min(1.0f, damping));
    if(engine_) {
        engine_->set_params(params_);
    }
}

void dsp_reverb_advanced::set_wet_level(float level) {
    params_.wet_level = std::max(0.0f, std::min(1.0f, level));
}

void dsp_reverb_advanced::set_dry_level(float level) {
    params_.dry_level = std::max(0.0f, std::min(1.0f, level));
}

void dsp_reverb_advanced::set_width(float width) {
    params_.width = std::max(0.0f, std::min(1.0f, width));
}

void dsp_reverb_advanced::set_predelay(float ms) {
    params_.predelay = std::max(0.0f, std::min(100.0f, ms));
    if(engine_) {
        engine_->set_params(params_);
    }
}

void dsp_reverb_advanced::set_decay_time(float seconds) {
    params_.decay_time = std::max(0.1f, std::min(10.0f, seconds));
    if(engine_) {
        engine_->set_params(params_);
    }
}

void dsp_reverb_advanced::set_diffusion(float diffusion) {
    params_.diffusion = std::max(0.0f, std::min(1.0f, diffusion));
    if(engine_) {
        engine_->set_params(params_);
    }
}

void dsp_reverb_advanced::set_modulation_rate(float rate) {
    params_.modulation_rate = std::max(0.01f, std::min(10.0f, rate));
    if(modulation_) {
        modulation_->set_rate(rate);
    }
}

void dsp_reverb_advanced::set_modulation_depth(float depth) {
    params_.modulation_depth = std::max(0.0f, std::min(1.0f, depth));
    if(modulation_) {
        modulation_->set_depth(depth);
    }
}

void dsp_reverb_advanced::enable_modulation(bool enable) {
    params_.enable_modulation = enable;
}

void dsp_reverb_advanced::enable_filtering(bool enable) {
    params_.enable_filtering = enable;
}

void dsp_reverb_advanced::load_room_preset(float room_size) {
    params_.type = reverb_type::room;
    params_.room_size = room_size;
    params_.damping = 0.3f + 0.4f * room_size;
    params_.wet_level = 0.2f + 0.3f * room_size;
    params_.decay_time = 0.5f + 1.5f * room_size;
    params_.diffusion = 0.6f + 0.3f * room_size;
    
    create_reverb_engine();
}

void dsp_reverb_advanced::load_hall_preset(float room_size) {
    params_.type = reverb_type::hall;
    params_.room_size = room_size;
    params_.damping = 0.2f + 0.3f * room_size;
    params_.wet_level = 0.3f + 0.4f * room_size;
    params_.decay_time = 1.0f + 2.0f * room_size;
    params_.diffusion = 0.7f + 0.2f * room_size;
    params_.predelay = 10.0f + 20.0f * room_size;
    
    create_reverb_engine();
}

void dsp_reverb_advanced::load_plate_preset() {
    params_.type = reverb_type::plate;
    params_.room_size = 0.8f;
    params_.damping = 0.1f;
    params_.wet_level = 0.4f;
    params_.decay_time = 2.5f;
    params_.diffusion = 0.9f;
    params_.modulation_rate = 0.5f;
    params_.modulation_depth = 0.2f;
    
    create_reverb_engine();
}

void dsp_reverb_advanced::load_cathedral_preset() {
    params_.type = reverb_type::cathedral;
    params_.room_size = 0.95f;
    params_.damping = 0.1f;
    params_.wet_level = 0.5f;
    params_.decay_time = 5.0f;
    params_.diffusion = 0.95f;
    params_.predelay = 50.0f;
    params_.width = 1.0f;
    
    create_reverb_engine();
}

void dsp_reverb_advanced::set_small_room() {
    load_room_preset(0.2f);
}

void dsp_reverb_advanced::set_medium_room() {
    load_room_preset(0.5f);
}

void dsp_reverb_advanced::set_large_room() {
    load_room_preset(0.8f);
}

void dsp_reverb_advanced::set_concert_hall() {
    load_hall_preset(0.7f);
}

void dsp_reverb_advanced::set_cathedral() {
    load_cathedral_preset();
}

void dsp_reverb_advanced::process_chunk_internal(audio_chunk& chunk, abort_callback& abort) {
    if(!engine_ || chunk.is_empty() || abort.is_aborting()) {
        return;
    }
    
    // 淇濆瓨鍘熷鏁版嵁
    audio_chunk original_chunk;
    original_chunk.copy(chunk);
    
    // 搴旂敤杈撳叆婊ゆ尝
    if(params_.enable_filtering && input_filter_) {
        apply_input_filtering(chunk);
    }
    
    // 鍒涘缓婀夸俊鍙风紦鍐?
    if(!wet_buffer_ || wet_buffer_->get_sample_count() < chunk.get_sample_count() ||
       wet_buffer_->get_channels() != chunk.get_channels()) {
        wet_buffer_ = audio_chunk_utils::create_chunk(chunk.get_sample_count(), 
                                                     chunk.get_channels(), 
                                                     chunk.get_sample_rate());
    }
    
    wet_buffer_->copy(chunk);
    
    // 澶勭悊娣峰搷
    engine_->process(*wet_buffer_);
    
    // 搴旂敤璋冨埗
    if(params_.enable_modulation && modulation_) {
        apply_modulation(*wet_buffer_);
    }
    
    // 搴旂敤杈撳嚭婊ゆ尝
    if(params_.enable_filtering && output_filter_) {
        apply_output_filtering(*wet_buffer_);
    }
    
    // 娣峰悎骞叉箍淇″彿
    mix_wet_dry_signals(chunk, original_chunk);
    
    // 搴旂敤绔嬩綋澹板搴?
    apply_stereo_width(chunk);
}

void dsp_reverb_advanced::update_cpu_usage(float usage) {
    // 瀹炵幇CPU浣跨敤鐜囪绠?
    // 杩欓噷鍙互鏍规嵁澶勭悊鏃堕棿鍜岄噰鏍锋暟鏉ヤ及绠?
    set_cpu_usage(usage);
}

void dsp_reverb_advanced::create_reverb_engine() {
    switch(params_.type) {
        case reverb_type::room:
            engine_ = std::make_unique<room_reverb_engine>(params_, 44100); // 鍋囪鏍囧噯閲囨牱鐜?
            break;
            
        case reverb_type::hall:
            engine_ = std::make_unique<hall_reverb_engine>(params_, 44100);
            break;
            
        case reverb_type::plate:
            engine_ = std::make_unique<plate_reverb_engine>(params_, 44100);
            break;
            
        default:
            engine_ = std::make_unique<room_reverb_engine>(params_, 44100);
            break;
    }
}

void dsp_reverb_advanced::create_modulation() {
    modulation_ = std::make_unique<modulator>(params_.modulation_rate, params_.modulation_depth, 44100);
}

void dsp_reverb_advanced::create_filters() {
    if(params_.enable_filtering) {
        // 鍒涘缓杈撳叆婊ゆ尝鍣紙楂橀€氾紝鍘婚櫎浣庨鍣０锛?
        input_filter_ = std::make_unique<biquad_filter>();
        
        float b0, b1, b2, a0, a1, a2;
        biquad_filter::design_high_pass(80.0f, 0.7f, 44100.0f, b0, b1, b2, a0, a1, a2);
        input_filter_->set_coefficients(b0, b1, b2, a0, a1, a2);
        
        // 鍒涘缓杈撳嚭婊ゆ尝鍣紙浣庨€氾紝鍘婚櫎楂橀鍣０锛?
        output_filter_ = std::make_unique<biquad_filter>();
        
        biquad_filter::design_low_pass(18000.0f, 0.7f, 44100.0f, b0, b1, b2, a0, a1, a2);
        output_filter_->set_coefficients(b0, b1, b2, a0, a1, a2);
    }
}

void dsp_reverb_advanced::calculate_delays() {
    // 鏍规嵁鍙傛暟璁＄畻寤惰繜鏃堕棿
    // 杩欓噷鍙互瀹炵幇鏇村鏉傜殑寤惰繜璁＄畻閫昏緫
}

void dsp_reverb_advanced::apply_input_filtering(audio_chunk& chunk) {
    if(input_filter_) {
        input_filter_->process_block(chunk.get_data(), 
                                   chunk.get_sample_count() * chunk.get_channels());
    }
}

void dsp_reverb_advanced::apply_output_filtering(audio_chunk& chunk) {
    if(output_filter_) {
        output_filter_->process_block(chunk.get_data(), 
                                    chunk.get_sample_count() * chunk.get_channels());
    }
}

void dsp_reverb_advanced::apply_modulation(audio_chunk& chunk) {
    if(!modulation_ || !params_.enable_modulation) return;
    
    float* data = chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    for(size_t i = 0; i < total_samples; ++i) {
        float modulation = modulation_->process();
        data[i] *= (1.0f + modulation * params_.modulation_depth);
    }
}

void dsp_reverb_advanced::mix_wet_dry_signals(audio_chunk& chunk, const audio_chunk& dry_chunk) {
    float* wet_data = chunk.get_data();
    const float* dry_data = dry_chunk.get_data();
    size_t total_samples = chunk.get_sample_count() * chunk.get_channels();
    
    float wet_level = params_.wet_level;
    float dry_level = params_.dry_level;
    
    for(size_t i = 0; i < total_samples; ++i) {
        float wet = wet_data[i];
        float dry = dry_data[i];
        wet_data[i] = dry * dry_level + wet * wet_level;
    }
}

void dsp_reverb_advanced::apply_stereo_width(audio_chunk& chunk) {
    if(chunk.get_channels() != 2) return;
    
    float* data = chunk.get_data();
    size_t samples = chunk.get_sample_count();
    float width = params_.width;
    
    for(size_t i = 0; i < samples; ++i) {
        float left = data[i * 2];
        float right = data[i * 2 + 1];
        
        // MS杞崲
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // 搴旂敤瀹藉害
        side *= width;
        
        // 杞崲鍥濴R
        data[i * 2] = mid + side;
        data[i * 2 + 1] = mid - side;
    }
}

// 娣峰搷宸ュ叿鍑芥暟瀹炵幇
namespace reverb_utils {

float calculate_reverb_time(float room_size, float damping, float diffusion) {
    // 绠€鍖栫殑娣峰搷鏃堕棿璁＄畻
    float base_rt = 0.5f + 2.0f * room_size;
    float damping_factor = 1.0f - 0.5f * damping;
    float diffusion_factor = 1.0f + 0.2f * diffusion;
    
    return base_rt * damping_factor * diffusion_factor;
}

float calculate_reverb_density(float room_size, float diffusion) {
    // 绠€鍖栫殑娣峰搷瀵嗗害璁＄畻
    return 0.5f + 0.5f * room_size + 0.3f * diffusion;
}

std::vector<size_t> calculate_comb_delays(float room_size, float sample_rate) {
    std::vector<size_t> delays;
    delays.reserve(8);
    
    // 鍩轰簬鎴块棿澶у皬鐨勮川鏁板欢杩?
    static const size_t base_delays[8] = {1553, 1613, 1759, 1831, 1933, 2011, 2087, 2153};
    
    float size_factor = 0.8f + 0.4f * room_size;
    
    for(size_t i = 0; i < 8; ++i) {
        delays.push_back(static_cast<size_t>(base_delays[i] * size_factor));
    }
    
    return delays;
}

std::vector<size_t> calculate_allpass_delays(float room_size, float sample_rate) {
    std::vector<size_t> delays;
    delays.reserve(4);
    
    // 鍩轰簬鎴块棿澶у皬鐨勫叏閫氬欢杩?
    for(size_t i = 0; i < 4; ++i) {
        float base_delay_ms = 100.0f + 50.0f * i;
        delays.push_back(static_cast<size_t>(base_delay_ms * sample_rate / 1000.0f));
    }
    
    return delays;
}

room_acoustics analyze_room_acoustics(const std::vector<float>& impulse_response,
                                     float sample_rate) {
    room_acoustics acoustics;
    
    if(impulse_response.empty()) {
        return acoustics;
    }
    
    // 璁＄畻RT60锛堢畝鍖栫増锛?
    size_t peak_index = 0;
    float peak_value = 0.0f;
    
    for(size_t i = 0; i < impulse_response.size(); ++i) {
        if(std::abs(impulse_response[i]) > peak_value) {
            peak_value = std::abs(impulse_response[i]);
            peak_index = i;
        }
    }
    
    // 鎵惧埌琛板噺60dB鐨勭偣锛堢畝鍖栬绠楋級
    float target_level = peak_value * 0.001f; // -60dB
    size_t rt60_index = impulse_response.size() - 1;
    
    for(size_t i = peak_index; i < impulse_response.size(); ++i) {
        if(std::abs(impulse_response[i]) < target_level) {
            rt60_index = i;
            break;
        }
    }
    
    acoustics.rt60 = static_cast<float>(rt60_index - peak_index) / sample_rate;
    
    // 鍏朵粬鍙傛暟锛堢畝鍖栬绠楋級
    acoustics.clarity = 0.8f;      // 榛樿鍊?
    acoustics.definition = 0.7f;   // 榛樿鍊?
    acoustics.envelopment = 0.9f;  // 榛樿鍊?
    acoustics.warmth = 0.6f;       // 榛樿鍊?
    acoustics.brilliance = 0.5f;   // 榛樿鍊?
    
    return acoustics;
}

reverb_parameters generate_room_reverb(float room_size, float room_type) {
    reverb_parameters params;
    params.type = reverb_type::room;
    params.room_size = room_size;
    params.damping = 0.3f + 0.4f * room_size;
    params.wet_level = 0.2f + 0.3f * room_size;
    params.decay_time = 0.5f + 1.5f * room_size;
    params.diffusion = 0.6f + 0.3f * room_size;
    params.predelay = 0.0f;
    
    return params;
}

reverb_parameters generate_hall_reverb(float hall_size, float hall_type) {
    reverb_parameters params;
    params.type = reverb_type::hall;
    params.room_size = hall_size;
    params.damping = 0.2f + 0.3f * hall_size;
    params.wet_level = 0.3f + 0.4f * hall_size;
    params.decay_time = 1.0f + 2.0f * hall_size;
    params.diffusion = 0.7f + 0.2f * hall_size;
    params.predelay = 10.0f + 20.0f * hall_size;
    
    return params;
}

reverb_parameters generate_plate_reverb(float plate_type) {
    reverb_parameters params;
    params.type = reverb_type::plate;
    params.room_size = 0.8f;
    params.damping = 0.1f;
    params.wet_level = 0.4f;
    params.decay_time = 2.5f;
    params.diffusion = 0.9f;
    params.modulation_rate = 0.5f;
    params.modulation_depth = 0.2f;
    
    return params;
}

float calculate_reverb_quality(const audio_chunk& dry_signal,
                              const audio_chunk& wet_signal,
                              const reverb_parameters& params) {
    // 绠€鍖栫殑娣峰搷璐ㄩ噺璇勪及
    // 瀹為檯瀹炵幇闇€瑕佸鏉傜殑闊抽鍒嗘瀽绠楁硶
    
    float quality = 0.8f; // 鍩虹璐ㄩ噺
    
    // 鏍规嵁鍙傛暟璋冩暣
    if(params.room_size > 0.5f) quality += 0.1f;
    if(params.diffusion > 0.7f) quality += 0.05f;
    if(params.modulation_depth < 0.3f) quality += 0.05f;
    
    return std::min(1.0f, quality);
}

std::string generate_reverb_report(const dsp_reverb_advanced& reverb) {
    std::stringstream report;
    report << "Reverb Engine Report:\n";
    report << "  Type: ";
    
    switch(reverb.get_params().type) {
        case reverb_type::room: report << "Room"; break;
        case reverb_type::hall: report << "Hall"; break;
        case reverb_type::plate: report << "Plate"; break;
        default: report << "Unknown"; break;
    }
    
    report << "\n  Room Size: " << reverb.get_params().room_size;
    report << "\n  Damping: " << reverb.get_params().damping;
    report << "\n  Wet Level: " << reverb.get_params().wet_level;
    report << "\n  Decay Time: " << reverb.get_params().decay_time << "s";
    report << "\n  Latency: " << reverb.get_latency() << "ms";
    
    return report.str();
}

} // namespace reverb_utils

} // namespace fb2k