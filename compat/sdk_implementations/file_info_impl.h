/**
 * @file file_info_impl.h
 * @brief file_info 鎺ュ彛鐨勫疄鐜帮紝鏀寔澶氬€煎瓧娈? * @date 2025-12-09
 * 
 * 杩欐槸鍏冩暟鎹吋瀹规€х殑鏍稿績銆俧oobar2000 鏀寔澶氬€煎瓧娈靛拰澶嶆潅鍏冩暟鎹搷浣滐紝
 * 鑰屽綋鍓嶉」鐩粎鏀寔绠€鍗曠殑閿€煎銆傛湰瀹炵幇妗ユ帴杩欎竴宸窛銆? */

#pragma once

#include "common_includes.h"
#include "file_info_interface.h"
#include "file_info_types.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

// 鍓嶅悜澹版槑 xpumusic_sdk 涓殑绫诲瀷
namespace xpumusic_sdk {
struct audio_info;
struct file_stats;
struct field_value;
}

namespace foobar2000_sdk {

// 浣跨敤 xpumusic_sdk 涓殑绫诲瀷
using xpumusic_sdk::audio_info;
using xpumusic_sdk::file_stats;
using xpumusic_sdk::field_value;

/**
 * @class file_info_impl
 * @brief file_info 鎺ュ彛鐨勫畬鏁村疄鐜? * 
 * 鍏抽敭鐗规€э細
 * - 鏀寔澶氬€煎瓧娈碉紙tagger::["artist1", "artist2"]锛? * - 鍐呯疆鐨勯煶棰戜俊鎭拰鏂囦欢缁熻
 * - 绾跨▼瀹夊叏鐨勫厓鏁版嵁鎿嶄綔
 * - 涓?foobar2000 鏌ヨ璇硶鐨勫吋瀹规€? */
class file_info_impl : public file_info_interface {
public:
    // 鏋勯€犲嚱鏁?    file_info_impl();
    
    // 澶嶅埗鏋勯€犲嚱鏁?    file_info_impl(const file_info_impl& other);
    
    // 璧嬪€兼搷浣滅
    file_info_impl& operator=(const file_info_impl& other);
    
    // 鏋愭瀯鍑芥暟
    ~file_info_impl() override = default;

private:
    // 鍏冩暟鎹瓧娈垫槧灏勶細瀛楁鍚?-> 鍊煎垪琛?    std::unordered_map<std::string, xpumusic_sdk::field_value> meta_fields_;
    
    // 闊抽淇℃伅锛堥噰鏍风巼銆侀€氶亾鏁扮瓑锛?    xpumusic_sdk::audio_info audio_info_;
    
    // 鏂囦欢缁熻淇℃伅锛堝ぇ灏忋€佹椂闂存埑锛?    xpumusic_sdk::file_stats stats_;
    
    // 浜掓枼閿侊紝鐢ㄤ簬绾跨▼瀹夊叏鎿嶄綔
    mutable std::mutex mutex_;
    
    /**
     * @brief 鍐呴儴杈呭姪锛氳幏鍙栨垨鍒涘缓瀛楁
     * @param name 瀛楁鍚?     * @return 瀛楁鍊肩殑寮曠敤
     */
    xpumusic_sdk::field_value& get_or_create_field(const char* name);
    
    /**
     * @brief 鍐呴儴杈呭姪锛氳鑼冨寲瀛楁鍚?     * @param name 鍘熷瀛楁鍚?     * @return 瑙勮寖鍖栧悗鐨勫瓧娈靛悕
     * 
     * fooar2000 瀛楁鍚嶉€氬父鏄ぇ灏忓啓涓嶆晱鎰熺殑
     * 骞朵笖鏈変竴浜涘埆鍚嶏紙渚嬪 "date" 鍜?"year"锛?     */
    std::string normalize_field_name(const std::string& name) const;

public:
    
    //==================================================================
    // file_info 鎺ュ彛瀹炵幇
    //==================================================================
    
    /**
     * @brief 浠庡厓鏁版嵁鑾峰彇瀛楁鍊?     * @param p_name 瀛楁鍚嶏紙渚嬪 "artist", "title"锛?     * @param p_index 鍊肩储寮曪紙瀵逛簬澶氬€煎瓧娈碉級
     * @return 瀛楁鍊硷紝濡傛灉涓嶅瓨鍦ㄨ繑鍥?nullptr
     */
    const char* meta_get(const char* p_name, size_t p_index) const override;
    
    /**
     * @brief 鑾峰彇瀛楁鐨勫€兼暟閲?     * @param p_name 瀛楁鍚?     * @return 鍊肩殑鏁伴噺锛屽瓧娈典笉瀛樺湪杩斿洖 0
     */
    size_t meta_get_count(const char* p_name) const override;
    
    /**
     * @brief 璁剧疆瀛楁鍊硷紙鏇挎崲鎵€鏈夌幇鏈夊€硷級
     * @param p_name 瀛楁鍚?     * @param p_value 鏂板€?     * @return true 濡傛灉瀛楁琚慨鏀?     */
    bool meta_set(const char* p_name, const char* p_value) override;
    
    /**
     * @brief 绉婚櫎瀛楁鐨勬墍鏈夊€?     * @param p_name 瀛楁鍚?     * @return true 濡傛灉瀛楁瀛樺湪骞惰绉婚櫎
     */
    bool meta_remove(const char* p_name) override;
    
