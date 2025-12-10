/**
 * @file service_registry_bridge.h
 * @brief ServiceRegistryBridge 实现
 * @date 2025-12-09
 */

#pragma once

#include "plugin_loader.h"
#include "../../core/service_registry.h"
#include <unordered_map>
#include <functional>

namespace mp {
namespace compat {

/**
 * @class ServiceRegistryBridgeImpl
 * @brief ServiceRegistryBridge 的具体实现
 * 
 * 这个类桥接了 XpuMusic 的服务模型（基于 GUID）和
 * 我们的 ServiceRegistry（基于 ServiceID 哈希）。
 */
class ServiceRegistryBridgeImpl : public ServiceRegistryBridge {
private:
    // 指向我们的服务注册表
    core::ServiceRegistry* service_registry_;
    
    // 映射 GUID -> ServiceFactoryWrapper
    struct ServiceEntry {
        GUID guid;
        ServiceFactoryWrapper* wrapper;
        service_ptr instance;  // 可选的单例缓存
        bool is_singleton;
        
        ServiceEntry() : wrapper(nullptr), is_singleton(false) {
            memset(&guid, 0, sizeof(GUID));
        }
    };
    
    std::unordered_map<uint64_t, ServiceEntry> services_;
    
    // 互斥锁用于线程安全
    mutable std::mutex mutex_;
    
    /**
     * @brief GUID -> uint64_t 哈希（用于 unordered_map）
     * @param guid foobar2000 GUID
     * @return 64-bit 哈希值
     */
    static uint64_t hash_guid(const GUID& guid);
    
    /**
     * @brief 从 GUID 计算 ServiceID
     * @param guid foobar2000 GUID
     * @return ServiceID（哈希）
     */
    static mp::ServiceID guid_to_service_id(const GUID& guid);
    
public:
    /**
     * @brief 构造函数
     * @param service_registry 父服务注册表
     */
    explicit ServiceRegistryBridgeImpl(core::ServiceRegistry* service_registry);
    
    /**
     * @brief 析构函数
     */
    ~ServiceRegistryBridgeImpl() override;
    
    /**
     * @brief 注册服务
     * @param guid XpuMusic 服务 GUID
     * @param factory_wrapper 包装工厂
     * @return Result 成功或错误
     */
    Result register_service(const xpumusic_sdk::GUID& guid, ServiceFactoryWrapper* factory_wrapper) override;

    /**
     * @brief 注销服务
     * @param guid 服务 GUID
     * @return Result 成功或错误
     */
    Result unregister_service(const xpumusic_sdk::GUID& guid) override;

    /**
     * @brief 按 GUID 查询服务
     * @param guid 服务 GUID
     * @return 服务实例或 nullptr
     */
    xpumusic_sdk::service_ptr query_service(const xpumusic_sdk::GUID& guid) override;

    /**
     * @brief 按 GUID 查询工厂
     * @param guid 服务 GUID
     * @return 工厂指针或 nullptr
     */
    xpumusic_sdk::service_factory_base* query_factory(const xpumusic_sdk::GUID& guid) override;

    /**
     * @brief 获取所有已注册服务
     * @return GUID 向量
     */
    std::vector<xpumusic_sdk::GUID> get_registered_services() const override;
    
    /**
     * @brief 获取服务数量
     * @return 服务数量
     */
    size_t get_service_count() const override;

    /**
     * @brief 清除所有服务
     */
    void clear() override;
};

// ServiceRegistryBridgeImpl 实现

ServiceRegistryBridgeImpl::ServiceRegistryBridgeImpl(core::ServiceRegistry* service_registry)
    : service_registry_(service_registry) {
    if (!service_registry) {
        throw std::invalid_argument("ServiceRegistry cannot be null");
    }
}

ServiceRegistryBridgeImpl::~ServiceRegistryBridgeImpl() {
    clear();
}

uint64_t ServiceRegistryBridgeImpl::hash_guid(const GUID& guid) {
    // 简单哈希：组合 GUID 的字段
    uint64_t hash = guid.Data1;
    hash = (hash << 16) ^ guid.Data2;
    hash = (hash << 16) ^ guid.Data3;
    
    // 也包括前 8 字节的 Data4
    uint32_t* data4_int = reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(guid.Data4));
    hash ^= data4_int[0];
    hash ^= (static_cast<uint64_t>(data4_int[1]) << 32);
    
