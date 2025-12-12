#pragma once

#include "../stage1_4/fb2k_com_base.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>

namespace fb2k {

// 璁惧绫诲瀷鏋氫妇
enum class device_type {
    unknown = 0,
    desktop,
    laptop,
    tablet,
    phone,
    wearable,
    tv,
    car
};

// 骞冲彴绫诲瀷鏋氫妇
enum class mobile_platform {
    unknown = 0,
    ios,
    android,
    windows_phone,
    mobile_web
};

// 灞忓箷灏哄鍒嗙被
enum class screen_size {
    unknown = 0,
    small,      // < 4.5" (phones)
    medium,     // 4.5" - 5.5" (large phones)
    large,      // 5.5" - 7" (phablets/small tablets)
    xlarge,     // 7" - 10" (tablets)
    xxlarge     // > 10" (large tablets/desktop)
};

// 灞忓箷鏂瑰悜
enum class screen_orientation {
    unknown = 0,
    portrait,
    landscape,
    portrait_upside_down,
    landscape_left,
    landscape_right
};

// 杈撳叆鏂瑰紡
enum class input_method {
    unknown = 0,
    touch,
    mouse,
    keyboard,
    stylus,
    voice,
    gesture
};

// 鎬ц兘绛夌骇
enum class performance_class {
    unknown = 0,
    low,        // 浣庣璁惧
    medium,     // 涓璁惧
    high,       // 楂樼璁惧
    premium     // 鏃楄埌璁惧
};

// 缃戠粶绫诲瀷
enum class network_type {
    unknown = 0,
    none,
    wifi,
    cellular_2g,
    cellular_3g,
    cellular_4g,
    cellular_5g,
    ethernet
};

// 鐢垫睜鐘舵€?
struct battery_status {
    bool is_charging;
    int charge_level_percent;  // 0-100
    bool is_low_battery;
    int estimated_time_minutes;
    std::string power_source;  // "battery", "ac", "usb"
};

// 璁惧鑳藉姏
struct device_capabilities {
    bool has_touch_screen;
    bool has_multi_touch;
    bool has_stylus;
    bool has_camera;
    bool has_microphone;
    bool has_speakers;
    bool has_headphones;
    bool has_bluetooth;
    bool has_wifi;
    bool has_cellular;
    bool has_gps;
    bool has_accelerometer;
    bool has_gyroscope;
    bool has_compass;
    bool has_vibration;
    
    int max_simultaneous_touches;
    int screen_density_dpi;
    float screen_diagonal_inches;
    
    performance_class performance_level;
    int64_t total_memory_bytes;
    int64_t available_memory_bytes;
    int64_t storage_bytes;
    int64_t available_storage_bytes;
};

// 绉诲姩绔厤缃?
struct mobile_config {
    // 鐣岄潰閫傞厤
    bool enable_responsive_ui = true;
    bool enable_touch_optimization = true;
    bool enable_gesture_controls = true;
    bool enable_voice_commands = false;
    
    // 鎬ц兘浼樺寲
    bool enable_power_saving_mode = true;
    bool reduce_animations_on_low_battery = true;
    bool pause_background_tasks_on_low_battery = true;
    int low_battery_threshold = 20; // 20%
    
    // 缃戠粶浼樺寲
    bool sync_only_on_wifi = false;
    bool reduce_quality_on_cellular = true;
    bool enable_offline_mode = true;
    int max_cellular_data_usage_mb = 100;
    
    // 瀛樺偍浼樺寲
    bool enable_smart_cache = true;
    int max_cache_size_mb = 500;
    bool auto_cleanup_cache = true;
    int cache_cleanup_threshold_days = 7;
    
    // 閫氱煡璁剧疆
    bool enable_push_notifications = true;
    bool enable_background_refresh = true;
    bool show_now_playing_notification = true;
    
    // 鎵嬪娍閰嶇疆
    double swipe_threshold_mm = 5.0;
    double tap_timeout_ms = 300.0;
    double double_tap_timeout_ms = 200.0;
    double long_press_timeout_ms = 500.0;
    
    // 鎬ц兘閰嶇疆
    int max_simultaneous_decoders = 2;
    int max_background_threads = 2;
    int ui_refresh_rate_hz = 60;
    bool enable_hardware_acceleration = true;
    
