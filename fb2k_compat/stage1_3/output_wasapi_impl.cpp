#include "output_wasapi.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <sstream>

#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")

namespace fb2k {

// WASAPI杈撳嚭璁惧瀹炵幇
class output_wasapi_impl : public output_wasapi {
public:
    output_wasapi_impl();
    ~output_wasapi_impl();

    // output鎺ュ彛瀹炵幇
    void open(t_uint32 sample_rate, t_uint32 channels, t_uint32 flags, abort_callback& p_abort) override;
    void close(abort_callback& p_abort) override;
    t_uint32 get_latency() override;
    void write(const void* buffer, t_size bytes, abort_callback& p_abort) override;
    void pause(bool state) override;
    void flush(abort_callback& p_abort) override;
    void volume_set(float volume) override;
    bool is_playing() override;
    bool can_write() override;
    bool requires_spec_ex() override;
    t_uint32 get_latency_ex() override;
    void get_device_name(pfc::string_base& out) override;
    void get_device_desc(pfc::string_base& out) override;
    t_uint32 get_device_id() override;
    void estimate_latency(double& latency_seconds, t_uint32 sample_rate, t_uint32 channels) override;
    void update_device_list() override;
    bool is_realtime() override;
    void on_idle() override;
    void process_samples(const audio_chunk* p_chunk, t_uint32 p_samples_written, t_uint32 p_samples_total, abort_callback& p_abort) override;
    void pause_ex(bool p_state, t_uint32 p_samples_written) override;
    void set_volume_ex(float p_volume, t_uint32 p_samples_written) override;
    void get_latency_ex2(t_uint32& p_samples, t_uint32& p_samples_total) override;
    void get_latency_ex3(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer) override;
    void get_latency_ex4(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer, t_uint32& p_samples_in_device_buffer) override;

    // output_wasapi鎺ュ彛瀹炵幇
    void set_device(const char* device_id) override;
    void set_exclusive_mode(bool exclusive) override;
    void set_event_driven(bool event_driven) override;
    void set_buffer_duration(t_uint32 milliseconds) override;
    void set_device_role(ERole role) override;
    void set_stream_category(EAudioSessionCategory category) override;
    void set_stream_option(t_uint32 option) override;
    bool is_exclusive_mode() const override;
    bool is_event_driven() const override;
    t_uint32 get_buffer_duration() const override;
    ERole get_device_role() const override;
    EAudioSessionCategory get_stream_category() const override;
    t_uint32 get_stream_option() const override;
    void get_current_device(pfc::string_base& out) override;
    void enumerate_devices(pfc::string_list_impl& out) override;
    void get_device_info(const char* device_id, device_info& info) override;

private:
    // WASAPI鎺ュ彛
    IMMDeviceEnumerator* device_enumerator_;
    IMMDevice* audio_device_;
    IAudioClient* audio_client_;
    IAudioRenderClient* render_client_;
    IAudioClock* audio_clock_;
    IAudioStreamVolume* stream_volume_;
    
    // 闊抽鏍煎紡
    WAVEFORMATEXTENSIBLE format_;
    t_uint32 sample_rate_;
    t_uint32 channels_;
    t_uint32 bits_per_sample_;
    t_uint32 bytes_per_sample_;
    t_uint32 buffer_size_;
    
    // 鐘舵€佺鐞?
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<bool> is_initialized_;
    std::atomic<bool> is_exclusive_mode_;
    std::atomic<bool> is_event_driven_;
    std::atomic<bool> should_stop_;
    
    // 缂撳啿绠＄悊
    std::vector<uint8_t> output_buffer_;
    t_uint32 buffer_frame_count_;
    t_uint32 actual_buffer_duration_ms_;
    
    // 绾跨▼绠＄悊
    std::thread render_thread_;
    std::atomic<bool> render_thread_running_;
    std::condition_variable render_cv_;
    std::mutex render_mutex_;
    
    // 闊抽噺鎺у埗
    std::atomic<float> volume_;
    float last_set_volume_;
    
    // 鎬ц兘鐩戞帶
    std::chrono::steady_clock::time_point start_time_;
    t_uint64 total_samples_written_;
    t_uint64 total_samples_played_;
    
    // 閰嶇疆鍙傛暟
    std::string device_id_;
    ERole device_role_;
    EAudioSessionCategory stream_category_;
    t_uint32 requested_buffer_duration_ms_;
    t_uint32 stream_options_;
    
    // 璁惧淇℃伅
    device_info current_device_info_;
    
    // 绉佹湁鏂规硶
    HRESULT initialize_wasapi();
    HRESULT create_audio_client();
    HRESULT get_mix_format();
    HRESULT initialize_audio_client();
    HRESULT get_render_client();
    HRESULT start_streaming();
    HRESULT stop_streaming();
    HRESULT cleanup_wasapi();
    HRESULT get_device_list();
    HRESULT select_device(const std::string& device_id);
    
    void render_thread_func();
    HRESULT render_audio_data();
    HRESULT write_to_device(const void* data, t_uint32 frames);
    t_uint32 get_padding_frames();
    t_uint32 get_available_frames();
    
    HRESULT get_current_position(t_uint64& position, t_uint64& qpc_position);
    void update_latency_stats();
    
    std::string get_device_friendly_name(IMMDevice* device);
    std::string get_device_id(IMMDevice* device);
    device_info get_device_info(IMMDevice* device);
    
    bool validate_format(const WAVEFORMATEX* format);
    HRESULT convert_to_exclusive_format(const WAVEFORMATEX* shared_format, WAVEFORMATEXTENSIBLE& exclusive_format);
    
