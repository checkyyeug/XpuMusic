/**
 * @file plugin_loader.h
 * @brief foobar2000 DLL 插件加载器
 * @date 2025-12-09
 * 
 * 这是修复 #2 的核心：能够加载 Windows DLL（.dll 文件）并解析
 * foobar2000 的服务工厂导出，使原生插件可用。
 */

#pragma once

// Windows: 避免 GUID 重定义
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
// Linux/macOS: 使用 dlopen
#include <dlfcn.h>
#endif

// 使用 xpumusic_sdk 命名空间中的类型
using xpumusic_sdk::service_factory_base;
// Note: service_ptr is not defined, using service_ptr_t instead
template<typename T>
using service_ptr = xpumusic_sdk::service_ptr_t<T>;
using xpumusic_sdk::service_base;
#ifdef GUID_DEFINED
// Windows 已经定义了 GUID，使用全局命名空间
using ::GUID;
#else
using xpumusic_sdk::GUID;
#endif

namespace mp {
namespace compat {

// 前向声明
class ServiceRegistryBridge;

/**
 * @struct PluginModuleInfo
 * @brief 已加载插件模块的信息
 */
struct PluginModuleInfo {
    std::string path;                    // DLL 文件路径
    std::string name;                    // 插件名称（从 file_info 提取）
    std::string version;                 // 插件版本
    void* library_handle = nullptr;      // 平台相关的库句柄
    bool loaded = false;                 // 是否成功加载
    bool initialized = false;            // 是否已初始化
    std::string error;                   // 加载错误信息
    
    // 统计信息
    uint32_t service_count = 0;          // 注册的服务数量
    uint64_t load_time_ms = 0;           // 加载耗时（毫秒）
    
    PluginModuleInfo() = default;
};

/**
 * @struct ServiceExportInfo
 * @brief 从 DLL 导出的服务信息
 */
struct ServiceExportInfo {
    std::string name;                    // 服务名称
    GUID guid;                           // 服务 GUID
    service_factory_base* factory;       // 服务工厂指针
    bool available;                      // 是否可用
    
    ServiceExportInfo() : factory(nullptr), available(false) {
        memset(&guid, 0, sizeof(GUID));
    }
};

/**
 * @class ServiceFactoryWrapper
 * @brief 包装 foobar2000 服务工厂到我们系统
 *
 * 这桥接了 foobar2000 的 service_factory_base 和
 * 我们自己的服务注册表。
 */
class ServiceFactoryWrapper : public xpumusic_sdk::service_factory_base {
private:
    xpumusic_sdk::service_factory_base* foobar_factory_;  // 原始的 foobar2000 工厂

public:
    /**
     * @brief 构造函数
     * @param foobar_factory foobar2000 服务工厂
     */
    explicit ServiceFactoryWrapper(xpumusic_sdk::service_factory_base* foobar_factory);

    /**
     * @brief 析构函数
     */
    ~ServiceFactoryWrapper() override = default;

    /**
     * @brief 创建服务实例
     * @return 服务指针
     */
    xpumusic_sdk::service_base* create_service();

    // service_base implementation
    int service_add_ref() override;
    int service_release() override;

    /**
     * @brief 获取服务 GUID
     * @return GUID 引用
     */
    const xpumusic_sdk::GUID& get_guid() const override;

    /**
     * @brief 获取原始工厂
     * @return foobar2000 工厂指针
     */
    xpumusic_sdk::service_factory_base* get_original_factory() const { return foobar_factory_; }
};

/**
 * @class XpuMusicPluginLoader
 * @brief 加载和管理 foobar2000 插件 DLL 的主类
 * 
 * 关键功能：
 * - DLL 加载（Windows: LoadLibrary，Linux: dlopen）
 * - 服务工厂枚举
 * - ABI 兼容性检查
 * - 依赖解析
 * - 错误处理和报告
 */
class XpuMusicPluginLoader {
private:
    // 已加载模块列表
    std::vector<PluginModuleInfo> modules_;
    
    // 注册的服务 wrapper
    std::vector<std::unique_ptr<ServiceFactoryWrapper>> service_wrappers_;
    
    // 用于线程安全
    mutable std::mutex mutex_;
    
    // 父兼容性管理器
    class XpuMusicCompatManager* compat_manager_ = nullptr;
    
    // 服务注册表桥接
    std::unique_ptr<ServiceRegistryBridge> registry_bridge_;
    
    //==================================================================
    // 内部方法
    //==================================================================
    
    /**
     * @brief 平台相关的 DLL 加载
     * @param path DLL 文件路径
     * @param error_msg 错误信息输出
     * @return 库句柄或 nullptr
     */
    void* load_library_internal(const char* path, std::string& error_msg);
    
    /**
     * @brief 平台相关的库卸载
     * @param handle 库句柄
     */
    void unload_library_internal(void* handle);
    
    /**
     * @brief 解析 foobar2000 的 _foobar2000_client_entry
     * @param handle 库句柄
     * @param module_info 模块信息输出
     * @return Result 成功或错误
     */
    Result parse_client_entry(void* handle, PluginModuleInfo& module_info);
    