    // 杈呭姪鍔熻兘
    bool enable_large_text_mode = false;
    bool enable_high_contrast_mode = false;
    bool enable_screen_reader_support = true;
    bool enable_haptic_feedback = true;
};

// 鍝嶅簲寮廢I鎺ュ彛
struct IResponsiveUI : public IFB2KService {
    static const GUID iid;
    static const char* interface_name;
    
    // 璁惧淇℃伅
    virtual HRESULT get_device_type(device_type& type) const = 0;
    virtual HRESULT get_mobile_platform(mobile_platform& platform) const = 0;
    virtual HRESULT get_screen_size(screen_size& size) const = 0;
    virtual HRESULT get_screen_orientation(screen_orientation& orientation) const = 0;
    virtual HRESULT get_input_method(input_method& method) const = 0;
    virtual HRESULT get_performance_class(performance_class& class_level) const = 0;
    
    // 璁惧鑳藉姏
    virtual HRESULT get_device_capabilities(device_capabilities& capabilities) const = 0;
    virtual HRESULT get_battery_status(battery_status& status) const = 0;
    virtual HRESULT get_network_type(network_type& type) const = 0;
    virtual HRESULT get_network_strength(int& strength_percent) const = 0;
    
    // UI閫傞厤
    virtual HRESULT set_ui_scale(float scale) = 0;
    virtual HRESULT get_ui_scale(float& scale) const = 0;
    virtual HRESULT set_layout_mode(const std::string& mode) = 0;
    virtual HRESULT get_layout_mode(std::string& mode) const = 0;
    virtual HRESULT adapt_to_screen_size(screen_size size) = 0;
    virtual HRESULT adapt_to_orientation(screen_orientation orientation) = 0;
    
    // 杈撳叆澶勭悊
    virtual HRESULT enable_touch_input(bool enable) = 0;
    virtual HRESULT enable_gesture_input(bool enable) = 0;
    virtual HRESULT enable_voice_input(bool enable) = 0;
    virtual HRESULT process_touch_event(int pointer_id, float x, float y, int action) = 0;
    virtual HRESULT process_gesture_event(const std::string& gesture_type, float x, float y, float delta) = 0;
    virtual HRESULT process_voice_command(const std::string& command) = 0;
    
    // 鎬ц兘绠＄悊
    virtual HRESULT set_performance_mode(const std::string& mode) = 0;
    virtual HRESULT get_performance_mode(std::string& mode) const = 0;
    virtual HRESULT optimize_for_low_battery(bool optimize) = 0;
    virtual HRESULT optimize_for_low_performance(bool optimize) = 0;
    virtual HRESULT optimize_for_limited_network(bool optimize) = 0;
    
    // 閫氱煡鍜屽弽棣?
    virtual HRESULT show_notification(const std::string& title, const std::string& message, int priority) = 0;
    virtual HRESULT hide_notification(int notification_id) = 0;
    virtual HRESULT provide_haptic_feedback(const std::string& type) = 0;
    virtual HRESULT play_system_sound(const std::string& sound_type) = 0;
    
    // 鍚庡彴澶勭悊
    virtual HRESULT enter_background_mode() = 0;
    virtual HRESULT exit_background_mode() = 0;
    virtual HRESULT is_in_background(bool& background) const = 0;
    virtual HRESULT handle_background_refresh() = 0;
    virtual HRESULT handle_background_task(const std::string& task_id) = 0;
    
    // 鏁版嵁鎸佷箙鍖?
    virtual HRESULT save_application_state() = 0;
    virtual HRESULT restore_application_state() = 0;
    virtual HRESULT clear_application_cache() = 0;
    
    // 浜嬩欢鍥炶皟
    virtual void set_device_event_callback(std::function<void(const std::string& event, const std::string& data)> callback) {
        device_event_callback_ = callback;
    }
    
    virtual void set_orientation_change_callback(std::function<void(screen_orientation new_orientation)> callback) {
        orientation_change_callback_ = callback;
    }
    
protected:
    std::function<void(const std::string& event, const std::string& data)> device_event_callback_;
    std::function<void(screen_orientation new_orientation)> orientation_change_callback_;
    
