/**
 * @file metadb_handle_impl.cpp
 * @brief metadb_handle_impl 瀹炵幇
 * @date 2025-12-09
 */

#include "metadb_handle_impl.h"
#include <algorithm>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#endif

namespace foobar2000_sdk {

// 浣跨敤 xpumusic_sdk 涓殑绫诲瀷
using xpumusic_sdk::abort_callback;

// TrackStatistics 瀹炵幇
typedef std::chrono::system_clock::time_point time_point;
typedef std::chrono::system_clock::duration duration;

static uint64_t get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

//==============================================================================
// metadb_handle_impl 瀹炵幇
//==============================================================================

metadb_handle_impl::metadb_handle_impl() {
    reset();
}

void metadb_handle_impl::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    location_ = playable_location();
    info_ = std::make_unique<file_info_impl>();
    stats_ = TrackStatistics();
    file_stats_ = xpumusic_sdk::file_stats();
    parent_db_ = nullptr;
    initialized_ = false;
}

Result metadb_handle_impl::initialize(const playable_location& loc, metadb_impl* parent) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    reset();
    
    location_ = loc;
    parent_db_ = parent;
    
    // 灏濊瘯浠庢枃浠跺姞杞藉厓鏁版嵁
    Result result = load_metadata_from_file();
    if (result != Result::Success) {
        // 鍙互娌℃湁鏈夋晥鍏冩暟鎹户缁?
        // 鏍囪涓哄垵濮嬪寲浣嗕粛鍙敤
    }
    
    initialized_ = true;
    return Result::Success;
}

Result metadb_handle_impl::load_metadata_from_file() {
    // 杩欎釜瀹炵幇鏄畝鍖栫殑
    // 鐪熸鐨勫疄鐜伴渶瑕佽皟鐢ㄨВ鐮佸櫒鏉ヨ幏鍙栧厓鏁版嵁
    
    const std::string& path = location_.get_path();
    if (!path || strlen(path) == 0) {
        return Result::InvalidParameter;
    }
    
    // 鑾峰彇鏂囦欢缁熻锛堝ぇ灏忋€佹椂闂存埑锛?
#ifdef _WIN32
    struct _stat64 st;
    if (_stat64(path, &st) != 0) {
        return Result::FileNotFound;
    }
#else
    struct stat st;
    if (stat(path, &st) != 0) {
        return Result::FileNotFound;
    }
#endif
    
    file_stats_.m_size = st.st_size;
    file_stats_.m_timestamp = st.st_mtime;
    
    info_->set_file_stats(file_stats_);
    
    // 娉ㄦ剰锛氳繖閲屾病鏈夊疄闄呰鍙栭煶棰戞枃浠剁殑鍏冩暟鎹?
    // 鍦ㄥ疄闄呯郴缁熶腑锛岃繖浼氳皟鐢ㄨВ鐮佸櫒鐨?get_info 鏂规硶
    // 涓虹畝鍗曡捣瑙侊紝鎴戜滑鍙～鍏呬簡涓€浜涘熀纭€淇℃伅
    
    return Result::Success;
}

const file_info_interface& metadb_handle_impl::get_info() const {
    // 娉ㄦ剰锛氳繑鍥炵殑寮曠敤鍙兘鍦ㄦ病鏈夐攣鐨勬儏鍐典笅琚慨鏀?
    // 鍦ㄥ疄闄?foobar2000 涓紝杩欓渶瑕佹洿澶嶆潅鐨勭嚎绋嬪畨鍏ㄦ満鍒?
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!info_) {
        static const file_info_impl empty_info;
        return empty_info;
    }
    
    return *info_;
}

file_info_interface& metadb_handle_impl::get_info() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!info_) {
        info_ = std::make_unique<file_info_impl>();
    }
    
    return *info_;
}

void metadb_handle_impl::get_info_async(file_info_interface& p_info, abort_callback& p_abort, 
                                       bool p_can_expire) const {
    // 寮傛鐗堟湰锛氱畝鍗曞湴濮旀墭缁欏悓姝ョ増鏈?
    // 鍦ㄧ湡瀹?foobar2000 涓紝杩欎細鍦ㄥ悗鍙扮嚎绋嬫墽琛?
    (void)p_abort;
    (void)p_can_expire;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (info_) {
        p_info.copy_from(*info_);
    }
}

void metadb_handle_impl::update_info(const file_info_interface& p_info, abort_callback& p_abort) {
    (void)p_abort;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!info_) {
        info_ = std::make_unique<file_info_impl>();
    }
    
    info_->copy_from(p_info);
    
    // 鍦ㄥ疄闄呯郴缁熶腑锛岃繖浼氳Е鍙戞暟鎹簱鏇存柊鍜岄€氱煡
    // 鐜板湪鍙槸绠€鍗曚繚瀛?
}

void metadb_handle_impl::refresh_info(abort_callback& p_abort) {
    // 閲嶆柊浠庢枃浠跺姞杞藉厓鏁版嵁
    (void)p_abort;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    load_metadata_from_file();
}

std::string metadb_handle_impl::get_filename() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const std::string& path = location_.get_path();
    if (!path || strlen(path) == 0) {
        return "";
    }
    
#ifdef _WIN32
    const char* filename = PathFindFileNameA(path);
    return filename ? filename : "";
#else
    // Unix: 鎵惧埌鏈€鍚庝竴涓?"/"
    const char* filename = std::strrchr(path, '/');
    return filename ? (filename + 1) : path;
#endif
}

std::string metadb_handle_impl::get_directory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const std::string& path = location_.get_path();
    if (!path || strlen(path) == 0) {
        return "";
    }
    
    std::string path_str = path;
    size_t last_slash = path_str.find_last_of("\\/");
    
    if (last_slash == std::string::npos) {
        return ".";  // 褰撳墠鐩綍
    }
    
    return path_str.substr(0, last_slash);
}

bool metadb_handle_impl::is_file_valid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const char* path = location_.get_path();
    if (!path || strlen(path) == 0) {
        return false;
    }
    
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

std::string metadb_handle_impl::get_identifier() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 鍒涘缓鍞竴鐨勬枃浠舵爣璇?
    // 鍖呮嫭璺緞銆佸ぇ灏忓拰鏃堕棿鎴?
    std::stringstream ss;
    ss << location_.get_path();
    ss << "|" << file_stats_.m_size;
    ss << "|" << file_stats_.m_timestamp;
    ss << "|" << location_.get_subsong_index();
    
    return ss.str();
}

bool metadb_handle_impl::equals(const metadb_handle_impl& other) const {
    const metadb_handle_impl* other_impl = &other;  // 宸茬粡鏄纭殑绫诲瀷
    if (!other_impl) {
        return false;
    }
    
    // 姣旇緝浣嶇疆锛堣矾寰?+ subsong锛?
    return location_.get_path() == other_impl->location_.get_path() &&
           location_.get_subsong_index() == other_impl->location_.get_subsong_index();
}

// 杈呭姪鍑芥暟
std::unique_ptr<metadb_handle_impl> metadb_handle_create(const playable_location& loc) {
    auto handle = std::make_unique<metadb_handle_impl>();
    if (handle->initialize(loc) == Result::Success) {
        return handle;
    }
    return nullptr;
}

} // namespace foobar2000_sdk
