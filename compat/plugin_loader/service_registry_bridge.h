/**
 * @file service_registry_bridge.h
 * @brief ServiceRegistryBridge 瀹炵幇
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
 * @brief ServiceRegistryBridge 鐨勫叿浣撳疄鐜?
 * 
 * 杩欎釜绫绘ˉ鎺ヤ簡 XpuMusic 鐨勬湇鍔℃ā鍨嬶紙鍩轰簬 GUID锛夊拰
 * 鎴戜滑鐨?ServiceRegistry锛堝熀浜?ServiceID 鍝堝笇锛夈€?
 */
class ServiceRegistryBridgeImpl : public ServiceRegistryBridge {
private:
    // 鎸囧悜鎴戜滑鐨勬湇鍔℃敞鍐岃〃
    core::ServiceRegistry* service_registry_;
    
    // 鏄犲皠 GUID -> ServiceFactoryWrapper
    struct ServiceEntry {
        GUID guid;
        ServiceFactoryWrapper* wrapper;
        service_ptr instance;  // 鍙€夌殑鍗曚緥缂撳瓨
        bool is_singleton;
        
        ServiceEntry() : wrapper(nullptr), is_singleton(false) {
            memset(&guid, 0, sizeof(GUID));
        }
    };
    
    std::unordered_map<uint64_t, ServiceEntry> services_;
    
    // 浜掓枼閿佺敤浜庣嚎绋嬪畨鍏?
    mutable std::mutex mutex_;
    
    /**
     * @brief GUID -> uint64_t 鍝堝笇锛堢敤浜?unordered_map锛?
     * @param guid foobar2000 GUID
     * @return 64-bit 鍝堝笇鍊?
     */
    static uint64_t hash_guid(const GUID& guid);
    
    /**
     * @brief 浠?GUID 璁＄畻 ServiceID
     * @param guid foobar2000 GUID
     * @return ServiceID锛堝搱甯岋級
     */
    static mp::ServiceID guid_to_service_id(const GUID& guid);
    
public:
    /**
     * @brief 鏋勯€犲嚱鏁?
     * @param service_registry 鐖舵湇鍔℃敞鍐岃〃
     */
    explicit ServiceRegistryBridgeImpl(core::ServiceRegistry* service_registry);
    
    /**
     * @brief 鏋愭瀯鍑芥暟
     */
    ~ServiceRegistryBridgeImpl() override;
    
    /**
     * @brief 娉ㄥ唽鏈嶅姟
     * @param guid XpuMusic 鏈嶅姟 GUID
     * @param factory_wrapper 鍖呰宸ュ巶
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result register_service(const xpumusic_sdk::GUID& guid, ServiceFactoryWrapper* factory_wrapper) override;

    /**
     * @brief 娉ㄩ攢鏈嶅姟
     * @param guid 鏈嶅姟 GUID
     * @return Result 鎴愬姛鎴栭敊璇?
     */
    Result unregister_service(const xpumusic_sdk::GUID& guid) override;

    /**
     * @brief 鎸?GUID 鏌ヨ鏈嶅姟
     * @param guid 鏈嶅姟 GUID
     * @return 鏈嶅姟瀹炰緥鎴?nullptr
     */
    xpumusic_sdk::service_ptr query_service(const xpumusic_sdk::GUID& guid) override;

    /**
     * @brief 鎸?GUID 鏌ヨ宸ュ巶
     * @param guid 鏈嶅姟 GUID
     * @return 宸ュ巶鎸囬拡鎴?nullptr
     */
    xpumusic_sdk::service_factory_base* query_factory(const xpumusic_sdk::GUID& guid) override;

    /**
     * @brief 鑾峰彇鎵€鏈夊凡娉ㄥ唽鏈嶅姟
     * @return GUID 鍚戦噺
     */
    std::vector<xpumusic_sdk::GUID> get_registered_services() const override;
    
    /**
     * @brief 鑾峰彇鏈嶅姟鏁伴噺
     * @return 鏈嶅姟鏁伴噺
     */
    size_t get_service_count() const override;

    /**
     * @brief 娓呴櫎鎵€鏈夋湇鍔?
     */
    void clear() override;
};

