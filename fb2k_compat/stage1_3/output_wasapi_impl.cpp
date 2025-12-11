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

// WASAPI输出设备实现
class output_wasapi_impl : public output_wasapi {
public:
    output_wasapi_impl();
    ~output_wasapi_impl();

    // output接口实现
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

    // output_wasapi接口实现
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
    // WASAPI接口
    IMMDeviceEnumerator* device_enumerator_;
    IMMDevice* audio_device_;
    IAudioClient* audio_client_;
    IAudioRenderClient* render_client_;
    IAudioClock* audio_clock_;
    IAudioStreamVolume* stream_volume_;
    
    // 音频格式
    WAVEFORMATEXTENSIBLE format_;
    t_uint32 sample_rate_;
    t_uint32 channels_;
    t_uint32 bits_per_sample_;
    t_uint32 bytes_per_sample_;
    t_uint32 buffer_size_;
    
    // 状态管理
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<bool> is_initialized_;
    std::atomic<bool> is_exclusive_mode_;
    std::atomic<bool> is_event_driven_;
    std::atomic<bool> should_stop_;
    
    // 缓冲管理
    std::vector<uint8_t> output_buffer_;
    t_uint32 buffer_frame_count_;
    t_uint32 actual_buffer_duration_ms_;
    
    // 线程管理
    std::thread render_thread_;
    std::atomic<bool> render_thread_running_;
    std::condition_variable render_cv_;
    std::mutex render_mutex_;
    
    // 音量控制
    std::atomic<float> volume_;
    float last_set_volume_;
    
    // 性能监控
    std::chrono::steady_clock::time_point start_time_;
    t_uint64 total_samples_written_;
    t_uint64 total_samples_played_;
    
    // 配置参数
    std::string device_id_;
    ERole device_role_;
    EAudioSessionCategory stream_category_;
    t_uint32 requested_buffer_duration_ms_;
    t_uint32 stream_options_;
    
    // 设备信息
    device_info current_device_info_;
    
    // 私有方法
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

// 构造函数
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
    
    // 初始化WASAPI
    HRESULT hr = initialize_wasapi();
    if(FAILED(hr)) {
        std::cerr << "[WASAPI] 初始化失败: 0x" << std::hex << hr << std::dec << std::endl;
    }
}

// 析构函数
output_wasapi_impl::~output_wasapi_impl() {
    std::cout << "[WASAPI] 销毁输出设备..." << std::endl;
    
    // 确保停止播放
    if(is_playing_.load()) {
        abort_callback_dummy abort;
        close(abort);
    }
    
    cleanup_wasapi();
}

// 初始化WASAPI
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
    
    std::cout << "[WASAPI] 设备枚举器创建成功" << std::endl;
    return S_OK;
}

// 打开输出设备
void output_wasapi_impl::open(t_uint32 sample_rate, t_uint32 channels, t_uint32 flags, abort_callback& p_abort) {
    std::cout << "[WASAPI] 打开输出设备 - " << sample_rate << "Hz, " << channels << "ch" << std::endl;
    
    if(is_initialized_.load()) {
        std::cerr << "[WASAPI] 设备已经打开" << std::endl;
        return;
    }
    
    p_abort.check();
    
    try {
        // 保存音频格式
        sample_rate_ = sample_rate;
        channels_ = channels;
        bits_per_sample_ = 32; // 使用32位浮点数
        bytes_per_sample_ = bits_per_sample_ / 8;
        
        // 设置音频格式
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
        
        // 创建音频客户端
        HRESULT hr = create_audio_client();
        if(FAILED(hr)) {
            throw std::runtime_error("创建音频客户端失败");
        }
        
        // 初始化音频客户端
        hr = initialize_audio_client();
        if(FAILED(hr)) {
            throw std::runtime_error("初始化音频客户端失败");
        }
        
        // 获取渲染客户端
        hr = get_render_client();
        if(FAILED(hr)) {
            throw std::runtime_error("获取渲染客户端失败");
        }
        
        // 获取音频时钟
        hr = audio_client_->GetService(__uuidof(IAudioClock), (void**)&audio_clock_);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 获取音频时钟失败: 0x" << std::hex << hr << std::dec << std::endl;
            audio_clock_ = nullptr; // 非关键组件
        }
        
        // 获取流音量控制
        hr = audio_client_->GetService(__uuidof(IAudioStreamVolume), (void**)&stream_volume_);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 获取流音量控制失败: 0x" << std::hex << hr << std::dec << std::endl;
            stream_volume_ = nullptr; // 非关键组件
        }
        
        // 设置音量
        if(stream_volume_) {
            float* volumes = new float[channels_];
            std::fill(volumes, volumes + channels_, volume_.load());
            stream_volume_->SetAllVolumes(channels_, volumes);
            delete[] volumes;
        }
        
        is_initialized_.store(true);
        std::cout << "[WASAPI] 输出设备打开成功" << std::endl;
        log_device_info();
        
    } catch(const std::exception& e) {
        std::cerr << "[WASAPI] 打开设备失败: " << e.what() << std::endl;
        cleanup_wasapi();
        throw;
    }
}