    return hash;
}

mp::ServiceID ServiceRegistryBridgeImpl::guid_to_service_id(const GUID& guid) {
    // 生成一致的 ServiceID 从 GUID
    // 使用 MP 的 hash_string 算法，但适应 GUID 结构
    
    std::stringstream ss;
    ss << guid.Data1 << "-" << guid.Data2 << "-" << guid.Data3 << "-";
    for (int i = 0; i < 8; ++i) {
        ss << static_cast<int>(guid.Data4[i]);
    }
    
    // 注意：直接返回 GUID 哈希作为 ServiceID
    // 这不是理想的方法，但最简单的桥接方式
    return static_cast<mp::ServiceID>(hash_guid(guid));
}

Result ServiceRegistryBridgeImpl::register_service(const GUID& guid, 
                                                  ServiceFactoryWrapper* factory_wrapper) {
    if (!factory_wrapper) {
        return Result::InvalidParameter;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t hash = hash_guid(guid);
    
    // 检查是否已注册
    auto it = services_.find(hash);
    if (it != services_.end()) {
        return Result::AlreadyInitialized;
    }
    
    // 创建服务条目
    ServiceEntry entry;
    entry.guid = guid;
    entry.wrapper = factory_wrapper;
    entry.is_singleton = false;  // TODO: 检测单例模式
    
    // 计算 ServiceID（从 GUID）
    mp::ServiceID service_id = guid_to_service_id(guid);
    
    // 注册到我们的服务注册表
    // 注意：这需要注册 void* 指针（服务实例或工厂）
    Result result = service_registry_->register_service(service_id, factory_wrapper);
    if (result != Result::Success) {
        return result;
    }
    
    // 保存条目
    services_[hash] = entry;
    
    return Result::Success;
}

Result ServiceRegistryBridgeImpl::unregister_service(const GUID& guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t hash = hash_guid(guid);
    
    auto it = services_.find(hash);
    if (it == services_.end()) {
        return Result::FileNotFound;
    }
    
    // 从我们的服务注册表注销
    mp::ServiceID service_id = guid_to_service_id(guid);
    Result result = service_registry_->unregister_service(service_id);
    
    if (result == Result::Success) {
        services_.erase(it);
    }
    
    return result;
}

service_ptr ServiceRegistryBridgeImpl::query_service(const GUID& guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t hash = hash_guid(guid);
    
    auto it = services_.find(hash);
    if (it == services_.end()) {
        return service_ptr();
    }
    
    ServiceEntry& entry = it->second;
    
    // 如果这是单例且我们已有实例，返回它
    if (entry.is_singleton && entry.instance.is_valid()) {
        // 调用 add_ref 增加引用计数
        if (entry.instance.get_ptr()) {
            entry.instance.get_ptr()->service_add_ref();
        }
        return entry.instance;
    }
    
    // 否则从工厂创建新实例
    if (!entry.wrapper) {
        return service_ptr();
    }
    
    service_ptr instance = entry.wrapper->create_service();
    
    // 如果是单例，缓存实例
    if (entry.is_singleton) {
        entry.instance = instance;
    }
    
    return instance;
}

service_factory_base* ServiceRegistryBridgeImpl::query_factory(const GUID& guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t hash = hash_guid(guid);
    
    auto it = services_.find(hash);
    if (it == services_.end()) {
        return nullptr;
    }
    
    return it->second.wrapper;
}

std::vector<GUID> ServiceRegistryBridgeImpl::get_registered_services() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GUID> guids;
    guids.reserve(services_.size());
    
    for (const auto& pair : services_) {
        guids.push_back(pair.second.guid);
    }
    
    return guids;
}

size_t ServiceRegistryBridgeImpl::get_service_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return services_.size();
}

void ServiceRegistryBridgeImpl::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 注销所有服务
    for (const auto& pair : services_) {
        const GUID& guid = pair.second.guid;
        mp::ServiceID service_id = guid_to_service_id(guid);
        service_registry_->unregister_service(service_id);
    }
    
    services_.clear();
}

} // namespace compat
} // namespace mp
