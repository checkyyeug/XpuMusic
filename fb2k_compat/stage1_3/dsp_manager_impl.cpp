#include "dsp_manager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace fb2k {

// DSP绠＄悊鍣ㄥ疄鐜?

dsp_manager::dsp_manager() 
    : use_multithreading_(false),
      preset_manager_(std::make_unique<dsp_preset_manager>()),
      performance_monitor_(std::make_unique<dsp_performance_monitor>()) {
}

dsp_manager::~dsp_manager() {
    shutdown();
}

bool dsp_manager::initialize(const dsp_config& config) {
    std::cout << "[DSPManager] 鍒濆鍖朌SP绠＄悊鍣?.." << std::endl;
    
    config_ = config;
    
    // 鍒濆鍖栨€ц兘鐩戞帶
    if(config_.enable_performance_monitoring) {
        performance_monitor_->start_monitoring();
    }
    
    // 鍒濆鍖栧绾跨▼澶勭悊鍣?
    if(config_.enable_multithreading) {
        thread_pool_ = std::make_unique<multithreaded_dsp_processor>(config_.max_threads);
        if(!thread_pool_->start()) {
            std::cerr << "[DSPManager] 澶氱嚎绋嬪鐞嗗櫒鍚姩澶辫触" << std::endl;
            return false;
        }
        use_multithreading_ = true;
        std::cout << "[DSPManager] 澶氱嚎绋嬪鐞嗗櫒宸插惎鍔紝绾跨▼鏁? " << config_.max_threads << std::endl;
    }
    
    // 鍔犺浇鏍囧噯鏁堟灉鍣?
    if(config_.enable_standard_effects) {
        load_standard_effects();
    }
    
    std::cout << "[DSPManager] DSP绠＄悊鍣ㄥ垵濮嬪寲瀹屾垚" << std::endl;
    std::cout << "[DSPManager] 閰嶇疆: " << std::endl;
    std::cout << "  - 澶氱嚎绋? " << (config_.enable_multithreading ? "鍚敤" : "绂佺敤") << std::endl;
    std::cout << "  - 鎬ц兘鐩戞帶: " << (config_.enable_performance_monitoring ? "鍚敤" : "绂佺敤") << std::endl;
    std::cout << "  - 鍐呭瓨姹? " << (config_.memory_pool_size / (1024*1024)) << "MB" << std::endl;
    
    return true;
}

void dsp_manager::shutdown() {
    std::cout << "[DSPManager] 鍏抽棴DSP绠＄悊鍣?.." << std::endl;
    
    // 鍋滄鎬ц兘鐩戞帶
    if(performance_monitor_) {
        performance_monitor_->stop_monitoring();
    }
    
    // 鍋滄澶氱嚎绋嬪鐞嗗櫒
    if(thread_pool_) {
        thread_pool_->stop();
        use_multithreading_ = false;
    }
    
    // 娓呯悊鏁堟灉鍣?
    clear_effects();
    
    std::cout << "[DSPManager] DSP绠＄悊鍣ㄥ凡鍏抽棴" << std::endl;
}

bool dsp_manager::add_effect(std::unique_ptr<dsp_effect_advanced> effect) {
    if(!effect) {
        return false;
    }
    
    if(effects_.size() >= config_.max_effects) {
        std::cerr << "[DSPManager] 鏁堟灉鍣ㄦ暟閲忓凡杈句笂闄? " << config_.max_effects << std::endl;
        return false;
    }
    
    effects_.push_back(std::move(effect));
    
    std::cout << "[DSPManager] 娣诲姞鏁堟灉鍣? " << effects_.back()->get_name() << std::endl;
    return true;
}

bool dsp_manager::remove_effect(size_t index) {
    if(index >= effects_.size()) {
        return false;
    }
    
    std::cout << "[DSPManager] 绉婚櫎鏁堟灉鍣? " << effects_[index]->get_name() << std::endl;
    effects_.erase(effects_.begin() + index);
    return true;
}

void dsp_manager::clear_effects() {
    std::cout << "[DSPManager] 娓呴櫎鎵€鏈夋晥鏋滃櫒锛堟暟閲? " << effects_.size() << ")" << std::endl;
    effects_.clear();
}

size_t dsp_manager::get_effect_count() const {
    return effects_.size();
}

