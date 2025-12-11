#pragma once

#include "../stage1_4/fb2k_com_base.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <future>

namespace fb2k {

// 云服务提供商类型
enum class cloud_provider {
    none = 0,
    local,          // 本地文件同步
    dropbox,        // Dropbox
    googledrive,    // Google Drive
    onedrive,       // Microsoft OneDrive
    icloud,         // Apple iCloud
    custom          // 自定义云服务器
};

// 同步数据类型
enum class sync_data_type {
    none = 0,
    playlists,      // 播放列表
    preferences,    // 用户偏好设置
    library_cache,  // 媒体库缓存
    play_stats,     // 播放统计
    dsp_presets,    // DSP预设
    component_config, // 组件配置
    user_data,      // 用户自定义数据
    audio_analysis, // 音频分析结果
    album_art,      // 专辑封面
    lyrics,         // 歌词数据
    bookmarks       // 书签和收藏
};

// 同步操作类型
enum class sync_operation {
    upload,
    download,
    delete_op,
    rename,
    conflict_resolution
};

// 同步状态
enum class sync_status {
    idle,
    connecting,
    syncing,
    completed,
    failed,
    conflicted,
    paused,
    cancelled
};

// 云服务配置
struct cloud_service_config {
    cloud_provider provider = cloud_provider::none;
    std::string api_key;
    std::string api_secret;
    std::string access_token;
    std::string refresh_token;
    std::string user_id;
    std::string sync_directory = "/fb2k_sync";
    
    // 同步选项
    bool auto_sync = true;
    int sync_interval_seconds = 300; // 5分钟
    bool sync_playlists = true;
    bool sync_preferences = true;
    bool sync_library = true;
    bool sync_stats = true;
    bool sync_dsp_presets = true;
    bool sync_album_art = false; // 大文件，可选
    bool sync_lyrics = true;
    
    // 性能选项
    int max_concurrent_syncs = 3;
    int chunk_size_bytes = 1024 * 1024; // 1MB
    int retry_count = 3;
    int retry_delay_seconds = 5;
    double timeout_seconds = 30.0;
    
    // 安全选项
    bool encrypt_data = true;
    std::string encryption_key;
    bool compress_data = true;
    int compression_level = 6;
};

// 同步项目
struct sync_item {
    std::string id;
    std::string local_path;
    std::string remote_path;
    sync_data_type data_type;
    sync_operation operation;
    sync_status status;
    
    int64_t local_timestamp;
    int64_t remote_timestamp;
    int64_t local_size;
    int64_t remote_size;
    std::string local_hash;
    std::string remote_hash;
    
    int priority;
    int retry_count;
    std::string error_message;
    std::map<std::string, std::string> metadata;
};

// 同步会话
struct sync_session {
    std::string session_id;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    sync_status status;
    
    std::vector<sync_item> items;
    int completed_items;
    int failed_items;
    int conflicted_items;
    
    int64_t total_bytes_transferred;
    double total_time_seconds;
    double average_speed_bps;
    
    std::string error_message;
    std::map<std::string, std::string> statistics;
};

// 冲突解决策略
enum class conflict_resolution {
    keep_local,     // 保留本地版本
    keep_remote,    // 保留远程版本
    keep_newer,     // 保留较新的版本
    keep_older,     // 保留较旧的版本
    merge,          // 合并版本
    manual,         // 手动解决
    skip            // 跳过冲突
};

// 在线服务接口
struct IOnlineService : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 服务连接
    virtual HRESULT connect(const cloud_service_config& config) = 0;
    virtual HRESULT disconnect() = 0;
    virtual HRESULT is_connected(bool& connected) const = 0;
    virtual HRESULT get_connection_status(std::string& status) const = 0;
    
    // 认证管理
    virtual HRESULT authenticate(const std::string& username, const std::string& password) = 0;
    virtual HRESULT refresh_authentication() = 0;
    virtual HRESULT is_authenticated(bool& authenticated) const = 0;
    virtual HRESULT get_user_info(std::map<std::string, std::string>& info) const = 0;
    
