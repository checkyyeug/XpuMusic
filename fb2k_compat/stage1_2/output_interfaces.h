#pragma once

// 阶段1.2：输出设备接口
// 音频输出设备接口，支持WASAPI、DirectSound等

#include <string>
#include <vector>
#include <memory>
#include "audio_chunk.h"
#include "../stage1_1/real_minihost.h"

namespace fb2k {

// 输出设备状态
enum class output_state {
    closed,     // 关闭状态
    opening,    // 正在打开
    open,       // 已打开
    playing,    // 正在播放
    paused,     // 已暂停
    error       // 错误状态
};

// 输出设备格式
struct output_format {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bits_per_sample;
    audio_format format;
    
    output_format() : sample_rate(44100), channels(2), bits_per_sample(16), format(audio_format::int16) {}
    output_format(uint32_t rate, uint32_t ch, uint32_t bits, audio_format fmt)
        : sample_rate(rate), channels(ch), bits_per_sample(bits), format(fmt) {}
    
    bool is_valid() const {
        return sample_rate >= 8000 && sample_rate <= 192000 &&
               channels >= 1 && channels <= 8 &&
               bits_per_sample >= 8 && bits_per_sample <= 32;
    }
    
    bool operator==(const output_format& other) const {
        return sample_rate == other.sample_rate &&
               channels == other.channels &&
               bits_per_sample == other.bits_per_sample &&
               format == other.format;
    }
};

// 输出设备能力
struct output_device_caps {
    std::string name;
    std::string description;
    std::vector<output_format> supported_formats;
    double min_latency_ms;
    double max_latency_ms;
    bool supports_exclusive_mode;
    bool supports_event_driven;
    
    output_device_caps() : min_latency_ms(1.0), max_latency_ms(1000.0), 
                          supports_exclusive_mode(false), supports_event_driven(false) {}
};

// 输出设备接口 - 符合foobar2000规范
class output_device : public ServiceBase {
public:
    // 设备管理
    virtual bool open(uint32_t sample_rate, uint32_t channels, abort_callback& abort) = 0;
    virtual void close(abort_callback& abort) = 0;
    virtual bool is_open() const = 0;
    
    // 音频处理
    virtual void process_chunk(audio_chunk& chunk, abort_callback& abort) = 0;
    virtual void flush(abort_callback& abort) = 0;
    
    // 格式支持
    virtual bool can_update_format() const = 0;
    virtual bool set_format(const output_format& format, abort_callback& abort) = 0;
    virtual output_format get_current_format() const = 0;
    virtual std::vector<output_format> get_supported_formats() const = 0;
    
    // 延迟和缓冲
    virtual double get_latency() const = 0;
    virtual size_t get_buffer_size() const = 0;
    virtual bool set_buffer_size(size_t size, abort_callback& abort) = 0;
    
    // 设备信息
    virtual const char* get_name() const = 0;
    virtual const char* get_description() const = 0;
    virtual output_device_caps get_device_caps() const = 0;
    
    // 状态管理
    virtual output_state get_state() const = 0;
    virtual bool is_playing() const = 0;
    virtual bool is_paused() const = 0;
    
    // 高级功能
    virtual bool supports_exclusive_mode() const = 0;
    virtual bool set_exclusive_mode(bool exclusive, abort_callback& abort) = 0;
    virtual bool get_exclusive_mode() const = 0;
    
    // 事件和回调
    virtual void set_event_callback(std::function<void(output_state)> callback) = 0;
};

// 音频缓冲管理器
class audio_buffer {
private:
    std::vector<uint8_t> buffer_;
    size_t capacity_;
    size_t write_pos_;
    size_t read_pos_;
    size_t data_size_;
    
public:
    audio_buffer(size_t capacity = 65536) 
        : capacity_(capacity), write_pos_(0), read_pos_(0), data_size_(0) {
        buffer_.resize(capacity);
    }
    
    size_t write(const void* data, size_t size) {
        if(!data || size == 0) return 0;
        
        size_t writable = std::min(size, get_free_space());
        if(writable == 0) return 0;
        
        const uint8_t* src = static_cast<const uint8_t*>(data);
        
        // 处理环形缓冲的写入
        size_t first_part = std::min(writable, capacity_ - write_pos_);
        std::memcpy(&buffer_[write_pos_], src, first_part);
        
        if(writable > first_part) {
            size_t second_part = writable - first_part;
            std::memcpy(&buffer_[0], src + first_part, second_part);
        }
        
        write_pos_ = (write_pos_ + writable) % capacity_;
        data_size_ += writable;
        
        return writable;
    }
    