dsp_effect_advanced* dsp_manager::get_effect(size_t index) {
    return index < effects_.size() ? effects_[index].get() : nullptr;
}

const dsp_effect_advanced* dsp_manager::get_effect(size_t index) const {
    return index < effects_.size() ? effects_[index].get() : nullptr;
}

bool dsp_manager::process_chain(audio_chunk& chunk, abort_callback& abort) {
    if(effects_.empty() || abort.is_aborting()) {
        return true;
    }
    
    // 璁板綍澶勭悊寮€濮?
    if(performance_monitor_) {
        performance_monitor_->record_processing_start();
    }
    
    bool success = true;
    
    try {
        if(use_multithreading_ && effects_.size() > 1) {
            success = process_chain_multithread(chunk, abort);
        } else {
            success = process_chain_singlethread(chunk, abort);
        }
    } catch(const std::exception& e) {
        std::cerr << "[DSPManager] 澶勭悊閾惧紓甯? " << e.what() << std::endl;
        if(performance_monitor_) {
            performance_monitor_->record_error("processing_exception");
        }
        success = false;
    }
    
    // 璁板綍澶勭悊缁撴潫
    if(performance_monitor_) {
        performance_monitor_->record_processing_end(chunk.get_sample_count() * chunk.get_channels());
    }
    
    return success;
}

bool dsp_manager::process_chain_singlethread(audio_chunk& chunk, abort_callback& abort) {
    // 鍗曠嚎绋嬪鐞?
    for(auto& effect : effects_) {
        if(!effect || effect->is_bypassed()) {
            continue;
        }
        
        if(abort.is_aborting()) {
            return false;
        }
        
        // 瀹炰緥鍖栨晥鏋滃櫒锛堝鏋滈渶瑕侊級
        if(!effect->instantiate(chunk, chunk.get_sample_rate(), chunk.get_channels())) {
            std::cerr << "[DSPManager] 鏁堟灉鍣ㄥ疄渚嬪寲澶辫触: " << effect->get_name() << std::endl;
            continue;
        }
        
        // 澶勭悊闊抽
        effect->run(chunk, abort);
        
        if(abort.is_aborting()) {
            return false;
        }
    }
    
    return true;
}

bool dsp_manager::process_chain_multithread(audio_chunk& chunk, abort_callback& abort) {
    // 澶氱嚎绋嬪鐞嗭紙绠€鍖栧疄鐜帮級
    // 瀹為檯瀹炵幇闇€瑕佹洿澶嶆潅鐨勪换鍔″垎鍓插拰鍚屾
    
    std::vector<std::unique_ptr<dsp_task>> tasks;
    
    // 涓烘瘡涓晥鏋滃櫒鍒涘缓浠诲姟
    for(auto& effect : effects_) {
        if(!effect || effect->is_bypassed()) {
            continue;
        }
        
        // 杩欓噷绠€鍖栧鐞嗭細姣忎釜鏁堟灉鍣ㄤ綔涓轰竴涓嫭绔嬩换鍔?
        // 瀹為檯瀹炵幇鍙兘闇€瑕佸垎鍓查煶棰戞暟鎹?
        auto task = std::make_unique<dsp_task>(&chunk, &chunk, nullptr, &abort);
        tasks.push_back(std::move(task));
    }
    
    // 鎻愪氦浠诲姟鍒扮嚎绋嬫睜
    for(auto& task : tasks) {
        thread_pool_->submit_task(std::move(task));
    }
    
    // 绛夊緟鎵€鏈変换鍔″畬鎴?
    thread_pool_->wait_for_completion();
    
    return true;
}

