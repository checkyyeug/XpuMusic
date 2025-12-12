/**
 * @file plugin_loader.h
 * @brief foobar2000 DLL 鎻掍欢鍔犺浇鍣?
 * @date 2025-12-09
 * 
 * 杩欐槸淇 #2 鐨勬牳蹇冿細鑳藉鍔犺浇 Windows DLL锛?dll 鏂囦欢锛夊苟瑙ｆ瀽
 * foobar2000 鐨勬湇鍔″伐鍘傚鍑猴紝浣垮師鐢熸彃浠跺彲鐢ㄣ€?
 */

#pragma once

// Windows: 閬垮厤 GUID 閲嶅畾涔?
#ifdef _WIN32
#ifndef GUID_DEFINED
#include <windows.h>
#endif
#endif

#include "../xpumusic_sdk/foobar2000_sdk.h"
#include "../sdk_implementations/service_base.h"
#include "../../sdk/headers/mp_types.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstring>

#ifndef _WIN32
// Linux/macOS: 浣跨敤 dlopen
#include <dlfcn.h>
#endif

// 浣跨敤 xpumusic_sdk 鍛藉悕绌洪棿涓殑绫诲瀷
using xpumusic_sdk::service_factory_base;
// Note: service_ptr is not defined, using service_ptr_t instead
template<typename T>
using service_ptr = xpumusic_sdk::service_ptr_t<T>;
using xpumusic_sdk::service_base;
#ifdef GUID_DEFINED
// Windows 宸茬粡瀹氫箟浜?GUID锛屼娇鐢ㄥ叏灞€鍛藉悕绌洪棿
using ::GUID;
#else
using xpumusic_sdk::GUID;
#endif

namespace mp {
namespace compat {

// 鍓嶅悜澹版槑
class ServiceRegistryBridge;

/**
 * @struct PluginModuleInfo
 * @brief 宸插姞杞芥彃浠舵ā鍧楃殑淇℃伅
 */
struct PluginModuleInfo {
    std::string path;                    // DLL 鏂囦欢璺緞
    std::string name;                    // 鎻掍欢鍚嶇О锛堜粠 file_info 鎻愬彇锛?
    std::string version;                 // 鎻掍欢鐗堟湰
    void* library_handle = nullptr;      // 骞冲彴鐩稿叧鐨勫簱鍙ユ焺
    bool loaded = false;                 // 鏄惁鎴愬姛鍔犺浇
    bool initialized = false;            // 鏄惁宸插垵濮嬪寲
    std::string error;                   // 鍔犺浇閿欒淇℃伅
    
    // 缁熻淇℃伅
    uint32_t service_count = 0;          // 娉ㄥ唽鐨勬湇鍔℃暟閲?
    uint64_t load_time_ms = 0;           // 鍔犺浇鑰楁椂锛堟绉掞級
    
    PluginModuleInfo() = default;
};

/**
 * @struct ServiceExportInfo
 * @brief 浠?DLL 瀵煎嚭鐨勬湇鍔′俊鎭?
 */
struct ServiceExportInfo {
    std::string name;                    // 鏈嶅姟鍚嶇О
    GUID guid;                           // 鏈嶅姟 GUID
    service_factory_base* factory;       // 鏈嶅姟宸ュ巶鎸囬拡
    bool available;                      // 鏄惁鍙敤
    
    ServiceExportInfo() : factory(nullptr), available(false) {
        memset(&guid, 0, sizeof(GUID));
    }
};

/**
 * @class ServiceFactoryWrapper
 * @brief 鍖呰 foobar2000 鏈嶅姟宸ュ巶鍒版垜浠郴缁?
 *
 * 杩欐ˉ鎺ヤ簡 foobar2000 鐨?service_factory_base 鍜?
 * 鎴戜滑鑷繁鐨勬湇鍔℃敞鍐岃〃銆?
 */
class ServiceFactoryWrapper : public xpumusic_sdk::service_factory_base {
private:
    xpumusic_sdk::service_factory_base* foobar_factory_;  // 鍘熷鐨?foobar2000 宸ュ巶

public:
    /**
     * @brief 鏋勯€犲嚱鏁?
     * @param foobar_factory foobar2000 鏈嶅姟宸ュ巶
     */
    explicit ServiceFactoryWrapper(xpumusic_sdk::service_factory_base* foobar_factory);

    /**
     * @brief 鏋愭瀯鍑芥暟
     */
    ~ServiceFactoryWrapper() override = default;

    /**
     * @brief 鍒涘缓鏈嶅姟瀹炰緥
     * @return 鏈嶅姟鎸囬拡
     */
    xpumusic_sdk::service_base* create_service();

    // service_base implementation
    int service_add_ref() override;
    int service_release() override;

    /**
     * @brief 鑾峰彇鏈嶅姟 GUID
     * @return GUID 寮曠敤
     */
    const xpumusic_sdk::GUID& get_guid() const override;