    // 数据操作
    virtual HRESULT upload_data(const std::string& local_path, const std::string& remote_path, sync_data_type type) = 0;
    virtual HRESULT download_data(const std::string& remote_path, const std::string& local_path, sync_data_type type) = 0;
    virtual HRESULT delete_remote_data(const std::string& remote_path) = 0;
    virtual HRESULT list_remote_data(const std::string& remote_path, std::vector<std::string>& items) const = 0;
    
    // 元数据操作
    virtual HRESULT get_remote_metadata(const std::string& remote_path, std::map<std::string, std::string>& metadata) const = 0;
    virtual HRESULT set_remote_metadata(const std::string& remote_path, const std::map<std::string, std::string>& metadata) = 0;
    
    // 配额和限制
    virtual HRESULT get_quota_info(int64_t& total_space, int64_t& used_space, int64_t& available_space) const = 0;
    virtual HRESULT get_rate_limit_info(int& requests_per_minute, int& remaining_requests, int& reset_time_seconds) const = 0;
};

// 同步管理器接口
struct ISyncManager : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 同步配置
    virtual HRESULT set_config(const cloud_service_config& config) = 0;
    virtual HRESULT get_config(cloud_service_config& config) const = 0;
    virtual HRESULT add_online_service(cloud_provider provider, IOnlineService* service) = 0;
    virtual HRESULT remove_online_service(cloud_provider provider) = 0;
    
    // 同步控制
    virtual HRESULT start_sync(const std::vector<sync_data_type>& types) = 0;
    virtual HRESULT stop_sync() = 0;
    virtual HRESULT pause_sync() = 0;
    virtual HRESULT resume_sync() = 0;
    virtual HRESULT is_syncing(bool& syncing) const = 0;
    virtual HRESULT get_sync_status(sync_status& status) const = 0;
    
    // 同步项目管理
    virtual HRESULT add_sync_item(const sync_item& item) = 0;
    virtual HRESULT remove_sync_item(const std::string& item_id) = 0;
    virtual HRESULT get_sync_items(std::vector<sync_item>& items) const = 0;
    virtual HRESULT get_sync_item(const std::string& item_id, sync_item& item) const = 0;
    
    // 同步会话
    virtual HRESULT get_current_session(sync_session& session) const = 0;
    virtual HRESULT get_sync_history(std::vector<sync_session>& history, int max_sessions = 10) const = 0;
    virtual HRESULT clear_sync_history() = 0;
    
    // 冲突处理
    virtual HRESULT set_conflict_resolution(conflict_resolution resolution) = 0;
    virtual HRESULT get_conflict_resolution(conflict_resolution& resolution) const = 0;
    virtual HRESULT resolve_conflict(const std::string& item_id, conflict_resolution resolution) = 0;
    virtual HRESULT get_conflicts(std::vector<sync_item>& conflicts) const = 0;
    
    // 进度和统计
    virtual HRESULT get_sync_progress(double& progress, std::string& current_operation) const = 0;
    virtual HRESULT get_sync_statistics(std::map<std::string, double>& statistics) const = 0;
    virtual HRESULT reset_sync_statistics() = 0;
    
    // 计划任务
    virtual HRESULT enable_auto_sync(bool enable) = 0;
    virtual HRESULT is_auto_sync_enabled(bool& enabled) const = 0;
    virtual HRESULT set_sync_interval(int interval_seconds) = 0;
    virtual HRESULT get_sync_interval(int& interval_seconds) const = 0;
    
    // 事件回调
    virtual void set_sync_event_callback(std::function<void(const std::string& event, const sync_item& item)> callback) {
        sync_event_callback_ = callback;
    }
    
protected:
    std::function<void(const std::string& event, const sync_item& item)> sync_event_callback_;
    
    void notify_sync_event(const std::string& event, const sync_item& item) {
        if (sync_event_callback_) {
            sync_event_callback_(event, item);
        }
    }
};

