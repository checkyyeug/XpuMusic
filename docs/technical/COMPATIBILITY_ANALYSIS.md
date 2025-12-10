# XpuMusic v2.0 核心兼容性问题分析

## 问题概述

XpuMusic的foobar2000兼容层存在严重的架构问题，导致167+个编译错误。主要问题是同时存在两套冲突的SDK定义。

## 核心问题分析

### 1. 双SDK系统冲突

#### 问题文件
- `compat/foobar_sdk/foobar2000_sdk.h` - 主要SDK定义
- `compat/sdk_implementations/file_info_types.h` - 重复定义
- `compat/sdk_implementations/metadb_handle_interface.h` - 重复类定义

#### 冲突详情
```cpp
// foobar2000_sdk.h 定义
struct audio_info {
    uint32_t m_sample_rate = 44100;
    uint32_t m_channels = 2;
    uint32_t m_bitrate = 0;
    double m_length = 0.0;
};

// file_info_types.h 重复定义
struct audio_info {
    uint32_t m_sample_rate;      // 无默认值
    uint32_t m_channels;
    uint32_t m_bitrate;
    double   m_length;
};
```

### 2. 抽象类实例化错误

#### playable_location类问题
- 定义为抽象类，但在实现中尝试实例化
- 缺少纯虚函数的实现

```cpp
class playable_location : public location_t {
public:
    // 缺少抽象方法实现
    virtual const char* get_subsong() const = 0;  // 未实现
    virtual int get_subsong_index() const = 0;     // 未实现
};
```

### 3. 服务基类方法缺失

#### service_base子类问题
- ServiceFactoryWrapper缺少必需的service_add_ref/service_release实现
- service_ptr_t模板实现不完整

```cpp
class ServiceFactoryWrapper : public foobar2000_sdk::service_factory_base {
    // 错误：没有实现这些纯虚函数
    virtual int service_add_ref() override;  // 缺少实现
    virtual int service_release() override; // 缺少实现
};
```

### 4. 不完整类型使用

#### service_ptr问题
- service_ptr被前向声明但从未完整定义
- 在service_base.cpp中尝试使用不完整类型

## 解决方案

### 第一阶段：统一SDK定义

1. **删除重复定义文件**
   - 删除 `compat/sdk_implementations/file_info_types.h`
   - 删除 `compat/sdk_implementations/metadb_handle_interface.h`
   - 将所有定义集中在 `foobar2000_sdk.h`

2. **完善foobar2000_sdk.h**
   - 添加所有缺失的结构体定义
   - 完善service_ptr_t模板实现
   - 添加缺失的抽象方法

### 第二阶段：实现抽象类方法

1. **playable_location类实现**
```cpp
class playable_location : public location_t {
private:
    int subsong_index_ = 0;

public:
    playable_location() = default;
    playable_location(const std::string& path, int subsong)
        : location_t(path), subsong_index_(subsong) {}

    const char* get_subsong() const override {
        // 返回格式化的子歌曲信息
        static thread_local std::string subsong;
        subsong = std::to_string(subsong_index_);
        return subsong.c_str();
    }

    int get_subsong_index() const override {
        return subsong_index_;
    }
};
```

2. **ServiceFactoryWrapper实现**
```cpp
class ServiceFactoryWrapper : public foobar2000_sdk::service_factory_base {
private:
    std::atomic<int> ref_count_{1};

public:
    int service_add_ref() override {
        return ++ref_count_;
    }

    int service_release() override {
        if (--ref_count_ == 0) {
            delete this;
            return 0;
        }
        return ref_count_;
    }
};
```

### 第三阶段：完善服务桥接

1. **ServiceRegistryBridge完整实现**
```cpp
class ServiceRegistryBridge {
private:
    std::unordered_map<GUID, std::unique_ptr<service_factory_base>> services_;
    std::mutex mutex_;

public:
    template<typename T>
    void register_service(const GUID& guid, std::unique_ptr<T> factory) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[guid] = std::move(factory);
    }

    template<typename T>
    service_ptr_t<T> get_service(const GUID& guid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(guid);
        if (it != services_.end()) {
            auto* ptr = static_cast<T*>(it->second->create_service());
            return service_ptr_t<T>(ptr);
        }
        return service_ptr_t<T>();
    }
};
```

## 具体修复步骤

### 步骤1：修复SDK头文件

1. 更新 `compat/foobar_sdk/foobar2000_sdk.h`
2. 添加完整的playable_location实现
3. 完善service_ptr_t模板
4. 添加所有必需的接口定义

### 步骤2：移除冲突文件

```bash
# 删除重复定义文件
rm compat/sdk_implementations/file_info_types.h
rm compat/sdk_implementations/metadb_handle_interface.h
```

### 步骤3：更新实现文件

1. 更新 `compat/sdk_implementations/service_base.cpp`
   - 实现完整的服务基类
   - 添加引用计数实现

2. 更新 `compat/sdk_implementations/metadb_handle_impl.cpp`
   - 修复抽象类问题
   - 实现所有虚函数

3. 更新 `compat/plugin_loader/plugin_loader.cpp`
   - 修复ServiceFactoryWrapper
   - 添加必需的虚函数实现

### 步骤4：完善测试

创建兼容性测试：
```cpp
// tests/test_foobar_compatibility.cpp
TEST(FoobarCompatibility, ServiceRegistry) {
    ServiceRegistryBridge& bridge = ServiceRegistryBridge::get_instance();

    // 注册服务
    auto factory = std::make_unique<TestServiceFactory>();
    bridge.register_service<TestService>(TEST_GUID, std::move(factory));

    // 获取服务
    auto service = bridge.get_service<TestService>(TEST_GUID);
    EXPECT_TRUE(service.is_valid());
}
```

## 实现优先级

### 高优先级（必须修复）
1. ✅ 统一SDK定义 - 防止符号重定义
2. ✅ 完成service_base虚函数实现
3. ✅ 实现playable_location具体类
4. ✅ 完善service_ptr_t模板

### 中优先级（影响功能）
1. 实现ServiceRegistryBridge
2. 完善插件加载器
3. 添加错误处理机制

### 低优先级（增强功能）
1. 性能优化
2. 添加调试支持
3. 完善文档

## 预期结果

修复后应该能够：
- 编译通过所有兼容层代码
- 成功加载foobar2000插件
- 正确处理服务注册和查找
- 提供稳定的插件接口

## 风险评估

- **风险**：修改核心SDK可能影响现有代码
- **缓解**：保持向后兼容，逐步迁移
- **测试**：建立完整的测试套件

## 时间估算

- 统一SDK定义：2小时
- 修复抽象类：3小时
- 实现服务桥接：2小时
- 测试和验证：1小时

**总计**：约8小时