    /**
     * @brief 娣诲姞瀛楁鍊硷紙澧炲姞鍒扮幇鏈夊€煎垪琛級
     * @param p_name 瀛楁鍚?     * @param p_value 瑕佹坊鍔犵殑鍊?     * @return true 濡傛灉鍊兼垚鍔熸坊鍔?     */                               
    bool meta_add(const char* p_name, const char* p_value) override;
    
    /**
     * @brief 鍒犻櫎瀛楁鐨勭壒瀹氬€?     * @param p_name 瀛楁鍚?     * @param p_value 瑕佸垹闄ょ殑鍊?     * @return true 濡傛灉鍊煎瓨鍦ㄥ苟琚垹闄?     */
    bool meta_remove_value(const char* p_name, const char* p_value) override;
    
    /**
     * @brief 鍒犻櫎瀛楁鐨勭壒瀹氱储寮曞€?     * @param p_name 瀛楁鍚?     * @param p_index 鍊肩储寮?     * @return true 濡傛灉鍊煎瓨鍦ㄥ苟琚垹闄?     */
    bool meta_remove_index(const char* p_name, size_t p_index) override;
    
    /**
     * @brief 鑾峰彇鎵€鏈夊瓧娈靛悕
     * @return 瀛楁鍚嶅悜閲?     */
    std::vector<std::string> meta_enum_field_names() const override;
    
    //==================================================================
    // 闊抽淇℃伅璁块棶
    //==================================================================
    
    /**
     * @brief 璁剧疆闊抽娴佷俊鎭?     * @param info 闊抽淇℃伅
     */
    void set_audio_info(const xpumusic_sdk::audio_info& info) override {
        std::lock_guard<std::mutex> lock(mutex_);
        audio_info_ = info;
    }
    
    /**
     * @brief 鑾峰彇闊抽娴佷俊鎭紙const 鐗堟湰锛?     * @return 闊抽淇℃伅寮曠敤
     */
    const xpumusic_sdk::audio_info& get_audio_info() const override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return audio_info_; 
    }
    
    /**
     * @brief 鑾峰彇闊抽娴佷俊鎭紙闈?const 鐗堟湰锛?     * @return 闊抽淇℃伅寮曠敤
     */
    xpumusic_sdk::audio_info& get_audio_info() override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return audio_info_; 
    }
    
    //==================================================================
    // 鏂囦欢缁熻淇℃伅
    //==================================================================
    
    /**
     * @brief 璁剧疆鏂囦欢缁熻淇℃伅锛堝ぇ灏忋€佹椂闂存埑锛?     * @param stats 鏂囦欢缁熻
     */
    void set_file_stats(const xpumusic_sdk::file_stats& stats) override { 
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = stats; 
    }
    
    /**
     * @brief 鑾峰彇鏂囦欢缁熻淇℃伅锛坈onst 鐗堟湰锛?     * @return 鏂囦欢缁熻寮曠敤
     */
    const xpumusic_sdk::file_stats& get_file_stats() const override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_; 
    }
    
    /**
     * @brief 鑾峰彇鏂囦欢缁熻淇℃伅锛堥潪 const 鐗堟湰锛?     * @return 鏂囦欢缁熻寮曠敤
     */
    xpumusic_sdk::file_stats& get_file_stats() override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_; 
    }
    
    /**
     * @brief 閲嶇疆鎵€鏈夊厓鏁版嵁锛堟竻绌烘墍鏈夊瓧娈碉級
     */
    void reset() override;
    
    /**
     * @brief 浠庡彟涓€涓?file_info 瀵硅薄澶嶅埗鎵€鏈夋暟鎹?     * @param other 婧愬璞?     */
    void copy_from(const file_info_interface& other) override;
    
    /**
     * @brief 浠庡彟涓€涓?file_info 瀵硅薄鍚堝苟闈炵┖瀛楁
     * @param other 婧愬璞?     * @details 淇濈暀鐜版湁鍊硷紝娣诲姞缂哄け鐨勫瓧娈?     */
    void merge_from(const file_info_interface& other) override;
    
    /**
     * @brief 姣旇緝涓や釜 file_info 瀵硅薄鐨勫厓鏁版嵁鏄惁鐩哥瓑
     * @param other 瑕佹瘮杈冪殑瀵硅薄
     * @return true 濡傛灉鍏冩暟鎹浉绛?     */
    bool meta_equals(const file_info_interface& other) const override;
    
    /**
     * @brief 鑾峰彇瀛楄妭澶у皬锛堢敤浜庤皟璇曞拰搴忓垪鍖栵級
     * @return 瀵硅薄鐨勬€诲ぇ灏忥紙杩戜技鍊硷級
     */
    size_t get_approximate_size() const;
};

// 绠€鍖栨搷浣滆緟鍔╁嚱鏁?std::unique_ptr<file_info_impl> file_info_create();

// 浠庡寮哄厓鏁版嵁鍒涘缓 file_info锛堟ˉ鎺ュ嚱鏁帮級
std::unique_ptr<file_info_impl> file_info_from_metadata(
    const class EnhancedTrackInfo& track_info);

} // namespace foobar2000_sdk
