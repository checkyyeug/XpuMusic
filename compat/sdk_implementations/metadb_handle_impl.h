/**
 * @file metadb_handle_impl.h
 * @brief metadb_handle 鎺ュ彛瀹炵幇
 * @date 2025-12-09
 * 
 * metadb_handle 鏄?foobar2000 鍏冩暟鎹郴缁熺殑鏍稿績锛岃〃绀哄崟涓煶棰戞枃浠剁殑
 * 鍏冩暟鎹拰缁熻淇℃伅銆傚畠鍚屾椂鏀寔瀹炴椂鍜屾寔涔呭寲鍏冩暟鎹€?
 */

#pragma once

#include "common_includes.h"
#include "metadb_handle_interface.h"
#include "file_info_impl.h"
#include "file_info_types.h"
#include "metadb_handle_types.h"
#include "../../sdk/headers/mp_types.h"
#include <memory>
#include <string>
#include <chrono>
#include <mutex>

namespace xpumusic_sdk {

// 浣跨敤 xpumusic_sdk 涓殑绫诲瀷
using xpumusic_sdk::abort_callback;

// 鍓嶅悜澹版槑
class metadb_impl;

/**
 * @class metadb_handle_impl
 * @brief metadb_handle 鎺ュ彛鐨勫疄鐜?
 * 
 * metadb_handle 琛ㄧず鍗曚釜闊抽鏂囦欢鐨勬寔涔呭寲鍏冩暟鎹拰瀹炴椂鐘舵€併€?
 * 瀹冩敮鎸侊細
 * - 澶氬€煎厓鏁版嵁瀛楁
 * - 鎾斁缁熻锛堟挱鏀炬鏁般€佹渶鍚庢挱鏀剧瓑锛?
 * - 鍔ㄦ€佸厓鏁版嵁鏇存柊
 * - 鏂囦欢鏍囪瘑鍜岄獙璇?
 */
class metadb_handle_impl : public metadb_handle_interface {
private:
    // 鏂囦欢浣嶇疆
    playable_location location_;
    
    // 缂撳瓨鐨勫厓鏁版嵁锛堝彲鏇存柊锛?
    std::unique_ptr<file_info_impl> info_;
    
    // 鎾斁缁熻锛堜娇鐢ㄦ帴鍙ｄ腑瀹氫箟鐨勭粨鏋勶級
    TrackStatistics stats_;
    
    // 鏂囦欢鏍囪瘑锛堢敤浜庢娴嬫枃浠跺彉鍖栵級
    xpumusic_sdk::file_stats file_stats_;
    
    // 鎸囧悜 metadb 鐨勬寚閽堬紙鐢ㄤ簬鍙嶅悜寮曠敤锛?
    metadb_impl* parent_db_ = nullptr;
    
    // 浜掓枼閿侊紝鐢ㄤ簬绾跨▼瀹夊叏
    mutable std::mutex mutex_;
    
    // 鏄惁宸插垵濮嬪寲鏍囧織
    bool initialized_ = false;
    
    // 鍔犺浇鍏冩暟鎹粠鏂囦欢
    Result load_metadata_from_file();
    
public:
    /**
     * @brief 鏋勯€犲嚱鏁?
     */
    metadb_handle_impl();
    
    /**
     * @brief 鏋愭瀯鍑芥暟
         */
    ~metadb_handle_impl() override = default;
    
    /**
     * @brief 鐢?playable_location 鍒濆鍖?
     * @param loc 鏂囦欢浣嶇疆
     * @param parent 鐖?metadb锛堝彲閫夛級
     */
    Result initialize(const playable_location& loc, metadb_impl* parent = nullptr);
    
    /**
     * @brief 閲嶇疆鐘舵€侊紙娓呴櫎鎵€鏈夋暟鎹級
     */
    void reset();
    
    //==========================================================================
    // metadb_handle 鎺ュ彛瀹炵幇
    //==========================================================================
    
    /**
     * @brief 鑾峰彇鏂囦欢浣嶇疆锛坈onst 鐗堟湰锛?
     * @return 浣嶇疆鐨勬寚閽?
     */
    const playable_location& get_location() const override { return location_; }
    