    void handle_error(const std::string& operation, HRESULT hr);
    void log_device_info();
};

// 鏋勯€犲嚱鏁?
output_wasapi_impl::output_wasapi_impl()
    : device_enumerator_(nullptr),
      audio_device_(nullptr),
      audio_client_(nullptr),
      render_client_(nullptr),
      audio_clock_(nullptr),
      stream_volume_(nullptr),
      sample_rate_(0),
      channels_(0),
      bits_per_sample_(0),
      bytes_per_sample_(0),
      buffer_size_(0),
      is_playing_(false),
      is_paused_(false),
      is_initialized_(false),
      is_exclusive_mode_(false),
      is_event_driven_(true),
      should_stop_(false),
      buffer_frame_count_(0),
      actual_buffer_duration_ms_(0),
      render_thread_running_(false),
      volume_(1.0f),
      last_set_volume_(1.0f),
      device_role_(eConsole),
      stream_category_(AudioCategory_Media),
      requested_buffer_duration_ms_(50),
      stream_options_(0),
      total_samples_written_(0),
      total_samples_played_(0) {
    
    // 鍒濆鍖朩ASAPI
    HRESULT hr = initialize_wasapi();
    if(FAILED(hr)) {
        std::cerr << "[WASAPI] 鍒濆鍖栧け璐? 0x" << std::hex << hr << std::dec << std::endl;
    }
}

// 鏋愭瀯鍑芥暟
output_wasapi_impl::~output_wasapi_impl() {
    std::cout << "[WASAPI] 閿€姣佽緭鍑鸿澶?.." << std::endl;
    
    // 纭繚鍋滄鎾斁
    if(is_playing_.load()) {
        abort_callback_dummy abort;
        close(abort);
    }
    
    cleanup_wasapi();
}

// 鍒濆鍖朩ASAPI
HRESULT output_wasapi_impl::initialize_wasapi() {
    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&device_enumerator_
    );
    
    if(FAILED(hr)) {
        handle_error("CoCreateInstance(MMDeviceEnumerator)", hr);
        return hr;
    }
    
    std::cout << "[WASAPI] 璁惧鏋氫妇鍣ㄥ垱寤烘垚鍔? << std::endl;
    return S_OK;
}

// 鎵撳紑杈撳嚭璁惧
void output_wasapi_impl::open(t_uint32 sample_rate, t_uint32 channels, t_uint32 flags, abort_callback& p_abort) {
    std::cout << "[WASAPI] 鎵撳紑杈撳嚭璁惧 - " << sample_rate << "Hz, " << channels << "ch" << std::endl;
    
    if(is_initialized_.load()) {
        std::cerr << "[WASAPI] 璁惧宸茬粡鎵撳紑" << std::endl;
        return;
    }
    
    p_abort.check();
    
    try {
        // 淇濆瓨闊抽鏍煎紡
        sample_rate_ = sample_rate;
        channels_ = channels;
        bits_per_sample_ = 32; // 浣跨敤32浣嶆诞鐐规暟
        bytes_per_sample_ = bits_per_sample_ / 8;
        
        // 璁剧疆闊抽鏍煎紡
        memset(&format_, 0, sizeof(format_));
        format_.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        format_.Format.nChannels = channels;
        format_.Format.nSamplesPerSec = sample_rate;
        format_.Format.wBitsPerSample = bits_per_sample_;
        format_.Format.nBlockAlign = channels * bytes_per_sample_;
        format_.Format.nAvgBytesPerSec = sample_rate * format_.Format.nBlockAlign;
        format_.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        
        format_.Samples.wValidBitsPerSample = bits_per_sample_;
        format_.dwChannelMask = channels == 2 ? SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT : SPEAKER_FRONT_CENTER;
        format_.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        
        // 鍒涘缓闊抽瀹㈡埛绔?
        HRESULT hr = create_audio_client();
        if(FAILED(hr)) {
            throw std::runtime_error("鍒涘缓闊抽瀹㈡埛绔け璐?);
        }
        
        // 鍒濆鍖栭煶棰戝鎴风
        hr = initialize_audio_client();
        if(FAILED(hr)) {
            throw std::runtime_error("鍒濆鍖栭煶棰戝鎴风澶辫触");
        }
        
        // 鑾峰彇娓叉煋瀹㈡埛绔?
        hr = get_render_client();
        if(FAILED(hr)) {
            throw std::runtime_error("鑾峰彇娓叉煋瀹㈡埛绔け璐?);
        }
        
        // 鑾峰彇闊抽鏃堕挓
        hr = audio_client_->GetService(__uuidof(IAudioClock), (void**)&audio_clock_);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 鑾峰彇闊抽鏃堕挓澶辫触: 0x" << std::hex << hr << std::dec << std::endl;
            audio_clock_ = nullptr; // 闈炲叧閿粍浠?
        }
        
        // 鑾峰彇娴侀煶閲忔帶鍒?
        hr = audio_client_->GetService(__uuidof(IAudioStreamVolume), (void**)&stream_volume_);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 鑾峰彇娴侀煶閲忔帶鍒跺け璐? 0x" << std::hex << hr << std::dec << std::endl;
            stream_volume_ = nullptr; // 闈炲叧閿粍浠?
        }
        
        // 璁剧疆闊抽噺
        if(stream_volume_) {
            float* volumes = new float[channels_];
            std::fill(volumes, volumes + channels_, volume_.load());
            stream_volume_->SetAllVolumes(channels_, volumes);
            delete[] volumes;
        }
        
        is_initialized_.store(true);
        std::cout << "[WASAPI] 杈撳嚭璁惧鎵撳紑鎴愬姛" << std::endl;
        log_device_info();
        
    } catch(const std::exception& e) {
        std::cerr << "[WASAPI] 鎵撳紑璁惧澶辫触: " << e.what() << std::endl;
        cleanup_wasapi();
        throw;
    }
}