// 关闭输出设备
void output_wasapi_impl::close(abort_callback& p_abort) {
    std::cout << "[WASAPI] 关闭输出设备" << std::endl;
    
    if(!is_initialized_.load()) {
        return;
    }
    
    should_stop_.store(true);
    
    // 停止渲染线程
    if(render_thread_running_.load()) {
        render_cv_.notify_all();
        if(render_thread_.joinable()) {
            render_thread_.join();
        }
        render_thread_running_.store(false);
    }
    
    // 停止音频流
    stop_streaming();
    
    // 清理资源
    cleanup_wasapi();
    
    is_initialized_.store(false);
    is_playing_.store(false);
    is_paused_.store(false);
    
    std::cout << "[WASAPI] 输出设备已关闭" << std::endl;
}

// 获取延迟
t_uint32 output_wasapi_impl::get_latency() {
    return static_cast<t_uint32>(actual_buffer_duration_ms_);
}

// 写入音频数据
void output_wasapi_impl::write(const void* buffer, t_size bytes, abort_callback& p_abort) {
    if(!is_initialized_.load() || !is_playing_.load()) {
        return;
    }
    
    p_abort.check();
    
    const uint8_t* data = static_cast<const uint8_t*>(buffer);
    t_size bytes_written = 0;
    
    while(bytes_written < bytes && !p_abort.is_aborting()) {
        // 检查是否有足够的空间
        t_uint32 available_frames = get_available_frames();
        if(available_frames == 0) {
            // 等待空间可用
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // 计算可以写入的帧数
        t_uint32 frames_to_write = std::min(available_frames, 
                                           static_cast<t_uint32>(bytes / format_.Format.nBlockAlign));
        if(frames_to_write == 0) {
            continue;
        }
        
        // 写入数据
        t_size bytes_to_write = frames_to_write * format_.Format.nBlockAlign;
        if(bytes_to_write > bytes - bytes_written) {
            bytes_to_write = bytes - bytes_written;
            frames_to_write = bytes_to_write / format_.Format.nBlockAlign;
        }
        
        HRESULT hr = write_to_device(data + bytes_written, frames_to_write);
        if(FAILED(hr)) {
            handle_error("写入音频数据", hr);
            break;
        }
        
        bytes_written += bytes_to_write;
        total_samples_written_ += frames_to_write * channels_;
    }
}

// 暂停/恢复
void output_wasapi_impl::pause(bool state) {
    if(!is_initialized_.load() || !is_playing_.load()) {
        return;
    }
    
    is_paused_.store(state);
    
    if(audio_client_) {
        HRESULT hr;
        if(state) {
            hr = audio_client_->Stop();
            std::cout << "[WASAPI] 暂停音频流" << std::endl;
        } else {
            hr = audio_client_->Start();
            std::cout << "[WASAPI] 恢复音频流" << std::endl;
        }
        
        if(FAILED(hr)) {
            handle_error("暂停/恢复音频流", hr);
        }
    }
}

// 清空缓冲区
void output_wasapi_impl::flush(abort_callback& p_abort) {
    std::cout << "[WASAPI] 清空缓冲区" << std::endl;
    
    if(!is_initialized_.load() || !audio_client_) {
        return;
    }
    
    p_abort.check();
    
    // 重置音频客户端
    HRESULT hr = audio_client_->Reset();
    if(FAILED(hr)) {
        handle_error("重置音频客户端", hr);
    }
}

// 设置音量
void output_wasapi_impl::volume_set(float volume) {
    volume_.store(volume);
    last_set_volume_ = volume;
    
    if(stream_volume_ && channels_ > 0) {
        float* volumes = new float[channels_];
        std::fill(volumes, volumes + channels_, volume);
        
        HRESULT hr = stream_volume_->SetAllVolumes(channels_, volumes);
        if(FAILED(hr)) {
            handle_error("设置音量", hr);
        }
        
        delete[] volumes;
    }
}

// 检查是否正在播放
bool output_wasapi_impl::is_playing() {
    return is_playing_.load() && !is_paused_.load();
}

// 检查是否可以写入
bool output_wasapi_impl::can_write() {
    if(!is_initialized_.load() || !is_playing_.load()) {
        return false;
    }
    
    return get_available_frames() > 0;
}

// 创建音频客户端
HRESULT output_wasapi_impl::create_audio_client() {
    HRESULT hr = S_OK;
    
    // 如果已选择特定设备，使用它；否则使用默认设备
    if(!device_id_.empty()) {
        hr = select_device(device_id_);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 选择指定设备失败，尝试默认设备" << std::endl;
        }
    }
    
    if(FAILED(hr) || device_id_.empty()) {
        // 使用默认设备
        hr = device_enumerator_->GetDefaultAudioEndpoint(eRender, device_role_, &audio_device_);
        if(FAILED(hr)) {
            handle_error("获取默认音频端点", hr);
            return hr;
        }
    }
    
    // 获取音频客户端
    hr = audio_device_->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void**)&audio_client_
    );
    
    if(FAILED(hr)) {
        handle_error("激活音频客户端", hr);
        return hr;
    }
    
    std::cout << "[WASAPI] 音频客户端创建成功" << std::endl;
    return S_OK;
}