    void notify_device_event(const std::string& event, const std::string& data) {
        if (device_event_callback_) {
            device_event_callback_(event, data);
        }
    }
    
    void notify_orientation_change(screen_orientation new_orientation) {
        if (orientation_change_callback_) {
            orientation_change_callback_(new_orientation);
        }
    }
};

// 瑙︽懜鎵嬪娍璇嗗埆鍣?
class touch_gesture_recognizer {
public:
    touch_gesture_recognizer();
    ~touch_gesture_recognizer();
    
    // 鎵嬪娍绫诲瀷
    enum class gesture_type {
        none = 0,
        tap,
        double_tap,
        long_press,
        swipe_left,
        swipe_right,
        swipe_up,
        swipe_down,
        pinch_in,
        pinch_out,
        rotate_clockwise,
        rotate_counter_clockwise,
        drag,
        flick
    };
    
    // 鎵嬪娍浜嬩欢
    struct gesture_event {
        gesture_type type;
        float start_x, start_y;
        float end_x, end_y;
        float velocity_x, velocity_y;
        float scale;
        float rotation;
        int64_t duration_ms;
        int pointer_count;
        bool is_complete;
    };
    
    // 瑙︽懜浜嬩欢澶勭悊
    bool process_touch_down(int pointer_id, float x, float y, int64_t timestamp_ms);
    bool process_touch_move(int pointer_id, float x, float y, int64_t timestamp_ms);
    bool process_touch_up(int pointer_id, float x, float y, int64_t timestamp_ms);
    bool process_touch_cancel(int64_t timestamp_ms);
    
    // 鎵嬪娍璇嗗埆
    bool detect_gestures(std::vector<gesture_event>& gestures);
    gesture_event get_primary_gesture() const;
    std::vector<gesture_event> get_all_gestures() const;
    
    // 閰嶇疆
    void set_config(const mobile_config& config);
    void reset();
    void clear_all_pointers();
    
private:
    struct touch_pointer {
        int id;
        float start_x, start_y;
        float current_x, current_y;
        float last_x, last_y;
        int64_t start_time;
        int64_t last_time;
        bool is_active;
        std::vector<std::pair<float, float>> history;
    };
    
    mobile_config config_;
    std::map<int, touch_pointer> active_pointers_;
    std::vector<gesture_event> detected_gestures_;
    mutable std::mutex gesture_mutex_;
    
    // 鎵嬪娍妫€娴嬬畻娉?
    gesture_type detect_tap(const touch_pointer& pointer) const;
    gesture_type detect_double_tap(const touch_pointer& pointer) const;
    gesture_type detect_long_press(const touch_pointer& pointer) const;
    gesture_type detect_swipe(const touch_pointer& pointer) const;
    gesture_type detect_pinch(const std::vector<touch_pointer>& pointers) const;
    gesture_type detect_rotation(const std::vector<touch_pointer>& pointers) const;
    gesture_type detect_drag(const touch_pointer& pointer) const;
    gesture_type detect_flick(const touch_pointer& pointer) const;
    
    // 杈呭姪鍑芥暟
    float calculate_distance(float x1, float y1, float x2, float y2) const;
    float calculate_velocity(float distance, int64_t time_ms) const;
    float calculate_angle(float x1, float y1, float x2, float y2) const;
    bool is_point_in_rect(float x, float y, float rect_x, float rect_y, float rect_width, float rect_height) const;
};

// 鍝嶅簲寮忓竷灞€绠＄悊鍣?
class responsive_layout_manager {
public:
    responsive_layout_manager();
    ~responsive_layout_manager();
    
    // 甯冨眬绫诲瀷
    enum class layout_type {
        none = 0,
        phone_portrait,
        phone_landscape,
        tablet_portrait,
        tablet_landscape,
        desktop,
        tv,
        car
    };
    
    // UI鍏冪礌
    struct ui_element {
        std::string id;
        std::string type; // "button", "text", "image", "container", etc.
        float x, y, width, height; // 鐩稿鍧愭爣 0.0-1.0
        bool visible;
        int priority; // 鏄剧ず浼樺厛绾?
        std::map<std::string, std::string> properties;
        std::vector<std::string> children;
    };
    