    /**
     * @brief 鑾峰彇鍘熷宸ュ巶
     * @return foobar2000 宸ュ巶鎸囬拡
     */
    xpumusic_sdk::service_factory_base* get_original_factory() const { return foobar_factory_; }
};

/**
 * @class XpuMusicPluginLoader
 * @brief 鍔犺浇鍜岀鐞?foobar2000 鎻掍欢 DLL 鐨勪富绫?
 * 
 * 鍏抽敭鍔熻兘锛?
 * - DLL 鍔犺浇锛圵indows: LoadLibrary锛孡inux: dlopen锛?
 * - 鏈嶅姟宸ュ巶鏋氫妇
 * - ABI 鍏煎鎬ф鏌?
 * - 渚濊禆瑙ｆ瀽
 * - 閿欒澶勭悊鍜屾姤鍛?
 */
class XpuMusicPluginLoader {
private:
    // 宸插姞杞芥ā鍧楀垪琛?
    std::vector<PluginModuleInfo> modules_;
    
    // 娉ㄥ唽鐨勬湇鍔?wrapper
    std::vector<std::unique_ptr<ServiceFactoryWrapper>> service_wrappers_;
    
    // 鐢ㄤ簬绾跨▼瀹夊叏
    mutable std::mutex mutex_;
    
    // 鐖跺吋瀹规€х鐞嗗櫒
    class XpuMusicCompatManager* compat_manager_ = nullptr;
    
    // 鏈嶅姟娉ㄥ唽琛ㄦˉ鎺?
    std::unique_ptr<ServiceRegistryBridge> registry_bridge_;
    
    //==================================================================
    // 鍐呴儴鏂规硶
    //==================================================================
    
    /**
     * @brief 骞冲彴鐩稿叧鐨?DLL 鍔犺浇
     * @param path DLL 鏂囦欢璺緞
     * @param error_msg 閿欒淇℃伅杈撳嚭
     * @return 搴撳彞鏌勬垨 nullptr
     */
    void* load_library_internal(const char* path, std::string& error_msg);
    
    /**
     * @brief 骞冲彴鐩稿叧鐨勫簱鍗歌浇
     * @param handle 搴撳彞鏌?
     */
    void unload_library_internal(void* handle);
    
    /**
     * @brief 瑙ｆ瀽 foobar2000 鐨?_foobar2000_client_entry
     * @param handle 搴撳彞鏌?
     * @param module_info 妯″潡淇℃伅杈撳嚭
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result parse_client_entry(void* handle, PluginModuleInfo& module_info);
    
    /**
     * @brief 浠?DLL 瀵煎嚭瑙ｆ瀽鏈嶅姟宸ュ巶
     * @param handle 搴撳彞鏌?
     * @param services 鏈嶅姟鍒楄〃杈撳嚭
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result enumerate_services(void* handle, std::vector<ServiceExportInfo>& services);
    
    /**
     * @brief 娉ㄥ唽鏈嶅姟鍒扮郴缁?
     * @param services 瑕佹敞鍐岀殑鏈嶅姟鍒楄〃
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result register_services(const std::vector<ServiceExportInfo>& services);
    
    /**
     * @brief 楠岃瘉 ABI 鍏煎鎬?
     * @param handle 搴撳彞鏌?
     * @return true 濡傛灉 ABI 鍏煎
     */
    bool validate_abi_compatibility(void* handle);
    
    /**
     * @brief 楠岃瘉鎻掍欢渚濊禆
     * @param module_info 妯″潡淇℃伅
     * @return true 濡傛灉鎵€鏈変緷璧栨弧瓒?
     */
    bool validate_dependencies(const PluginModuleInfo& module_info);
    
    /**
     * @brief 鑾峰彇妯″潡淇℃伅鐨勮緟鍔╁嚱鏁?
     * @param handle 搴撳彞鏌?
     * @return 妯″潡淇℃伅
     */
    PluginModuleInfo get_module_info(void* handle, const std::string& path);
    
public:
    /**
     * @brief 鏋勯€犲嚱鏁?
     * @param compat_manager 鐖跺吋瀹规€х鐞嗗櫒
     */
    explicit XpuMusicPluginLoader(XpuMusicCompatManager* compat_manager);
    
    /**
     * @brief 鏋愭瀯鍑芥暟
     */
    ~XpuMusicPluginLoader();
    
    /**
     * @brief 鍔犺浇 foobar2000 鎻掍欢 DLL
     * @param dll_path DLL 鏂囦欢璺緞
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result load_plugin(const char* dll_path);
    
    /**
     * @brief 鍗歌浇鎻掍欢
     * @param dll_path DLL 鏂囦欢璺緞
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result unload_plugin(const char* dll_path);
    
    /**
     * @brief 鎵归噺鍔犺浇鐩綍涓殑鎵€鏈夋彃浠?
     * @param directory 鐩綍璺緞
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result load_plugins_from_directory(const char* directory);
    
    /**
     * @brief 鍗歌浇鎵€鏈夊凡鍔犺浇鎻掍欢
     */
    void unload_all();
    
