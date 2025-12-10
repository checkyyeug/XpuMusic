/**
 * @file metadb_handle_impl.cpp
 * @brief metadb_handle_impl 实现
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

// TrackStatistics 实现
typedef std::chrono::system_clock::time_point time_point;
typedef std::chrono::system_clock::duration duration;

static uint64_t get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

//==============================================================================
// metadb_handle_impl 实现
//==============================================================================

metadb_handle_impl::metadb_handle_impl() {
    reset();
}

void metadb_handle_impl::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    location_ = playable_location();
    info_ = std::make_unique<file_info_impl>();
    stats_ = TrackStatistics();
    file_stats_ = file_stats();
    parent_db_ = nullptr;
    initialized_ = false;
}

Result metadb_handle_impl::initialize(const playable_location& loc, metadb_impl* parent) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    reset();
    
    location_ = loc;
    parent_db_ = parent;
    
    // 尝试从文件加载元数据
    Result result = load_metadata_from_file();
    if (result != Result::Success) {
        // 可以没有有效元数据继续
        // 标记为初始化但仍可用
    }
    
    initialized_ = true;
    return Result::Success;
}

Result metadb_handle_impl::load_metadata_from_file() {
    // 这个实现是简化的
    // 真正的实现需要调用解码器来获取元数据
    
    const char* path = location_.get_path();
    if (!path || strlen(path) == 0) {
        return Result::InvalidParameter;
    }
    
    // 获取文件统计（大小、时间戳）
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
    
    // 注意：这里没有实际读取音频文件的元数据
    // 在实际系统中，这会调用解码器的 get_info 方法
    // 为简单起见，我们只填充了一些基础信息
    
    return Result::Success;
}

const file_info_interface& metadb_handle_impl::get_info() const {
    // 注意：返回的引用可能在没有锁的情况下被修改
    // 在实际 foobar2000 中，这需要更复杂的线程安全机制
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
    // 异步版本：简单地委托给同步版本
    // 在真实 foobar2000 中，这会在后台线程执行
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
    
    // 在实际系统中，这会触发数据库更新和通知
    // 现在只是简单保存
}

void metadb_handle_impl::refresh_info(abort_callback& p_abort) {
    // 重新从文件加载元数据
    (void)p_abort;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    load_metadata_from_file();
}

std::string metadb_handle_impl::get_filename() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const char* path = location_.get_path();
    if (!path || strlen(path) == 0) {
        return "";
    }
    
#ifdef _WIN32
    const char* filename = PathFindFileNameA(path);
    return filename ? filename : "";
#else
    // Unix: 找到最后一个 "/"
    const char* filename = std::strrchr(path, '/');
    return filename ? (filename + 1) : path;
#endif
}

std::string metadb_handle_impl::get_directory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const char* path = location_.get_path();
    if (!path || strlen(path) == 0) {
        return "";
    }
    
    std::string path_str = path;
    size_t last_slash = path_str.find_last_of("\\/");
    
    if (last_slash == std::string::npos) {
        return ".";  // 当前目录
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
    
    // 创建唯一的文件标识
    // 包括路径、大小和时间戳
    std::stringstream ss;
    ss << location_.get_path();
    ss << "|" << file_stats_.m_size;
    ss << "|" << file_stats_.m_timestamp;
    ss << "|" << location_.get_subsong();
    
    return ss.str();
}

bool metadb_handle_impl::equals(const metadb_handle_impl& other) const {
    const metadb_handle_impl* other_impl = &other;  // 已经是正确的类型
    if (!other_impl) {
        return false;
    }
    
    // 比较位置（路径 + subsong）
    return location_.get_path() == other_impl->location_.get_path() &&
           location_.get_subsong() == other_impl->location_.get_subsong();
}

// 辅助函数
std::unique_ptr<metadb_handle_impl> metadb_handle_create(const playable_location& loc) {
    auto handle = std::make_unique<metadb_handle_impl>();
    if (handle->initialize(loc) == Result::Success) {
        return handle;
    }
    return nullptr;
}

} // namespace foobar2000_sdk