    // 甯冨眬瑙勫垯
    struct layout_rule {
        layout_type target_layout;
        screen_size target_screen_size;
        screen_orientation target_orientation;
        performance_class target_performance;
        
        std::vector<std::string> show_elements;
        std::vector<std::string> hide_elements;
        std::vector<std::string> resize_elements;
        std::vector<std::string> reposition_elements;
        
        float ui_scale;
        int font_size_multiplier;
        bool enable_animations;
        bool reduce_transparency;
    };
    
    // 甯冨眬閫傞厤
    bool adapt_layout(device_type device, screen_size screen, screen_orientation orientation, performance_class performance);
    layout_type get_current_layout() const;
    std::vector<ui_element> get_current_layout_elements() const;
    
    // 鍏冪礌绠＄悊
    bool add_element(const ui_element& element);
    bool remove_element(const std::string& element_id);
    bool update_element(const ui_element& element);
    ui_element get_element(const std::string& element_id) const;
    
    // 瑙勫垯绠＄悊
    bool add_layout_rule(const layout_rule& rule);
    bool remove_layout_rule(layout_type type);
    std::vector<layout_rule> get_layout_rules() const;
    
    // 鍝嶅簲寮忚绠?
    float calculate_optimal_font_size(float base_size) const;
    float calculate_optimal_button_size(float base_size) const;
    float calculate_optimal_spacing(float base_spacing) const;
    std::pair<float, float> calculate_optimal_position(float x, float y) const;
    
private:
    layout_type current_layout_;
    std::map<std::string, ui_element> elements_;
    std::vector<layout_rule> layout_rules_;
    mutable std::mutex layout_mutex_;
    
    device_type current_device_;
    screen_size current_screen_size_;
    screen_orientation current_orientation_;
    performance_class current_performance_;
    
    // 甯冨眬璁＄畻
    layout_type determine_layout(device_type device, screen_size screen, screen_orientation orientation, performance_class performance) const;
    layout_rule find_best_layout_rule(layout_type type) const;
    void apply_layout_rule(const layout_rule& rule);
    void recalculate_element_positions();
    
    // 鍝嶅簲寮忕畻娉?
    float calculate_scale_factor() const;
    float calculate_font_scale_factor() const;
    float calculate_spacing_scale_factor() const;
    bool should_reduce_animations() const;
    bool should_reduce_transparency() const;
};

// 鎬ц兘浼樺寲绠＄悊鍣?
class performance_optimizer {
public:
    performance_optimizer();
    ~performance_optimizer();
    
    // 浼樺寲绾у埆
    enum class optimization_level {
        none = 0,
        minimal,
        moderate,
        aggressive,
        maximum
    };
    
    // 浼樺寲绛栫暐
    struct optimization_strategy {
        optimization_level level;
        bool reduce_animations;
        bool reduce_transparency;
        bool reduce_effects;
        bool lower_sample_rate;
        bool reduce_buffer_size;
        bool disable_background_tasks;
        bool reduce_ui_refresh_rate;
        bool enable_power_saving;
        
        int max_decoders;
        int max_threads;
        int ui_refresh_rate;
        int audio_buffer_size;
        int fft_size;
        bool enable_oversampling;
    };
    
    // 鎬ц兘鐩戞帶
    void start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const { return monitoring_; }
    
    // 浼樺寲寤鸿
    optimization_strategy suggest_optimization_strategy() const;
    bool apply_optimization_strategy(const optimization_strategy& strategy);
    void rollback_optimization_strategy();
    
    // 瀹炴椂浼樺寲
    void optimize_for_battery_level(int battery_percent);
    void optimize_for_network_type(network_type type);
    void optimize_for_performance_class(performance_class class_level);
    void optimize_for_memory_pressure(int available_memory_percent);
    void optimize_for_cpu_usage(int cpu_usage_percent);
    
    // 鎬ц兘鎸囨爣
    struct performance_metrics {
        double cpu_usage_percent;
        double memory_usage_percent;
        double battery_level_percent;
        double network_speed_mbps;
        int frame_rate_fps;
        double audio_latency_ms;
        double ui_responsiveness_ms;
        int dropped_frames_count;
    };
    
