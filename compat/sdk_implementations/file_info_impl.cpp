/**
 * @file file_info_impl.cpp
 * @brief file_info_impl 类的实现
 * @date 2025-12-09
 */

#include "file_info_impl.h"
#include <algorithm>
#include <sstream>
#include <cstring>

namespace foobar2000_sdk {

// 使用 xpumusic_sdk 中的类型
using xpumusic_sdk::audio_info;
using xpumusic_sdk::file_stats;
using xpumusic_sdk::field_value;

//==============================================================================
// file_info_impl 实现
//==============================================================================

file_info_impl::file_info_impl() : file_info_interface() {
    // 默认音频信息
    audio_info_.m_sample_rate = 44100;
    audio_info_.m_channels = 2;
    audio_info_.m_bitrate = 0;
    audio_info_.m_length = 0;
    
    // 默认文件统计
    stats_.m_size = 0;
    stats_.m_timestamp = 0;
}

file_info_impl::file_info_impl(const file_info_impl& other) : file_info_interface() {
    std::lock_guard<std::mutex> lock(other.mutex_);
    
    meta_fields_ = other.meta_fields_;
    audio_info_ = other.audio_info_;
    stats_ = other.stats_;
}

file_info_impl& file_info_impl::operator=(const file_info_impl& other) {
    if (this == &other) {
        return *this;
    }
    
    std::lock_guard<std::mutex> lock_this(mutex_);
    std::lock_guard<std::mutex> lock_other(other.mutex_);
    
    meta_fields_ = other.meta_fields_;
    audio_info_ = other.audio_info_;
    stats_ = other.stats_;
    
    return *this;
}

std::string file_info_impl::normalize_field_name(const std::string& name) const {
    if (name.empty()) {
        return name;
    }
    
    std::string normalized = name;
    
    // fooar2000 字段名通常是大小写不敏感的
    // 转换为小写进行内部存储
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // 处理常见别名
    if (normalized == "date" || normalized == "year") {
        return "date";  // 标准化为 "date"
    }
    
    // 移除首尾空格
    size_t start = normalized.find_first_not_of(" \t\r\n");
    size_t end = normalized.find_last_not_of(" \t\r\n");
    
    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }
    
    return normalized.substr(start, end - start + 1);
}

xpumusic_sdk::field_value& file_info_impl::get_or_create_field(const char* name) {
    std::string normalized_name = normalize_field_name(name);

    auto it = meta_fields_.find(normalized_name);
    if (it == meta_fields_.end()) {
        // 创建新字段
        xpumusic_sdk::field_value new_field;
        auto result = meta_fields_.emplace(normalized_name, std::move(new_field));
        it = result.first;
    }

    return it->second;
}

const char* file_info_impl::meta_get(const char* p_name, size_t p_index) const {
    if (!p_name) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalized_name = normalize_field_name(p_name);
    auto it = meta_fields_.find(normalized_name);
    
    if (it == meta_fields_.end()) {
        return nullptr;  // 字段不存在
    }
    
    const auto& values = it->second.values;
    if (p_index >= values.size()) {
        return nullptr;  // 索引超出范围
    }
    
    return values[p_index].c_str();  // 返回 C 字符串
}

size_t file_info_impl::meta_get_count(const char* p_name) const {
    if (!p_name) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalized_name = normalize_field_name(p_name);
    auto it = meta_fields_.find(normalized_name);
    
    if (it == meta_fields_.end()) {
        return 0;  // 字段不存在
    }
    
    return it->second.values.size();
}

bool file_info_impl::meta_set(const char* p_name, const char* p_value) {
    if (!p_name || !p_value) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    xpumusic_sdk::field_value& field = get_or_create_field(p_name);
    field.values.clear();          // 移除所有现有值
    field.values.push_back(p_value); // 添加新值
    field.cache_valid = false;     // 使缓存失效
    
    return true;
}

bool file_info_impl::meta_remove(const char* p_name) {
    if (!p_name) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalized_name = normalize_field_name(p_name);
    size_t erased = meta_fields_.erase(normalized_name);
    
    return erased > 0;
}