// 获取混合格式
HRESULT output_wasapi_impl::get_mix_format() {
    WAVEFORMATEX* device_format = nullptr;
    HRESULT hr = audio_client_->GetMixFormat(&device_format);
    
    if(FAILED(hr)) {
        handle_error("获取混合格式", hr);
        return hr;
    }
    
    std::cout << "[WASAPI] 设备混合格式:" << std::endl;
    std::cout << "  - 采样率: " << device_format->nSamplesPerSec << "Hz" << std::endl;
    std::cout << "  - 声道数: " << device_format->nChannels << std::endl;
    std::cout << "  - 位深度: " << device_format->wBitsPerSample << "bit" << std::endl;
    
    CoTaskMemFree(device_format);
    return S_OK;
}

// 初始化音频客户端
HRESULT output_wasapi_impl::initialize_audio_client() {
    HRESULT hr;
    
    if(is_exclusive_mode_.load()) {
        // 独占模式
        WAVEFORMATEXTENSIBLE exclusive_format;
        hr = convert_to_exclusive_format(&format_.Format, exclusive_format);
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 转换到独占格式失败，尝试共享模式" << std::endl;
            is_exclusive_mode_.store(false);
        } else {
            hr = audio_client_->Initialize(
                AUDCLNT_SHAREMODE_EXCLUSIVE,
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                requested_buffer_duration_ms_ * 10000, // 100纳秒单位
                0,
                (WAVEFORMATEX*)&exclusive_format,
                nullptr
            );
            
            if(FAILED(hr)) {
                std::cerr << "[WASAPI] 独占模式初始化失败，尝试共享模式" << std::endl;
                is_exclusive_mode_.store(false);
            } else {
                std::cout << "[WASAPI] 使用独占模式" << std::endl;
            }
        }
    }
    
    if(!is_exclusive_mode_.load()) {
        // 共享模式
        REFERENCE_TIME buffer_duration = requested_buffer_duration_ms_ * 10000; // 100纳秒单位
        
        hr = audio_client_->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            is_event_driven_.load() ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0,
            buffer_duration,
            0,
            (WAVEFORMATEX*)&format_,
            nullptr
        );
        
        if(FAILED(hr)) {
            handle_error("初始化音频客户端（共享模式）", hr);
            return hr;
        }
        
        std::cout << "[WASAPI] 使用共享模式" << std::endl;
    }
    
    // 获取实际缓冲区大小
    hr = audio_client_->GetBufferSize(&buffer_frame_count_);
    if(FAILED(hr)) {
        handle_error("获取缓冲区大小", hr);
        return hr;
    }
    
    actual_buffer_duration_ms_ = (buffer_frame_count_ * 1000) / sample_rate_;
    
    std::cout << "[WASAPI] 缓冲区设置:" << std::endl;
    std::cout << "  - 帧数: " << buffer_frame_count_ << std::endl;
    std::cout << "  - 持续时间: " << actual_buffer_duration_ms_ << "ms" << std::endl;
    
    return S_OK;
}