    performance_metrics get_current_metrics() const;
    std::vector<performance_metrics> get_metrics_history(int count = 100) const;
    
private:
    std::atomic<bool> monitoring_;
    std::thread monitoring_thread_;
    
    optimization_strategy current_strategy_;
    optimization_strategy previous_strategy_;
    mutable std::mutex strategy_mutex_;
    
    std::vector<performance_metrics> metrics_history_;
    mutable std::mutex metrics_mutex_;
    
    void monitoring_thread_func();
    performance_metrics collect_current_metrics() const;
    optimization_level calculate_required_optimization_level(const performance_metrics& metrics) const;
    optimization_strategy create_optimization_strategy(optimization_level level) const;
    bool can_apply_optimization(const optimization_strategy& strategy) const;
};

// 绉诲姩绔疷I鎺у埗鍣?
class mobile_ui_controller {
public:
    mobile_ui_controller();
    ~mobile_ui_controller();
    
    // 鍒濆鍖?
    bool initialize(const mobile_config& config);
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // 涓荤晫闈?
    void show_main_interface();
    void hide_main_interface();
    void update_main_interface();
    
    // 鎾斁鐣岄潰
    void show_now_playing();
    void hide_now_playing();
    void update_now_playing(const std::string& track_title, const std::string& artist, const std::string& album);
    
    // 鎾斁鍒楄〃鐣岄潰
    void show_playlist_interface();
    void hide_playlist_interface();
    void update_playlist_interface(const std::vector<std::string>& items);
    
    // 搴撶晫闈?
    void show_library_interface();
    void hide_library_interface();
    void update_library_interface(const std::map<std::string, std::vector<std::string>>& categories);
    
    // 璁剧疆鐣岄潰
    void show_settings_interface();
    void hide_settings_interface();
    void update_settings_interface();
    
    // 寮瑰嚭鑿滃崟
    int show_context_menu(const std::vector<std::string>& items, float x, float y);
    void hide_context_menu(int menu_id);
    
    // 閫氱煡
    void show_notification(const std::string& title, const std::string& message, int priority = 0);
    void hide_notification(int notification_id);
    void update_notification(int notification_id, const std::string& title, const std::string& message);
    
    // 杩涘害鏉?
    void show_progress_bar(const std::string& title, const std::string& message, float progress);
    void update_progress_bar(float progress, const std::string& message = "");
    void hide_progress_bar();
    
    // 杈撳叆澶勭悊
    bool handle_touch_event(int pointer_id, float x, float y, int action);
    bool handle_gesture_event(const touch_gesture_recognizer::gesture_event& gesture);
    bool handle_key_event(int key_code, int action);
    
    // 鐣岄潰鐘舵€?
    std::string get_current_interface() const;
    bool is_interface_visible(const std::string& interface_name) const;
    void set_interface_visibility(const std::string& interface_name, bool visible);
    
private:
    mobile_config config_;
    std::unique_ptr<IResponsiveUI> responsive_ui_;
    std::unique_ptr<touch_gesture_recognizer> gesture_recognizer_;
    std::unique_ptr<responsive_layout_manager> layout_manager_;
    std::unique_ptr<performance_optimizer> performance_optimizer_;
    
    bool initialized_;
    std::string current_interface_;
    std::map<std::string, bool> interface_visibility_;
    std::map<int, std::string> active_menus_;
    std::map<int, std::pair<std::string, std::string>> active_notifications_;
    int progress_bar_active_;
    
    mutable std::mutex ui_mutex_;
    
    // UI浜嬩欢澶勭悊
    void handle_play_pause_button();
    void handle_next_button();
    void handle_previous_button();
    void handle_volume_change(float volume);
    void handle_seek_bar_change(float position);
    void handle_playlist_item_selected(int index);
    void handle_library_item_selected(const std::string& category, int index);
    void handle_settings_item_selected(const std::string& setting);
    void handle_context_menu_item_selected(int menu_id, int item_index);
    
    // 鐣岄潰鏇存柊
    void update_playback_controls(bool is_playing, float volume, float position);
    void update_track_info(const std::string& title, const std::string& artist, const std::string& album);
    void update_playback_progress(float position, float duration);
    