bool file_info_impl::meta_add(const char* p_name, const char* p_value) {
    if (!p_name || !p_value) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    xpumusic_sdk::field_value& field = get_or_create_field(p_name);
    field.values.push_back(p_value);  // 添加到值列表
    field.cache_valid = false;        // 使缓存失效
    
    return true;
}

bool file_info_impl::meta_remove_value(const char* p_name, const char* p_value) {
    if (!p_name || !p_value) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalized_name = normalize_field_name(p_name);
    auto it = meta_fields_.find(normalized_name);
    if (it == meta_fields_.end()) {
        return false;  // 字段不存在
    }
    
    auto& values = it->second.values;
    auto value_it = std::find(values.begin(), values.end(), p_value);
    if (value_it == values.end()) {
        return false;  // 值不存在
    }
    
    values.erase(value_it);  // 删除匹配的值
    it->second.cache_valid = false;
    
    // 如果没有值了，删除整个字段
    if (values.empty()) {
        meta_fields_.erase(it);
    }
    
    return true;
}

bool file_info_impl::meta_remove_index(const char* p_name, size_t p_index) {
    if (!p_name) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalized_name = normalize_field_name(p_name);
    auto it = meta_fields_.find(normalized_name);
    if (it == meta_fields_.end()) {
        return false;  // 字段不存在
    }
    
    auto& values = it->second.values;
    if (p_index >= values.size()) {
        return false;  // 索引超出范围
    }
    
    values.erase(values.begin() + p_index);  // 删除索引处的值
    it->second.cache_valid = false;
    
    // 如果没有值了，删除整个字段
    if (values.empty()) {
        meta_fields_.erase(it);
    }
    
    return true;
}

std::vector<std::string> file_info_impl::meta_enum_field_names() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> field_names;
    field_names.reserve(meta_fields_.size());
    
    for (const auto& pair : meta_fields_) {
        field_names.push_back(pair.first);
    }
    
    return field_names;
}

void file_info_impl::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    meta_fields_.clear();
    
    // 重置音频信息为默认值
    audio_info_.m_sample_rate = 44100;
    audio_info_.m_channels = 2;
    audio_info_.m_bitrate = 0;
    audio_info_.m_length = 0;
    
    // 重置文件统计
    stats_.m_size = 0;
    stats_.m_timestamp = 0;
}

void file_info_impl::copy_from(const file_info_interface& other) {
    // 使用公共接口复制数据
    reset();  // 重置

    // 复制音频信息
    const xpumusic_sdk::audio_info& other_audio = other.get_audio_info();
    audio_info_.m_sample_rate = other_audio.m_sample_rate;
    audio_info_.m_channels = other_audio.m_channels;
    audio_info_.m_bitrate = other_audio.m_bitrate;
    audio_info_.m_length = other_audio.m_length;

    // 复制文件统计
    const xpumusic_sdk::file_stats& other_stats = other.get_file_stats();
    stats_.m_size = other_stats.m_size;
    stats_.m_timestamp = other_stats.m_timestamp;

    // 复制元数据字段
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& field_name : meta_enum_field_names()) {
        size_t count = other.meta_get_count(field_name.c_str());
        for (size_t i = 0; i < count; ++i) {
            const char* value = other.meta_get(field_name.c_str(), i);
            if (value) {
                meta_add(field_name.c_str(), value);
            }
        }
    }
}

