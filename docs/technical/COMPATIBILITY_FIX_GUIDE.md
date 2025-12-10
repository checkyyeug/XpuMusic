# XpuMusic v2.0 兼容性修复完整指南

## 修复概述

本指南提供了修复foobar2000兼容层167+个编译错误的完整方案。

## 修复步骤

### 第一步：替换冲突文件

1. **删除重复定义的文件**
```bash
rm -f compat/sdk_implementations/file_info_types.h
rm -f compat/sdk_implementations/metadb_handle_interface.h
rm -f compat/sdk_implementations/metadb_handle_impl.h
```

2. **使用新的统一SDK**
   - 将 `compat/foobar_sdk/foobar2000_sdk_complete.h` 作为主要SDK文件
   - 更新所有包含旧SDK的文件

### 第二步：更新包含路径

所有需要使用foobar2000兼容层的文件应该包含：
```cpp
#include "compat/foobar_sdk/foobar2000_sdk_complete.h"
// 或者使用新实现
#include "compat/sdk_implementations/metadb_handle_impl_new.h"
#include "compat/plugin_loader/service_registry_bridge_new.h"
```

### 第三步：修复编译错误

#### 1. service_base实现错误

**错误示例**：
```
error: 'class foobar2000_sdk::service_factory_base' has no member named 'create_service'
```

**修复方案**：使用新的service_factory_base实现（已在foobar2000_sdk_complete.h中）

#### 2. 抽象类实例化错误

**错误示例**：
```
error: cannot declare field 'foobar2000_sdk::metadb_handle_impl::location_'
to be of abstract type 'foobar2000_sdk::playable_location'
```

**修复方案**：使用新的metadb_handle_impl_new.h，其中playable_location是具体类

#### 3. service_ptr不完整类型错误

**错误示例**：
```
error: invalid use of incomplete type 'class foobar2000_sdk::service_ptr'
```

**修复方案**：使用foobar2000_sdk_complete.h中的完整service_ptr_t模板

### 第四步：关键文件修复

#### 1. 修复 compat/sdk_implementations/service_base.cpp

```cpp
#include "compat/foobar_sdk/foobar2000_sdk_complete.h"

namespace foobar2000_sdk {

// Simple service base implementation with reference counting
class service_base_impl : public service_base {
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

} // namespace foobar2000_sdk
```

#### 2. 修复 compat/plugin_loader/plugin_loader.cpp

```cpp
#include "compat/foobar_sdk/foobar2000_sdk_complete.h"
#include "compat/plugin_loader/service_registry_bridge_new.h"

namespace mp::compat {

class ServiceFactoryWrapper : public foobar2000_sdk::service_factory_base {
private:
    GUID guid_;
    std::function<foobar2000_sdk::service_base*()> creator_;
    std::atomic<int> ref_count_{1};

public:
    ServiceFactoryWrapper(const GUID& guid,
                         std::function<foobar2000_sdk::service_base*()> creator)
        : guid_(guid), creator_(creator) {}

    const GUID& get_guid() const override {
        return guid_;
    }

    foobar2000_sdk::service_base* create_service() override {
        return creator_ ? creator_() : nullptr;
    }

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

} // namespace mp::compat
```

### 第五步：更新CMakeLists.txt

```cmake
# 在 compat/CMakeLists.txt 中
set(FOOBAR_COMPAT_SOURCES
    foobar_sdk/foobar2000_sdk_complete.h
    sdk_implementations/metadb_handle_impl_new.h
    plugin_loader/service_registry_bridge_new.h
    foobar_compat_manager.cpp
)

# 确保使用新的头文件
target_include_directories(foobar_compat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/foobar_sdk
)
```

### 第六步：测试修复

创建测试文件 `tests/test_foobar_compatibility.cpp`：