// 鍏抽棴杈撳嚭璁惧
void output_wasapi_impl::close(abort_callback& p_abort) {
    std::cout << "[WASAPI] 鍏抽棴杈撳嚭璁惧" << std::endl;
    
    if(!is_initialized_.load()) {
        return;
    }
    
    should_stop_.store(true);
    
    // 鍋滄娓叉煋绾跨▼
    if(render_thread_running_.load()) {
        render_cv_.notify_all();
        if(render_thread_.joinable()) {
            render_thread_.join();
        }
        render_thread_running_.store(false);
    }
    
    // 鍋滄闊抽娴?
    stop_streaming();
    
    // 娓呯悊璧勬簮
    cleanup_wasapi();
    
    is_initialized_.store(false);
    is_playing_.store(false);
    is_paused_.store(false);
    
    std::cout << "[WASAPI] 杈撳嚭璁惧宸插叧闂? << std::endl;
}

// 鑾峰彇寤惰繜
t_uint32 output_wasapi_impl::get_latency() {
    return static_cast<t_uint32>(actual_buffer_duration_ms_);
}

// 鍐欏叆闊抽鏁版嵁
void output_wasapi_impl::write(const void* buffer, t_size bytes, abort_callback& p_abort) {
    if(!is_initialized_.load() || !is_playing_.load()) {
        return;
    }
    
    p_abort.check();
    
    const uint8_t* data = static_cast<const uint8_t*>(buffer);
    t_size bytes_written = 0;
    
    while(bytes_written < bytes && !p_abort.is_aborting()) {
        // 妫€鏌ユ槸鍚︽湁瓒冲鐨勭┖闂?
        t_uint32 available_frames = get_available_frames();
        if(available_frames == 0) {
            // 绛夊緟绌洪棿鍙敤
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // 璁＄畻鍙互鍐欏叆鐨勫抚鏁?
        t_uint32 frames_to_write = std::min(available_frames, 
                                           static_cast<t_uint32>(bytes / format_.Format.nBlockAlign));
        if(frames_to_write == 0) {
            continue;
        }
        
        // 鍐欏叆鏁版嵁
        t_size bytes_to_write = frames_to_write * format_.Format.nBlockAlign;
        if(bytes_to_write > bytes - bytes_written) {
            bytes_to_write = bytes - bytes_written;
            frames_to_write = bytes_to_write / format_.Format.nBlockAlign;
        }
        
        HRESULT hr = write_to_device(data + bytes_written, frames_to_write);
        if(FAILED(hr)) {
            handle_error("鍐欏叆闊抽鏁版嵁", hr);
            break;
        }
        
        bytes_written += bytes_to_write;
        total_samples_written_ += frames_to_write * channels_;
    }
}

// 鏆傚仠/鎭㈠
void output_wasapi_impl::pause(bool state) {
    if(!is_initialized_.load() || !is_playing_.load()) {
        return;
    }
    
    is_paused_.store(state);
    
    if(audio_client_) {
        HRESULT hr;
        if(state) {
            hr = audio_client_->Stop();
            std::cout << "[WASAPI] 鏆傚仠闊抽娴? << std::endl;
        } else {
            hr = audio_client_->Start();
            std::cout << "[WASAPI] 鎭㈠闊抽娴? << std::endl;
        }
        
        if(FAILED(hr)) {
            handle_error("鏆傚仠/鎭㈠闊抽娴?, hr);
        }
    }
}

// 娓呯┖缂撳啿鍖?
void output_wasapi_impl::flush(abort_callback& p_abort) {
    std::cout << "[WASAPI] 娓呯┖缂撳啿鍖? << std::endl;
    
    if(!is_initialized_.load() || !audio_client_) {
        return;
    }
    
    p_abort.check();
    
    // 閲嶇疆闊抽瀹㈡埛绔?
    HRESULT hr = audio_client_->Reset();
    if(FAILED(hr)) {
        handle_error("閲嶇疆闊抽瀹㈡埛绔?, hr);
    }
}

// 璁剧疆闊抽噺
void output_wasapi_impl::volume_set(float volume) {
    volume_.store(volume);
    last_set_volume_ = volume;
    
    if(stream_volume_ && channels_ > 0) {
        float* volumes = new float[channels_];
        std::fill(volumes, volumes + channels_, volume);
        
        HRESULT hr = stream_volume_->SetAllVolumes(channels_, volumes);
        if(FAILED(hr)) {
            handle_error("璁剧疆闊抽噺", hr);
        }
        
        delete[] volumes;
    }
}

// 妫€鏌ユ槸鍚︽鍦ㄦ挱鏀?
bool output_wasapi_impl::is_playing() {
    return is_playing_.load() && !is_paused_.load();
}

// 妫€鏌ユ槸鍚﹀彲浠ュ啓鍏?
bool output_wasapi_impl::can_write() {
    if(!is_initialized_.load() || !is_playing_.load()) {
        return false;
    }
    
    return get_available_frames() > 0;
}

// 鍒涘缓闊抽瀹㈡埛绔?
HRESULT output_wasapi_impl::create_audio_client() {
    HRESULT hr = S_OK;
    
    // 濡傛灉宸查€夋嫨鐗瑰畾璁惧锛屼娇鐢ㄥ畠锛涘惁鍒欎娇鐢ㄩ粯璁よ澶?
    if(!device_id_.empty()) {
        hr = select_device(device_id_);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 閫夋嫨鎸囧畾璁惧澶辫触锛屽皾璇曢粯璁よ澶? << std::endl;
        }
    }
    
    if(FAILED(hr) || device_id_.empty()) {
        // 浣跨敤榛樿璁惧
        hr = device_enumerator_->GetDefaultAudioEndpoint(eRender, device_role_, &audio_device_);
        if(FAILED(hr)) {
            handle_error("鑾峰彇榛樿闊抽绔偣", hr);
            return hr;
        }
    }
    
    // 鑾峰彇闊抽瀹㈡埛绔?
    hr = audio_device_->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&audio_client_
    );
    
    if(FAILED(hr)) {
        handle_error("婵€娲婚煶棰戝鎴风", hr);
        return hr;
    }
    
    std::cout << "[WASAPI] 闊抽瀹㈡埛绔垱寤烘垚鍔? << std::endl;
    return S_OK;
}

// 鑾峰彇娣峰悎鏍煎紡
HRESULT output_wasapi_impl::get_mix_format() {
    WAVEFORMATEX* device_format = nullptr;
    HRESULT hr = audio_client_->GetMixFormat(&device_format);
    
    if(FAILED(hr)) {
        handle_error("鑾峰彇娣峰悎鏍煎紡", hr);
        return hr;
    }
    
    std::cout << "[WASAPI] 璁惧娣峰悎鏍煎紡:" << std::endl;
    std::cout << "  - 閲囨牱鐜? " << device_format->nSamplesPerSec << "Hz" << std::endl;
    std::cout << "  - 澹伴亾鏁? " << device_format->nChannels << std::endl;
    std::cout << "  - 浣嶆繁搴? " << device_format->wBitsPerSample << "bit" << std::endl;
    
    CoTaskMemFree(device_format);
    return S_OK;
}

// 鍒濆鍖栭煶棰戝鎴风
HRESULT output_wasapi_impl::initialize_audio_client() {
    HRESULT hr;
    
    if(is_exclusive_mode_.load()) {
        // 鐙崰妯″紡
        WAVEFORMATEXTENSIBLE exclusive_format;
        hr = convert_to_exclusive_format(&format_.Format, exclusive_format);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 杞崲鍒扮嫭鍗犳牸寮忓け璐ワ紝灏濊瘯鍏变韩妯″紡" << std::endl;
            is_exclusive_mode_.store(false);
        } else {
            hr = audio_client_->Initialize(
                AUDCLNT_SHAREMODE_EXCLUSIVE,
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                requested_buffer_duration_ms_ * 10000, // 100绾崇鍗曚綅
                0,
                (WAVEFORMATEX*)&exclusive_format,
                nullptr
            );
            
            if(FAILED(hr)) {
                std::cerr << "[WASAPI] 鐙崰妯″紡鍒濆鍖栧け璐ワ紝灏濊瘯鍏变韩妯″紡" << std::endl;
                is_exclusive_mode_.store(false);
            } else {
                std::cout << "[WASAPI] 浣跨敤鐙崰妯″紡" << std::endl;
            }
        }
    }
    
    if(!is_exclusive_mode_.load()) {
        // 鍏变韩妯″紡
        REFERENCE_TIME buffer_duration = requested_buffer_duration_ms_ * 10000; // 100绾崇鍗曚綅
        
        hr = audio_client_->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            is_event_driven_.load() ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0,
            buffer_duration,
            0,
            (WAVEFORMATEX*)&format_,
            nullptr
        );
        
        if(FAILED(hr)) {
            handle_error("鍒濆鍖栭煶棰戝鎴风锛堝叡浜ā寮忥級", hr);
            return hr;
        }
        
        std::cout << "[WASAPI] 浣跨敤鍏变韩妯″紡" << std::endl;
    }
    
    // 鑾峰彇瀹為檯缂撳啿鍖哄ぇ灏?
    hr = audio_client_->GetBufferSize(&buffer_frame_count_);
    if(FAILED(hr)) {
        handle_error("鑾峰彇缂撳啿鍖哄ぇ灏?, hr);
        return hr;
    }
    
    actual_buffer_duration_ms_ = (buffer_frame_count_ * 1000) / sample_rate_;
    
    std::cout << "[WASAPI] 缂撳啿鍖鸿缃?" << std::endl;
    std::cout << "  - 甯ф暟: " << buffer_frame_count_ << std::endl;
    std::cout << "  - 鎸佺画鏃堕棿: " << actual_buffer_duration_ms_ << "ms" << std::endl;
    
    return S_OK;
}