// 鏍囧噯鏁堟灉鍣ㄥ伐鍘?
std::unique_ptr<dsp_effect_advanced> dsp_manager::create_equalizer_10band() {
    auto params = create_default_eq_params();
    params.type = dsp_effect_type::equalizer;
    params.name = "10-Band Equalizer";
    params.description = "Professional 10-band parametric equalizer";
    
    auto eq = std::make_unique<dsp_equalizer_10band>(params);
    eq->load_iso_preset();
    
    std::cout << "[DSPManager] 鍒涘缓10娈靛潎琛″櫒" << std::endl;
    return eq;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_equalizer_31band() {
    auto params = create_default_eq_params();
    params.type = dsp_effect_type::equalizer;
    params.name = "31-Band Equalizer";
    params.description = "Professional 31-band graphic equalizer";
    
    auto eq = std::make_unique<dsp_equalizer_advanced>(params);
    
    // 娣诲姞31涓狪SO棰戞
    const float iso_31_freqs[31] = {
        20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f, 125.0f, 160.0f,
        200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f, 800.0f, 1000.0f, 1250.0f, 1600.0f,
        2000.0f, 2500.0f, 3150.0f, 4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f
    };
    
    for(size_t i = 0; i < 31; ++i) {
        eq_band_params band_params;
        band_params.frequency = iso_31_freqs[i];
        band_params.gain = 0.0f;
        band_params.bandwidth = 1.0f;
        band_params.type = filter_type::peak;
        band_params.is_enabled = true;
        band_params.name = "ISO_" + std::to_string(i);
        
        eq->add_band(band_params);
    }
    
    std::cout << "[DSPManager] 鍒涘缓31娈靛潎琛″櫒" << std::endl;
    return eq;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_reverb() {
    auto params = create_default_reverb_params();
    params.type = dsp_effect_type::reverb;
    params.name = "Reverb";
    params.description = "Professional reverb effect";
    params.latency_ms = 10.0; // 10ms寤惰繜
    
    auto reverb = std::make_unique<dsp_reverb_advanced>(params);
    reverb->load_room_preset(0.5f); // 涓瓑鎴块棿
    
    std::cout << "[DSPManager] 鍒涘缓娣峰搷鏁堟灉鍣? << std::endl;
    return reverb;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_compressor() {
    auto params = create_default_compressor_params();
    params.type = dsp_effect_type::compressor;
    params.name = "Compressor";
    params.description = "Dynamic range compressor";
    
    // 杩欓噷搴旇瀹炵幇鍘嬬缉鍣ㄦ晥鏋滃櫒
    // 鐩墠杩斿洖涓€涓崰浣嶇
    auto compressor = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 鍒涘缓鍘嬬缉鍣ㄦ晥鏋滃櫒" << std::endl;
    return compressor;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_limiter() {
    auto params = create_default_limiter_params();
    params.type = dsp_effect_type::limiter;
    params.name = "Limiter";
    params.description = "Peak limiter for loudness control";
    
    // 杩欓噷搴旇瀹炵幇闄愬埗鍣ㄦ晥鏋滃櫒
    auto limiter = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 鍒涘缓闄愬埗鍣ㄦ晥鏋滃櫒" << std::endl;
    return limiter;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_volume_control() {
    auto params = create_default_volume_params();
    params.type = dsp_effect_type::volume;
    params.name = "Volume Control";
    params.description = "Advanced volume control with limiting";
    
    // 杩欓噷搴旇瀹炵幇楂樼骇闊抽噺鎺у埗鏁堟灉鍣?
    auto volume = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 鍒涘缓闊抽噺鎺у埗鏁堟灉鍣? << std::endl;
    return volume;
}

// 楂樼骇鏁堟灉鍣?
std::unique_ptr<dsp_effect_advanced> dsp_manager::create_convolver() {
    auto params = create_default_convolver_params();
    params.type = dsp_effect_type::convolver;
    params.name = "Convolver";
    params.description = "Impulse response convolver";
    params.latency_ms = 50.0; // 杈冮珮鐨勫欢杩?
    
    // 杩欓噷搴旇瀹炵幇鍗风Н鏁堟灉鍣?
    auto convolver = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 鍒涘缓鍗风Н鏁堟灉鍣? << std::endl;
    return convolver;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_crossfeed() {
    auto params = create_default_crossfeed_params();
    params.type = dsp_effect_type::crossfeed;
    params.name = "Crossfeed";
    params.description = "Headphone crossfeed for natural sound";
    
    // 杩欓噷搴旇瀹炵幇浜ゅ弶棣堥€佹晥鏋滃櫒
    auto crossfeed = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 鍒涘缓浜ゅ弶棣堥€佹晥鏋滃櫒" << std::endl;
    return crossfeed;
}

std::unique_ptr<dsp_effect_advanced> dsp_manager::create_resampler(uint32_t target_rate) {
    auto params = create_default_resampler_params();
    params.type = dsp_effect_type::resampler;
    params.name = "Resampler";
    params.description = "High-quality audio resampler";
    
    // 杩欓噷搴旇瀹炵幇閲嶉噰鏍锋晥鏋滃櫒
    auto resampler = std::make_unique<dsp_effect_advanced>(params);
    
    std::cout << "[DSPManager] 鍒涘缓閲嶉噰鏍锋晥鏋滃櫒锛岀洰鏍囬噰鏍风巼: " << target_rate << std::endl;
    return resampler;
}

// 棰勮绠＄悊
bool dsp_manager::save_effect_preset(size_t effect_index, const std::string& preset_name) {
    if(effect_index >= effects_.size() || !preset_manager_) {
        return false;
    }
    
    auto* effect = effects_[effect_index].get();
    if(!effect) {
        return false;
    }
    
    // 鍒涘缓棰勮
    auto preset = std::make_unique<dsp_preset_impl>(preset_name);
    effect->get_preset(*preset);
    
    // 淇濆瓨棰勮
    return preset_manager_->save_preset(preset_name, *preset);
}

bool dsp_manager::load_effect_preset(size_t effect_index, const std::string& preset_name) {
    if(effect_index >= effects_.size() || !preset_manager_) {
        return false;
    }
    
    auto* effect = effects_[effect_index].get();
    if(!effect) {
        return false;
    }
    
    // 鍔犺浇棰勮
    auto preset = std::make_unique<dsp_preset_impl>();
    if(!preset_manager_->load_preset(preset_name, *preset)) {
        return false;
    }
    
    // 搴旂敤棰勮
    effect->set_preset(*preset);
    return true;
}

std::vector<std::string> dsp_manager::get_available_presets() const {
    if(!preset_manager_) {
        return {};
    }
    return preset_manager_->get_preset_list();
}

// 鎬ц兘鐩戞帶
void dsp_manager::start_performance_monitoring() {
    if(performance_monitor_) {
        performance_monitor_->start_monitoring();
    }
}

void dsp_manager::stop_performance_monitoring() {
    if(performance_monitor_) {
        performance_monitor_->stop_monitoring();
    }
}

dsp_performance_stats dsp_manager::get_overall_stats() const {
    if(performance_monitor_) {
        return performance_monitor_->get_stats();
    }
    return dsp_performance_stats();
}

std::vector<dsp_performance_stats> dsp_manager::get_all_effect_stats() const {
    std::vector<dsp_performance_stats> stats;
    stats.reserve(effects_.size());
    
    for(const auto& effect : effects_) {
        if(effect) {
            stats.push_back(effect->get_performance_stats());
        }
    }
    
    return stats;
}

// 閰嶇疆绠＄悊
void dsp_manager::update_config(const dsp_config& config) {
    std::cout << "[DSPManager] 鏇存柊閰嶇疆" << std::endl;
    config_ = config;
    
    // 鏍规嵁鏂伴厤缃皟鏁寸郴缁?
    if(config.enable_multithreading && !use_multithreading_) {
        // 鍚姩澶氱嚎绋?
        thread_pool_ = std::make_unique<multithreaded_dsp_processor>(config.max_threads);
        thread_pool_->start();
        use_multithreading_ = true;
    } else if(!config.enable_multithreading && use_multithreading_) {
        // 鍋滄澶氱嚎绋?
        thread_pool_->stop();
        use_multithreading_ = false;
    }
}

// 瀹炵敤宸ュ叿
float dsp_manager::estimate_total_cpu_usage() const {
    float total_usage = 0.0f;
    
    for(const auto& effect : effects_) {
        if(effect) {
            total_usage += effect->get_cpu_usage();
        }
    }
    
    return total_usage;
}

double dsp_manager::estimate_total_latency() const {
    double total_latency = 0.0;
    
    for(const auto& effect : effects_) {
        if(effect) {
            total_latency += effect->get_params().latency_ms;
        }
    }
    
    return total_latency;
}

bool dsp_manager::validate_dsp_chain() const {
    // 妫€鏌ユ晥鏋滃櫒閾剧殑鏈夋晥鎬?
    for(const auto& effect : effects_) {
        if(!effect || !effect->is_valid()) {
            return false;
        }
    }
    
    // 妫€鏌ユ槸鍚︽湁閲嶅鐨勬晥鏋滃櫒绫诲瀷
    std::set<dsp_effect_type> seen_types;
    for(const auto& effect : effects_) {
        if(effect) {
            if(seen_types.count(effect->get_params().type) > 0) {
                // 鏌愪簺鏁堟灉鍣ㄧ被鍨嬩笉搴旇閲嶅
                if(effect->get_params().type == dsp_effect_type::limiter) {
                    return false;
                }
            }
            seen_types.insert(effect->get_params().type);
        }
    }
    
    return true;
}

std::string dsp_manager::generate_dsp_report() const {
    std::stringstream report;
    report << "DSP绯荤粺鎶ュ憡:\n";
    report << "===================\n\n";
    
    // 绯荤粺淇℃伅
    report << "绯荤粺閰嶇疆:\n";
    report << "  鏁堟灉鍣ㄦ暟閲? " << effects_.size() << "\n";
    report << "  澶氱嚎绋? " << (use_multithreading_ ? "鍚敤" : "绂佺敤") << "\n";
    report << "  鎬ц兘鐩戞帶: " << (performance_monitor_ ? "鍚敤" : "绂佺敤") << "\n";
    report << "  鍐呭瓨姹? " << (config_.memory_pool_size / (1024*1024)) << "MB\n\n";
    
    // 鏁堟灉鍣ㄥ垪琛?
    report << "鏁堟灉鍣ㄥ垪琛?\n";
    for(size_t i = 0; i < effects_.size(); ++i) {
        const auto* effect = effects_[i].get();
        if(effect) {
            report << "  [" << i << "] " << effect->get_name() << "\n";
            report << "      绫诲瀷: " << static_cast<int>(effect->get_params().type) << "\n";
            report << "      寤惰繜: " << effect->get_params().latency_ms << "ms\n";
            report << "      CPU鍗犵敤: " << effect->get_cpu_usage() << "%\n";
            report << "      鐘舵€? " << (effect->is_enabled() ? "鍚敤" : "绂佺敤") 
                   << (effect->is_bypassed() ? " (鏃佽矾)" : "") << "\n\n";
        }
    }
    
    // 鎬ц兘缁熻
    if(performance_monitor_ && performance_monitor_->is_monitoring()) {
        auto stats = performance_monitor_->get_stats();
        report << "鎬ц兘缁熻:\n";
        report << "  鎬婚噰鏍锋暟: " << stats.total_samples_processed << "\n";
        report << "  鎬诲鐞嗘椂闂? " << stats.total_processing_time_ms << "ms\n";
        report << "  骞冲潎澶勭悊鏃堕棿: " << stats.average_time_ms << "ms\n";
        report << "  CPU浣跨敤鐜? " << stats.cpu_usage_percent << "%\n";
        report << "  瀹炴椂鍊嶆暟: " << performance_monitor_->get_realtime_factor() << "x\n";
    }
    
    // 鎬ц兘寤鸿
    auto warnings = get_performance_warnings();
    if(!warnings.empty()) {
        report << "\n鎬ц兘璀﹀憡:\n";
        for(const auto& warning : warnings) {
            report << "  - " << warning << "\n";
        }
    }
    
    return report.str();
}

// 鏍囧噯DSP鏁堟灉鍣ㄩ摼
std::unique_ptr<dsp_chain> dsp_manager::create_standard_dsp_chain() {
    auto chain = std::make_unique<dsp_chain>();
    
    // 娣诲姞鏍囧噯鏁堟灉鍣?
    chain->add_effect(service_ptr_t<dsp>(create_equalizer_10band()));
    chain->add_effect(service_ptr_t<dsp>(create_volume_control()));
    
    std::cout << "[DSPManager] 鍒涘缓鏍囧噯DSP閾? << std::endl;
    return chain;
}

// 鍔犺浇鏍囧噯鏁堟灉鍣?
void dsp_manager::load_standard_effects() {
    std::cout << "[DSPManager] 鍔犺浇鏍囧噯鏁堟灉鍣?.." << std::endl;
    
    // 娣诲姞鍩虹鏁堟灉鍣?
    add_effect(create_equalizer_10band());
    add_effect(create_reverb());
    add_effect(create_volume_control());
    
    std::cout << "[DSPManager] 鏍囧噯鏁堟灉鍣ㄥ姞杞藉畬鎴愶紙鏁伴噺: " << effects_.size() << ")" << std::endl;
}

// 杈呭姪鍑芥暟
dsp_effect_params dsp_manager::create_default_eq_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::equalizer;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 5.0f; // 浼扮畻5% CPU鍗犵敤
    params.latency_ms = 0.1; // 0.1ms寤惰繜
    
    // 閰嶇疆鍙傛暟
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"gain", "Overall Gain", 0.0f, -24.0f, 24.0f, 0.1f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_reverb_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::reverb;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 15.0f; // 浼扮畻15% CPU鍗犵敤
    params.latency_ms = 10.0; // 10ms寤惰繜
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"room_size", "Room Size", 0.5f, 0.0f, 1.0f, 0.01f},
        {"damping", "Damping", 0.5f, 0.0f, 1.0f, 0.01f},
        {"wet_level", "Wet Level", 0.3f, 0.0f, 1.0f, 0.01f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_compressor_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::compressor;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 8.0f;
    params.latency_ms = 2.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"threshold", "Threshold", -20.0f, -60.0f, 0.0f, 1.0f},
        {"ratio", "Ratio", 4.0f, 1.0f, 20.0f, 0.1f},
        {"attack", "Attack", 10.0f, 0.1f, 100.0f, 0.1f},
        {"release", "Release", 100.0f, 10.0f, 1000.0f, 1.0f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_limiter_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::limiter;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 10.0f;
    params.latency_ms = 5.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"threshold", "Threshold", -3.0f, -20.0f, 0.0f, 0.1f},
        {"release", "Release", 50.0f, 1.0f, 500.0f, 1.0f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_volume_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::volume;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 1.0f;
    params.latency_ms = 0.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"volume", "Volume", 0.0f, -60.0f, 12.0f, 0.1f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_convolver_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::convolver;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 30.0f;
    params.latency_ms = 50.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"impulse_response", "Impulse Response", 0.0f, 0.0f, 1.0f, 1.0f},
        {"mix", "Mix Level", 0.5f, 0.0f, 1.0f, 0.01f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_crossfeed_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::crossfeed;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 3.0f;
    params.latency_ms = 1.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"intensity", "Crossfeed Intensity", 0.5f, 0.0f, 1.0f, 0.01f},
        {"frequency", "Crossfeed Frequency", 700.0f, 100.0f, 2000.0f, 10.0f}
    };
    
    return params;
}

dsp_effect_params dsp_manager::create_default_resampler_params() {
    dsp_effect_params params;
    params.type = dsp_effect_type::resampler;
    params.is_enabled = true;
    params.is_bypassed = false;
    params.cpu_usage_estimate = 20.0f;
    params.latency_ms = 20.0;
    
    params.config_params = {
        {"bypass", "Bypass", 0.0f, 0.0f, 1.0f, 1.0f},
        {"target_rate", "Target Sample Rate", 48000.0f, 8000.0f, 192000.0f, 100.0f},
        {"quality", "Resampling Quality", 0.8f, 0.0f, 1.0f, 0.01f}
    };
    
    return params;
}

std::vector<std::string> dsp_manager::get_performance_warnings() const {
    std::vector<std::string> warnings;
    
    if(!performance_monitor_) {
        return warnings;
    }
    
    auto stats = performance_monitor_->get_stats();
    
    // CPU浣跨敤鐜囪繃楂?
    if(stats.cpu_usage_percent > config_.target_cpu_usage * 1.5f) {
        warnings.push_back("CPU浣跨敤鐜囪繃楂橈紝鑰冭檻鍑忓皯鏁堟灉鍣ㄦ垨浼樺寲绠楁硶");
    }
    
    // 寤惰繜杩囬珮
    if(estimate_total_latency() > config_.max_latency_ms * 1.5f) {
        warnings.push_back("鎬诲欢杩熻繃楂橈紝鑰冭檻鍑忓皯鏁堟灉鍣ㄦ暟閲?);
    }
    
    // 鍐呭瓨浣跨敤杩囬珮
    // 杩欓噷鍙互娣诲姞鍐呭瓨浣跨敤妫€鏌?
    
    // 閿欒鐜囪繃楂?
    if(stats.error_count > 0) {
        warnings.push_back("澶勭悊杩囩▼涓嚭鐜伴敊璇紝寤鸿妫€鏌ユ晥鏋滃櫒閰嶇疆");
    }
    
    return warnings;
}

} // namespace fb2k