    /**
     * @brief 从 DLL 导出解析服务工厂
     * @param handle 库句柄
     * @param services 服务列表输出
     * @return Result 成功或错误
     */
    Result enumerate_services(void* handle, std::vector<ServiceExportInfo>& services);
    
    /**
     * @brief 注册服务到系统
     * @param services 要注册的服务列表
     * @return Result 成功或错误
     */
    Result register_services(const std::vector<ServiceExportInfo>& services);
    
    /**
     * @brief 验证 ABI 兼容性
     * @param handle 库句柄
     * @return true 如果 ABI 兼容
     */
    bool validate_abi_compatibility(void* handle);
    
    /**
     * @brief 验证插件依赖
     * @param module_info 模块信息
     * @return true 如果所有依赖满足
     */
    bool validate_dependencies(const PluginModuleInfo& module_info);
    
    /**
     * @brief 获取模块信息的辅助函数
     * @param handle 库句柄
     * @return 模块信息
     */
    PluginModuleInfo get_module_info(void* handle, const std::string& path);
    
public:
    /**
     * @brief 构造函数
     * @param compat_manager 父兼容性管理器
     */
    explicit XpuMusicPluginLoader(XpuMusicCompatManager* compat_manager);
    
    /**
     * @brief 析构函数
     */
    ~XpuMusicPluginLoader();
    
    /**
     * @brief 加载 foobar2000 插件 DLL
     * @param dll_path DLL 文件路径
     * @return Result 成功或错误
     */
    Result load_plugin(const char* dll_path);
    
    /**
     * @brief 卸载插件
     * @param dll_path DLL 文件路径
     * @return Result 成功或错误
     */
    Result unload_plugin(const char* dll_path);
    
    /**
     * @brief 批量加载目录中的所有插件
     * @param directory 目录路径
     * @return Result 成功或错误
     */
    Result load_plugins_from_directory(const char* directory);
    
    /**
     * @brief 卸载所有已加载插件
     */
    void unload_all();
    
    /**
     * @brief 获取已加载模块数量
     * @return 模块数量
     */
    size_t get_module_count() const { return modules_.size(); }
    
    /**
     * @brief 获取所有已加载模块的信息
     * @return 模块信息数组
     */
    const std::vector<PluginModuleInfo>& get_modules() const { return modules_; }
    
    /**
     * @brief 通过路径获取模块信息
     * @param path DLL 路径
     * @return 模块信息指针或 nullptr
     */
    const PluginModuleInfo* get_module(const char* path) const;
    
    /**
     * @brief 检查插件是否已加载
     * @param path DLL 路径
     * @return true 如果已加载
     */
    bool is_plugin_loaded(const char* path) const;
    
    /**
     * @brief 获取最后加载错误（用于调试）
     * @return 错误信息字符串
     */
    std::string get_last_error() const { return last_error_; }
    
    /**
     * @brief 设置服务注册表桥接
     * @param bridge 桥接实例
     */
    void set_registry_bridge(std::unique_ptr<ServiceRegistryBridge> bridge);
    
    /**
     * @brief 获取所有注册的服务
     * @return 服务导出信息的向量
     */
    const std::vector<ServiceExportInfo>& get_services() const { return registered_services_; }
    
private:
    // 最后错误信息
    mutable std::string last_error_;
    
    // 已注册服务缓存
    std::vector<ServiceExportInfo> registered_services_;
};

/**
 * @class ServiceRegistryBridge
 * @brief 桥接 foobar2000 服务到我们的 ServiceRegistry
 *
 * 这是使 foobar2000 服务在我们的播放器中可用的关键组件。
 * 它将 foobar2000 的服务模型（基于 GUID）映射到我们的 ServiceRegistry（基于 ServiceID）。
 */
class ServiceRegistryBridge {
public:
    /**
     * @brief 注册服务
     * @param guid foobar2000 服务 GUID
     * @param factory_wrapper 包装工厂
     * @return Result 成功或错误
     */
    virtual Result register_service(const xpumusic_sdk::GUID& guid,
                                   ServiceFactoryWrapper* factory_wrapper) = 0;

    /**
     * @brief 注销服务
     * @param guid 服务 GUID
     * @return Result 成功或错误
     */
    virtual Result unregister_service(const xpumusic_sdk::GUID& guid) = 0;

    /**
     * @brief 按 GUID 查询服务
     * @param guid 服务 GUID
     * @return 服务实例或 nullptr
     */
    virtual xpumusic_sdk::service_base* query_service(const xpumusic_sdk::GUID& guid) = 0;

    /**
     * @brief 按 GUID 查询工厂
     * @param guid 服务 GUID
     * @return 工厂指针或 nullptr
     */
    virtual xpumusic_sdk::service_factory_base* query_factory(const xpumusic_sdk::GUID& guid) = 0;

    /**
     * @brief 获取所有已注册服务
     * @return GUID 向量
     */
    virtual std::vector<xpumusic_sdk::GUID> get_registered_services() const = 0;

    /**
     * @brief 获取服务数量
     * @return 服务数量
     */
    virtual size_t get_service_count() const = 0;

    /**
     * @brief 清除所有服务
     */
    virtual void clear() = 0;

    /**
     * @brief 虚拟析构函数
     */
    virtual ~ServiceRegistryBridge() = default;
};

} // namespace compat
} // namespace mp
