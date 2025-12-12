#pragma once

// 闃舵1.3锛氬弬鏁板潎琛″櫒
// 涓撲笟绾у弬鏁板潎琛″櫒锛屾敮鎸佸娈甸鐜囪皟鑺?

#include "dsp_manager.h"
#include <vector>
#include <complex>
#include <cmath>

namespace fb2k {

// 婊ゆ尝鍣ㄧ被鍨?
enum class filter_type {
    low_shelf,      // 浣庢灦婊ゆ尝鍣?
    high_shelf,     // 楂樻灦婊ゆ尝鍣?
    peak,           // 宄板€兼护娉㈠櫒
    low_pass,       // 浣庨€氭护娉㈠櫒
    high_pass,      // 楂橀€氭护娉㈠櫒
    band_pass,      // 甯﹂€氭护娉㈠櫒
    notch           // 闄锋尝婊ゆ尝鍣?
};

// 鍧囪　鍣ㄩ娈靛弬鏁?
struct eq_band_params {
    float frequency;        // 涓績棰戠巼 (Hz)
    float gain;            // 澧炵泭 (dB)
    float bandwidth;       // 甯﹀ (Q鍊兼垨octaves)
    filter_type type;      // 婊ゆ尝鍣ㄧ被鍨?
    bool is_enabled;       // 鏄惁鍚敤
    std::string name;      // 棰戞鍚嶇О
    
    eq_band_params() : 
        frequency(1000.0f), gain(0.0f), bandwidth(1.0f), 
        type(filter_type::peak), is_enabled(true), name("") {}
    
    eq_band_params(float freq, float gain_db, float bw, filter_type t) :
        frequency(freq), gain(gain_db), bandwidth(bw), type(t), 
        is_enabled(true), name("") {}
};

// 鍙屼簩闃舵护娉㈠櫒锛圔iquad Filter锛?
class biquad_filter {
private:
    // 婊ゆ尝鍣ㄧ郴鏁?
    float a0_, a1_, a2_;  // 鍒嗘瘝绯绘暟
    float b0_, b1_, b2_;  // 鍒嗗瓙绯绘暟
    
    // 鐘舵€佸彉閲?
    float x1_, x2_;       // 杈撳叆鍘嗗彶
    float y1_, y2_;       // 杈撳嚭鍘嗗彶
    
public:
    biquad_filter() : a0_(1.0f), a1_(0.0f), a2_(0.0f), 
                     b0_(1.0f), b1_(0.0f), b2_(0.0f),
                     x1_(0.0f), x2_(0.0f), y1_(0.0f), y2_(0.0f) {}
    
    // 璁剧疆婊ゆ尝鍣ㄧ郴鏁?
    void set_coefficients(float b0, float b1, float b2, float a0, float a1, float a2);
    
    // 澶勭悊鍗曚釜閲囨牱
    float process(float input);
    
    // 澶勭悊闊抽鍧?
    void process_block(float* data, size_t samples);
    
    // 閲嶇疆鐘舵€?
    void reset();
    
    // 鑾峰彇棰戠巼鍝嶅簲
    std::complex<float> get_frequency_response(float frequency, float sample_rate) const;
    
    // 璁捐婊ゆ尝鍣?
    static void design_peaking(float frequency, float gain_db, float bandwidth, 
                              float sample_rate, float& b0, float& b1, float& b2,
                              float& a0, float& a1, float& a2);
    
    static void design_low_shelf(float frequency, float gain_db, float slope,
                                float sample_rate, float& b0, float& b1, float& b2,
                                float& a0, float& a1, float& a2);
    
    static void design_high_shelf(float frequency, float gain_db, float slope,
                                 float sample_rate, float& b0, float& b1, float& b2,
                                 float& a0, float& a1, float& a2);
    
    static void design_low_pass(float frequency, float q, float sample_rate,
                               float& b0, float& b1, float& b2,
                               float& a0, float& a1, float& a2);
    
    static void design_high_pass(float frequency, float q, float sample_rate,
                                float& b0, float& b1, float& b2,
                                float& a0, float& a1, float& a2);
};

// 鍙傛暟鍧囪　鍣ㄩ娈?
class eq_band {
private:
    eq_band_params params_;
    biquad_filter filter_;
    bool needs_update_;
    
public:
    eq_band() : needs_update_(true) {}
    explicit eq_band(const eq_band_params& params) : params_(params), needs_update_(true) {}
    
    // 鍙傛暟璁剧疆
    void set_params(const eq_band_params& params);
    const eq_band_params& get_params() const { return params_; }
    
    // 瀹炴椂鍙傛暟璋冭妭
    void set_frequency(float frequency);
    void set_gain(float gain_db);
    void set_bandwidth(float bandwidth);
    void set_enabled(bool enabled);
    
