# foobar2000 兼容层 (fb2k_compat)

## 🎯 项目目标
实现一个能够加载和运行foobar2000组件的独立播放器，提供与原生的fb2k相同的插件体验。

## 📈 开发阶段

### 阶段1：最小可运行框架（当前）✅
- ✅ 基础COM接口模拟
- ✅ 服务系统实现  
- ✅ 文件信息接口
- ✅ 中止回调机制
- ✅ 输入解码器接口
- ✅ 测试程序框架
- 🔄 集成测试和bug修复

### 阶段2：核心功能完善（计划中）
- 支持更多组件类型（DSP、Output）
- 配置系统（cfg_var）
- 播放控制接口
- 元数据库（metadb）

### 阶段3：高级功能（未来）
- UI组件支持
- 高级DSP效果
- 主题系统
- 网络功能

## 🚀 快速开始

### 前提条件
- Windows 7 或更高版本
- Visual Studio 2019 或更高版本
- CMake 3.10 或更高版本
- foobar2000 已安装（用于获取组件）

### 一键构建
```bash
cd fb2k_compat
build_and_test.bat
```

### 手动构建
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### 运行测试
```bash
cd build\Release
fb2k_compat_test.exe
```

## 🔧 技术架构

### 核心设计理念
```
foobar2000 组件 DLL
        ↓
    COM接口模拟
        ↓  
    服务定位器
        ↓
    主机接口实现
        ↓
    XpuMusic 音频引擎
```

### 接口层次结构
```
IUnknown (COM基础)
  └── service_base (fb2k服务基类)
      ├── file_info (文件元数据)
      ├── abort_callback (操作取消)
      ├── input_decoder (输入解码 - 核心)
      ├── dsp (数字信号处理)
      ├── output (音频输出)
      └── playback_control (播放控制)
```

### 关键组件

#### 1. 主机系统 (minihost)
- **作用**: 模拟foobar2000主程序环境
- **功能**: 组件加载、服务管理、接口协调
- **实现**: COM接口模拟 + 服务工厂

#### 2. 服务系统
- **service_base**: 所有服务的基类
- **service_ptr_t**: 智能指针管理
- **service_factory**: 服务创建工厂
- **GUID系统**: 服务标识和定位

#### 3. 解码器接口
- **input_decoder**: 核心解码接口
- **file_info**: 文件元数据管理
- **abort_callback**: 操作取消机制

## 📊 兼容性目标

### 支持的组件类型
| 类型 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| input | 🔴 高 | 🔄 进行中 | 音频输入/解码器 |
| dsp | 🟡 中 | 📋 计划中 | 数字信号处理 |
| output | 🟡 中 | 📋 计划中 | 音频输出 |
| ui | 🟢 低 | ❌ 未计划 | 用户界面 |
| visualization | 🟢 低 | ❌ 未计划 | 可视化 |

### 目标组件列表
- ✅ foo_input_std (标准输入)
- ✅ foo_input_flac (FLAC解码)
- ✅ foo_input_monkey (APE解码)  
- ✅ foo_input_ffmpeg (FFmpeg支持)
- ✅ foo_dsp_std (标准DSP)
- ✅ foo_out_wasapi (WASAPI输出)
- ✅ foo_out_asio (ASIO输出)

## 🧪 测试策略

### 自动化测试
```bash
# 运行完整测试套件
fb2k_compat_test.exe

# 测试特定组件
fb2k_compat_test.exe --component foo_input_std.dll

# 测试特定音频格式
fb2k_compat_test.exe --format FLAC --file test.flac
```

### 手动测试流程
1. **组件加载测试**: 验证DLL能正确加载
2. **接口查询测试**: 检查服务获取是否成功
3. **文件打开测试**: 测试音频文件打开
4. **解码测试**: 验证音频数据能正确解码
5. **性能测试**: 检查解码性能和稳定性

## 🔍 调试和故障排除

### 常见问题和解决方案

#### 1. COM初始化失败
```
错误: Failed to initialize COM
解决: 确保在Windows 7+系统上运行，安装Visual C++运行库
```

#### 2. DLL加载失败
```
错误: Failed to load DLL
解决: 检查依赖项，使用Dependency Walker分析缺少的DLL
```

#### 3. 服务获取失败
```
错误: Failed to get input decoder service
解决: 验证GUID定义，检查组件版本兼容性
```

#### 4. 解码失败
```
错误: Failed to decode audio
解决: 检查音频文件格式，验证文件完整性
```

### 调试工具
- **Visual Studio调试器**: 源码级调试
- **Dependency Walker**: DLL依赖分析
- **Process Monitor**: 系统调用跟踪
- **DebugView**: 日志输出查看

## 📈 性能目标

### 解码性能
- 解码速度: ≥ 实时速度的10倍
- 内存使用: ≤ 原生的120%
- CPU占用: ≤ 原生的110%

### 兼容性指标
- 组件加载成功率: ≥ 90%
- 音频格式支持: ≥ 95%
- 稳定性: ≥ 99.9% (无崩溃)

## 🔐 法律声明

### 合规性
- ✅ 不复制foobar2000代码
- ✅ 不打包官方DLL文件
- ✅ 开源实现，独立开发
- ✅ 仅用于组件兼容性

### 许可证
本项目使用MIT许可证，允许商业和非商业使用。

## 🤝 贡献指南

### 如何贡献
1. Fork项目仓库
2. 创建特性分支
3. 提交代码变更
4. 创建Pull Request

### 代码规范
- 使用现代C++17特性
- 遵循RAII原则
- 添加详细注释
- 包含单元测试
- 通过代码审查

### 需要帮助的地方
- 🔴 **COM接口专家** - 完善COM实现
- 🔴 **音频处理专家** - 优化解码流程
- 🟡 **foobar2000开发者** - 提供技术细节
- 🟢 **测试志愿者** - 测试各种组件

## 📞 联系方式

- **Issues**: 报告bug和功能请求
- **Discussions**: 技术讨论和建议
- **Wiki**: 技术文档和教程

---

**当前状态**: 阶段1开发中 🔄  
**目标**: 2025年Q1完成阶段1测试 ✅  
**愿景**: 成为最好的foobar2000替代方案 🎯