# XpuMusic v2.0 插件生命周期管理

## 概述

XpuMusic v2.0 的插件系统提供了完整的生命周期管理，包括热加载、依赖管理、版本兼容性检查、配置持久化和错误处理。

## 插件生命周期

### 状态图

```
[NotLoaded] → [Loading] → [Loaded]
     ↑             ↓           ↓
     └── [Error] ← [Unloading] ←─┘
```

### 状态说明

| 状态 | 描述 | 可执行操作 |
|------|------|------------|
| NotLoaded | 插件未加载 | 加载 |
| Loading | 正在加载插件 | 等待 |
| Loaded | 插件已就绪 | 使用、卸载、重载 |
| Unloading | 正在卸载插件 | 等待 |
| Error | 加载/运行错误 | 重新加载 |
| Disabled | 已禁用 | 启用 |

## 热加载/卸载

### 配置热加载

```cpp
// 启用热加载
PluginManagerEnhanced manager;
HotReloadConfig config;
config.enabled = true;
config.watch_interval_ms = 1000;
config.auto_reload_on_change = true;
config.watch_extensions = {".so", ".dll", ".dylib"};

manager.initialize_enhanced(config);
```

### 手动热加载

```cpp
// 重新加载单个插件
bool success = manager.reload_plugin("my_decoder.so");

// 重新加载插件及其依赖
bool success = manager.reload_plugin_with_dependencies("my_decoder.so");

// 批量操作
PluginManagerEnhanced::BatchOperation op;
op.plugins_to_reload = {"decoder1.so", "decoder2.so"};
op.resolve_dependencies = true;
op.continue_on_error = false;
bool success = manager.execute_batch_operation(op);
```

### 文件监控机制

- **监控线程**：独立线程监控插件文件变化
- **检测间隔**：可配置，默认1000ms
- **触发条件**：文件修改时间变化
- **自动重载**：支持配置是否自动重载

## 依赖管理

### 依赖声明

插件需要在元数据中声明依赖：

```cpp
// 在插件的 get_info() 方法中返回
PluginInfo MyPlugin::get_info() const {
    PluginInfo info;
    info.name = "MyPlugin";
    info.version = "1.2.0";
    info.dependencies = {
        {"BaseLibrary", ">=1.0.0", false},  // 必需依赖
        {"OptionalLib", ">=0.5.0", true}   // 可选依赖
    };
    return info;
}
```

### 依赖解析策略

```cpp
DependencyConfig config;
config.auto_resolve = true;          // 自动解析依赖
config.allow_downgrade = false;       // 不允许降级
config.check_optional_deps = true;    // 检查可选依赖
config.max_resolve_attempts = 10;    // 最大解析尝试次数
```

### 依赖检查API

```cpp
// 检查插件依赖是否满足
bool satisfied = manager.check_dependencies("MyPlugin");

// 获取依赖树
auto deps = manager.get_dependency_tree("MyPlugin");

// 获取依赖此插件的其他插件
auto dependents = manager.get_dependents("MyPlugin");

// 检查是否可以安全卸载
bool safe = manager.can_safely_unload("MyPlugin");
```

## 版本兼容性

### 版本格式

XpuMusic 使用语义化版本号格式：`MAJOR.MINOR.PATCH`

- **MAJOR**：不兼容的API修改
- **MINOR**：向后兼容的功能新增
- **PATCH**：向后兼容的问题修正

### 兼容性检查

```cpp
// 检查插件是否兼容特定版本
bool compatible = manager.is_version_compatible("MyPlugin", "2.0.0");

// 获取插件的兼容版本范围
std::string range = manager.get_compatible_version_range("MyPlugin");
// 输出: "1.2.0 (compatible: >=1.0.0, <3.0.0)"
```

### API版本检查

```cpp
// 插件声明支持的API版本
const uint32_t MY_PLUGIN_API_VERSION = QODER_MAKE_VERSION(2, 0, 0);

// 主机检查插件API兼容性
bool PluginManagerEnhanced::check_api_version(const PluginInfo& info) {
    // 要求主版本号匹配
    uint32_t required_major = QODER_PLUGIN_API_VERSION >> 24;
    uint32_t plugin_major = info.api_version >> 24;
    return required_major == plugin_major;
}
```

## 配置持久化

### 配置文件结构

```json
{
    "version": "1.0.0",
    "plugins": {
        "my_decoder": {
            "quality": "high",
            "buffer_size": 8192,
            "enable_caching": true
        },
        "my_dsp": {
            "preset": "rock",
            "intensity": 0.8
        }
    },
    "metadata": {
        "my_decoder": {
            "name": "My Decoder",
            "version": "1.2.0",
            "author": "Developer",
            "auto_load": true,
            "hot_reloadable": true
        }
    },
    "versions": {
        "my_decoder": {
            "version": "1.2.0",
            "min_compatible": "1.0.0",
            "max_compatible": "2.0.0",
            "api_version": 67108864
        }
    }
}
```

### 配置管理API

```cpp
// 设置插件配置
nlohmann::json config;
config["quality"] = "high";
config["buffer_size"] = 8192;
manager.set_plugin_config("my_decoder", config);

// 获取插件配置（带默认值）
auto config = manager.get_plugin_config("my_decoder", {
    {"quality", "default"},
    {"buffer_size", 4096}
});

// 保存所有插件配置
manager.save_all_plugin_configs();

// 加载所有插件配置
manager.load_all_plugin_configs();
```

### 配置模式验证

插件可以提供配置模式用于验证：

```cpp
nlohmann::json MyPlugin::get_config_schema() const {
    return {
        {"quality", {
            {"type", "string"},
            {"enum", {"low", "medium", "high", "best"}},
            {"default", "medium"},
            {"description", "Decoding quality"}
        }},
        {"buffer_size", {
            {"type", "integer"},
            {"min", 1024},
            {"max", 65536},
            {"default", 4096},
            {"description", "Buffer size in samples"}
        }}
    };
}
```