    // 闊抽澶勭悊
    void process(audio_chunk& chunk);
    void process_block(float* data, size_t samples);
    
    // 鐘舵€佺鐞?
    void reset();
    bool is_enabled() const { return params_.is_enabled; }
    
    // 棰戠巼鍝嶅簲鍒嗘瀽
    std::complex<float> get_frequency_response(float frequency, float sample_rate) const;
    float get_gain_at_frequency(float frequency, float sample_rate) const;
    
private:
    void update_filter_coefficients();
    void validate_params();
};

// 鍙傛暟鍧囪　鍣ㄤ富绫?
class dsp_equalizer_advanced : public dsp_effect_advanced {
private:
    std::vector<std::unique_ptr<eq_band>> bands_;
    static constexpr size_t MAX_BANDS = 32;
    
    // 棰勮棰戞棰戠巼锛堢鍚圛SO鏍囧噯锛?
    static constexpr float ISO_FREQUENCIES[10] = {
        31.25f, 62.5f, 125.0f, 250.0f, 500.0f,
        1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
    };
    
    // 鐘舵€?
    bool needs_coefficient_update_;
    std::atomic<bool> is_updating_coefficients_;
    
public:
    dsp_equalizer_advanced();
    explicit dsp_equalizer_advanced(const dsp_effect_params& params);
    ~dsp_equalizer_advanced() override;
    
    // DSP鎺ュ彛瀹炵幇
    bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                    uint32_t channels) override;
    void run(audio_chunk& chunk, abort_callback& abort) override;
    void reset() override;
    
    // 鍧囪　鍣ㄧ壒瀹氭帴鍙?
    size_t add_band(const eq_band_params& params);
    bool remove_band(size_t index);
    void clear_bands();
    
    size_t get_band_count() const { return bands_.size(); }
    eq_band* get_band(size_t index);
    const eq_band* get_band(size_t index) const;
    
    // 棰勮绠＄悊
    void load_iso_preset();
    void load_classical_preset();
    void load_rock_preset();
    void load_jazz_preset();
    void load_pop_preset();
    void load_headphone_preset();
    
    // 棰戠巼鍝嶅簲鍒嗘瀽
    std::vector<std::complex<float>> get_frequency_response(
        const std::vector<float>& frequencies, float sample_rate) const;
    
    float get_response_at_frequency(float frequency, float sample_rate) const;
    
    // 瀹炴椂鍙傛暟璋冭妭锛堢嚎绋嬪畨鍏級
    bool set_band_frequency(size_t band, float frequency);
    bool set_band_gain(size_t band, float gain_db);
    bool set_band_bandwidth(size_t band, float bandwidth);
    bool set_band_enabled(size_t band, bool enabled);
    
    // 鎵归噺鎿嶄綔
    void set_all_bands_gain(float gain_db);
    void reset_all_bands();
    void copy_band_settings(size_t source_band, size_t target_band);
    
    // 楂樼骇鍔熻兘
    void auto_gain_compensate();
    void analyze_and_suggest_corrections(const audio_chunk& reference);
    std::vector<eq_band_params> suggest_corrections_for_room_response(
        const std::vector<float>& room_response);
    
protected:
    // dsp_effect_advanced 鎺ュ彛瀹炵幇
    void process_chunk_internal(audio_chunk& chunk, abort_callback& abort) override;
    void update_cpu_usage(float usage) override;
    
private:
    void initialize_default_bands();
    void update_all_coefficients();
    void validate_band_index(size_t index);
    
    // 宸ュ巶鏂规硶
    static std::unique_ptr<eq_band> create_band(const eq_band_params& params);
};

// 10娈靛弬鏁板潎琛″櫒锛堢鍚圛SO鏍囧噯锛?
class dsp_equalizer_10band : public dsp_equalizer_advanced {
public:
    dsp_equalizer_10band();
    explicit dsp_equalizer_10band(const dsp_effect_params& params);
    
    // 蹇€熼璁?
    void load_flat_response();
    void load_v_shape();
    void load_smiley_curve();
    void load_loudness_contour();
};

// 鍥惧舰鍧囪　鍣紙鍥哄畾棰戞锛?
class dsp_graphic_equalizer : public dsp_effect_advanced {
private:
    std::vector<std::unique_ptr<eq_band>> iso_bands_;
    static constexpr size_t ISO_BAND_COUNT = 10;
    
public:
    dsp_graphic_equalizer();
    explicit dsp_graphic_equalizer(const dsp_effect_params& params);
    