    /**
     * @brief 鑾峰彇宸插姞杞芥ā鍧楁暟閲?
     * @return 妯″潡鏁伴噺
     */
    size_t get_module_count() const { return modules_.size(); }
    
    /**
     * @brief 鑾峰彇鎵€鏈夊凡鍔犺浇妯″潡鐨勪俊鎭?
     * @return 妯″潡淇℃伅鏁扮粍
     */
    const std::vector<PluginModuleInfo>& get_modules() const { return modules_; }
    
    /**
     * @brief 閫氳繃璺緞鑾峰彇妯″潡淇℃伅
     * @param path DLL 璺緞
     * @return 妯″潡淇℃伅鎸囬拡鎴?nullptr
     */
    const PluginModuleInfo* get_module(const char* path) const;
    
    /**
     * @brief 妫€鏌ユ彃浠舵槸鍚﹀凡鍔犺浇
     * @param path DLL 璺緞
     * @return true 濡傛灉宸插姞杞?
     */
    bool is_plugin_loaded(const char* path) const;
    
    /**
     * @brief 鑾峰彇鏈€鍚庡姞杞介敊璇紙鐢ㄤ簬璋冭瘯锛?
     * @return 閿欒淇℃伅瀛楃涓?
     */
    std::string get_last_error() const { return last_error_; }
    
    /**
     * @brief 璁剧疆鏈嶅姟娉ㄥ唽琛ㄦˉ鎺?
     * @param bridge 妗ユ帴瀹炰緥
     */
    void set_registry_bridge(std::unique_ptr<ServiceRegistryBridge> bridge);
    
    /**
     * @brief 鑾峰彇鎵€鏈夋敞鍐岀殑鏈嶅姟
     * @return 鏈嶅姟瀵煎嚭淇℃伅鐨勫悜閲?
     */
    const std::vector<ServiceExportInfo>& get_services() const { return registered_services_; }
    
private:
    // 鏈€鍚庨敊璇俊鎭?
    mutable std::string last_error_;
    
    // 宸叉敞鍐屾湇鍔＄紦瀛?
    std::vector<ServiceExportInfo> registered_services_;
};

/**
 * @class ServiceRegistryBridge
 * @brief 妗ユ帴 foobar2000 鏈嶅姟鍒版垜浠殑 ServiceRegistry
 *
 * 杩欐槸浣?foobar2000 鏈嶅姟鍦ㄦ垜浠殑鎾斁鍣ㄤ腑鍙敤鐨勫叧閿粍浠躲€?
 * 瀹冨皢 foobar2000 鐨勬湇鍔℃ā鍨嬶紙鍩轰簬 GUID锛夋槧灏勫埌鎴戜滑鐨?ServiceRegistry锛堝熀浜?ServiceID锛夈€?
 */
class ServiceRegistryBridge {
public:
    /**
     * @brief 娉ㄥ唽鏈嶅姟
     * @param guid foobar2000 鏈嶅姟 GUID
     * @param factory_wrapper 鍖呰宸ュ巶
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    virtual Result register_service(const xpumusic_sdk::GUID& guid,
                                   ServiceFactoryWrapper* factory_wrapper) = 0;

    /**
     * @brief 娉ㄩ攢鏈嶅姟
     * @param guid 鏈嶅姟 GUID
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    virtual Result unregister_service(const xpumusic_sdk::GUID& guid) = 0;

    /**
     * @brief 鎸?GUID 鏌ヨ鏈嶅姟
     * @param guid 鏈嶅姟 GUID
     * @return 鏈嶅姟瀹炰緥鎴?nullptr
     */
    virtual xpumusic_sdk::service_base* query_service(const xpumusic_sdk::GUID& guid) = 0;

    /**
     * @brief 鎸?GUID 鏌ヨ宸ュ巶
     * @param guid 鏈嶅姟 GUID
     * @return 宸ュ巶鎸囬拡鎴?nullptr
     */
    virtual xpumusic_sdk::service_factory_base* query_factory(const xpumusic_sdk::GUID& guid) = 0;

    /**
     * @brief 鑾峰彇鎵€鏈夊凡娉ㄥ唽鏈嶅姟
     * @return GUID 鍚戦噺
     */
    virtual std::vector<xpumusic_sdk::GUID> get_registered_services() const = 0;

    /**
     * @brief 鑾峰彇鏈嶅姟鏁伴噺
     * @return 鏈嶅姟鏁伴噺
     */
    virtual size_t get_service_count() const = 0;

    /**
     * @brief 娓呴櫎鎵€鏈夋湇鍔?
     */
    virtual void clear() = 0;

    /**
     * @brief 铏氭嫙鏋愭瀯鍑芥暟
     */
    virtual ~ServiceRegistryBridge() = default;
};

} // namespace compat
} // namespace mp