    // 鎵嬪娍澶勭悊
    void handle_play_gesture();
    void handle_pause_gesture();
    void handle_next_gesture();
    void handle_previous_gesture();
    void handle_volume_gesture(float delta);
    void handle_seek_gesture(float delta);
};

// 绉诲姩绔€傞厤绠＄悊鍣?
class mobile_adaptation_manager {
public:
    static mobile_adaptation_manager& get_instance();
    
    // 鍒濆鍖?
    bool initialize(const mobile_config& config);
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // 璁惧閫傞厤
    bool adapt_to_device();
    bool adapt_to_screen_size(screen_size size);
    bool adapt_to_orientation(screen_orientation orientation);
    bool adapt_to_performance_level(performance_class level);
    bool adapt_to_battery_level(int battery_percent);
    
    // 鎬ц兘鐩戞帶
    void start_performance_monitoring();
    void stop_performance_monitoring();
    void update_performance_metrics();
    
    // 浜嬩欢澶勭悊
    void handle_device_event(const std::string& event, const std::string& data);
    void handle_orientation_change(screen_orientation new_orientation);
    void handle_battery_level_change(int battery_level);
    void handle_network_change(network_type type);
    void handle_memory_pressure(int available_memory_percent);
    
    // 鑾峰彇褰撳墠鐘舵€?
    device_type get_current_device_type() const;
    screen_size get_current_screen_size() const;
    screen_orientation get_current_orientation() const;
    performance_class get_current_performance_class() const;
    battery_status get_current_battery_status() const;
    network_type get_current_network_type() const;
    
    // 缁勪欢璁块棶
    IResponsiveUI* get_responsive_ui() { return responsive_ui_.get(); }
    touch_gesture_recognizer* get_gesture_recognizer() { return gesture_recognizer_.get(); }
    responsive_layout_manager* get_layout_manager() { return layout_manager_.get(); }
    performance_optimizer* get_performance_optimizer() { return performance_optimizer_.get(); }
    mobile_ui_controller* get_ui_controller() { return ui_controller_.get(); }
    
private:
    mobile_adaptation_manager();
    ~mobile_adaptation_manager();
    mobile_adaptation_manager(const mobile_adaptation_manager&) = delete;
    mobile_adaptation_manager& operator=(const mobile_adaptation_manager&) = delete;
    
    mobile_config config_;
    bool initialized_;
    
    // 缁勪欢
    std::unique_ptr<IResponsiveUI> responsive_ui_;
    std::unique_ptr<touch_gesture_recognizer> gesture_recognizer_;
    std::unique_ptr<responsive_layout_manager> layout_manager_;
    std::unique_ptr<performance_optimizer> performance_optimizer_;
    std::unique_ptr<mobile_ui_controller> ui_controller_;
    
    // 鐘舵€佺洃鎺?
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_;
    
    // 璁惧鐘舵€?
    device_type current_device_type_;
    screen_size current_screen_size_;
    screen_orientation current_orientation_;
    performance_class current_performance_class_;
    battery_status current_battery_status_;
    network_type current_network_type_;
    
    mutable std::mutex state_mutex_;
    
    // 鐩戞帶鍑芥暟
    void monitoring_thread_func();
    void monitor_battery_level();
    void monitor_network_status();
    void monitor_performance_metrics();
    void monitor_memory_usage();
    
    // 鑷€傚簲鍑芥暟
    void perform_full_adaptation();
    void adapt_performance_settings();
    void adapt_ui_layout();
    void adapt_network_behavior();
    void adapt_storage_usage();
    
    // 浜嬩欢鍝嶅簲
    void respond_to_low_battery();
    void respond_to_low_memory();
    void respond_to_network_change(network_type new_type);
    void respond_to_orientation_change(screen_orientation new_orientation);
};

// 鍏ㄥ眬绉诲姩绔€傞厤璁块棶
mobile_adaptation_manager& get_mobile_adaptation_manager();
IResponsiveUI* get_responsive_ui();

// 绉诲姩绔湇鍔″垵濮嬪寲
void initialize_mobile_services();
void shutdown_mobile_services();

} // namespace fb2k