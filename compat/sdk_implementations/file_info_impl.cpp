/**
 * @file file_info_impl.cpp
 * @brief file_info_impl 绫荤殑瀹炵幇
 * @date 2025-12-09
 */

#include "file_info_impl.h"
#include <algorithm>
#include <sstream>
#include <cstring>

namespace foobar2000_sdk {

// 浣跨敤 xpumusic_sdk 涓殑绫诲瀷
using xpumusic_sdk::audio_info;
using xpumusic_sdk::file_stats;
using xpumusic_sdk::field_value;

//==============================================================================
// file_info_impl 瀹炵幇
//==============================================================================

file_info_impl::file_info_impl() : file_info_interface() {
    // 榛樿闊抽淇℃伅
    audio_info_.m_sample_rate = 44100;
    audio_info_.m_channels = 2;
    audio_info_.m_bitrate = 0;
    audio_info_.m_length = 0;
    
    // 榛樿鏂囦欢缁熻
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
    
    // fooar2000 瀛楁鍚嶉€氬父鏄ぇ灏忓啓涓嶆晱鎰熺殑
    // 杞崲涓哄皬鍐欒繘琛屽唴閮ㄥ瓨鍌?    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // 澶勭悊甯歌鍒悕
    if (normalized == "date" || normalized == "year") {
        return "date";  // 鏍囧噯鍖栦负 "date"
    }
    
    // 绉婚櫎棣栧熬绌烘牸
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
        // 鍒涘缓鏂板瓧娈?        xpumusic_sdk::field_value new_field;
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
        return nullptr;  // 瀛楁涓嶅瓨鍦?    }
    
    const auto& values = it->second.values;
    if (p_index >= values.size()) {
        return nullptr;  // 绱㈠紩瓒呭嚭鑼冨洿
    }
    
    return values[p_index].c_str();  // 杩斿洖 C 瀛楃涓?}

size_t file_info_impl::meta_get_count(const char* p_name) const {
    if (!p_name) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalized_name = normalize_field_name(p_name);
    auto it = meta_fields_.find(normalized_name);
    
    if (it == meta_fields_.end()) {
        return 0;  // 瀛楁涓嶅瓨鍦?    }
    
    return it->second.values.size();
}

bool file_info_impl::meta_set(const char* p_name, const char* p_value) {
    if (!p_name || !p_value) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    xpumusic_sdk::field_value& field = get_or_create_field(p_name);
    field.values.clear();          // 绉婚櫎鎵€鏈夌幇鏈夊€?    field.values.push_back(p_value); // 娣诲姞鏂板€?    field.cache_valid = false;     // 浣跨紦瀛樺け鏁?    
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
    field.values.push_back(p_value);  // 娣诲姞鍒板€煎垪琛?    field.cache_valid = false;        // 浣跨紦瀛樺け鏁?    
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
        return false;  // 瀛楁涓嶅瓨鍦?    }
    
    auto& values = it->second.values;
    auto value_it = std::find(values.begin(), values.end(), p_value);
    if (value_it == values.end()) {
        return false;  // 鍊间笉瀛樺湪
    }
    
    values.erase(value_it);  // 鍒犻櫎鍖归厤鐨勫€?    it->second.cache_valid = false;
    
    // 濡傛灉娌℃湁鍊间簡锛屽垹闄ゆ暣涓瓧娈?    if (values.empty()) {
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
        return false;  // 瀛楁涓嶅瓨鍦?    }
    
    auto& values = it->second.values;
    if (p_index >= values.size()) {
        return false;  // 绱㈠紩瓒呭嚭鑼冨洿
    }
    
    values.erase(values.begin() + p_index);  // 鍒犻櫎绱㈠紩澶勭殑鍊?    it->second.cache_valid = false;
    
    // 濡傛灉娌℃湁鍊间簡锛屽垹闄ゆ暣涓瓧娈?    if (values.empty()) {
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
    
    // 閲嶇疆闊抽淇℃伅涓洪粯璁ゅ€?    audio_info_.m_sample_rate = 44100;
    audio_info_.m_channels = 2;
    audio_info_.m_bitrate = 0;
    audio_info_.m_length = 0;
    
    // 閲嶇疆鏂囦欢缁熻
    stats_.m_size = 0;
    stats_.m_timestamp = 0;
}

void file_info_impl::copy_from(const file_info_interface& other) {
    // 浣跨敤鍏叡鎺ュ彛澶嶅埗鏁版嵁
    reset();  // 閲嶇疆

    // 澶嶅埗闊抽淇℃伅
    const xpumusic_sdk::audio_info& other_audio = other.get_audio_info();
    audio_info_.m_sample_rate = other_audio.m_sample_rate;
    audio_info_.m_channels = other_audio.m_channels;
    audio_info_.m_bitrate = other_audio.m_bitrate;
    audio_info_.m_length = other_audio.m_length;

    // 澶嶅埗鏂囦欢缁熻
    const xpumusic_sdk::file_stats& other_stats = other.get_file_stats();
    stats_.m_size = other_stats.m_size;
    stats_.m_timestamp = other_stats.m_timestamp;

    // 澶嶅埗鍏冩暟鎹瓧娈?    std::lock_guard<std::mutex> lock(mutex_);
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
    
    // 鍚堝苟闊抽淇℃伅锛堜繚鐣欓潪闆跺€硷級
    const xpumusic_sdk::audio_info& other_audio = other.get_audio_info();

    if (other_audio.m_sample_rate != 0) audio_info_.m_sample_rate = other_audio.m_sample_rate;
    if (other_audio.m_channels != 0) audio_info_.m_channels = other_audio.m_channels;
    if (other_audio.m_bitrate != 0) audio_info_.m_bitrate = other_audio.m_bitrate;
    if (other_audio.m_length != 0) audio_info_.m_length = other_audio.m_length;
    
    // 鍚堝苟鏂囦欢缁熻锛堜繚鐣欐渶澶у€硷級
    const xpumusic_sdk::file_stats& other_stats = other.get_file_stats();
    if (other_stats.m_size > stats_.m_size) stats_.m_size = other_stats.m_size;
    if (other_stats.m_timestamp > stats_.m_timestamp) stats_.m_timestamp = other_stats.m_timestamp;
    
    // 鍚堝苟闈炵┖瀛楁
    std::vector<std::string> field_names;
    // 鑾峰彇瀛楁鍚嶅垪琛?- 浣跨敤鎺ュ彛鏂规硶
    // 杩欓噷闇€瑕侀亶鍘嗗彲鑳界殑瀛楁鍚嶏紝鏆傛椂浣跨敤甯歌鐨勫瓧娈靛悕
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
                // 鍊煎凡缁忓瓨鍦ㄥ垯涓嶆坊鍔狅紙閬垮厤閲嶅锛?                bool exists = false;
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
    
    // 鍔ㄦ€佽浆鎹㈠埌 file_info_impl
    const file_info_impl* other_impl = dynamic_cast<const file_info_impl*>(&other);
    if (other_impl) {
        // 鏄?file_info_impl锛岀洿鎺ユ瘮杈?        std::lock_guard<std::mutex> lock_other(other_impl->mutex_);
        
        if (meta_fields_.size() != other_impl->meta_fields_.size()) {
            return false;
        }
        
        for (const auto& pair : meta_fields_) {
            auto it = other_impl->meta_fields_.find(pair.first);
            if (it == other_impl->meta_fields_.end()) {
                return false;  // 瀛楁鍚嶄笉鍖归厤
            }
            
            const auto& values = pair.second.values;
            const auto& other_values = it->second.values;
            
            if (values.size() != other_values.size()) {
                return false;  // 鍊兼暟閲忎笉鍖归厤
            }
            
            for (size_t i = 0; i < values.size(); ++i) {
                if (values[i] != other_values[i]) {
                    return false;  // 鍊间笉鍖归厤
                }
            }
        }
        
        return true;
    }
    
    // 涓嶆槸 file_info_impl锛屼娇鐢ㄥ姩鎬佹瘮杈?    if (meta_fields_.size() != other.meta_get_count("__field_count_hack__")) {
        return false;  // 瀛楁鏁伴噺涓嶅尮閰?    }
    
    for (const auto& pair : meta_fields_) {
        size_t count = other.meta_get_count(pair.first.c_str());
        if (count != pair.second.values.size()) {
            return false;  // 鍊兼暟閲忎笉鍖归厤
        }
        
        for (size_t i = 0; i < count; ++i) {
            const char* other_value = other.meta_get(pair.first.c_str(), i);
            if (!other_value || pair.second.values[i] != other_value) {
                return false;  // 鍊间笉鍖归厤
            }
        }
    }
    
    return true;
}

size_t file_info_impl::get_approximate_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t size = sizeof(*this);
    
    // 瀛楁鍚嶇殑杩戜技澶у皬
    for (const auto& pair : meta_fields_) {
        size += pair.first.capacity();  // 瀛楃涓插閲?        
        // 鍊煎瓧绗︿覆鐨勫ぇ灏?        for (const auto& value : pair.second.values) {
            size += value.capacity();
        }
    }
    
    return size;
}

// 杈呭姪鍑芥暟
std::unique_ptr<file_info_impl> file_info_create() {
    return std::make_unique<file_info_impl>();
}

// 娉ㄦ剰锛欵nhancedTrackInfo 鐩墠鍦ㄥ厓鏁版嵁鐨勫ご鏂囦欢涓墠鍚戝０鏄?// 杩欏皢鍦ㄩ噸鏋勫悗绉诲姩
#if 0
std::unique_ptr<file_info_impl> file_info_from_metadata(
    const EnhancedTrackInfo& track_info) {
    
    auto file_info = std::make_unique<file_info_impl>();
    
    // 澶嶅埗澧炲己鍏冩暟鎹埌 file_info
    // 杩欏皢鍦?EnhancedMetadata 绫诲畬鎴愬悗瀹炵幇
    
    return file_info;
}
#endif

} // namespace foobar2000_sdk
