/**
 * @file file_info_interface.h
 * @brief 鐙珛鐨?file_info 鎺ュ彛瀹氫箟锛堢敤浜庨伩鍏嶅惊鐜緷璧栵級
 * @date 2025-12-09
 */

#pragma once

#include "../xpumusic_sdk/foobar2000_sdk.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>

// 鍓嶅悜澹版槑 xpumusic_sdk 涓殑绫诲瀷
namespace xpumusic_sdk {
struct audio_info;
struct file_stats;
struct field_value;
}

namespace foobar2000_sdk {

/**
 * @class file_info_interface
 * @brief file_info 鎶借薄鎺ュ彛锛堢函铏氱被锛? */
class file_info_interface {
public:
    virtual ~file_info_interface() = default;
    
    // 榛樿鏋勯€犲嚱鏁?    file_info_interface() = default;
    
    /**
     * @brief 浠庡厓鏁版嵁鑾峰彇瀛楁鍊?     * @param p_name 瀛楁鍚嶏紙渚嬪 "artist", "title"锛?     * @param p_index 鍊肩储寮曪紙瀵逛簬澶氬€煎瓧娈碉級
     * @return 瀛楁鍊硷紝濡傛灉涓嶅瓨鍦ㄨ繑鍥?nullptr
     */
    virtual const char* meta_get(const char* p_name, size_t p_index) const = 0;
    
    /**
     * @brief 鑾峰彇瀛楁鐨勫€兼暟閲?     * @param p_name 瀛楁鍚?     * @return 鍊肩殑鏁伴噺锛屽瓧娈典笉瀛樺湪杩斿洖 0
     */
    virtual size_t meta_get_count(const char* p_name) const = 0;
    
    /**
     * @brief 璁剧疆瀛楁鍊硷紙鏇挎崲鎵€鏈夌幇鏈夊€硷級
     * @param p_name 瀛楁鍚?     * @param p_value 鏂板€?     * @return true 濡傛灉瀛楁琚慨鏀?     */
    virtual bool meta_set(const char* p_name, const char* p_value) = 0;
    
    /**
     * @brief 绉婚櫎瀛楁鐨勬墍鏈夊€?     * @param p_name 瀛楁鍚?     * @return true 濡傛灉瀛楁瀛樺湪骞惰绉婚櫎
     */
    virtual bool meta_remove(const char* p_name) = 0;
    
    /**
     * @brief 娣诲姞瀛楁鍊硷紙澧炲姞鍒扮幇鏈夊€煎垪琛級
     * @param p_name 瀛楁鍚?     * @param p_value 瑕佹坊鍔犵殑鍊?     * @return true 濡傛灉鍊兼垚鍔熸坊鍔?     */
    virtual bool meta_add(const char* p_name, const char* p_value) = 0;
    
    /**
     * @brief 鍒犻櫎瀛楁鐨勭壒瀹氬€?     * @param p_name 瀛楁鍚?     * @param p_value 瑕佸垹闄ょ殑鍊?     * @return true 濡傛灉鍊煎瓨鍦ㄥ苟琚垹闄?     */
    virtual bool meta_remove_value(const char* p_name, const char* p_value) = 0;
    
    /**
     * @brief 鍒犻櫎瀛楁鐨勭壒瀹氱储寮曞€?     * @param p_name 瀛楁鍚?     * @param p_index 鍊肩储寮?     * @return true 濡傛灉鍊煎瓨鍦ㄥ苟琚垹闄?     */
    virtual bool meta_remove_index(const char* p_name, size_t p_index) = 0;
    
    /**
     * @brief 鑾峰彇鎵€鏈夊瓧娈靛悕
     * @return 瀛楁鍚嶅悜閲?     */
    virtual std::vector<std::string> meta_enum_field_names() const = 0;
    
    /**
     * @brief 璁剧疆闊抽娴佷俊鎭?     * @param p_info 闊抽淇℃伅
     */
    virtual void set_audio_info(const xpumusic_sdk::audio_info& p_info) = 0;
    
    /**
     * @brief 鑾峰彇闊抽娴佷俊鎭紙const 鐗堟湰锛?     * @return 闊抽淇℃伅寮曠敤
     */
    virtual const xpumusic_sdk::audio_info& get_audio_info() const = 0;
    
    /**
     * @brief 鑾峰彇闊抽娴佷俊鎭紙闈?const 鐗堟湰锛?     * @return 闊抽淇℃伅寮曠敤
     */
    virtual xpumusic_sdk::audio_info& get_audio_info() = 0;
    
    /**
     * @brief 璁剧疆鏂囦欢缁熻淇℃伅锛堝ぇ灏忋€佹椂闂存埑锛?     * @param p_stats 鏂囦欢缁熻
     */
    virtual void set_file_stats(const xpumusic_sdk::file_stats& p_stats) = 0;
    
    /**
     * @brief 鑾峰彇鏂囦欢缁熻淇℃伅锛坈onst 鐗堟湰锛?     * @return 鏂囦欢缁熻寮曠敤
     */
    virtual const xpumusic_sdk::file_stats& get_file_stats() const = 0;
    
    /**
     * @brief 鑾峰彇鏂囦欢缁熻淇℃伅锛堥潪 const 鐗堟湰锛?     * @return 鏂囦欢缁熻寮曠敤
     */
    virtual xpumusic_sdk::file_stats& get_file_stats() = 0;
    
    /**
     * @brief 閲嶇疆鎵€鏈夊厓鏁版嵁锛堟竻绌烘墍鏈夊瓧娈碉級
     */
    virtual void reset() = 0;
    
    /**
     * @brief 浠庡彟涓€涓?file_info 瀵硅薄澶嶅埗鎵€鏈夋暟鎹?     * @param other 婧愬璞?     */
    virtual void copy_from(const file_info_interface& other) = 0;
    
    /**
     * @brief 浠庡彟涓€涓?file_info 瀵硅薄鍚堝苟闈炵┖瀛楁
     * @param other 婧愬璞?     * @details 淇濈暀鐜版湁鍊硷紝娣诲姞缂哄け鐨勫瓧娈?     */
    virtual void merge_from(const file_info_interface& other) = 0;
    
    /**
     * @brief 姣旇緝涓や釜 file_info 瀵硅薄鐨勫厓鏁版嵁鏄惁鐩哥瓑
     * @param other 瑕佹瘮杈冪殑瀵硅薄
     * @return true 濡傛灉鍏冩暟鎹浉绛?     */
    virtual bool meta_equals(const file_info_interface& other) const = 0;
};

} // namespace foobar2000_sdk