    /**
     * @brief 鑾峰彇鍏冩暟鎹紙const 鐗堟湰锛?
     * @return file_info 鐨勫紩鐢?
     * 
     * @note 澶氱嚎绋嬭皟鐢ㄦ椂绾跨▼瀹夊叏
     * @note 杩斿洖瀵硅薄鐨勫唴閮ㄦ暟鎹彲鑳借鍏朵粬璋冪敤淇敼
     */
    const file_info_interface& get_info() const override;
    
    /**
     * @brief 鑾峰彇鍏冩暟鎹紙闈?const 鐗堟湰锛?
     * @return file_info 鐨勫紩鐢?
     */
    file_info_interface& get_info() override;
    
    /**
     * @brief 浠庡疄鏃舵枃浠惰幏鍙栧厓鏁版嵁锛堝彲鑳芥壂鎻忥級
     * @param p_info 杈撳嚭 file_info
     * @param p_abort 涓鍥炶皟
     * @param p_can_expire 鏄惁鍏佽杩囨湡缂撳瓨
     */
    void get_info_async(file_info_interface& p_info, abort_callback& p_abort, 
                       bool p_can_expire = false) const override;
    
    //==========================================================================
    // 缁熻淇℃伅璁块棶
    //==========================================================================
    
    /**
     * @brief 鑾峰彇鎾斁缁熻锛坈onst 鐗堟湰锛?
     * @return TrackStatistics 鐨勫紩鐢?
     */
    const TrackStatistics& get_statistics() const { return stats_; }
    
    /**
     * @brief 鑾峰彇鎾斁缁熻锛堥潪 const 鐗堟湰锛?
     * @return TrackStatistics 鐨勫紩鐢?
     */
    TrackStatistics& get_statistics() { return stats_; }
    
    /**
     * @brief 鏇存柊鍏冩暟鎹?
     * @param p_info 鏂扮殑鍏冩暟鎹?
     * @param p_abort 涓鍥炶皟
     * 
     * @note 杩欎細淇敼鍐呴儴 file_info 骞跺彲鑳藉啓鍏ユ暟鎹簱
     */
    void update_info(const file_info_interface& p_info, abort_callback& p_abort) override;
    
    /**
     * @brief 浠庢枃浠跺埛鏂板厓鏁版嵁锛堝鏋滄枃浠跺凡鏇存敼锛?
     * @param p_abort 涓鍥炶皟
     */
    void refresh_info(abort_callback& p_abort) override;
    
    //==========================================================================
    // 瀹炵敤鍑芥暟
    //==========================================================================
    
    /**
     * @brief 鑾峰彇鏂囦欢鍚嶏紙浠庝綅缃彁鍙栵級
     * @return 鏂囦欢鍚?
     */
    std::string get_filename() const;
    
    /**
     * @brief 鑾峰彇鐩綍璺緞
     * @return 鐩綍璺緞
     */
    std::string get_directory() const;
    
    /**
     * @brief 妫€鏌ユ枃浠舵槸鍚﹀瓨鍦ㄥ苟鍙闂?
     * @return true 濡傛灉鏂囦欢鍙闂?
     */
    bool is_file_valid() const;
    
    /**
     * @brief 璁剧疆鐖?metadb
     * @param parent 鐖?metadb_impl
     */
    void set_parent(metadb_impl* parent) { parent_db_ = parent; }
    
    /**
     * @brief 鑾峰彇鐖?metadb
     * @return 鐖?metadb_impl 鎸囬拡
     */
    metadb_impl* get_parent() const { return parent_db_; }
    
    /**
     * @brief 鑾峰彇鍞竴鏍囪瘑瀛楃涓?
     * @return 鏂囦欢鐨勫敮涓€鏍囪瘑
     */
    std::string get_identifier() const;
    
    /**
     * @brief 妫€鏌ヤ袱涓?handle 鏄惁鎸囧悜鍚屼竴鏂囦欢
     * @param other 鍙︿竴涓?metadb_handle
     * @return true 濡傛灉鎸囧悜鍚屼竴鏂囦欢
     */
    bool equals(const metadb_handle_impl& other) const;
};

// 杈呭姪姣旇緝鍑芥暟
inline bool operator==(const metadb_handle_impl& a, const metadb_handle_impl& b) {
    return a.equals(b);
}

inline bool operator!=(const metadb_handle_impl& a, const metadb_handle_impl& b) {
    return !a.equals(b);
}

// 杈呭姪鍑芥暟锛氬垱寤?metadb_handle
std::unique_ptr<metadb_handle_impl> metadb_handle_create(const playable_location& loc);

} // namespace foobar2000_sdk