void file_info_impl::merge_from(const file_info_interface& other) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 合并音频信息（保留非零值）
    const xpumusic_sdk::audio_info& other_audio = other.get_audio_info();

    if (other_audio.m_sample_rate != 0) audio_info_.m_sample_rate = other_audio.m_sample_rate;
    if (other_audio.m_channels != 0) audio_info_.m_channels = other_audio.m_channels;
    if (other_audio.m_bitrate != 0) audio_info_.m_bitrate = other_audio.m_bitrate;
    if (other_audio.m_length != 0) audio_info_.m_length = other_audio.m_length;
    
    // 合并文件统计（保留最大值）
    const xpumusic_sdk::file_stats& other_stats = other.get_file_stats();
    if (other_stats.m_size > stats_.m_size) stats_.m_size = other_stats.m_size;
    if (other_stats.m_timestamp > stats_.m_timestamp) stats_.m_timestamp = other_stats.m_timestamp;
    
    // 合并非空字段
    std::vector<std::string> field_names;
    // 获取字段名列表 - 使用接口方法
    // 这里需要遍历可能的字段名，暂时使用常见的字段名
    const char* common_fields[] = {"artist", "title", "album", "date", "genre", "comment"};
    for (const char* field_name : common_fields) {
        if (other.meta_get_count(field_name) > 0) {
            field_names.push_back(field_name);
        }
    }
    for (const auto& field_name : field_names) {
        size_t count = other.meta_get_count(field_name.c_str());
        for (size_t i = 0; i < count; ++i) {
            const char* value = other.meta_get(field_name.c_str(), i);
            if (value && strlen(value) > 0) {
                // 值已经存在则不添加（避免重复）
                bool exists = false;
                size_t existing_count = meta_get_count(field_name.c_str());
                for (size_t j = 0; j < existing_count; ++j) {
                    const char* existing = meta_get(field_name.c_str(), j);
                    if (existing && strcmp(existing, value) == 0) {
                        exists = true;
                        break;
                    }
                }
                
                if (!exists) {
                    meta_add(field_name.c_str(), value);
                }
            }
        }
    }
}

bool file_info_impl::meta_equals(const file_info_interface& other) const {
    std::lock_guard<std::mutex> lock_this(mutex_);
    
    // 动态转换到 file_info_impl
    const file_info_impl* other_impl = dynamic_cast<const file_info_impl*>(&other);
    if (other_impl) {
        // 是 file_info_impl，直接比较
        std::lock_guard<std::mutex> lock_other(other_impl->mutex_);
        
        if (meta_fields_.size() != other_impl->meta_fields_.size()) {
            return false;
        }
        
        for (const auto& pair : meta_fields_) {
            auto it = other_impl->meta_fields_.find(pair.first);
            if (it == other_impl->meta_fields_.end()) {
                return false;  // 字段名不匹配
            }
            
            const auto& values = pair.second.values;
            const auto& other_values = it->second.values;
            
            if (values.size() != other_values.size()) {
                return false;  // 值数量不匹配
            }
            
            for (size_t i = 0; i < values.size(); ++i) {
                if (values[i] != other_values[i]) {
                    return false;  // 值不匹配
                }
            }
        }
        
        return true;
    }
    
    // 不是 file_info_impl，使用动态比较
    if (meta_fields_.size() != other.meta_get_count("__field_count_hack__")) {
        return false;  // 字段数量不匹配
    }
    
    for (const auto& pair : meta_fields_) {
        size_t count = other.meta_get_count(pair.first.c_str());
        if (count != pair.second.values.size()) {
            return false;  // 值数量不匹配
        }
        
        for (size_t i = 0; i < count; ++i) {
            const char* other_value = other.meta_get(pair.first.c_str(), i);
            if (!other_value || pair.second.values[i] != other_value) {
                return false;  // 值不匹配
            }
        }
    }
    
    return true;
}

size_t file_info_impl::get_approximate_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t size = sizeof(*this);
    
    // 字段名的近似大小
    for (const auto& pair : meta_fields_) {
        size += pair.first.capacity();  // 字符串容量
        
        // 值字符串的大小
        for (const auto& value : pair.second.values) {
            size += value.capacity();
        }
    }
    
    return size;
}

// 辅助函数
std::unique_ptr<file_info_impl> file_info_create() {
    return std::make_unique<file_info_impl>();
}

// 注意：EnhancedTrackInfo 目前在元数据的头文件中前向声明
// 这将在重构后移动
#if 0
std::unique_ptr<file_info_impl> file_info_from_metadata(
    const EnhancedTrackInfo& track_info) {
    
    auto file_info = std::make_unique<file_info_impl>();
    
    // 复制增强元数据到 file_info
    // 这将在 EnhancedMetadata 类完成后实现
    
    return file_info;
}
#endif

} // namespace foobar2000_sdk