// ServiceRegistryBridgeImpl 瀹炵幇

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
    // 绠€鍗曞搱甯岋細缁勫悎 GUID 鐨勫瓧娈?
    uint64_t hash = guid.Data1;
    hash = (hash << 16) ^ guid.Data2;
    hash = (hash << 16) ^ guid.Data3;
    
    // 涔熷寘鎷墠 8 瀛楄妭鐨?Data4
    uint32_t* data4_int = reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(guid.Data4));
    hash ^= data4_int[0];
    hash ^= (static_cast<uint64_t>(data4_int[1]) << 32);
    
    return hash;
}

mp::ServiceID ServiceRegistryBridgeImpl::guid_to_service_id(const GUID& guid) {
    // 鐢熸垚涓€鑷寸殑 ServiceID 浠?GUID
    // 浣跨敤 MP 鐨?hash_string 绠楁硶锛屼絾閫傚簲 GUID 缁撴瀯
    
    std::stringstream ss;
    ss << guid.Data1 << "-" << guid.Data2 << "-" << guid.Data3 << "-";
    for (int i = 0; i < 8; ++i) {
        ss << static_cast<int>(guid.Data4[i]);
    }
    
    // 娉ㄦ剰锛氱洿鎺ヨ繑鍥?GUID 鍝堝笇浣滀负 ServiceID
    // 杩欎笉鏄悊鎯崇殑鏂规硶锛屼絾鏈€绠€鍗曠殑妗ユ帴鏂瑰紡
    return static_cast<mp::ServiceID>(hash_guid(guid));
}

Result ServiceRegistryBridgeImpl::register_service(const GUID& guid, 
                                                  ServiceFactoryWrapper* factory_wrapper) {
    if (!factory_wrapper) {
        return Result::InvalidParameter;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t hash = hash_guid(guid);
    
    // 妫€鏌ユ槸鍚﹀凡娉ㄥ唽
    auto it = services_.find(hash);
    if (it != services_.end()) {
        return Result::AlreadyInitialized;
    }
    
    // 鍒涘缓鏈嶅姟鏉＄洰
    ServiceEntry entry;
    entry.guid = guid;
    entry.wrapper = factory_wrapper;
    entry.is_singleton = false;  // TODO: 妫€娴嬪崟渚嬫ā寮?
    
    // 璁＄畻 ServiceID锛堜粠 GUID锛?
    mp::ServiceID service_id = guid_to_service_id(guid);
    
    // 娉ㄥ唽鍒版垜浠殑鏈嶅姟娉ㄥ唽琛?
    // 娉ㄦ剰锛氳繖闇€瑕佹敞鍐?void* 鎸囬拡锛堟湇鍔″疄渚嬫垨宸ュ巶锛?
    Result result = service_registry_->register_service(service_id, factory_wrapper);
    if (result != Result::Success) {
        return result;
    }
    
    // 淇濆瓨鏉＄洰
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
    
    // 浠庢垜浠殑鏈嶅姟娉ㄥ唽琛ㄦ敞閿€
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
    
    // 濡傛灉杩欐槸鍗曚緥涓旀垜浠凡鏈夊疄渚嬶紝杩斿洖瀹?
    if (entry.is_singleton && entry.instance.is_valid()) {
        // 璋冪敤 add_ref 澧炲姞寮曠敤璁℃暟
        if (entry.instance.get_ptr()) {
            entry.instance.get_ptr()->service_add_ref();
        }
        return entry.instance;
    }
    
    // 鍚﹀垯浠庡伐鍘傚垱寤烘柊瀹炰緥
    if (!entry.wrapper) {
        return service_ptr();
    }
    
    service_ptr instance = entry.wrapper->create_service();
    
    // 濡傛灉鏄崟渚嬶紝缂撳瓨瀹炰緥
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
    
    // 娉ㄩ攢鎵€鏈夋湇鍔?
    for (const auto& pair : services_) {
        const GUID& guid = pair.second.guid;
        mp::ServiceID service_id = guid_to_service_id(guid);
        service_registry_->unregister_service(service_id);
    }
    
    services_.clear();
}

} // namespace compat
} // namespace mp