// 获取渲染客户端
HRESULT output_wasapi_impl::get_render_client() {
    HRESULT hr = audio_client_->GetService(
        __uuidof(IAudioRenderClient),
        (void**)&render_client_
    );
    
    if(FAILED(hr)) {
        handle_error("获取渲染客户端", hr);
        return hr;
    }
    
    std::cout << "[WASAPI] 渲染客户端获取成功" << std::endl;
    return S_OK;
}

// 开始流式传输
HRESULT output_wasapi_impl::start_streaming() {
    if(!audio_client_) {
        return E_FAIL;
    }
    
    // 如果事件驱动，创建事件
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
    
    // 开始音频流
    HRESULT hr = audio_client_->Start();
    if(FAILED(hr)) {
        if(event_handle) CloseHandle(event_handle);
        handle_error("开始音频流", hr);
        return hr;
    }
    
    is_playing_.store(true);
    start_time_ = std::chrono::steady_clock::now();
    
    // 启动渲染线程（如果需要）
    if(!is_event_driven_.load()) {
        render_thread_running_.store(true);
        render_thread_ = std::thread(&output_wasapi_impl::render_thread_func, this);
    }
    
    std::cout << "[WASAPI] 音频流开始" << std::endl;
    return S_OK;
}

// 停止流式传输
HRESULT output_wasapi_impl::stop_streaming() {
    if(!audio_client_) {
        return S_OK;
    }
    
    is_playing_.store(false);
    
    // 停止音频流
    HRESULT hr = audio_client_->Stop();
    if(FAILED(hr)) {
        handle_error("停止音频流", hr);
    }
    
    // 重置音频客户端
    hr = audio_client_->Reset();
    if(FAILED(hr)) {
        handle_error("重置音频客户端", hr);
    }
    
    std::cout << "[WASAPI] 音频流停止" << std::endl;
    return S_OK;
}