## 错误处理和日志

### 错误类型

| 错误代码 | 描述 | 处理方式 |
|----------|------|----------|
| LOAD_ERROR | 插件加载失败 | 记录日志，继续其他插件 |
| DEPENDENCY_ERROR | 依赖不满足 | 停止加载，报告错误 |
| VERSION_ERROR | 版本不兼容 | 记录警告，尝试降级 |
| CONFIG_ERROR | 配置错误 | 使用默认配置 |
| RUNTIME_ERROR | 运行时错误 | 卸载插件，隔离错误 |

### 错误历史

```cpp
// 获取所有错误历史
auto errors = manager.get_error_history();

// 获取特定插件的错误
auto plugin_errors = manager.get_error_history("my_decoder");

// 获取最后一次错误
std::string last_error = manager.get_last_error("my_decoder");

// 清除错误历史
manager.clear_error_history();
manager.clear_error_history("my_decoder");
```

### 日志级别

```cpp
// 插件专用日志器
auto logger = spdlog::get("plugin");

// 日志级别和格式
logger->set_level(spdlog::level::info);
logger->set_pattern("[%H:%M:%S] [%^%l%$] [plugin:%n] %v");

// 记录日志
logger->info("Plugin {} loaded successfully", plugin_name);
logger->error("Failed to load plugin {}: {}", plugin_name, error);
```

### 事件通知

```cpp
// 监听插件生命周期事件
event_bus_->subscribe("plugin_lifecycle", [](const nlohmann::json& event) {
    std::string action = event["action"];
    std::string plugin = event["plugin"];

    if (action == "error") {
        // 处理错误
        std::cerr << "Plugin error: " << plugin << std::endl;
    }
});

// 监听配置变更
event_bus_->subscribe("plugin_config_changed", [](const nlohmann::json& event) {
    std::string plugin = event["plugin"];
    nlohmann::json config = event["config"];
    // 处理配置变更
});
```

## 最佳实践

### 1. 插件开发

```cpp
class MyPlugin : public IAudioDecoder {
public:
    bool initialize() override {
        // 加载配置
        auto config = get_manager().get_plugin_config(get_name());

        // 验证配置
        if (!validate_config(config)) {
            set_last_error("Invalid configuration");
            return false;
        }

        // 初始化插件
        return do_initialize(config);
    }

    void shutdown() override {
        // 保存配置
        save_current_config();

        // 清理资源
        do_shutdown();
    }
};
```

### 2. 错误处理

```cpp
// 使用Result类型包装错误
template<typename T>
using Result = std::variant<T, std::string>;

Result<std::unique_ptr<IPlugin>> load_plugin(const std::string& path) {
    try {
        auto plugin = create_plugin(path);
        if (!plugin->initialize()) {
            return plugin->get_last_error();
        }
        return plugin;
    } catch (const std::exception& e) {
        return std::string("Exception: ") + e.what();
    }
}
```

### 3. 资源管理

```cpp
// 使用RAII管理插件资源
class PluginGuard {
    std::unique_ptr<IPlugin> plugin_;

public:
    explicit PluginGuard(std::unique_ptr<IPlugin> p) : plugin_(std::move(p)) {
        if (plugin_ && !plugin_->initialize()) {
            plugin_.reset();
        }
    }

    ~PluginGuard() {
        if (plugin_) {
            plugin_->shutdown();
        }
    }

    IPlugin* get() const { return plugin_.get(); }
    operator bool() const { return plugin_ != nullptr; }
};
```

## 性能考虑

### 1. 延迟加载

- 只在需要时加载插件
- 使用插件代理延迟初始化

### 2. 缓存策略

- 缓存插件配置和元数据
- 使用LRU缓存限制内存使用

### 3. 异步操作

- 在后台线程执行插件扫描
- 异步加载插件配置

### 4. 内存管理

- 使用智能指针管理插件生命周期
- 及时卸载不用的插件

## 调试技巧

### 1. 插件调试

```cpp
// 启用详细日志
plugin_logger_->set_level(spdlog::level::debug);

// 打印插件状态
auto state = manager.get_plugin_state("my_plugin");
logger->debug("Plugin state: {}", static_cast<int>(state));
```

### 2. 依赖图可视化

```cpp
// 打印依赖树
auto deps = manager.get_dependency_tree("my_plugin");
logger->info("Dependency tree: [{}]", fmt::join(deps, ", "));
```

### 3. 配置验证

```cpp
// 验证所有插件配置
for (const auto& plugin : manager.get_plugin_list()) {
    auto config = manager.get_plugin_config(plugin.name);
    if (!validate_plugin_config(plugin.name, config)) {
        logger->error("Invalid config for plugin: {}", plugin.name);
    }
}
```

## 故障排除

### 常见问题

1. **插件加载失败**
   - 检查依赖是否满足
   - 验证API版本兼容性
   - 查看错误日志

2. **热加载不工作**
   - 确认文件监控已启用
   - 检查文件权限
   - 验证文件扩展名配置

3. **配置丢失**
   - 检查配置文件路径
   - 确认有写入权限
   - 验证JSON格式

4. **依赖冲突**
   - 使用 `get_dependency_tree` 查看依赖关系
   - 检查版本兼容性
   - 考虑使用沙箱隔离

### 调试命令

```bash
# 查看插件状态
./music-player --list-plugins --verbose

# 重新加载插件
./music-player --reload-plugin my_decoder

# 验证依赖
./music-player --check-dependencies my_decoder

# 重置配置
./music-player --reset-plugin-config
```