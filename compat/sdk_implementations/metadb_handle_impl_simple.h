/**
 * @file metadb_handle_impl_simple.h
 * @brief 绠€鍖栫殑 metadb_handle 瀹炵幇
 * @date 2025-12-11
 * 
 * 杩欐槸 metadb_handle 鐨勬渶灏忓寲瀹炵幇锛屽彧鎻愪緵鍩烘湰鍔熻兘浠ラ€氳繃缂栬瘧銆?
 */

#pragma once

#include "common_includes.h"
#include "metadb_handle_interface.h"
#include "file_info_impl.h"
#include "metadb_handle_types.h"
#include <memory>
#include <string>
#include <mutex>

namespace foobar2000_sdk {

/**
 * @class metadb_handle_impl_simple
 * @brief metadb_handle 鐨勭畝鍖栧疄鐜?
 * 
 * 鍙彁渚涙渶鍩烘湰鐨勫姛鑳斤紝纭繚椤圭洰鑳界紪璇戦€氳繃銆?
 */
class metadb_handle_impl_simple : public metadb_handle_interface {
private:
    // 鏂囦欢浣嶇疆
    playable_location location_;
    
    // 缂撳瓨鐨勫厓鏁版嵁
    std::unique_ptr<file_info_impl> info_;
    
    // 鏂囦欢缁熻淇℃伅
    xpumusic_sdk::file_stats file_stats_;
    
    // 浜掓枼閿侊紝鐢ㄤ簬绾跨▼瀹夊叏
    mutable std::mutex mutex_;
    
public:
    /**
     * @brief 鏋勯€犲嚱鏁?
     */
    metadb_handle_impl_simple() : metadb_handle_interface() {
        info_ = std::make_unique<file_info_impl>();
        file_stats_.m_size = 0;
        file_stats_.m_timestamp = 0;
    }
    
    /**
     * @brief 鏋愭瀯鍑芥暟
     */
    ~metadb_handle_impl_simple() override = default;
    
    /**
     * @brief 鐢?playable_location 鍒濆鍖?
     * @param loc 鏂囦欢浣嶇疆
     */
    void initialize(const playable_location& loc) {
        std::lock_guard<std::mutex> lock(mutex_);
        location_ = loc;
    }
    
    /**
     * @brief 鑾峰彇鏂囦欢浣嶇疆锛坈onst 鐗堟湰锛?
     * @return 浣嶇疆鐨勬寚閽?
     */
    const playable_location& get_location() const override { 
        std::lock_guard<std::mutex> lock(mutex_);
        return location_; 
    }
    
    /**
     * @brief 鑾峰彇鍏冩暟鎹紙const 鐗堟湰锛?
     * @return file_info 鐨勫紩鐢?
     */
    const file_info_interface& get_info() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return *info_;
    }
    
    /**
     * @brief 鑾峰彇鍏冩暟鎹紙闈?const 鐗堟湰锛?
     * @return file_info 鐨勫紩鐢?
     */
    file_info_interface& get_info() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return *info_;
    }
    
    /**
     * @brief 浠庡疄鏃舵枃浠惰幏鍙栧厓鏁版嵁锛堢畝鍖栫増鏈級
     * @param p_info 杈撳嚭 file_info
     * @param p_abort 涓鍥炶皟
     * @param p_can_expire 鏄惁鍏佽杩囨湡缂撳瓨
     */
    void get_info_async(file_info_interface& p_info, xpumusic_sdk::abort_callback& p_abort, 
                       bool p_can_expire = false) const override {
        // 绠€鍖栧疄鐜帮細鐩存帴澶嶅埗褰撳墠淇℃伅
        std::lock_guard<std::mutex> lock(mutex_);
        p_info.copy_from(*info_);
    }
    
    /**
     * @brief 鏇存柊鍏冩暟鎹?
     * @param p_info 鏂扮殑鍏冩暟鎹?
     * @param p_abort 涓鍥炶皟
     */
    void update_info(const file_info_interface& p_info, xpumusic_sdk::abort_callback& p_abort) override {
        std::lock_guard<std::mutex> lock(mutex_);
        info_->copy_from(p_info);
    }
    
    /**
     * @brief 浠庢枃浠跺埛鏂板厓鏁版嵁锛堢畝鍖栫増鏈級
     * @param p_abort 涓鍥炶皟
     */
    void refresh_info(xpumusic_sdk::abort_callback& p_abort) override {
        // 绠€鍖栧疄鐜帮細浠€涔堥兘涓嶅仛
        // 鍦ㄥ疄闄呭疄鐜颁腑锛岃繖閲屼細閲嶆柊鎵弿鏂囦欢
    }
    
    /**
     * @brief 鑾峰彇鏂囦欢缁熻淇℃伅
     * @return 鏂囦欢缁熻
     */
    const xpumusic_sdk::file_stats& get_file_stats() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return file_stats_;
    }
    
    /**
     * @brief 鑾峰彇璺緞
     * @return 鏂囦欢璺緞
     */
    std::string get_path() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return location_.get_path();
    }
    
    /**
     * @brief 鑾峰彇鏂囦欢鍚?
     * @return 鏂囦欢鍚?
     */
    std::string get_filename() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string path = location_.get_path();
        size_t pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }
    
    /**
     * @brief 鑾峰彇鐩綍
     * @return 鐩綍璺緞
     */
    std::string get_directory() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string path = location_.get_path();
        size_t pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(0, pos) : "";
    }
    
    /**
     * @brief 鑾峰彇浣嶇疆鍝堝笇
     * @return 浣嶇疆鐨勫搱甯屽€?
     */
    uint64_t get_location_hash() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::hash<std::string> hasher;
        return hasher(location_.get_path()) ^ (location_.get_subsong_index() << 32);
    }
    
    /**
     * @brief 妫€鏌ユ槸鍚︿笌鍙︿竴涓?handle 鐩稿悓
     * @param other 鍙︿竴涓?handle
     * @return true 濡傛灉鐩稿悓
     */
    bool is_same(const metadb_handle_interface& other) const override {
        return get_location_hash() == other.get_location_hash();
    }
    
    /**
     * @brief 妫€鏌ユ槸鍚︽湁鏁?
     * @return true 濡傛灉鏈夋晥
     */
    bool is_valid() const override {
        return !location_.get_path().empty();
    }
    
    /**
     * @brief 閲嶆柊鍔犺浇锛堢畝鍖栫増鏈級
     * @param p_abort 涓鍥炶皟
     */
    void reload(xpumusic_sdk::abort_callback& p_abort) override {
        refresh_info(p_abort);
    }
    
    /**
     * @brief 寮曠敤璁℃暟澧炲姞锛堢畝鍖栫増鏈級
     * 鍦ㄥ畬鏁村疄鐜颁腑锛岃繖浼氫娇鐢ㄥ疄闄呯殑寮曠敤璁℃暟
     */
    void ref_add_ref() override {
        // 绠€鍖栫増鏈細浠€涔堥兘涓嶅仛
    }
    
    /**
     * @brief 寮曠敤璁℃暟閲婃斁锛堢畝鍖栫増鏈級
     * 鍦ㄥ畬鏁村疄鐜颁腑锛岃繖浼氬噺灏戝紩鐢ㄨ鏁板苟鍦ㄩ渶瑕佹椂鍒犻櫎瀵硅薄
     */
    void ref_release() override {
        // 绠€鍖栫増鏈細浠€涔堥兘涓嶅仛
    }
};

// 绫诲瀷鍒悕锛屼繚鎸佸吋瀹规€?
typedef metadb_handle_impl_simple metadb_handle_impl;

} // namespace foobar2000_sdk