// 播放列表同步接口
struct IPlaylistSync : public IFB2KUnknown {
    static const GUID iid;
    static const char* interface_name;
    
    // 播放列表操作
    virtual HRESULT upload_playlist(const std::string& playlist_name, const std::vector<std::string>& items) = 0;
    virtual HRESULT download_playlist(const std::string& playlist_name, std::vector<std::string>& items) const = 0;
    virtual HRESULT delete_playlist(const std::string& playlist_name) = 0;
    virtual HRESULT list_playlists(std::vector<std::string>& playlist_names) const = 0;
    
    // 播放列表元数据
    virtual HRESULT set_playlist_metadata(const std::string& playlist_name, const std::map<std::string, std::string>& metadata) = 0;
    virtual HRESULT get_playlist_metadata(const std::string& playlist_name, std::map<std::string, std::string>& metadata) const = 0;
    
    // 播放列表版本管理
    virtual HRESULT get_playlist_version(const std::string& playlist_name, int& version) const = 0;
    virtual HRESULT update_playlist_version(const std::string& playlist_name, int version) = 0;
};

// 媒体库同步接口
struct ILibrarySync : public IFB2KUnknown {
    static const GUID iid;
    static const char* interface_name;
    
    // 媒体库数据
    virtual HRESULT upload_library_data(const std::map<std::string, std::string>& library_data) = 0;
    virtual HRESULT download_library_data(std::map<std::string, std::string>& library_data) const = 0;
    virtual HRESULT sync_library_changes(const std::vector<std::string>& changed_items) = 0;
    
    // 文件元数据
    virtual HRESULT upload_file_metadata(const std::string& file_path, const std::map<std::string, std::string>& metadata) = 0;
    virtual HRESULT download_file_metadata(const std::string& file_path, std::map<std::string, std::string>& metadata) const = 0;
    virtual HRESULT get_file_metadata_version(const std::string& file_path, int& version) const = 0;
    
    // 统计信息
    virtual HRESULT upload_play_statistics(const std::string& file_path, int play_count, int64_t last_played) = 0;
    virtual HRESULT download_play_statistics(const std::string& file_path, int& play_count, int64_t& last_played) const = 0;
};

// 用户偏好同步接口
struct IPreferencesSync : public IFB2KUnknown {
    static const GUID iid;
    static const char* interface_name;
    
    // 配置数据
    virtual HRESULT upload_preferences(const std::map<std::string, std::string>& preferences) = 0;
    virtual HRESULT download_preferences(std::map<std::string, std::string>& preferences) const = 0;
    virtual HRESULT sync_preference(const std::string& key, const std::string& value) = 0;
    virtual HRESULT delete_preference(const std::string& key) = 0;
    
    // 配置版本
    virtual HRESULT get_preferences_version(int& version) const = 0;
    virtual HRESULT set_preferences_version(int version) = 0;
};

// 云端播放列表服务
class cloud_playlist_service {
public:
    cloud_playlist_service(IOnlineService* online_service);
    ~cloud_playlist_service();
    
    // 播放列表管理
    bool upload_playlist(const std::string& name, const std::vector<std::string>& tracks);
    bool download_playlist(const std::string& name, std::vector<std::string>& tracks);
    bool delete_playlist(const std::string& name);
    bool list_playlists(std::vector<std::string>& names);
    
    // 播放列表共享
    bool share_playlist(const std::string& name, const std::string& share_code);
    bool import_shared_playlist(const std::string& share_code, std::string& playlist_name);
    bool get_shared_playlist_info(const std::string& share_code, std::map<std::string, std::string>& info);
    
    // 协作播放列表
    bool create_collaborative_playlist(const std::string& name);
    bool add_collaborator(const std::string& playlist_name, const std::string& user_id);
    bool remove_collaborator(const std::string& playlist_name, const std::string& user_id);
    bool get_collaborators(const std::string& playlist_name, std::vector<std::string>& collaborators);
    
