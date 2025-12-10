# XpuMusic - 架构设计文档

## 概述

XpuMusic 是一个基于 C++ 的模块化跨平台音乐播放器，采用插件架构设计，支持 foobar2000 插件兼容性。本文档详细描述了系统的整体架构、核心组件和设计原则。

## 核心架构

### 分层架构设计

```
┌─────────────────────────────────────────────────────┐
│                 应用层 (Applications)                 │
├─────────────────────────────────────────────────────┤
│  music-player-simple  │  music-player-plugin         │
└─────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────┐
│                兼容性层 (Compatibility)               │
├─────────────────────────────────────────────────────┤
│  Foobar2000 SDK    │  Plugin Loader  │  Adapters    │
└─────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────┐
│                 核心引擎层 (Core)                    │
├─────────────────────────────────────────────────────┤
│  Playback Engine  │  Event Bus     │  Plugin Host   │
│  Config Manager   │  Playlist Mgr  │  Decoder Reg.  │
└─────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────┐
│               插件系统层 (Plugins)                   │
├─────────────────────────────────────────────────────┤
│  MP3 Decoder      │  FLAC Decoder   │  OGG Decoder  │
└─────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────┐
│              平台抽象层 (Platform)                   │
├─────────────────────────────────────────────────────┤
│  Audio Output     │  File System   │  Threading     │
└─────────────────────────────────────────────────────┘
```

## 核心组件详解

### 1. 核心引擎 (Core Engine)

#### 1.1 CoreEngine
主引擎类，负责整个播放器的生命周期管理。

**职责：**
- 初始化各个子系统
- 管理播放状态
- 协调组件间交互

**关键接口：**
```cpp
class CoreEngine {
public:
    Result initialize();
    void shutdown();
    Result load_configuration(const std::string& config_path);
    Result save_configuration();
};
```

#### 1.2 ServiceRegistry
服务注册表，实现依赖注入模式。

**特点：**
- 单例模式
- 线程安全
- 支持服务生命周期管理

#### 1.3 EventBus
事件总线，实现发布-订阅模式。

**事件类型：**
- 播放状态事件
- 音频格式事件
- 插件生命周期事件
- 错误事件

### 2. 音频处理流水线

#### 2.1 解码流程
```
Audio File → Format Detector → Decoder Registry → Specific Decoder → Audio Samples
```

#### 2.2 播放流程
```
Audio Samples → Sample Rate Converter → Audio Output → Speakers
```

#### 2.3 关键接口

**IAudioDecoder**
```cpp
class IAudioDecoder {
public:
    virtual bool can_decode(const std::string& file_path) = 0;
    virtual Result decode(const std::string& file_path, AudioBuffer& buffer) = 0;
    virtual AudioFormat get_supported_format() const = 0;
};
```

**ISampleRateConverter**
```cpp
class ISampleRateConverter {
public:
    virtual Result convert(const AudioBuffer& input,
                          AudioBuffer& output,
                          double ratio) = 0;
    virtual void set_quality(ConversionQuality quality) = 0;
};
```

### 3. 插件系统

#### 3.1 插件架构

**插件生命周期：**
1. 加载 (Load)
2. 初始化 (Initialize)
3. 激活 (Activate)
4. 停用 (Deactivate)
5. 卸载 (Unload)

**插件类型：**
- 输入插件 (解码器)
- 输出插件 (音频设备)
- DSP 插件 (音效处理)
- UI 插件 (界面扩展)

#### 3.2 插件管理

**PluginHost**
- 动态加载/卸载插件
- 热重载支持
- 依赖关系管理
- 版本兼容性检查

**依赖管理策略：**
- 拓扑排序确定加载顺序
- 循环依赖检测
- 版本范围检查

### 4. 配置系统

#### 4.1 配置层次结构
```
Global Config
├── Audio Settings
│   ├── Sample Rate
│   ├── Bit Depth
│   └── Buffer Size
├── Plugin Config
│   ├── Enabled Plugins
│   └── Plugin Settings
└── User Preferences
    ├── UI Settings
    └── Playback Settings
```

#### 4.2 配置持久化
- JSON 格式存储
- 原子写入保证
- 备份机制

### 5. foobar2000 兼容层

#### 5.1 兼容性策略

**API 映射：**
- Service 接口适配
- 事件系统桥接
- 数据结构转换

**插件加载：**
- DLL/SO 动态加载
- ABI 兼容性处理
- API 版本检查

#### 5.2 核心适配器

**Input Decoder Adapter**
```cpp
class InputDecoderAdapter : public AdapterBase {
public:
    bool initialize() override;
    Result load_plugin(const std::string& plugin_path);
    AudioFormat get_supported_formats() const;
};
```

## 设计模式应用

### 1. 工厂模式
- AudioOutputFactory: 创建平台特定的音频输出
- DecoderFactory: 创建格式特定的解码器

### 2. 策略模式
- SampleRateConverter: 多种重采样算法
- AudioOutput: 多种音频后端

### 3. 观察者模式
- EventBus: 事件分发机制
- ConfigManager: 配置变更通知

### 4. 单例模式
- ServiceRegistry: 全局服务管理
- PluginHost: 插件生命周期管理

### 5. 适配器模式
- XpuMusicCompatManager: foobar2000 兼容性
- PlatformAdapter: 跨平台抽象

## 性能优化

### 1. 内存管理
- 智能指针避免内存泄漏
- 内存池减少分配开销
- 零拷贝优化音频数据传输

### 2. 并发设计
- 线程池处理 I/O 操作
- 无锁数据结构减少竞争
- 原子操作保证线程安全

### 3. 缓存策略
- 元数据缓存
- 音频数据预读
- 解码结果缓存

## 安全考虑

### 1. 插件沙箱
- API 权限控制
- 资源访问限制
- 异常隔离

### 2. 输入验证
- 文件格式验证
- 参数边界检查
- 缓冲区溢出防护

### 3. 错误处理
- 异常安全保证
- 优雅降级机制
- 错误日志记录

## 扩展性设计

### 1. 接口抽象
- 清晰的模块边界
- 版本化 API
- 向后兼容性

### 2. 热更新支持
- 插件热加载
- 配置热更新
- 代码热重载

### 3. 多语言支持
- C 接口规范
- 语言绑定框架
- 插件开发工具

## 平台特性

### Windows
- WASAPI 音频输出
- COM 接口支持
- 注册表集成

### Linux
- ALSA/PulseAudio 支持
- POSIX 接口使用
- 包管理器集成

### macOS
- CoreAudio 集成
- Bundle 结构
- App Store 兼容

## 未来规划

### 短期目标
- 完善可视化引擎
- 增加更多音频格式支持
- 优化资源使用

### 长期目标
- WebAssembly 支持
- 分布式音频处理
- AI 音频增强

## 总结

XpuMusic 的架构设计遵循了模块化、可扩展和跨平台的原则。通过清晰的分层设计和接口抽象，系统具有良好的可维护性和扩展性。插件系统和 foobar2000 兼容层为用户提供了丰富的功能和生态支持。