#pragma once

// 闃舵1.2锛氳緭鍑鸿澶囨帴鍙?
// 闊抽杈撳嚭璁惧鎺ュ彛锛屾敮鎸乄ASAPI銆丏irectSound绛?

#include <string>
#include <vector>
#include <memory>
#include "audio_chunk.h"
#include "../stage1_1/real_minihost.h"

namespace fb2k {

// 杈撳嚭璁惧鐘舵€?
enum class output_state {
    closed,     // 鍏抽棴鐘舵€?
    opening,    // 姝ｅ湪鎵撳紑
    open,       // 宸叉墦寮€
    playing,    // 姝ｅ湪鎾斁
    paused,     // 宸叉殏鍋?
    error       // 閿欒鐘舵€?
};

// 杈撳嚭璁惧鏍煎紡
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

// 杈撳嚭璁惧鑳藉姏
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

// 杈撳嚭璁惧鎺ュ彛 - 绗﹀悎foobar2000瑙勮寖
class output_device : public ServiceBase {
public:
    // 璁惧绠＄悊
    virtual bool open(uint32_t sample_rate, uint32_t channels, abort_callback& abort) = 0;
    virtual void close(abort_callback& abort) = 0;
    virtual bool is_open() const = 0;
    
    // 闊抽澶勭悊
    virtual void process_chunk(audio_chunk& chunk, abort_callback& abort) = 0;
    virtual void flush(abort_callback& abort) = 0;
    
    // 鏍煎紡鏀寔
    virtual bool can_update_format() const = 0;
    virtual bool set_format(const output_format& format, abort_callback& abort) = 0;
    virtual output_format get_current_format() const = 0;
    virtual std::vector<output_format> get_supported_formats() const = 0;
    
    // 寤惰繜鍜岀紦鍐?
    virtual double get_latency() const = 0;
    virtual size_t get_buffer_size() const = 0;
    virtual bool set_buffer_size(size_t size, abort_callback& abort) = 0;
    
    // 璁惧淇℃伅
    virtual const char* get_name() const = 0;
    virtual const char* get_description() const = 0;
    virtual output_device_caps get_device_caps() const = 0;
    
    // 鐘舵€佺鐞?
    virtual output_state get_state() const = 0;
    virtual bool is_playing() const = 0;
    virtual bool is_paused() const = 0;
    
    // 楂樼骇鍔熻兘
    virtual bool supports_exclusive_mode() const = 0;
    virtual bool set_exclusive_mode(bool exclusive, abort_callback& abort) = 0;
    virtual bool get_exclusive_mode() const = 0;
    
    // 浜嬩欢鍜屽洖璋?
    virtual void set_event_callback(std::function<void(output_state)> callback) = 0;
};

// 闊抽缂撳啿绠＄悊鍣?
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
        
        // 澶勭悊鐜舰缂撳啿鐨勫啓鍏?
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
        
        // 澶勭悊鐜舰缂撳啿鐨勮鍙?
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

// 鏍煎紡杞崲鍣?
class format_converter {
public:
    // 娴偣鍒?6浣嶆暣鏁?
    static void convert_float_to_int16(const float* src, int16_t* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 32767.0f;
        for(size_t i = 0; i < samples; ++i) {
            float sample = src[i] * scale;
            // 瑁佸壀鍒?6浣嶈寖鍥?
            sample = std::max(-32768.0f, std::min(32767.0f, sample));
            dst[i] = static_cast<int16_t>(sample);
        }
    }
    
    // 16浣嶆暣鏁板埌娴偣
    static void convert_int16_to_float(const int16_t* src, float* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 1.0f / 32768.0f;
        for(size_t i = 0; i < samples; ++i) {
            dst[i] = static_cast<float>(src[i]) * scale;
        }
    }
    