// 鑾峰彇娓叉煋瀹㈡埛绔?
HRESULT output_wasapi_impl::get_render_client() {
    HRESULT hr = audio_client_->GetService(
        __uuidof(IAudioRenderClient),
        (void**)&render_client_
    );
    
    if(FAILED(hr)) {
        handle_error("鑾峰彇娓叉煋瀹㈡埛绔?, hr);
        return hr;
    }
    
    std::cout << "[WASAPI] 娓叉煋瀹㈡埛绔幏鍙栨垚鍔? << std::endl;
    return S_OK;
}

// 寮€濮嬫祦寮忎紶杈?
HRESULT output_wasapi_impl::start_streaming() {
    if(!audio_client_) {
        return E_FAIL;
    }
    
    // 濡傛灉浜嬩欢椹卞姩锛屽垱寤轰簨浠?
    HANDLE event_handle = nullptr;
    if(is_event_driven_.load()) {
        event_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if(!event_handle) {
            return E_FAIL;
        }
        
        HRESULT hr = audio_client_->SetEventHandle(event_handle);
        if(FAILED(hr)) {
            CloseHandle(event_handle);
            return hr;
        }
    }
    
    // 寮€濮嬮煶棰戞祦
    HRESULT hr = audio_client_->Start();
    if(FAILED(hr)) {
        if(event_handle) CloseHandle(event_handle);
        handle_error("寮€濮嬮煶棰戞祦", hr);
        return hr;
    }
    
    is_playing_.store(true);
    start_time_ = std::chrono::steady_clock::now();
    
    // 鍚姩娓叉煋绾跨▼锛堝鏋滈渶瑕侊級
    if(!is_event_driven_.load()) {
        render_thread_running_.store(true);
        render_thread_ = std::thread(&output_wasapi_impl::render_thread_func, this);
    }
    
    std::cout << "[WASAPI] 闊抽娴佸紑濮? << std::endl;
    return S_OK;
}

// 鍋滄娴佸紡浼犺緭
HRESULT output_wasapi_impl::stop_streaming() {
    if(!audio_client_) {
        return S_OK;
    }
    
    is_playing_.store(false);
    
    // 鍋滄闊抽娴?
    HRESULT hr = audio_client_->Stop();
    if(FAILED(hr)) {
        handle_error("鍋滄闊抽娴?, hr);
    }
    
    // 閲嶇疆闊抽瀹㈡埛绔?
    hr = audio_client_->Reset();
    if(FAILED(hr)) {
        handle_error("閲嶇疆闊抽瀹㈡埛绔?, hr);
    }
    
    std::cout << "[WASAPI] 闊抽娴佸仠姝? << std::endl;
    return S_OK;
}

// 娓呯悊WASAPI
HRESULT output_wasapi_impl::cleanup_wasapi() {
    // 閲婃斁鎺ュ彛
    if(render_client_) {
        render_client_->Release();
        render_client_ = nullptr;
    }
    
    if(audio_clock_) {
        audio_clock_->Release();
        audio_clock_ = nullptr;
    }
    
    if(stream_volume_) {
        stream_volume_->Release();
        stream_volume_ = nullptr;
    }
    
    if(audio_client_) {
        audio_client_->Release();
        audio_client_ = nullptr;
    }
    
    if(audio_device_) {
        audio_device_->Release();
        audio_device_ = nullptr;
    }
    
    if(device_enumerator_) {
        device_enumerator_->Release();
        device_enumerator_ = nullptr;
    }
    
    return S_OK;
}

// 鍐欏叆璁惧
HRESULT output_wasapi_impl::write_to_device(const void* data, t_uint32 frames) {
    if(!render_client_) {
        return E_FAIL;
    }
    
    UINT32 buffer_frame_count = 0;
    BYTE* buffer_pointer = nullptr;
    HRESULT hr;
    
    // 鑾峰彇鍙敤缂撳啿鍖?
    hr = render_client_->GetBuffer(frames, &buffer_pointer);
    if(FAILED(hr)) {
        return hr;
    }
    
    if(!buffer_pointer) {
        return E_FAIL;
    }
    
    // 澶嶅埗鏁版嵁
    size_t bytes_to_copy = frames * format_.Format.nBlockAlign;
    memcpy(buffer_pointer, data, bytes_to_copy);
    
    // 閲婃斁缂撳啿鍖?
    hr = render_client_->ReleaseBuffer(frames, 0);
    if(FAILED(hr)) {
        handle_error("閲婃斁缂撳啿鍖?, hr);
        return hr;
    }
    
    return S_OK;
}

// 鑾峰彇鍙敤甯ф暟
t_uint32 output_wasapi_impl::get_available_frames() {
    if(!audio_client_) {
        return 0;
    }
    
    UINT32 padding_frames = 0;
    HRESULT hr = audio_client_->GetCurrentPadding(&padding_frames);
    if(FAILED(hr)) {
        return 0;
    }
    
    return buffer_frame_count_ - padding_frames;
}

// 鑾峰彇褰撳墠浣嶇疆
HRESULT output_wasapi_impl::get_current_position(t_uint64& position, t_uint64& qpc_position) {
    if(!audio_clock_) {
        return E_FAIL;
    }
    
    return audio_clock_->GetPosition(&position, &qpc_position);
}

// 娓叉煋绾跨▼鍑芥暟
void output_wasapi_impl::render_thread_func() {
    std::cout << "[WASAPI] 娓叉煋绾跨▼鍚姩" << std::endl;
    
    // 鎻愬崌绾跨▼浼樺厛绾?
    DWORD task_index = 0;
    HANDLE avrt_handle = AvSetMmThreadCharacteristics(L"Pro Audio", &task_index);
    if(avrt_handle) {
        std::cout << "[WASAPI] 娓叉煋绾跨▼浼樺厛绾у凡鎻愬崌" << std::endl;
    }
    
    while(render_thread_running_.load() && !should_stop_.load()) {
        // 妫€鏌ユ槸鍚﹂渶瑕佸仠姝?
        if(should_stop_.load()) {
            break;
        }
        
        // 娓叉煋闊抽鏁版嵁
        HRESULT hr = render_audio_data();
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 娓叉煋闊抽鏁版嵁澶辫触: 0x" << std::hex << hr << std::dec << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // 鏇存柊寤惰繜缁熻
        update_latency_stats();
        
        // 灏忓欢杩熶互閬垮厤CPU鍗犵敤杩囬珮
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // 鎭㈠绾跨▼浼樺厛绾?
    if(avrt_handle) {
        AvRevertMmThreadCharacteristics(avrt_handle);
    }
    
    std::cout << "[WASAPI] 娓叉煋绾跨▼鍋滄" << std::endl;
}

// 娓叉煋闊抽鏁版嵁
HRESULT output_wasapi_impl::render_audio_data() {
    if(!is_playing_.load() || is_paused_.load() || !audio_client_ || !render_client_) {
        return S_OK;
    }
    
    // 鑾峰彇鍙敤绌洪棿
    t_uint32 available_frames = get_available_frames();
    if(available_frames == 0) {
        return S_OK;
    }
    
    // 杩欓噷搴旇澶勭悊瀹為檯鐨勯煶棰戞暟鎹覆鏌?
    // 鐩墠鍙槸杩斿洖鎴愬姛
    
    return S_OK;
}

// 鏇存柊寤惰繜缁熻
void output_wasapi_impl::update_latency_stats() {
    if(!audio_clock_) {
        return;
    }
    
    t_uint64 position = 0;
    t_uint64 qpc_position = 0;
    HRESULT hr = get_current_position(position, qpc_position);
    
    if(SUCCEEDED(hr)) {
        total_samples_played_ = position;
    }
}

// 鑾峰彇璁惧鍙嬪ソ鍚嶇О
std::string output_wasapi_impl::get_device_friendly_name(IMMDevice* device) {
    if(!device) {
        return "Unknown Device";
    }
    
    IPropertyStore* property_store = nullptr;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, &property_store);
    if(FAILED(hr)) {
        return "Unknown Device";
    }
    
    PROPVARIANT prop_var;
    PropVariantInit(&prop_var);
    
    hr = property_store->GetValue(PKEY_Device_FriendlyName, &prop_var);
    if(SUCCEEDED(hr) && prop_var.vt == VT_LPWSTR) {
        std::wstring ws(prop_var.pwszVal);
        std::string result(ws.begin(), ws.end());
        PropVariantClear(&prop_var);
        property_store->Release();
        return result;
    }
    
    PropVariantClear(&prop_var);
    property_store->Release();
    return "Unknown Device";
}

// 鑾峰彇璁惧ID
std::string output_wasapi_impl::get_device_id(IMMDevice* device) {
    if(!device) {
        return "";
    }
    
    LPWSTR device_id = nullptr;
    HRESULT hr = device->GetId(&device_id);
    if(FAILED(hr)) {
        return "";
    }
    
    std::wstring ws(device_id);
    std::string result(ws.begin(), ws.end());
    
    CoTaskMemFree(device_id);
    return result;
}

// 鑾峰彇璁惧淇℃伅
device_info output_wasapi_impl::get_device_info(IMMDevice* device) {
    device_info info;
    
    if(!device) {
        return info;
    }
    
    info.id = get_device_id(device);
    info.name = get_device_friendly_name(device);
    
    // 鑾峰彇鍏朵粬璁惧灞炴€?
    IPropertyStore* property_store = nullptr;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, &property_store);
    if(SUCCEEDED(hr)) {
        PROPVARIANT prop_var;
        PropVariantInit(&prop_var);
        
        // 鑾峰彇璁惧鎻忚堪
        hr = property_store->GetValue(PKEY_Device_DeviceDesc, &prop_var);
        if(SUCCEEDED(hr) && prop_var.vt == VT_LPWSTR) {
            std::wstring ws(prop_var.pwszVal);
            info.description = std::string(ws.begin(), ws.end());
            PropVariantClear(&prop_var);
        }
        
        property_store->Release();
    }
    
    return info;
}

// 閫夋嫨璁惧
HRESULT output_wasapi_impl::select_device(const std::string& device_id) {
    if(!device_enumerator_ || device_id.empty()) {
        return E_FAIL;
    }
    
    // 灏嗚澶嘔D杞崲涓哄瀛楃涓?
    std::wstring ws_device_id(device_id.begin(), device_id.end());
    
    IMMDevice* device = nullptr;
    HRESULT hr = device_enumerator_->GetDevice(ws_device_id.c_str(), &device);
    if(FAILED(hr)) {
        handle_error("鑾峰彇鎸囧畾璁惧", hr);
        return hr;
    }
    
    // 閲婃斁鏃ц澶?
    if(audio_device_) {
        audio_device_->Release();
    }
    
    audio_device_ = device;
    current_device_info_ = get_device_info(device);
    
    std::cout << "[WASAPI] 閫夋嫨璁惧: " << current_device_info_.name << std::endl;
    return S_OK;
}

// 杞崲鍒扮嫭鍗犳牸寮?
HRESULT output_wasapi_impl::convert_to_exclusive_format(const WAVEFORMATEX* shared_format, 
                                                       WAVEFORMATEXTENSIBLE& exclusive_format) {
    // 绠€鍖栫殑鏍煎紡杞崲
    // 瀹為檯瀹炵幇闇€瑕佹洿澶嶆潅鐨勬牸寮忓尮閰嶉€昏緫
    
    memset(&exclusive_format, 0, sizeof(exclusive_format));
    
    exclusive_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    exclusive_format.Format.nChannels = shared_format->nChannels;
    exclusive_format.Format.nSamplesPerSec = shared_format->nSamplesPerSec;
    exclusive_format.Format.wBitsPerSample = shared_format->wBitsPerSample;
    exclusive_format.Format.nBlockAlign = shared_format->nBlockAlign;
    exclusive_format.Format.nAvgBytesPerSec = shared_format->nAvgBytesPerSec;
    exclusive_format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    
    exclusive_format.Samples.wValidBitsPerSample = shared_format->wBitsPerSample;
    
    switch(shared_format->nChannels) {
        case 1:
            exclusive_format.dwChannelMask = SPEAKER_FRONT_CENTER;
            break;
        case 2:
            exclusive_format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
            break;
        case 6:
            exclusive_format.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
            break;
        case 8:
            exclusive_format.dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
            break;
        default:
            exclusive_format.dwChannelMask = 0;
            break;
    }
    
    exclusive_format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    
    return S_OK;
}

// 閿欒澶勭悊
void output_wasapi_impl::handle_error(const std::string& operation, HRESULT hr) {
    std::stringstream ss;
    ss << "[WASAPI] 鎿嶄綔澶辫触: " << operation << " - ";
    
    switch(hr) {
        case AUDCLNT_E_NOT_INITIALIZED:
            ss << "闊抽瀹㈡埛绔湭鍒濆鍖?;
            break;
        case AUDCLNT_E_ALREADY_INITIALIZED:
            ss << "闊抽瀹㈡埛绔凡鍒濆鍖?;
            break;
        case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
            ss << "閿欒鐨勭鐐圭被鍨?;
            break;
        case AUDCLNT_E_DEVICE_INVALIDATED:
            ss << "璁惧鏃犳晥";
            break;
        case AUDCLNT_E_NOT_STOPPED:
            ss << "闊抽娴佹湭鍋滄";
            break;
        case AUDCLNT_E_BUFFER_TOO_LARGE:
            ss << "缂撳啿鍖鸿繃澶?;
            break;
        case AUDCLNT_E_OUT_OF_ORDER:
            ss << "鎿嶄綔椤哄簭閿欒";
            break;
        case AUDCLNT_E_UNSUPPORTED_FORMAT:
            ss << "涓嶆敮鎸佺殑鏍煎紡";
            break;
        case AUDCLNT_E_INVALID_SIZE:
            ss << "鏃犳晥鐨勫ぇ灏?;
            break;
        case AUDCLNT_E_DEVICE_IN_USE:
            ss << "璁惧姝ｅ湪浣跨敤";
            break;
        case AUDCLNT_E_BUFFER_OPERATION_PENDING:
            ss << "缂撳啿鍖烘搷浣滄寕璧?;
            break;
        case AUDCLNT_E_THREAD_NOT_REGISTERED:
            ss << "绾跨▼鏈敞鍐?;
            break;
        case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
            ss << "鐙崰妯″紡涓嶅厑璁?;
            break;
        case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
            ss << "绔偣鍒涘缓澶辫触";
            break;
        case AUDCLNT_E_SERVICE_NOT_RUNNING:
            ss << "鏈嶅姟鏈繍琛?;
            break;
        case E_POINTER:
            ss << "绌烘寚閽?;
            break;
        case E_INVALIDARG:
            ss << "鏃犳晥鍙傛暟";
            break;
        case E_OUTOFMEMORY:
            ss << "鍐呭瓨涓嶈冻";
            break;
        default:
            ss << "鏈煡閿欒 (0x" << std::hex << hr << std::dec << ")";
            break;
    }
    
    std::cerr << ss.str() << std::endl;
}

// 璁板綍璁惧淇℃伅
void output_wasapi_impl::log_device_info() {
    if(!current_device_info_.name.empty()) {
        std::cout << "[WASAPI] 褰撳墠璁惧淇℃伅:" << std::endl;
        std::cout << "  - 鍚嶇О: " << current_device_info_.name << std::endl;
        std::cout << "  - ID: " << current_device_info_.id << std::endl;
        std::cout << "  - 鎻忚堪: " << current_device_info_.description << std::endl;
    }
}

// 璁剧疆璁惧
void output_wasapi_impl::set_device(const char* device_id) {
    device_id_ = device_id ? device_id : "";
    std::cout << "[WASAPI] 璁剧疆璁惧: " << (device_id ? device_id : "榛樿") << std::endl;
}

// 璁剧疆鐙崰妯″紡
void output_wasapi_impl::set_exclusive_mode(bool exclusive) {
    is_exclusive_mode_.store(exclusive);
    std::cout << "[WASAPI] 璁剧疆鐙崰妯″紡: " << (exclusive ? "鍚敤" : "绂佺敤") << std::endl;
}

// 璁剧疆浜嬩欢椹卞姩
void output_wasapi_impl::set_event_driven(bool event_driven) {
    is_event_driven_.store(event_driven);
    std::cout << "[WASAPI] 璁剧疆浜嬩欢椹卞姩: " << (event_driven ? "鍚敤" : "绂佺敤") << std::endl;
}

// 璁剧疆缂撳啿鍖烘寔缁椂闂?
void output_wasapi_impl::set_buffer_duration(t_uint32 milliseconds) {
    requested_buffer_duration_ms_ = milliseconds;
    std::cout << "[WASAPI] 璁剧疆缂撳啿鍖烘寔缁椂闂? " << milliseconds << "ms" << std::endl;
}

// 璁剧疆璁惧瑙掕壊
void output_wasapi_impl::set_device_role(ERole role) {
    device_role_ = role;
    std::cout << "[WASAPI] 璁剧疆璁惧瑙掕壊: " << static_cast<int>(role) << std::endl;
}

// 璁剧疆娴佺被鍒?
void output_wasapi_impl::set_stream_category(EAudioSessionCategory category) {
    stream_category_ = category;
    std::cout << "[WASAPI] 璁剧疆娴佺被鍒? " << static_cast<int>(category) << std::endl;
}

// 璁剧疆娴侀€夐」
void output_wasapi_impl::set_stream_option(t_uint32 option) {
    stream_options_ = option;
}

// 鑾峰彇鐙崰妯″紡鐘舵€?
bool output_wasapi_impl::is_exclusive_mode() const {
    return is_exclusive_mode_.load();
}

// 鑾峰彇浜嬩欢椹卞姩鐘舵€?
bool output_wasapi_impl::is_event_driven() const {
    return is_event_driven_.load();
}

// 鑾峰彇缂撳啿鍖烘寔缁椂闂?
t_uint32 output_wasapi_impl::get_buffer_duration() const {
    return requested_buffer_duration_ms_;
}

// 鑾峰彇璁惧瑙掕壊
ERole output_wasapi_impl::get_device_role() const {
    return device_role_;
}

// 鑾峰彇娴佺被鍒?
EAudioSessionCategory output_wasapi_impl::get_stream_category() const {
    return stream_category_;
}

// 鑾峰彇娴侀€夐」
t_uint32 output_wasapi_impl::get_stream_option() const {
    return stream_options_;
}

// 鑾峰彇褰撳墠璁惧
void output_wasapi_impl::get_current_device(pfc::string_base& out) {
    out = current_device_info_.name.c_str();
}

// 鏋氫妇璁惧
void output_wasapi_impl::enumerate_devices(pfc::string_list_impl& out) {
    if(!device_enumerator_) {
        return;
    }
    
    IMMDeviceCollection* device_collection = nullptr;
    HRESULT hr = device_enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &device_collection);
    if(FAILED(hr)) {
        handle_error("鏋氫妇闊抽绔偣", hr);
        return;
    }
    
    UINT32 device_count = 0;
    hr = device_collection->GetCount(&device_count);
    if(FAILED(hr)) {
        device_collection->Release();
        return;
    }
    
    for(UINT32 i = 0; i < device_count; i++) {
        IMMDevice* device = nullptr;
        hr = device_collection->Item(i, &device);
        if(FAILED(hr)) {
            continue;
        }
        
        device_info info = get_device_info(device);
        out.add_item(info.name.c_str());
        
        device->Release();
    }
    
    device_collection->Release();
}

// 鑾峰彇璁惧淇℃伅
void output_wasapi_impl::get_device_info(const char* device_id, device_info& info) {
    if(!device_enumerator_ || !device_id) {
        return;
    }
    
    std::wstring ws_device_id(device_id, device_id + strlen(device_id));
    
    IMMDevice* device = nullptr;
    HRESULT hr = device_enumerator_->GetDevice(ws_device_id.c_str(), &device);
    if(FAILED(hr)) {
        return;
    }
    
    info = get_device_info(device);
    device->Release();
}

// 鑾峰彇璁惧鍚嶇О
void output_wasapi_impl::get_device_name(pfc::string_base& out) {
    out = current_device_info_.name.c_str();
}

// 鑾峰彇璁惧鎻忚堪
void output_wasapi_impl::get_device_desc(pfc::string_base& out) {
    out = current_device_info_.description.c_str();
}

// 鑾峰彇璁惧ID
t_uint32 output_wasapi_impl::get_device_id() {
    return static_cast<t_uint32>(std::hash<std::string>{}(current_device_info_.id));
}

// 浼拌寤惰繜
void output_wasapi_impl::estimate_latency(double& latency_seconds, t_uint32 sample_rate, t_uint32 channels) {
    latency_seconds = actual_buffer_duration_ms_ / 1000.0;
}

// 鏇存柊璁惧鍒楄〃
void output_wasapi_impl::update_device_list() {
    get_device_list();
}

// 鏄惁瀹炴椂
bool output_wasapi_impl::is_realtime() {
    return true;
}

// 绌洪棽澶勭悊
void output_wasapi_impl::on_idle() {
    // 鍙互鍦ㄨ繖閲屽鐞嗚澶囩姸鎬佹洿鏂?
}

// 澶勭悊閲囨牱
void output_wasapi_impl::process_samples(const audio_chunk* p_chunk, t_uint32 p_samples_written, t_uint32 p_samples_total, abort_callback& p_abort) {
    // 杩欓噷鍙互澶勭悊閲囨牱绾у埆鐨勬搷浣?
}

// 鏆傚仠鎵╁睍
void output_wasapi_impl::pause_ex(bool p_state, t_uint32 p_samples_written) {
    pause(p_state);
}

// 璁剧疆闊抽噺鎵╁睍
void output_wasapi_impl::set_volume_ex(float p_volume, t_uint32 p_samples_written) {
    volume_set(p_volume);
}

// 鑾峰彇寤惰繜鎵╁睍
void output_wasapi_impl::get_latency_ex2(t_uint32& p_samples, t_uint32& p_samples_total) {
    p_samples = 0;
    p_samples_total = buffer_frame_count_;
}

// 鑾峰彇寤惰繜鎵╁睍3
void output_wasapi_impl::get_latency_ex3(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer) {
    p_samples = 0;
    p_samples_total = buffer_frame_count_;
    p_samples_in_buffer = get_padding_frames();
}

// 鑾峰彇寤惰繜鎵╁睍4
void output_wasapi_impl::get_latency_ex4(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer, t_uint32& p_samples_in_device_buffer) {
    get_latency_ex3(p_samples, p_samples_total, p_samples_in_buffer);
    p_samples_in_device_buffer = p_samples_in_buffer;
}

// 鑾峰彇濉厖甯ф暟
t_uint32 output_wasapi_impl::get_padding_frames() {
    if(!audio_client_) {
        return 0;
    }
    
    UINT32 padding = 0;
    HRESULT hr = audio_client_->GetCurrentPadding(&padding);
    if(FAILED(hr)) {
        return 0;
    }
    
    return padding;
}

// 鑾峰彇璁惧鍒楄〃
HRESULT output_wasapi_impl::get_device_list() {
    if(!device_enumerator_) {
        return E_FAIL;
    }
    
    IMMDeviceCollection* device_collection = nullptr;
    HRESULT hr = device_enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &device_collection);
    if(FAILED(hr)) {
        handle_error("鏋氫妇闊抽绔偣", hr);
        return hr;
    }
    
    UINT32 device_count = 0;
    hr = device_collection->GetCount(&device_count);
    if(FAILED(hr)) {
        device_collection->Release();
        return hr;
    }
    
    std::cout << "[WASAPI] 鍙戠幇 " << device_count << " 涓煶棰戣澶? << std::endl;
    
    for(UINT32 i = 0; i < device_count; i++) {
        IMMDevice* device = nullptr;
        hr = device_collection->Item(i, &device);
        if(FAILED(hr)) {
            continue;
        }
        
        device_info info = get_device_info(device);
        std::cout << "  [" << i << "] " << info.name << std::endl;
        
        device->Release();
    }
    
    device_collection->Release();
    return S_OK;
}

// 鏄惁瑕佹眰鎵╁睍瑙勮寖
bool output_wasapi_impl::requires_spec_ex() {
    return true;
}

// 鑾峰彇寤惰繜鎵╁睍
t_uint32 output_wasapi_impl::get_latency_ex() {
    return get_latency();
}

// 鍒涘缓WASAPI杈撳嚭璁惧
std::unique_ptr<output_wasapi> create_wasapi_output() {
    return std::make_unique<output_wasapi_impl>();
}

} // namespace fb2k