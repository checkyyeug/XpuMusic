#include "output_asio.h"
#include <windows.h>
#include <comdef.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")

namespace fb2k {

// 帮助函数：将窄字符串转换为宽字符串
static std::wstring string_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// 帮助函数：将宽字符串转换为窄字符串
static std::string wstring_to_string(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

// ASIO驱动枚举器
class asio_driver_enumerator {
public:
    static std::vector<asio_driver_info> enumerate_drivers() {
        std::vector<asio_driver_info> drivers;
        
        // 这里应该实现真实的ASIO驱动枚举
        // 简化实现：返回一些常见的ASIO驱动
        
        asio_driver_info asio4all;
        asio4all.name = "ASIO4ALL v2";
        asio4all.id = "ASIO4ALL";
        asio4all.description = "Universal ASIO Driver";
        asio4all.version = "2.15";
        asio4all.clsid = "{8C5F1F63-D1E0-4F75-A2A2-8C5F1F63D1E0}";
        asio4all.is_active = true;
        asio4all.input_channels = 2;
        asio4all.output_channels = 2;
        asio4all.supported_sample_rates = {44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0};
        asio4all.buffer_size_min = 64;
        asio4all.buffer_size_max = 2048;
        asio4all.buffer_size_preferred = 512;
        asio4all.buffer_size_granularity = 32;
        drivers.push_back(asio4all);
        
        asio_driver_info flexasio;
        flexasio.name = "FlexASIO";
        flexasio.id = "FlexASIO";
        flexasio.description = "Flexible ASIO Driver";
        flexasio.version = "1.9";
        flexasio.clsid = "{F1F2F3F4-F5F6-F7F8-F9FA-FBFCFDFEFF00}";
        flexasio.is_active = true;
        flexasio.input_channels = 2;
        flexasio.output_channels = 2;
        flexasio.supported_sample_rates = {44100.0, 48000.0, 88200.0, 96000.0};
        flexasio.buffer_size_min = 128;
        flexasio.buffer_size_max = 4096;
        flexasio.buffer_size_preferred = 256;
        flexasio.buffer_size_granularity = 64;
        drivers.push_back(flexasio);
        
        return drivers;
    }
};

// ASIO缓冲区管理器实现
asio_buffer_manager::asio_buffer_manager()
    : num_channels_(0)
    , buffer_size_(0)
    , current_buffer_index_(0)
    , sample_type_(ASIOST_FLOAT32LSB) {
}

asio_buffer_manager::~asio_buffer_manager() {
    cleanup();
}

bool asio_buffer_manager::initialize(long num_channels, long buffer_size, asio_sample_type sample_type) {
    cleanup();
    
    num_channels_ = num_channels;
    buffer_size_ = buffer_size;
    sample_type_ = sample_type;
    current_buffer_index_ = 0;
    
    return allocate_buffers();
}

void asio_buffer_manager::cleanup() {
    free_buffers();
    num_channels_ = 0;
    buffer_size_ = 0;
    current_buffer_index_ = 0;
}

bool asio_buffer_manager::allocate_buffers() {
    if (num_channels_ <= 0 || buffer_size_ <= 0) {
        return false;
    }
    
    buffer_infos_.resize(num_channels_);
    long sample_size = get_sample_size(sample_type_);
    
    for (long i = 0; i < num_channels_; ++i) {
        buffer_infos_[i].buffer_index = 0;
        buffer_infos_[i].channel_index = i;
        buffer_infos_[i].is_active = true;
        buffer_infos_[i].data_size = buffer_size_ * sample_size;
        buffer_infos_[i].sample_type = sample_type_;
        
        // 分配双缓冲
        for (int j = 0; j < 2; ++j) {
            buffer_infos_[i].buffer[j] = new uint8_t[buffer_infos_[i].data_size];
            std::memset(buffer_infos_[i].buffer[j], 0, buffer_infos_[i].data_size);
        }
    }
    
    return true;
}

void asio_buffer_manager::free_buffers() {
    for (auto& info : buffer_infos_) {
        for (int i = 0; i < 2; ++i) {
            if (info.buffer[i]) {
                delete[] static_cast<uint8_t*>(info.buffer[i]);
                info.buffer[i] = nullptr;
            }
        }
    }
    buffer_infos_.clear();
}

asio_buffer_info* asio_buffer_manager::get_buffer_info(long channel) {
    if (channel < 0 || channel >= num_channels_) {
        return nullptr;
    }
    return &buffer_infos_[channel];
}

void asio_buffer_manager::switch_buffers() {
    current_buffer_index_ = 1 - current_buffer_index_;
}

float* asio_buffer_manager::get_input_buffer(long channel, long buffer_index) {
    if (channel < 0 || channel >= num_channels_) {
        return nullptr;
    }
    
    if (sample_type_ != ASIOST_FLOAT32LSB && sample_type_ != ASIOST_FLOAT32MSB) {
        return nullptr;
    }
    
    return static_cast<float*>(buffer_infos_[channel].buffer[buffer_index]);
}

float* asio_buffer_manager::get_output_buffer(long channel, long buffer_index) {
    return get_input_buffer(channel, buffer_index); // 同样的缓冲区用于输入输出
}

long asio_buffer_manager::get_sample_size(asio_sample_type type) const {
    switch (type) {
        case ASIOST_16LSB:
        case ASIOST_16MSB:
            return 2;
        case ASIOST_24LSB:
        case ASIOST_24MSB:
            return 3;
        case ASIOST_32LSB:
        case ASIOST_32MSB:
        case ASIOST_FLOAT32LSB:
        case ASIOST_FLOAT32MSB:
            return 4;
        case ASIOST_FLOAT64LSB:
        case ASIOST_FLOAT64MSB:
            return 8;
        default:
            return 4; // 默认32位
    }
}

// ASIO时间管理器实现
asio_time_manager::asio_time_manager() {
    std::memset(&time_info_, 0, sizeof(time_info_));
    time_info_.sampleRate = 44100.0;
}

asio_time_manager::~asio_time_manager() = default;

void asio_time_manager::update_time_info(const asio_time_info& info) {
    std::lock_guard<std::mutex> lock(time_mutex_);
    time_info_ = info;
}

asio_time_info asio_time_manager::get_current_time_info() const {
    std::lock_guard<std::mutex> lock(time_mutex_);
    return time_info_;
}

double asio_time_manager::get_sample_position() const {
    std::lock_guard<std::mutex> lock(time_mutex_);
    return time_info_.samplePos;
}

double asio_time_manager::get_system_time() const {
    std::lock_guard<std::mutex> lock(time_mutex_);
    return time_info_.nanoSeconds * 1e-9; // 转换为秒
}

double asio_time_manager::get_sample_rate() const {
    std::lock_guard<std::mutex> lock(time_mutex_);
    return time_info_.sampleRate;
}

bool asio_time_manager::is_sample_rate_changed() const {
    std::lock_guard<std::mutex> lock(time_mutex_);
    return (time_info_.flags & kVstSampleRateChanged) != 0;
}

bool asio_time_manager::is_clock_source_changed() const {
    std::lock_guard<std::mutex> lock(time_mutex_);
    return (time_info_.flags & kVstTransportChanged) != 0;
}

void asio_time_manager::clear_flags() {
    std::lock_guard<std::mutex> lock(time_mutex_);
    time_info_.flags = 0;
}

// ASIO回调处理器实现
asio_callback_handler::asio_callback_handler()
    : buffer_switch_callback_(nullptr)
    , sample_rate_callback_(nullptr)
    , message_callback_(nullptr)
    , num_channels_(0)
    , buffer_size_(0) {
}

asio_callback_handler::~asio_callback_handler() = default;

void asio_callback_handler::set_buffer_switch_callback(asio_buffer_switch_callback callback) {
    buffer_switch_callback_ = callback;
}

void asio_callback_handler::set_sample_rate_callback(asio_sample_rate_callback callback) {
    sample_rate_callback_ = callback;
}

void asio_callback_handler::set_message_callback(asio_message_callback callback) {
    message_callback_ = callback;
}

void asio_callback_handler::on_buffer_switch(long buffer_index, long direct_process) {
    if (buffer_switch_callback_) {
        buffer_switch_callback_(buffer_index);
    }
}

void asio_callback_handler::on_sample_rate_changed(double sample_rate) {
    if (sample_rate_callback_) {
        sample_rate_callback_(sample_rate);
    }
}

long asio_callback_handler::on_message(long selector, long value, void* message, double* opt) {
    if (message_callback_) {
        return message_callback_(selector, value, message, opt);
    }
    return 0;
}

void asio_callback_handler::process_input_buffers(long buffer_index) {
    // 处理输入缓冲区（如果需要录音功能）
    if (!input_buffers_.empty() && audio_processor_) {
        float** inputs = new float*[num_channels_];
        for (long i = 0; i < num_channels_; ++i) {
            inputs[i] = input_buffers_[i].data() + buffer_index * buffer_size_;
        }
        
        audio_processor_(inputs, nullptr, num_channels_, buffer_size_);
        
        delete[] inputs;
    }
}

void asio_callback_handler::process_output_buffers(long buffer_index) {
    // 处理输出缓冲区
    if (!output_buffers_.empty() && audio_processor_) {
        float** outputs = new float*[num_channels_];
        for (long i = 0; i < num_channels_; ++i) {
            outputs[i] = output_buffers_[i].data() + buffer_index * buffer_size_;
        }
        
        audio_processor_(nullptr, outputs, num_channels_, buffer_size_);
        
        delete[] outputs;
    }
}

void asio_callback_handler::set_audio_processor(std::function<void(float**, float**, long, long)> processor) {
    audio_processor_ = processor;
}

void asio_callback_handler::set_input_data(float** input_data, long num_channels, long buffer_size) {
    num_channels_ = num_channels;
    buffer_size_ = buffer_size;
    
    input_buffers_.resize(num_channels);
    for (long i = 0; i < num_channels; ++i) {
        input_buffers_[i].assign(input_data[i], input_data[i] + buffer_size);
    }
}

void asio_callback_handler::get_output_data(float** output_data, long num_channels, long buffer_size) {
    num_channels_ = num_channels;
    buffer_size_ = buffer_size;
    
    output_buffers_.resize(num_channels);
    for (long i = 0; i < num_channels; ++i) {
        if (output_data[i]) {
            std::memcpy(output_data[i], output_buffers_[i].data(), buffer_size * sizeof(float));
        }
    }
}

// 错误处理
std::string asio_error_handler::get_error_string(long error_code) {
    switch (error_code) {
        case ASE_OK: return "Success";
        case ASE_SUCCESS: return "Operation successful";
        case ASE_NotPresent: return "ASIO not present";
        case ASE_HWMalfunction: return "Hardware malfunction";
        case ASE_InvalidParameter: return "Invalid parameter";
        case ASE_InvalidMode: return "Invalid mode";
        case ASE_SPNotAdvancing: return "Sample position not advancing";
        case ASE_NoClock: return "No clock";
        case ASE_NoMemory: return "Out of memory";
        default: return "Unknown ASIO error: " + std::to_string(error_code);
    }
}

void asio_error_handler::handle_error(const std::string& operation, long error_code) {
    std::string error_msg = get_error_string(error_code);
    std::cerr << "[ASIO] " << operation << " failed: " << error_msg << std::endl;
}

bool asio_error_handler::is_recoverable_error(long error_code) {
    switch (error_code) {
        case ASE_OK:
        case ASE_SUCCESS:
        case ASE_SPNotAdvancing:
            return true;
        default:
            return false;
    }
}

void asio_error_handler::log_asio_error(const std::string& context, long error_code) {
    std::string error_msg = get_error_string(error_code);
    std::cerr << "[ASIO] " << context << " - Error: " << error_code << " (" << error_msg << ")" << std::endl;
}

// output_asio_impl实现
output_asio_impl::output_asio_impl()
    : device_enumerator_(nullptr)
    , driver_(nullptr)
    , buffer_manager_(nullptr)
    , time_manager_(nullptr)
    , callback_handler_(nullptr)
    , sample_rate_(44100)
    , channels_(2)
    , bits_per_sample_(32)
    , buffer_size_(512)
    , is_initialized_(false)
    , is_playing_(false)
    , is_paused_(false)
    , volume_(1.0f)
    , input_latency_(0)
    , output_latency_(0)
    , cpu_load_(0.0)
    , current_driver_name_("None")
    , current_driver_id_("")
    , supports_time_code_(false)
    , supports_input_monitoring_(false)
    , supports_variable_buffer_size_(false) {
    
    // 创建ASIO组件
    buffer_manager_ = new asio_buffer_manager();
    time_manager_ = new asio_time_manager();
    callback_handler_ = new asio_callback_handler();
}

output_asio_impl::~output_asio_impl() {
    std::cout << "[ASIO] 销毁ASIO输出设备" << std::endl;
    
    if (is_initialized_) {
        abort_callback_dummy abort;
        close(abort);
    }
    
    delete buffer_manager_;
    delete time_manager_;
    delete callback_handler_;
}

// ASIO输出设备接口实现
void output_asio_impl::open(t_uint32 sample_rate, t_uint32 channels, t_uint32 flags, abort_callback& p_abort) {
    std::cout << "[ASIO] 打开ASIO输出设备 - " << sample_rate << "Hz, " << channels << "ch" << std::endl;
    
    if (is_initialized_) {
        std::cerr << "[ASIO] 设备已经初始化" << std::endl;
        return;
    }
    
    p_abort.check();
    
    try {
        // 保存音频格式
        sample_rate_ = sample_rate;
        channels_ = channels;
        bits_per_sample_ = 32; // ASIO通常使用32位浮点
        
        // 检查当前驱动是否支持格式
        if (!current_driver_name_.empty() && !validate_asio_format(ASIOST_FLOAT32LSB, channels, sample_rate)) {
            throw std::runtime_error("ASIO驱动不支持指定格式");
        }
        
        // 创建并初始化ASIO驱动
        if (!load_driver(current_driver_id_)) {
            throw std::runtime_error("无法加载ASIO驱动");
        }
        
        // 初始化ASIO缓冲区管理器
        if (!buffer_manager_->initialize(channels, buffer_size_, ASIOST_FLOAT32LSB)) {
            throw std::runtime_error("ASIO缓冲区初始化失败");
        }
        
        // 创建ASIO缓冲区
        if (!create_asio_buffers()) {
            throw std::runtime_error("ASIO缓冲区创建失败");
        }
        
        // 设置回调处理器
        setup_callbacks();
        
        is_initialized_ = true;
        std::cout << "[ASIO] ASIO输出设备打开成功" << std::endl;
        log_asio_info();
        
    } catch (const std::exception& e) {
        std::cerr << "[ASIO] 打开设备失败: " << e.what() << std::endl;
        cleanup_asio();
        throw;
    }
}

void output_asio_impl::close(abort_callback& p_abort) {
    std::cout << "[ASIO] 关闭ASIO输出设备" << std::endl;
    
    if (!is_initialized_) {
        return;
    }
    
    p_abort.check();
    
    // 停止音频流
    stop_asio_streaming();
    
    // 清理资源
    cleanup_asio();
    
    is_initialized_ = false;
    is_playing_ = false;
    is_paused_ = false;
    
    std::cout << "[ASIO] ASIO输出设备已关闭" << std::endl;
}

t_uint32 output_asio_impl::get_latency() {
    return static_cast<t_uint32>(output_latency_ + input_latency_);
}

void output_asio_impl::write(const void* buffer, t_size bytes, abort_callback& p_abort) {
    if (!is_initialized_ || !is_playing_ || is_paused_) {
        return;
    }
    
    p_abort.check();
    
    const uint8_t* data = static_cast<const uint8_t*>(buffer);
    t_size bytes_written = 0;
    
    while (bytes_written < bytes && !p_abort.is_aborting()) {
        // 检查缓冲区空间
        long available_frames = get_available_frames();
        if (available_frames <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // 计算可以写入的帧数
        long frames_to_write = std::min(available_frames, 
                                       static_cast<long>((bytes - bytes_written) / (channels_ * sizeof(float))));
        if (frames_to_write <= 0) {
            continue;
        }
        
        // 写入数据到ASIO缓冲区
        HRESULT hr = write_to_asio_buffers(data + bytes_written, frames_to_write);
        if (FAILED(hr)) {
            handle_asio_error("写入ASIO缓冲区", hr);
            break;
        }
        
        bytes_written += frames_to_write * channels_ * sizeof(float);
        total_samples_written_ += frames_to_write * channels_;
    }
}

void output_asio_impl::pause(bool state) {
    if (!is_initialized_) {
        return;
    }
    
    is_paused_.store(state);
    
    // ASIO通常不支持软件暂停，这里只是状态标记
    std::cout << "[ASIO] " << (state ? "暂停" : "恢复") << "音频流" << std::endl;
}

void output_asio_impl::flush(abort_callback& p_abort) {
    std::cout << "[ASIO] 清空ASIO缓冲区" << std::endl;
    
    if (!is_initialized_) {
        return;
    }
    
    p_abort.check();
    
    // ASIO缓冲区重置通常通过驱动完成
    if (driver_) {
        // driver_->ResetBuffers(); // 需要实际的ASIO接口
    }
}

void output_asio_impl::volume_set(float volume) {
    volume_.store(volume);
    
    // ASIO通常通过硬件混音器控制音量
    // 这里只是软件音量标记
    std::cout << "[ASIO] 设置音量: " << volume << std::endl;
}

bool output_asio_impl::is_playing() {
    return is_playing_.load() && !is_paused_.load();
}

bool output_asio_impl::can_write() {
    if (!is_initialized_ || !is_playing_) {
        return false;
    }
    
    return get_available_frames() > 0;
}

// ASIO特定接口实现
bool output_asio_impl::enum_drivers(std::vector<asio_driver_info>& drivers) {
    drivers = asio_driver_enumerator::enumerate_drivers();
    return !drivers.empty();
}

bool output_asio_impl::load_driver(const std::string& driver_id) {
    if (driver_id.empty()) {
        return false;
    }
    
    // 卸载当前驱动
    unload_driver();
    
    // 这里应该实现真实的ASIO驱动加载
    // 简化实现：模拟驱动加载
    
    auto drivers = asio_driver_enumerator::enumerate_drivers();
    for (const auto& driver : drivers) {
        if (driver.id == driver_id) {
            current_driver_name_ = driver.name;
            current_driver_id_ = driver_id;
            
            // 设置驱动信息
            input_latency_ = driver.buffer_size_preferred / 2; // 估算输入延迟
            output_latency_ = driver.buffer_size_preferred / 2; // 估算输出延迟
            buffer_size_ = driver.buffer_size_preferred;
            
            // 设置支持的功能
            supports_time_code_ = false; // 简化实现
            supports_input_monitoring_ = false;
            supports_variable_buffer_size_ = (driver.buffer_size_granularity > 0);
            
            std::cout << "[ASIO] 驱动加载成功: " << driver.name << std::endl;
            return true;
        }
    }
    
    std::cerr << "[ASIO] 未找到指定驱动: " << driver_id << std::endl;
    return false;
}

void output_asio_impl::unload_driver() {
    if (!current_driver_id_.empty()) {
        std::cout << "[ASIO] 卸载驱动: " << current_driver_name_ << std::endl;
        
        // 停止音频流
        if (is_playing_) {
            stop_asio_streaming();
        }
        
        current_driver_name_ = "None";
        current_driver_id_.clear();
        driver_ = nullptr;
    }
}

bool output_asio_impl::is_driver_loaded() const {
    return !current_driver_id_.empty();
}

std::string output_asio_impl::get_current_driver_name() const {
    return current_driver_name_;
}

void output_asio_impl::set_buffer_size(long size) {
    if (size >= 64 && size <= 4096) {
        buffer_size_ = size;
        std::cout << "[ASIO] 设置缓冲区大小: " << size << std::endl;
    }
}

long output_asio_impl::get_buffer_size() const {
    return buffer_size_;
}

void output_asio_impl::set_sample_rate(double rate) {
    sample_rate_ = static_cast<t_uint32>(rate);
    std::cout << "[ASIO] 设置采样率: " << rate << "Hz" << std::endl;
}

double output_asio_impl::get_sample_rate() const {
    return static_cast<double>(sample_rate_);
}

std::vector<double> output_asio_impl::get_available_sample_rates() const {
    // 返回常见的ASIO采样率
    return {44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0};
}

long output_asio_impl::get_input_latency() const {
    return input_latency_;
}

long output_asio_impl::get_output_latency() const {
    return output_latency_;
}

double output_asio_impl::get_cpu_load() const {
    return cpu_load_.load();
}

bool output_asio_impl::supports_time_code() const {
    return supports_time_code_;
}

bool output_asio_impl::supports_input_monitoring() const {
    return supports_input_monitoring_;
}

bool output_asio_impl::supports_variable_buffer_size() const {
    return supports_variable_buffer_size_;
}

void output_asio_impl::show_control_panel() {
    std::cout << "[ASIO] 显示控制面板" << std::endl;
    
    // 这里应该调用驱动的控制面板
    // 简化实现：只是输出信息
    std::cout << "[ASIO] 驱动: " << current_driver_name_ << std::endl;
    std::cout << "[ASIO] 采样率: " << sample_rate_ << "Hz" << std::endl;
    std::cout << "[ASIO] 缓冲区大小: " << buffer_size_ << std::endl;
    std::cout << "[ASIO] 输入延迟: " << input_latency_ << " 采样" << std::endl;
    std::cout << "[ASIO] 输出延迟: " << output_latency_ << " 采样" << std::endl;
}

long output_asio_impl::get_buffer_size_min() const {
    return 64;
}

long output_asio_impl::get_buffer_size_max() const {
    return 4096;
}

long output_asio_impl::get_buffer_size_preferred() const {
    return 512;
}

long output_asio_impl::get_buffer_size_granularity() const {
    return supports_variable_buffer_size_ ? 32 : 0;
}

std::vector<std::string> output_asio_impl::get_clock_sources() const {
    // 返回时钟源列表
    return {"Internal", "Word Clock", "Digital Input", "S/PDIF"};
}

void output_asio_impl::set_clock_source(long index) {
    std::cout << "[ASIO] 设置时钟源: " << index << std::endl;
}

long output_asio_impl::get_current_clock_source() const {
    return 0; // Internal
}

// 音频处理回调
void output_asio_impl::process_samples(const audio_chunk* p_chunk, t_uint32 p_samples_written, t_uint32 p_samples_total, abort_callback& p_abort) {
    // ASIO使用回调模式，这里不需要实现
}

void output_asio_impl::pause_ex(bool p_state, t_uint32 p_samples_written) {
    pause(p_state);
}

void output_asio_impl::set_volume_ex(float p_volume, t_uint32 p_samples_written) {
    volume_set(p_volume);
}

void output_asio_impl::get_latency_ex2(t_uint32& p_samples, t_uint32& p_samples_total) {
    p_samples = 0;
    p_samples_total = buffer_size_;
}

void output_asio_impl::get_latency_ex3(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer) {
    get_latency_ex2(p_samples, p_samples_total);
    p_samples_in_buffer = get_padding_frames();
}

void output_asio_impl::get_latency_ex4(t_uint32& p_samples, t_uint32& p_samples_total, t_uint32& p_samples_in_buffer, t_uint32& p_samples_in_device_buffer) {
    get_latency_ex3(p_samples, p_samples_total, p_samples_in_buffer);
    p_samples_in_device_buffer = p_samples_in_buffer;
}

// 私有实现方法
bool output_asio_impl::initialize_asio() {
    std::cout << "[ASIO] 初始化ASIO系统" << std::endl;
    
    if (!driver_) {
        std::cerr << "[ASIO] 没有加载的驱动" << std::endl;
        return false;
    }
    
    // 初始化驱动
    // 简化实现：假设初始化成功
    return true;
}

bool output_asio_impl::create_asio_buffers() {
    std::cout << "[ASIO] 创建ASIO缓冲区" << std::endl;
    
    if (!driver_ || !buffer_manager_) {
        return false;
    }
    
    // 创建缓冲区信息数组
    std::vector<asio_buffer_info> buffer_infos(channels_);
    for (long i = 0; i < channels_; ++i) {
        buffer_infos[i].buffer_index = 0;
        buffer_infos[i].channel_index = i;
        buffer_infos[i].is_active = true;
        buffer_infos[i].data_size = buffer_size_ * sizeof(float);
        buffer_infos[i].sample_type = ASIOST_FLOAT32LSB;
        buffer_infos[i].buffer[0] = nullptr;
        buffer_infos[i].buffer[1] = nullptr;
    }
    
    // 这里应该调用真实的ASIO接口创建缓冲区
    // 简化实现：使用缓冲区管理器
    
    return true;
}

bool output_asio_impl::start_asio_streaming() {
    std::cout << "[ASIO] 开始ASIO流式传输" << std::endl;
    
    if (!driver_ || is_playing_) {
        return false;
    }
    
    // 启动音频流
    // 简化实现：标记为播放状态
    is_playing_.store(true);
    start_time_ = std::chrono::steady_clock::now();
    
    return true;
}

bool output_asio_impl::stop_asio_streaming() {
    std::cout << "[ASIO] 停止ASIO流式传输" << std::endl;
    
    if (!driver_ || !is_playing_) {
        return false;
    }
    
    // 停止音频流
    is_playing_.store(false);
    
    return true;
}

void output_asio_impl::cleanup_asio() {
    std::cout << "[ASIO] 清理ASIO资源" << std::endl;
    
    if (buffer_manager_) {
        buffer_manager_->cleanup();
    }
    
    if (time_manager_) {
        time_manager_->clear_flags();
    }
    
    // 卸载驱动
    unload_driver();
}

void output_asio_impl::asio_thread_func() {
    std::cout << "[ASIO] ASIO线程启动" << std::endl;
    
    // 提升线程优先级
    DWORD task_index = 0;
    HANDLE avrt_handle = AvSetMmThreadCharacteristics(L"Pro Audio", &task_index);
    if (avrt_handle) {
        std::cout << "[ASIO] ASIO线程优先级已提升" << std::endl;
    }
    
    while (asio_thread_running_.load() && !should_stop_.load()) {
        // ASIO音频处理循环
        if (is_playing_.load() && !is_paused_.load()) {
            // 处理音频数据
            update_performance_stats();
            
            // 小延迟以避免CPU占用过高
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    // 恢复线程优先级
    if (avrt_handle) {
        AvRevertMmThreadCharacteristics(avrt_handle);
    }
    
    std::cout << "[ASIO] ASIO线程停止" << std::endl;
}

void output_asio_impl::process_audio_data(float** input_channels, float** output_channels, long num_channels, long buffer_size) {
    // 处理音频数据
    if (!output_channels || num_channels <= 0 || buffer_size <= 0) {
        return;
    }
    
    // 应用音量
    float current_volume = volume_.load();
    
    for (long ch = 0; ch < num_channels; ++ch) {
        if (output_channels[ch]) {
            for (long i = 0; i < buffer_size; ++i) {
                output_channels[ch][i] *= current_volume;
            }
        }
    }
    
    total_samples_played_ += buffer_size * num_channels;
}

void output_asio_impl::update_performance_stats() {
    // 更新性能统计
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
    
    if (duration > 0) {
        double cpu_usage = get_current_cpu_usage();
        cpu_load_.store(cpu_usage);
    }
}

double output_asio_impl::get_current_cpu_usage() {
    // 简化的CPU使用率计算
    static FILETIME prev_idle, prev_kernel, prev_user;
    FILETIME idle, kernel, user;
    
    if (GetSystemTimes(&idle, &kernel, &user)) {
        if (prev_idle.dwLowDateTime != 0) {
            ULONGLONG idle_diff = (idle.dwLowDateTime + ((ULONGLONG)idle.dwHighDateTime << 32)) - 
                                 (prev_idle.dwLowDateTime + ((ULONGLONG)prev_idle.dwHighDateTime << 32));
            ULONGLONG kernel_diff = (kernel.dwLowDateTime + ((ULONGLONG)kernel.dwHighDateTime << 32)) - 
                                   (prev_kernel.dwLowDateTime + ((ULONGLONG)prev_kernel.dwHighDateTime << 32));
            ULONGLONG user_diff = (user.dwLowDateTime + ((ULONGLONG)user.dwHighDateTime << 32)) - 
                                 (prev_user.dwLowDateTime + ((ULONGLONG)prev_user.dwHighDateTime << 32));
            
            if (kernel_diff + user_diff > 0) {
                return (double)(kernel_diff + user_diff - idle_diff) / (kernel_diff + user_diff) * 100.0;
            }
        }
        
        prev_idle = idle;
        prev_kernel = kernel;
        prev_user = user;
    }
    
    return 0.0;
}

bool output_asio_impl::validate_asio_format(long asio_sample_type, long num_channels, double sample_rate) {
    // 检查ASIO驱动是否支持指定格式
    if (!driver_) return false;
    
    // 简化验证：检查常见格式
    if (asio_sample_type != ASIOST_FLOAT32LSB && asio_sample_type != ASIOST_FLOAT32MSB) {
        return false;
    }
    
    if (num_channels < 1 || num_channels > 32) {
        return false;
    }
    
    // 检查采样率
    auto supported_rates = get_available_sample_rates();
    if (std::find(supported_rates.begin(), supported_rates.end(), sample_rate) == supported_rates.end()) {
        return false;
    }
    
    return true;
}

t_uint32 output_asio_impl::get_padding_frames() {
    if (!driver_) {
        return 0;
    }
    
    // 这里应该调用ASIO驱动获取当前填充帧数
    // 简化实现：返回估算值
    return buffer_size_ / 2;
}

t_uint32 output_asio_impl::get_available_frames() {
    if (!driver_) {
        return 0;
    }
    
    // 这里应该调用ASIO驱动获取可用帧数
    // 简化实现：返回估算值
    return buffer_size_ - get_padding_frames();
}

HRESULT output_asio_impl::write_to_asio_buffers(const void* data, t_uint32 frames) {
    if (!buffer_manager_ || !driver_) {
        return E_FAIL;
    }
    
    // 将数据写入ASIO缓冲区
    const float* float_data = static_cast<const float*>(data);
    
    for (long ch = 0; ch < channels_; ++ch) {
        float* asio_buffer = buffer_manager_->get_output_buffer(ch, buffer_manager_->get_current_buffer_index());
        if (asio_buffer) {
            // 复制数据到ASIO缓冲区
            for (t_uint32 i = 0; i < frames; ++i) {
                asio_buffer[i] = float_data[i * channels_ + ch] * volume_.load();
            }
        }
    }
    
    return S_OK;
}

void output_asio_impl::setup_callbacks() {
    // 设置ASIO回调
    callback_handler_->set_buffer_switch_callback(
        [](long buffer_index) {
            // 缓冲区切换回调
            // 这里应该处理实际的缓冲区切换
        });
    
    callback_handler_->set_sample_rate_callback(
        [this](double sample_rate) {
            // 采样率变化回调
            sample_rate_ = static_cast<t_uint32>(sample_rate);
            std::cout << "[ASIO] 采样率改变: " << sample_rate << "Hz" << std::endl;
        });
    
    callback_handler_->set_message_callback(
        [](long selector, long value, void* ptr, double* opt) -> long {
            // 消息回调
            return 0;
        });
    
    // 设置音频处理器
    callback_handler_->set_audio_processor(
        [this](float** inputs, float** outputs, long num_channels, long buffer_size) {
            this->process_audio_data(inputs, outputs, num_channels, buffer_size);
        });
}

void output_asio_impl::log_asio_info() {
    std::cout << "[ASIO] 设备信息:" << std::endl;
    std::cout << "  驱动: " << current_driver_name_ << std::endl;
    std::cout << "  采样率: " << sample_rate_ << "Hz" << std::endl;
    std::cout << "  声道数: " << channels_ << std::endl;
    std::cout << "  缓冲区大小: " << buffer_size_ << std::endl;
    std::cout << "  输入延迟: " << input_latency_ << " 采样" << std::endl;
    std::cout << "  输出延迟: " << output_latency_ << " 采样" << std::endl;
    std::cout << "  支持时间码: " << (supports_time_code_ ? "是" : "否") << std::endl;
    std::cout << "  支持输入监听: " << (supports_input_monitoring_ ? "是" : "否") << std::endl;
}

void output_asio_impl::handle_asio_error(const std::string& operation, HRESULT hr) {
    asio_error_handler::handle_error(operation, hr);
}

// ASIO工具函数实现
namespace asio_utils {

std::vector<asio_driver_info> enumerate_asio_drivers() {
    return asio_driver_enumerator::enumerate_drivers();
}

bool is_asio_available() {
    auto drivers = enumerate_asio_drivers();
    return !drivers.empty();
}

std::string get_asio_version() {
    return "2.3";
}

long get_optimal_buffer_size(double sample_rate, int channels) {
    // 根据采样率和声道数推荐最优缓冲区大小
    if (sample_rate >= 192000) {
        return 256; // 高采样率使用小缓冲区
    } else if (sample_rate >= 96000) {
        return 512;
    } else if (sample_rate >= 48000) {
        return 512;
    } else {
        return 512; // 44.1kHz
    }
}

bool validate_asio_config(const asio_config& config) {
    if (config.preferred_buffer_size < config.minimum_buffer_size ||
        config.preferred_buffer_size > config.maximum_buffer_size) {
        return false;
    }
    
    if (config.num_output_channels <= 0 || config.num_output_channels > 32) {
        return false;
    }
    
    return true;
}

asio_driver_info get_driver_info(const std::string& driver_id) {
    auto drivers = enumerate_asio_drivers();
    for (const auto& driver : drivers) {
        if (driver.id == driver_id) {
            return driver;
        }
    }
    
    return asio_driver_info(); // 返回空信息
}

} // namespace asio_utils

// 创建ASIO输出设备
std::unique_ptr<output_asio> create_asio_output() {
    return std::make_unique<output_asio_impl>();
}

} // namespace fb2k