    // DSP鎺ュ彛瀹炵幇
    bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                    uint32_t channels) override;
    void run(audio_chunk& chunk, abort_callback& abort) override;
    void reset() override;
    
    // 鍥惧舰鍧囪　鍣ㄧ壒瀹氭帴鍙?
    void set_iso_band_gain(size_t band, float gain_db);
    float get_iso_band_gain(size_t band) const;
    void set_all_iso_bands(float gain_db);
    
    const std::vector<float>& get_iso_frequencies() const;
};

// 鎴块棿鍝嶅簲鍧囪　鍣?
class dsp_room_equalizer : public dsp_equalizer_advanced {
private:
    std::vector<float> room_response_;
    std::vector<float> target_response_;
    
public:
    dsp_room_equalizer();
    explicit dsp_room_equalizer(const dsp_effect_params& params);
    
    // 鎴块棿鍝嶅簲璁剧疆
    void set_room_response(const std::vector<float>& response, 
                          const std::vector<float>& frequencies);
    void set_target_response(const std::vector<float>& response);
    
    // 鑷姩鏍℃
    void auto_calculate_corrections();
    std::vector<eq_band_params> calculate_optimal_corrections();
    
    // 娴嬮噺鏀寔
    bool can_measure_room_response() const;
    void start_room_measurement();
    void stop_room_measurement();
    std::vector<float> get_measurement_progress() const;
};

// DSP鍧囪　鍣ㄥ伐鍏峰嚱鏁?
namespace eq_utils {

// 棰戠巼杞崲宸ュ叿
float frequency_to_midi(float frequency);
float midi_to_frequency(float midi_note);
float frequency_to_octave(float frequency, float reference_freq = 1000.0f);

// Q鍊煎拰甯﹀杞崲
float q_to_bandwidth(float q);
float bandwidth_to_q(float bandwidth);
float octave_to_q(float octaves);
float q_to_octave(float q);

// 澧炵泭杞崲
db_to_linear(float gain_db);
linear_to_db(float gain_linear);

// 棰戠巼鍝嶅簲璁＄畻
std::vector<std::complex<float>> calculate_combined_response(
    const std::vector<eq_band>& bands, 
    const std::vector<float>& frequencies, 
    float sample_rate);

// 鏈€浼橀娈垫斁缃?
std::vector<float> optimize_band_placement(
    const std::vector<float>& target_frequencies,
    size_t num_bands,
    float min_frequency,
    float max_frequency);

// 鎴块棿鍝嶅簲鍒嗘瀽
struct room_analysis_result {
    std::vector<float> measured_response;
    std::vector<float> recommended_corrections;
    std::vector<float> problematic_frequencies;
    float overall_gain_adjustment;
};

room_analysis_result analyze_room_response(
    const std::vector<float>& measured_response,
    const std::vector<float>& frequencies,
    const std::vector<float>& target_response);

} // namespace eq_utils

// 鏍囧噯DSP鏁堟灉鍣ㄥ伐鍘?
class dsp_std_effect_factory {
public:
    // 鏍囧噯鏁堟灉鍣?
    static std::unique_ptr<dsp_equalizer_advanced> create_equalizer_10band();
    static std::unique_ptr<dsp_equalizer_advanced> create_equalizer_31band();
    
    static std::unique_ptr<dsp_effect_advanced> create_reverb_basic();
    static std::unique_ptr<dsp_effect_advanced> create_reverb_room();
    static std::unique_ptr<dsp_effect_advanced> create_reverb_hall();
    
    static std::unique_ptr<dsp_effect_advanced> create_compressor_basic();
    static std::unique_ptr<dsp_effect_advanced> create_compressor_multiband();
    
    static std::unique_ptr<dsp_effect_advanced> create_limiter_basic();
    static std::unique_ptr<dsp_effect_advanced> create_limiter_lookahead();
    
    static std::unique_ptr<dsp_effect_advanced> create_volume_control_advanced();
    static std::unique_ptr<dsp_effect_advanced> create_crossfeed_natural();
    static std::unique_ptr<dsp_effect_advanced> create_crossfeed_jmeier();
    
    // 棰勮DSP閾?
    static std::vector<std::unique_ptr<dsp_effect_advanced>> 
        create_hifi_effect_chain();
    
    static std::vector<std::unique_ptr<dsp_effect_advanced>> 
        create_headphone_effect_chain();
    
    static std::vector<std::unique_ptr<dsp_effect_advanced>> 
        create_room_correction_chain(const std::vector<float>& room_response);
    
    // 鏍规嵁foobar2000閰嶇疆鍒涘缓鏁堟灉鍣?
    static std::unique_ptr<dsp_effect_advanced> 
        create_from_fb2k_config(const std::string& config_name);
};

} // namespace fb2k