    size_t read(void* data, size_t size) {
        if(!data || size == 0) return 0;
        
        size_t readable = std::min(size, data_size_);
        if(readable == 0) return 0;
        
        uint8_t* dst = static_cast<uint8_t*>(data);
        
        // 处理环形缓冲的读取
        size_t first_part = std::min(readable, capacity_ - read_pos_);
        std::memcpy(dst, &buffer_[read_pos_], first_part);
        
        if(readable > first_part) {
            size_t second_part = readable - first_part;
            std::memcpy(dst + first_part, &buffer_[0], second_part);
        }
        
        read_pos_ = (read_pos_ + readable) % capacity_;
        data_size_ -= readable;
        
        return readable;
    }
    
    size_t get_free_space() const {
        return capacity_ - data_size_;
    }
    
    size_t get_data_size() const {
        return data_size_;
    }
    
    size_t get_capacity() const {
        return capacity_;
    }
    
    bool is_empty() const {
        return data_size_ == 0;
    }
    
    bool is_full() const {
        return data_size_ == capacity_;
    }
    
    void clear() {
        write_pos_ = 0;
        read_pos_ = 0;
        data_size_ = 0;
        std::memset(buffer_.data(), 0, buffer_.size());
    }
    
    void reset() {
        clear();
    }
};

// 格式转换器
class format_converter {
public:
    // 浮点到16位整数
    static void convert_float_to_int16(const float* src, int16_t* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 32767.0f;
        for(size_t i = 0; i < samples; ++i) {
            float sample = src[i] * scale;
            // 裁剪到16位范围
            sample = std::max(-32768.0f, std::min(32767.0f, sample));
            dst[i] = static_cast<int16_t>(sample);
        }
    }
    
    // 16位整数到浮点
    static void convert_int16_to_float(const int16_t* src, float* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 1.0f / 32768.0f;
        for(size_t i = 0; i < samples; ++i) {
            dst[i] = static_cast<float>(src[i]) * scale;
        }
    }
    
    // 浮点到24位整数（打包格式）
    static void convert_float_to_int24(const float* src, uint8_t* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 8388607.0f; // 2^23 - 1
        for(size_t i = 0; i < samples; ++i) {
            float sample = src[i] * scale;
            // 裁剪到24位范围
            sample = std::max(-8388608.0f, std::min(8388607.0f, sample));
            int32_t value = static_cast<int32_t>(sample);
            
            // 打包为3字节小端格式
            dst[i * 3 + 0] = static_cast<uint8_t>(value & 0xFF);
            dst[i * 3 + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
            dst[i * 3 + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        }
    }
    
    // 24位整数到浮点（打包格式）
    static void convert_int24_to_float(const uint8_t* src, float* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 1.0f / 8388608.0f;
        for(size_t i = 0; i < samples; ++i) {
            // 解包3字节小端格式
            int32_t value = static_cast<int32_t>(src[i * 3 + 0]) |
                           (static_cast<int32_t>(src[i * 3 + 1]) << 8) |
                           (static_cast<int32_t>(src[i * 3 + 2]) << 16);
            
            // 符号扩展
            if(value & 0x800000) {
                value |= 0xFF000000;
            }
            
            dst[i] = static_cast<float>(value) * scale;
        }
    }
    
    // 音频块格式转换
    static bool convert_chunk_format(const audio_chunk& src, audio_chunk& dst, 
                                    audio_format target_format) {
        if(src.is_empty()) return false;
        
        const float* src_data = src.get_data();
        size_t total_samples = src.get_sample_count() * src.get_channels();
        
        if(!src_data) return false;
        
        switch(target_format) {
            case audio_format::float32:
                // 已经是浮点格式，直接复制
                dst.copy(src);
                return true;
                
            case audio_format::int16: {
                // 转换为16位整数
                std::vector<int16_t> temp_buffer(total_samples);
                convert_float_to_int16(src_data, temp_buffer.data(), total_samples);
                
                // 这里需要将int16数据包装到audio_chunk中
                // 实际实现需要扩展audio_chunk接口
                return true;
            }
            
            case audio_format::int24: {
                // 转换为24位整数
                std::vector<uint8_t> temp_buffer(total_samples * 3);
                convert_float_to_int24(src_data, temp_buffer.data(), total_samples);
                
                // 这里需要将24位数据包装到audio_chunk中
                return true;
            }
            
            default:
                return false;
        }
    }
};

// 输出设备基类实现
class output_device_base : public output_device {
protected:
    output_state state_;
    output_format current_format_;
    std::function<void(output_state)> event_callback_;
    
public:
    output_device_base() : state_(output_state::closed) {
        current_format_ = output_format(); // 默认格式
    }
    
    void set_state(output_state new_state) {
        if(state_ != new_state) {
            state_ = new_state;
            if(event_callback_) {
                event_callback_(state_);
            }
        }
    }
    
    void set_event_callback(std::function<void(output_state)> callback) override {
        event_callback_ = callback;
    }
    
    output_state get_state() const override {
        return state_;
    }
    
    bool is_playing() const override {
        return state_ == output_state::playing;
    }
    
    bool is_paused() const override {
        return state_ == output_state::paused;
    }
    
    output_format get_current_format() const override {
        return current_format_;
    }
    
    bool is_open() const override {
        return state_ != output_state::closed;
    }
    
    bool supports_exclusive_mode() const override {
        return false; // 基类不支持独占模式
    }
    
    bool set_exclusive_mode(bool exclusive, abort_callback& abort) override {
        return !exclusive; // 只能关闭独占模式
    }
    
    bool get_exclusive_mode() const override {
        return false;
    }
};

// 输出设备管理器
class output_device_manager {
private:
    std::vector<service_ptr_t<output_device>> devices_;
    service_ptr_t<output_device> current_device_;
    
public:
    void register_device(service_ptr_t<output_device> device) {
        if(device.is_valid()) {
            devices_.push_back(device);
        }
    }
    
    std::vector<output_device_caps> enumerate_devices() const {
        std::vector<output_device_caps> caps;
        
        for(const auto& device : devices_) {
            if(device.is_valid()) {
                caps.push_back(device->get_device_caps());
            }
        }
        
        return caps;
    }
    
    bool set_current_device(service_ptr_t<output_device> device) {
        if(!device.is_valid()) return false;
        
        // 关闭当前设备
        if(current_device_.is_valid() && current_device_->is_open()) {
            abort_callback_dummy abort;
            current_device_->close(abort);
        }
        
        current_device_ = device;
        return true;
    }
    
    service_ptr_t<output_device> get_current_device() const {
        return current_device_;
    }
    
    output_device* get_current_device_ptr() const {
        return current_device_.get();
    }
    
    // 便利函数
    bool open_current_device(uint32_t sample_rate, uint32_t channels, abort_callback& abort) {
        if(!current_device_.is_valid()) return false;
        return current_device_->open(sample_rate, channels, abort);
    }
    
    void close_current_device(abort_callback& abort) {
        if(current_device_.is_valid()) {
            current_device_->close(abort);
        }
    }
    
    void process_audio_chunk(audio_chunk& chunk, abort_callback& abort) {
        if(current_device_.is_valid() && current_device_->is_open()) {
            current_device_->process_chunk(chunk, abort);
        }
    }
};

// 输出设备工具函数
class output_device_utils {
public:
    // 创建标准输出格式
    static output_format create_standard_format(uint32_t sample_rate, uint32_t channels) {
        return output_format(sample_rate, channels, 16, audio_format::int16);
    }
    
    static output_format create_float_format(uint32_t sample_rate, uint32_t channels) {
        return output_format(sample_rate, channels, 32, audio_format::float32);
    }
    
    // 验证输出格式
    static bool validate_output_format(const output_format& format) {
        return format.is_valid();
    }
    
    // 计算格式所需的缓冲区大小
    static size_t calculate_buffer_size(const output_format& format, size_t samples) {
        size_t bytes_per_sample = format.bits_per_sample / 8;
        return samples * format.channels * bytes_per_sample;
    }
    
    // 获取格式描述字符串
    static std::string get_format_description(const output_format& format) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%u Hz, %u ch, %u-bit %s",
                format.sample_rate, format.channels, format.bits_per_sample,
                format.format == audio_format::float32 ? "Float" : 
                format.format == audio_format::int16 ? "Int16" : "Unknown");
        return std::string(buffer);
    }
    