    // 播放列表推荐
    bool get_recommended_playlists(std::vector<std::map<std::string, std::string>>& recommendations);
    bool rate_playlist(const std::string& playlist_name, int rating, const std::string& comment = "");
    bool get_playlist_ratings(const std::string& playlist_name, std::vector<std::map<std::string, std::string>>& ratings);
    
private:
    IOnlineService* online_service_;
    std::string playlist_base_path_;
    
    std::string get_playlist_file_path(const std::string& name) const;
    std::string serialize_playlist(const std::vector<std::string>& tracks);
    std::vector<std::string> deserialize_playlist(const std::string& data);
    std::string generate_share_code();
    bool validate_share_code(const std::string& code);
};

// 云端媒体库服务
class cloud_library_service {
public:
    cloud_library_service(IOnlineService* online_service);
    ~cloud_library_service();
    
    // 媒体库同步
    bool sync_library(const std::vector<std::string>& music_directories);
    bool get_library_changes(std::vector<std::string>& added_files, std::vector<std::string>& removed_files, std::vector<std::string>& modified_files);
    bool apply_library_changes(const std::vector<std::string>& changes);
    
    // 文件元数据
    bool upload_file_metadata(const std::string& file_path, const std::map<std::string, std::string>& metadata);
    bool download_file_metadata(const std::string& file_path, std::map<std::string, std::string>& metadata);
    bool sync_file_metadata(const std::string& file_path);
    
    // 播放统计
    bool upload_play_statistics(const std::string& file_path, int play_count, int64_t last_played, int64_t total_play_time);
    bool download_play_statistics(const std::string& file_path, int& play_count, int64_t& last_played, int64_t& total_play_time);
    bool get_global_play_statistics(std::map<std::string, int>& top_tracks, std::map<std::string, int>& top_artists);
    
    // 音频分析结果
    bool upload_audio_analysis(const std::string& file_path, const std::map<std::string, double>& analysis_data);
    bool download_audio_analysis(const std::string& file_path, std::map<std::string, double>& analysis_data);
    bool get_similar_tracks(const std::string& file_path, std::vector<std::string>& similar_tracks);
    
private:
    IOnlineService* online_service_;
    std::string library_base_path_;
    std::string metadata_base_path_;
    std::string analysis_base_path_;
    
    std::string get_library_file_path(const std::string& relative_path) const;
    std::string get_metadata_file_path(const std::string& file_path) const;
    std::string get_analysis_file_path(const std::string& file_path) const;
    std::string normalize_file_path(const std::string& file_path);
};

// 同步任务队列
class sync_task_queue {
public:
    sync_task_queue(size_t max_size = 1000);
    ~sync_task_queue();
    
    // 任务管理
    bool enqueue(const sync_item& item);
    bool dequeue(sync_item& item);
    bool peek(sync_item& item) const;
    size_t size() const;
    bool empty() const;
    bool full() const;
    void clear();
    
    // 优先级管理
    bool enqueue_with_priority(const sync_item& item, int priority);
    void reorder_by_priority();
    std::vector<sync_item> get_items_by_type(sync_data_type type) const;
    std::vector<sync_item> get_items_by_status(sync_status status) const;
    
    // 批量操作
    bool enqueue_batch(const std::vector<sync_item>& items);
    bool dequeue_batch(std::vector<sync_item>& items, size_t count);
    
private:
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::priority_queue<sync_item, std::vector<sync_item>, decltype(&compare_sync_items)> queue_;
    size_t max_size_;
    std::atomic<bool> shutdown_;
    
    static bool compare_sync_items(const sync_item& a, const sync_item& b);
};

// 云端同步管理器实现
class cloud_sync_manager_impl : public fb2k_service_impl<ISyncManager> {
public:
    cloud_sync_manager_impl();
    ~cloud_sync_manager_impl();
    
protected:
    HRESULT do_initialize() override;
    HRESULT do_shutdown() override;
    