```cpp
#include "compat/foobar_sdk/foobar2000_sdk_complete.h"
#include "compat/sdk_implementations/metadb_handle_impl_new.h"
#include "compat/plugin_loader/service_registry_bridge_new.h"
#include <gtest/gtest.h>

using namespace foobar2000_sdk;

TEST(FoobarCompatibility, BasicSDK) {
    // Test GUID comparison
    GUID guid1 = {0x12345678, 0x1234, 0x5678, {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}};
    GUID guid2 = guid1;

    EXPECT_EQ(guid1, guid2);
}

TEST(FoobarCompatibility, ServicePointer) {
    // Test service_ptr_t
    service_ptr_t<service_base> ptr;
    EXPECT_TRUE(ptr.is_empty());

    // Test reference counting would require actual service implementation
}

TEST(FoobarCompatibility, PlayableLocation) {
    playable_location loc("/path/to/file.mp3", 1);

    EXPECT_STREQ(loc.get_path(), "/path/to/file.mp3");
    EXPECT_EQ(loc.get_subsong_index(), 1);
    EXPECT_STREQ(loc.get_subsong(), "1");
}

TEST(FoobarCompatibility, MetadbHandle) {
    auto handle = create_metadb_handle();
    EXPECT_TRUE(handle != nullptr);

    // Test file info access
    auto& info = handle->get_info_ref();
    auto audio_info = info.get_info();
    EXPECT_EQ(audio_info.m_sample_rate, 44100);
}

TEST(FoobarCompatibility, ServiceRegistry) {
    auto& registry = service_registry_bridge::get_instance();

    // Test service count
    EXPECT_EQ(registry.get_service_count(), 0);
}
```

### 第七步：构建验证

```bash
# 清理构建目录
rm -rf build
mkdir build
cd build

# 配置和构建
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4 2>&1 | tee build.log

# 检查错误
grep -i error build.log || echo "No errors found!"
```

## 修复后的文件结构

```
compat/
├── foobar_sdk/
│   └── foobar2000_sdk_complete.h     # 统一的SDK定义
├── sdk_implementations/
│   ├── metadb_handle_impl_new.h      # 新的metadb实现
│   └── service_base.cpp              # 基础服务实现
├── plugin_loader/
│   ├── service_registry_bridge_new.h # 新的服务桥接
│   └── plugin_loader.cpp             # 更新的插件加载器
└── foobar_compat_manager.cpp         # 兼容管理器
```

## 常见错误及解决方案

### 1. "redefinition of 'struct audio_info'"

**原因**：仍然包含了旧的头文件
**解决**：确保只包含 `foobar2000_sdk_complete.h`

### 2. "abstract class type 'playable_location'"

**原因**：使用了旧的抽象类定义
**解决**：使用新的具体类实现

### 3. "invalid use of incomplete type 'service_ptr'"

**原因**：使用了未定义的前向声明
**解决**：使用 `service_ptr_t<T>` 模板

### 4. "no member named 'service_add_ref'"

**原因**：service_factory_base缺少方法
**解决**：确保使用新版本的SDK定义

## 性能考虑

1. **引用计数优化**：使用原子操作确保线程安全
2. **单例缓存**：对常用服务使用单例模式
3. **延迟加载**：按需创建服务实例

## 调试技巧

1. **启用详细日志**：
   ```cpp
   #define FOOBAR_DEBUG 1
   ```

2. **使用gdb调试符号冲突**：
   ```bash
   gdb ./build/libfoobar_compat.so
   (gdb) info symbols audio_info
   ```

3. **编译时检查头文件包含**：
   ```bash
   g++ -E compat/foobar_sdk/foobar2000_sdk_complete.h | grep "struct audio_info"
   ```

## 后续优化建议

1. **减少头文件依赖**：使用前向声明
2. **实现更多foobar2000接口**：逐步完善兼容性
3. **添加插件示例**：提供完整的插件开发模板
4. **性能测试**：确保兼容层不影响性能

## 预期结果

修复后应该能够：
- ✅ 编译所有兼容层代码无错误
- ✅ 加载基本的foobar2000插件
- ✅ 正确处理服务注册和查找
- ✅ 运行兼容性测试套件

## 维护指南

1. **保持SDK更新**：定期同步foobar2000接口变化
2. **文档更新**：记录所有API变更
3. **测试覆盖**：为新功能添加测试
4. **性能监控**：定期检查性能退化