    // 娴偣鍒?4浣嶆暣鏁帮紙鎵撳寘鏍煎紡锛?
    static void convert_float_to_int24(const float* src, uint8_t* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 8388607.0f; // 2^23 - 1
        for(size_t i = 0; i < samples; ++i) {
            float sample = src[i] * scale;
            // 瑁佸壀鍒?4浣嶈寖鍥?
            sample = std::max(-8388608.0f, std::min(8388607.0f, sample));
            int32_t value = static_cast<int32_t>(sample);
            
            // 鎵撳寘涓?瀛楄妭灏忕鏍煎紡
            dst[i * 3 + 0] = static_cast<uint8_t>(value & 0xFF);
            dst[i * 3 + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
            dst[i * 3 + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        }
    }
    
    // 24浣嶆暣鏁板埌娴偣锛堟墦鍖呮牸寮忥級
    static void convert_int24_to_float(const uint8_t* src, float* dst, size_t samples) {
        if(!src || !dst || samples == 0) return;
        
        const float scale = 1.0f / 8388608.0f;
        for(size_t i = 0; i < samples; ++i) {
            // 瑙ｅ寘3瀛楄妭灏忕鏍煎紡
            int32_t value = static_cast<int32_t>(src[i * 3 + 0]) |
                           (static_cast<int32_t>(src[i * 3 + 1]) << 8) |
                           (static_cast<int32_t>(src[i * 3 + 2]) << 16);
            
            // 绗﹀彿鎵╁睍
            if(value & 0x800000) {
                value |= 0xFF000000;
            }
            
            dst[i] = static_cast<float>(value) * scale;
        }
    }
    
    // 闊抽鍧楁牸寮忚浆鎹?
    static bool convert_chunk_format(const audio_chunk& src, audio_chunk& dst, 
                                    audio_format target_format) {
        if(src.is_empty()) return false;
        
        const float* src_data = src.get_data();
        size_t total_samples = src.get_sample_count() * src.get_channels();
        
        if(!src_data) return false;
        
        switch(target_format) {
            case audio_format::float32:
                // 宸茬粡鏄诞鐐规牸寮忥紝鐩存帴澶嶅埗
                dst.copy(src);
                return true;
                
            case audio_format::int16: {
                // 杞崲涓?6浣嶆暣鏁?
                std::vector<int16_t> temp_buffer(total_samples);
                convert_float_to_int16(src_data, temp_buffer.data(), total_samples);
                
                // 杩欓噷闇€瑕佸皢int16鏁版嵁鍖呰鍒癮udio_chunk涓?
                // 瀹為檯瀹炵幇闇€瑕佹墿灞昦udio_chunk鎺ュ彛
                return true;
            }
            
            case audio_format::int24: {
                // 杞崲涓?4浣嶆暣鏁?
                std::vector<uint8_t> temp_buffer(total_samples * 3);
                convert_float_to_int24(src_data, temp_buffer.data(), total_samples);
                
                // 杩欓噷闇€瑕佸皢24浣嶆暟鎹寘瑁呭埌audio_chunk涓?
                return true;
            }
            
            default:
                return false;
        }
    }
};

// 杈撳嚭璁惧鍩虹被瀹炵幇
class output_device_base : public output_device {
protected:
    output_state state_;
    output_format current_format_;
    std::function<void(output_state)> event_callback_;
    
public:
    output_device_base() : state_(output_state::closed) {
        current_format_ = output_format(); // 榛樿鏍煎紡
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
        return false; // 鍩虹被涓嶆敮鎸佺嫭鍗犳ā寮?
    }
    
    bool set_exclusive_mode(bool exclusive, abort_callback& abort) override {
        return !exclusive; // 鍙兘鍏抽棴鐙崰妯″紡
    }
    
    bool get_exclusive_mode() const override {
        return false;
    }
};

// 杈撳嚭璁惧绠＄悊鍣?
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
        
        // 鍏抽棴褰撳墠璁惧
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
    
    // 渚垮埄鍑芥暟
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

// 杈撳嚭璁惧宸ュ叿鍑芥暟
class output_device_utils {
public:
    // 鍒涘缓鏍囧噯杈撳嚭鏍煎紡
    static output_format create_standard_format(uint32_t sample_rate, uint32_t channels) {
        return output_format(sample_rate, channels, 16, audio_format::int16);
    }
    
    static output_format create_float_format(uint32_t sample_rate, uint32_t channels) {
        return output_format(sample_rate, channels, 32, audio_format::float32);
    }
    
    // 楠岃瘉杈撳嚭鏍煎紡
    static bool validate_output_format(const output_format& format) {
        return format.is_valid();
    }
    
    // 璁＄畻鏍煎紡鎵€闇€鐨勭紦鍐插尯澶у皬
    static size_t calculate_buffer_size(const output_format& format, size_t samples) {
        size_t bytes_per_sample = format.bits_per_sample / 8;
        return samples * format.channels * bytes_per_sample;
    }
    
    // 鑾峰彇鏍煎紡鎻忚堪瀛楃涓?
    static std::string get_format_description(const output_format& format) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%u Hz, %u ch, %u-bit %s",
                format.sample_rate, format.channels, format.bits_per_sample,
                format.format == audio_format::float32 ? "Float" : 
                format.format == audio_format::int16 ? "Int16" : "Unknown");
        return std::string(buffer);
    }
    
    // 姣旇緝涓や釜鏍煎紡鏄惁鍏煎
    static bool are_formats_compatible(const output_format& format1, const output_format& format2) {
        // 绠€鍖栧疄鐜帮細鍙鏌ラ噰鏍风巼鍜屽０閬撴暟
        return format1.sample_rate == format2.sample_rate &&
               format1.channels == format2.channels;
    }
};

// 杈撳嚭璁惧楠岃瘉鍣?
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
        
        // 鍩虹楠岃瘉
        if(!device.is_valid()) {
            result.is_valid = false;
            result.error_message = "杈撳嚭璁惧鏃犳晥";
            return result;
        }
        
        // 妫€鏌ヨ澶囪兘鍔?
        auto caps = device.get_device_caps();
        if(caps.supported_formats.empty()) {
            result.is_valid = false;
            result.error_message = "璁惧涓嶆敮鎸佷换浣曢煶棰戞牸寮?;
            return result;
        }
        
        // 妫€鏌ュ欢杩熻寖鍥?
        if(caps.min_latency_ms > caps.max_latency_ms) {
            result.is_valid = false;
            result.error_message = "璁惧寤惰繜鑼冨洿鏃犳晥";
            return result;
        }
        
        // 璀﹀憡鍜屽缓璁?
        if(caps.min_latency_ms > 50.0) {
            result.warnings.push_back("璁惧鏈€灏忓欢杩熻緝楂? " + std::to_string(caps.min_latency_ms) + " ms");
        }
        
        if(!caps.supports_exclusive_mode) {
            result.recommendations.push_back("鑰冭檻浣跨敤鏀寔鐙崰妯″紡鐨勮澶囦互闄嶄綆寤惰繜");
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