    // ISyncManager实现
    HRESULT set_config(const cloud_service_config& config) override;
    HRESULT get_config(cloud_service_config& config) const override;
    HRESULT add_online_service(cloud_provider provider, IOnlineService* service) override;
    HRESULT remove_online_service(cloud_provider provider) override;
    HRESULT start_sync(const std::vector<sync_data_type>& types) override;
    HRESULT stop_sync() override;
    HRESULT pause_sync() override;
    HRESULT resume_sync() override;
    HRESULT is_syncing(bool& syncing) const override;
    HRESULT get_sync_status(sync_status& status) const override;
    HRESULT add_sync_item(const sync_item& item) override;
    HRESULT remove_sync_item(const std::string& item_id) override;
    HRESULT get_sync_items(std::vector<sync_item>& items) const override;
    HRESULT get_sync_item(const std::string& item_id, sync_item& item) const override;
    HRESULT get_current_session(sync_session& session) const override;
    HRESULT get_sync_history(std::vector<sync_session>& history, int max_sessions) const override;
    HRESULT clear_sync_history() override;
    HRESULT set_conflict_resolution(conflict_resolution resolution) override;
    HRESULT get_conflict_resolution(conflict_resolution& resolution) const override;
    HRESULT resolve_conflict(const std::string& item_id, conflict_resolution resolution) override;
    HRESULT get_conflicts(std::vector<sync_item>& conflicts) const override;
    HRESULT get_sync_progress(double& progress, std::string& current_operation) const override;
    HRESULT get_sync_statistics(std::map<std::string, double>& statistics) const override;
    HRESULT reset_sync_statistics() override;
    HRESULT enable_auto_sync(bool enable) override;
    HRESULT is_auto_sync_enabled(bool& enabled) const override;
    HRESULT set_sync_interval(int interval_seconds) override;
    HRESULT get_sync_interval(int& interval_seconds) const override;
    
private:
    cloud_service_config config_;
    std::map<cloud_provider, IOnlineService*> online_services_;
    std::mutex services_mutex_;
    
    // 同步状态
    std::atomic<bool> syncing_;
    std::atomic<bool> auto_sync_enabled_;
    std::atomic<bool> should_stop_;
    std::atomic<sync_status> sync_status_;
    std::atomic<conflict_resolution> conflict_resolution_;
    
    // 同步队列
    std::unique_ptr<sync_task_queue> task_queue_;
    std::vector<sync_item> active_items_;
    mutable std::mutex items_mutex_;
    
    // 同步线程
    std::thread sync_thread_;
    std::thread auto_sync_thread_;
    
    // 当前会话
    sync_session current_session_;
    std::vector<sync_session> session_history_;
    mutable std::mutex session_mutex_;
    
    // 冲突管理
    std::vector<sync_item> conflicts_;
    mutable std::mutex conflicts_mutex_;
    
    // 统计信息
    std::map<std::string, double> sync_statistics_;
    mutable std::mutex statistics_mutex_;
    
    // 云服务
    std::unique_ptr<cloud_playlist_service> playlist_service_;
    std::unique_ptr<cloud_library_service> library_service_;
    
    // 私有方法
    void sync_worker_thread();
    void auto_sync_worker_thread();
    void process_sync_item(const sync_item& item);
    void detect_conflicts(const sync_item& local_item, const sync_item& remote_item);
    void resolve_conflict_auto(const sync_item& item);
    void update_session_statistics();
    void cleanup_completed_items();
    bool validate_sync_item(const sync_item& item);
    sync_item create_sync_item_from_local_data(const std::string& local_path, sync_data_type type);
    sync_item create_sync_item_from_remote_data(const std::string& remote_path, sync_data_type type);
};

// 在线服务基类
class online_service_base : public fb2k_com_object<IOnlineService> {
public:
    online_service_base(cloud_provider provider);
    ~online_service_base() override;
    
    // IOnlineService基础实现
    HRESULT connect(const cloud_service_config& config) override;
    HRESULT disconnect() override;
    HRESULT is_connected(bool& connected) const override;
    HRESULT get_connection_status(std::string& status) const override;
    
protected:
    cloud_provider provider_;
    cloud_service_config config_;
    std::atomic<bool> connected_;
    std::string connection_status_;
    mutable std::mutex connection_mutex_;
    