// 清理WASAPI
HRESULT output_wasapi_impl::cleanup_wasapi() {
    // 释放接口
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

// 写入设备
HRESULT output_wasapi_impl::write_to_device(const void* data, t_uint32 frames) {
    if(!render_client_) {
        return E_FAIL;
    }
    
    UINT32 buffer_frame_count = 0;
    BYTE* buffer_pointer = nullptr;
    HRESULT hr;
    
    // 获取可用缓冲区
    hr = render_client_->GetBuffer(frames, &buffer_pointer);
    if(FAILED(hr)) {
        return hr;
    }
    
    if(!buffer_pointer) {
        return E_FAIL;
    }
    
    // 复制数据
    size_t bytes_to_copy = frames * format_.Format.nBlockAlign;
    memcpy(buffer_pointer, data, bytes_to_copy);
    
    // 释放缓冲区
    hr = render_client_->ReleaseBuffer(frames, 0);
    if(FAILED(hr)) {
        handle_error("释放缓冲区", hr);
        return hr;
    }
    
    return S_OK;
}

// 获取可用帧数
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

// 获取当前位置
HRESULT output_wasapi_impl::get_current_position(t_uint64& position, t_uint64& qpc_position) {
    if(!audio_clock_) {
        return E_FAIL;
    }
    
    return audio_clock_->GetPosition(&position, &qpc_position);
}

// 渲染线程函数
void output_wasapi_impl::render_thread_func() {
    std::cout << "[WASAPI] 渲染线程启动" << std::endl;
    
    // 提升线程优先级
    DWORD task_index = 0;
    HANDLE avrt_handle = AvSetMmThreadCharacteristics(L"Pro Audio", &task_index);
    if(avrt_handle) {
        std::cout << "[WASAPI] 渲染线程优先级已提升" << std::endl;
    }
    
    while(render_thread_running_.load() && !should_stop_.load()) {
        // 检查是否需要停止
        if(should_stop_.load()) {
            break;
        }
        
        // 渲染音频数据
        HRESULT hr = render_audio_data();
        if(FAILED(hr)) {
            std::cerr << "[WASAPI] 渲染音频数据失败: 0x" << std::hex << hr << std::dec << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // 更新延迟统计
        update_latency_stats();
        
        // 小延迟以避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // 恢复线程优先级
    if(avrt_handle) {
        AvRevertMmThreadCharacteristics(avrt_handle);
    }
    
    std::cout << "[WASAPI] 渲染线程停止" << std::endl;
}

// 渲染音频数据
HRESULT output_wasapi_impl::render_audio_data() {
    if(!is_playing_.load() || is_paused_.load() || !audio_client_ || !render_client_) {
        return S_OK;
    }
    
    // 获取可用空间
    t_uint32 available_frames = get_available_frames();
    if(available_frames == 0) {
        return S_OK;
    }
    
    // 这里应该处理实际的音频数据渲染
    // 目前只是返回成功
    
    return S_OK;
}

// 更新延迟统计
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

// 获取设备友好名称
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

// 获取设备ID
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

// 获取设备信息
device_info output_wasapi_impl::get_device_info(IMMDevice* device) {
    device_info info;
    
    if(!device) {
        return info;
    }
    
    info.id = get_device_id(device);
    info.name = get_device_friendly_name(device);
    
    // 获取其他设备属性
    IPropertyStore* property_store = nullptr;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, &property_store);
    if(SUCCEEDED(hr)) {
        PROPVARIANT prop_var;
        PropVariantInit(&prop_var);
        
        // 获取设备描述
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

// 选择设备
HRESULT output_wasapi_impl::select_device(const std::string& device_id) {
    if(!device_enumerator_ || device_id.empty()) {
        return E_FAIL;
    }
    
    // 将设备ID转换为宽字符串
    std::wstring ws_device_id(device_id.begin(), device_id.end());
    
    IMMDevice* device = nullptr;
    HRESULT hr = device_enumerator_->GetDevice(ws_device_id.c_str(), &device);
    if(FAILED(hr)) {
        handle_error("获取指定设备", hr);
        return hr;
    }
    
    // 释放旧设备
    if(audio_device_) {
        audio_device_->Release();
    }
    
    audio_device_ = device;
    current_device_info_ = get_device_info(device);
    
    std::cout << "[WASAPI] 选择设备: " << current_device_info_.name << std::endl;
    return S_OK;
}

// 转换到独占格式
HRESULT output_wasapi_impl::convert_to_exclusive_format(const WAVEFORMATEX* shared_format, 
                                                       WAVEFORMATEXTENSIBLE& exclusive_format) {
    // 简化的格式转换
    // 实际实现需要更复杂的格式匹配逻辑
    
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

// 错误处理
void output_wasapi_impl::handle_error(const std::string& operation, HRESULT hr) {
    std::stringstream ss;
    ss << "[WASAPI] 操作失败: " << operation << " - ";
    
    switch(hr) {
        case AUDCLNT_E_NOT_INITIALIZED:
            ss << "音频客户端未初始化";
            break;
        case AUDCLNT_E_ALREADY_INITIALIZED:
            ss << "音频客户端已初始化";
            break;
        case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
            ss << "错误的端点类型";
            break;
        case AUDCLNT_E_DEVICE_INVALIDATED:
            ss << "设备无效";
            break;
        case AUDCLNT_E_NOT_STOPPED:
            ss << "音频流未停止";
            break;
        case AUDCLNT_E_BUFFER_TOO_LARGE:
            ss << "缓冲区过大";
            break;
        case AUDCLNT_E_OUT_OF_ORDER:
            ss << "操作顺序错误";
            break;
        case AUDCLNT_E_UNSUPPORTED_FORMAT:
            ss << "不支持的格式";
            break;
        case AUDCLNT_E_INVALID_SIZE:
            ss << "无效的大小";
            break;
        case AUDCLNT_E_DEVICE_IN_USE:
            ss << "设备正在使用";
            break;
        case AUDCLNT_E_BUFFER_OPERATION_PENDING:
            ss << "缓冲区操作挂起";
            break;
        case AUDCLNT_E_THREAD_NOT_REGISTERED:
            ss << "线程未注册";
            break;
        case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
            ss << "独占模式不允许";
            break;
        case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
            ss << "端点创建失败";
            break;
        case AUDCLNT_E_SERVICE_NOT_RUNNING:
            ss << "服务未运行";
            break;
        case E_POINTER:
            ss << "空指针";
            break;
        case E_INVALIDARG:
            ss << "无效参数";
            break;
        case E_OUTOFMEMORY:
            ss << "内存不足";
            break;
        default:
            ss << "未知错误 (0x" << std::hex << hr << std::dec << ")";
            break;
    }
    
    std::cerr << ss.str() << std::endl;
}

// 记录设备信息
void output_wasapi_impl::log_device_info() {
    if(!current_device_info_.name.empty()) {
        std::cout << "[WASAPI] 当前设备信息:" << std::endl;
        std::cout << "  - 名称: " << current_device_info_.name << std::endl;
        std::cout << "  - ID: " << current_device_info_.id << std::endl;
        std::cout << "  - 描述: " << current_device_info_.description << std::endl;
    }
}

// 设置设备
void output_wasapi_impl::set_device(const char* device_id) {
    device_id_ = device_id ? device_id : "";
    std::cout << "[WASAPI] 设置设备: " << (device_id ? device_id : "默认") << std::endl;
}

// 设置独占模式
void output_wasapi_impl::set_exclusive_mode(bool exclusive) {
    is_exclusive_mode_.store(exclusive);
    std::cout << "[WASAPI] 设置独占模式: " << (exclusive ? "启用" : "禁用") << std::endl;
}

// 设置事件驱动
void output_wasapi_impl::set_event_driven(bool event_driven) {
    is_event_driven_.store(event_driven);
    std::cout << "[WASAPI] 设置事件驱动: " << (event_driven ? "启用" : "禁用") << std::endl;
}

// 设置缓冲区持续时间
void output_wasapi_impl::set_buffer_duration(t_uint32 milliseconds) {
    requested_buffer_duration_ms_ = milliseconds;
    std::cout << "[WASAPI] 设置缓冲区持续时间: " << milliseconds << "ms" << std::endl;
}

// 设置设备角色
void output_wasapi_impl::set_device_role(ERole role) {
    device_role_ = role;
    std::cout << "[WASAPI] 设置设备角色: " << static_cast<int>(role) << std::endl;
}

// 设置流类别
void output_wasapi_impl::set_stream_category(EAudioSessionCategory category) {
    stream_category_ = category;
    std::cout << "[WASAPI] 设置流类别: " << static_cast<int>(category) << std::endl;
}

// 设置流选项
void output_wasapi_impl::set_stream_option(t_uint32 option) {
    stream_options_ = option;
}

// 获取独占模式状态
bool output_wasapi_impl::is_exclusive_mode() const {
    return is_exclusive_mode_.load();
}

// 获取事件驱动状态
bool output_wasapi_impl::is_event_driven() const {
    return is_event_driven_.load();
}

// 获取缓冲区持续时间
t_uint32 output_wasapi_impl::get_buffer_duration() const {
    return requested_buffer_duration_ms_;
}

// 获取设备角色
ERole output_wasapi_impl::get_device_role() const {
    return device_role_;
}

// 获取流类别
EAudioSessionCategory output_wasapi_impl::get_stream_category() const {
    return stream_category_;
}

// 获取流选项
t_uint32 output_wasapi_impl::get_stream_option() const {
    return stream_options_;
}

// 获取当前设备
void output_wasapi_impl::get_current_device(pfc::string_base& out) {
    out = current_device_info_.name.c_str();
}

// 枚举设备
void output_wasapi_impl::enumerate_devices(pfc::string_list_impl& out) {
    if(!device_enumerator_) {
        return;
    }
    
    IMMDeviceCollection* device_collection = nullptr;
    HRESULT hr = device_enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &device_collection);
    if(FAILED(hr)) {
        handle_error("枚举音频端点", hr);
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

// 获取设备信息
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

// 获取设备名称
void output_wasapi_impl::get_device_name(pfc::string_base& out) {
    out = current_device_info_.name.c_str();
}

// 获取设备描述
void output_wasapi_impl::get_device_desc(pfc::string_base& out) {
    out = current_device_info_.description.c_str();
}

// 获取设备ID
t_uint32 output_wasapi_impl::get_device_id() {
    return static_cast<t_uint32>(std::hash<std::string>{}(current_device_info_.id));
}

// 估计延迟
void output_wasapi_impl::estimate_latency(double& latency_seconds, t_uint32 sample_rate, t_uint32 channels) {
    latency_seconds = actual_buffer_duration_ms_ / 1000.0;
}

// 更新设备列表
void output_wasapi_impl::update_device_list() {
    get_device_list();
}

// 是否实时
bool output_wasapi_impl::is_realtime() {
    return true;
}

// 空闲处理
void output_wasapi_impl::on_idle() {
    // 可以在这里处理设备状态更新
}

// 处理采样
void output_wasapi_impl::process_samples(const audio_chunk* p_chunk, t_uint32 p_samples_written, t_uint32 p_samples_total, abort_callback& p_abort) {
    // 这里可以处理采样级别的操作
}

// 暂停扩展
void output_wasapi_impl::pause_ex(bool p_state, t_uint32 p_samples_written) {
    pause(p_state);
}

// 设置音量扩展
void output_wasapi_impl::set_volume_ex(float p_volume, t_uint32 p_samples_written) {
    volume_set(p_volume);
}

// 获取延迟扩展
void output_wasapi_impl::get_latency_ex2(t_uint32& p_samples, t_uint32& p_samples_total) {
    p_samples = 0;
    p_samples_total = buffer_frame_count_;
}

// 获取延迟扩展3
void output_wasapi_impl::get_latency_ex3(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer) {
    p_samples = 0;
    p_samples_total = buffer_frame_count_;
    p_samples_in_buffer = get_padding_frames();
}

// 获取延迟扩展4
void output_wasapi_impl::get_latency_ex4(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer, t_uint32& p_samples_in_device_buffer) {
    get_latency_ex3(p_samples, p_samples_total, p_samples_in_buffer);
    p_samples_in_device_buffer = p_samples_in_buffer;
}

// 获取填充帧数
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

// 获取设备列表
HRESULT output_wasapi_impl::get_device_list() {
    if(!device_enumerator_) {
        return E_FAIL;
    }
    
    IMMDeviceCollection* device_collection = nullptr;
    HRESULT hr = device_enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &device_collection);
    if(FAILED(hr)) {
        handle_error("枚举音频端点", hr);
        return hr;
    }
    
    UINT32 device_count = 0;
    hr = device_collection->GetCount(&device_count);
    if(FAILED(hr)) {
        device_collection->Release();
        return hr;
    }
    
    std::cout << "[WASAPI] 发现 " << device_count << " 个音频设备" << std::endl;
    
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

// 是否要求扩展规范
bool output_wasapi_impl::requires_spec_ex() {
    return true;
}

// 获取延迟扩展
t_uint32 output_wasapi_impl::get_latency_ex() {
    return get_latency();
}

// 创建WASAPI输出设备
std::unique_ptr<output_wasapi> create_wasapi_output() {
    return std::make_unique<output_wasapi_impl>();
}

} // namespace fb2k