    // 比较两个格式是否兼容
    static bool are_formats_compatible(const output_format& format1, const output_format& format2) {
        // 简化实现：只检查采样率和声道数
        return format1.sample_rate == format2.sample_rate &&
               format1.channels == format2.channels;
    }
};

// 输出设备验证器
class output_device_validator {
public:
    struct validation_result {
        bool is_valid;
        std::string error_message;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
    };
    
    static validation_result validate_device(const output_device& device) {
        validation_result result;
        result.is_valid = true;
        
        // 基础验证
        if(!device.is_valid()) {
            result.is_valid = false;
            result.error_message = "输出设备无效";
            return result;
        }
        
        // 检查设备能力
        auto caps = device.get_device_caps();
        if(caps.supported_formats.empty()) {
            result.is_valid = false;
            result.error_message = "设备不支持任何音频格式";
            return result;
        }
        
        // 检查延迟范围
        if(caps.min_latency_ms > caps.max_latency_ms) {
            result.is_valid = false;
            result.error_message = "设备延迟范围无效";
            return result;
        }
        
        // 警告和建议
        if(caps.min_latency_ms > 50.0) {
            result.warnings.push_back("设备最小延迟较高: " + std::to_string(caps.min_latency_ms) + " ms");
        }
        
        if(!caps.supports_exclusive_mode) {
            result.recommendations.push_back("考虑使用支持独占模式的设备以降低延迟");
        }
        
        return result;
    }
    
    static bool validate_output_format(const output_device& device, const output_format& format) {
        auto supported_formats = device.get_supported_formats();
        
        return std::find(supported_formats.begin(), supported_formats.end(), format) != 
               supported_formats.end();
    }
};

} // namespace fb2k