    // 需要派生类实现的虚函数
    virtual HRESULT do_connect() = 0;
    virtual HRESULT do_disconnect() = 0;
    virtual HRESULT do_authenticate(const std::string& username, const std::string& password) = 0;
    virtual HRESULT do_refresh_authentication() = 0;
    virtual HRESULT do_upload_data(const std::string& local_path, const std::string& remote_path, sync_data_type type) = 0;
    virtual HRESULT do_download_data(const std::string& remote_path, const std::string& local_path, sync_data_type type) = 0;
    virtual HRESULT do_delete_remote_data(const std::string& remote_path) = 0;
    virtual HRESULT do_list_remote_data(const std::string& remote_path, std::vector<std::string>& items) const = 0;
    
    // 工具函数
    std::string get_provider_name() const;
    std::string get_base_url() const;
    bool validate_config() const;
    std::string generate_request_id() const;
    
    // HTTP客户端（简化实现）
    struct http_client {
        std::string get(const std::string& url, const std::map<std::string, std::string>& headers = {});
        std::string post(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers = {});
        std::string put(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers = {});
        std::string del(const std::string& url, const std::map<std::string, std::string>& headers = {});
    };
    
    std::unique_ptr<http_client> http_client_;
};

// 具体云服务实现
class dropbox_service : public online_service_base {
public:
    dropbox_service();
    
protected:
    HRESULT do_connect() override;
    HRESULT do_disconnect() override;
    HRESULT do_authenticate(const std::string& username, const std::string& password) override;
    HRESULT do_refresh_authentication() override;
    HRESULT do_upload_data(const std::string& local_path, const std::string& remote_path, sync_data_type type) override;
    HRESULT do_download_data(const std::string& remote_path, const std::string& local_path, sync_data_type type) override;
    HRESULT do_delete_remote_data(const std::string& remote_path) override;
    HRESULT do_list_remote_data(const std::string& remote_path, std::vector<std::string>& items) const override;
};

class googledrive_service : public online_service_base {
public:
    googledrive_service();
    
protected:
    HRESULT do_connect() override;
    HRESULT do_disconnect() override;
    HRESULT do_authenticate(const std::string& username, const std::string& password) override;
    HRESULT do_refresh_authentication() override;
    HRESULT do_upload_data(const std::string& local_path, const std::string& remote_path, sync_data_type type) override;
    HRESULT do_download_data(const std::string& remote_path, const std::string& local_path, sync_data_type type) override;
    HRESULT do_delete_remote_data(const std::string& remote_path) override;
    HRESULT do_list_remote_data(const std::string& remote_path, std::vector<std::string>& items) const override;
};

class onedrive_service : public online_service_base {
public:
    onedrive_service();
    
protected:
    HRESULT do_connect() override;
    HRESULT do_disconnect() override;
    HRESULT do_authenticate(const std::string& username, const std::string& password) override;
    HRESULT do_refresh_authentication() override;
    HRESULT do_upload_data(const std::string& local_path, const std::string& remote_path, sync_data_type type) override;
    HRESULT do_download_data(const std::string& remote_path, const std::string& local_path, sync_data_type type) override;
    HRESULT do_delete_remote_data(const std::string& remote_path) override;
    HRESULT do_list_remote_data(const std::string& remote_path, std::vector<std::string>& items) const override;
};

// 云服务工厂
class cloud_service_factory {
public:
    static std::unique_ptr<IOnlineService> create_service(cloud_provider provider);
    static std::vector<cloud_provider> get_available_providers();
    static bool is_provider_available(cloud_provider provider);
    static std::string get_provider_name(cloud_provider provider);
    static std::string get_provider_description(cloud_provider provider);
};

// 全局云服务访问
IOnlineService* get_cloud_service(cloud_provider provider);
ISyncManager* get_sync_manager();

// 云服务初始化
void initialize_cloud_services();
void shutdown_cloud_services();

} // namespace fb2k