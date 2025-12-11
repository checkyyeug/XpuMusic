# foobar2000 兼容层 - 阶段1快速开始

## 🎯 目标
在3-6个月内实现最小可运行的foobar2000组件兼容层，能够加载并运行foo_input_std组件。

## 📋 当前状态
✅ **阶段1基础框架已完成**
- 最小主机接口定义
- COM接口模拟
- 基础服务系统
- 测试程序框架

## 🚀 快速开始

### 1. 构建项目
```bash
cd fb2k_compat
build_and_test.bat
```

### 2. 手动构建（如果需要）
```bash
cd fb2k_compat
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### 3. 运行测试
```bash
cd build\Release
fb2k_compat_test.exe
```

## 🔧 技术架构

### 核心组件
```
minihost.h/.cpp      - 最小主机实现
├── COM接口模拟     - IUnknown派生类
├── 服务系统        - service_base + service_ptr_t
├── 文件信息        - file_info接口
├── 中止回调        - abort_callback接口
└── 输入解码器      - input_decoder接口（核心）
```

### 接口层次
```
IUnknown (COM基础)
  └── service_base (fb2k服务基类)
      ├── file_info (文件信息)
      ├── abort_callback (中止回调)
      └── input_decoder (输入解码器 - 核心！)
```

## 📊 测试流程

1. **自动搜索组件** - 查找foobar2000安装目录
2. **加载核心组件** - 优先加载foo_input_std
3. **创建解码器** - 为指定音频文件创建解码器
4. **测试解码** - 打开文件并解码一小段数据
5. **验证结果** - 检查解码是否成功

## 🎯 下一步计划

### 阶段1.1：完善基础功能（1-2周）
- [ ] 修复COM接口实现问题
- [ ] 完善服务工厂机制
- [ ] 添加更多GUID定义
- [ ] 改进错误处理

### 阶段1.2：扩展兼容性（2-4周）
- [ ] 支持更多组件类型（DSP、Output）
- [ ] 实现配置系统（cfg_var）
- [ ] 添加播放控制接口
- [ ] 支持更多音频格式

### 阶段1.3：集成到主项目（1-2周）
- [ ] 创建插件桥接器
- [ ] 集成现有音频引擎
- [ ] 添加UI支持
- [ ] 性能优化

## 🔍 调试指南

### 常见问题
1. **COM初始化失败** - 确保Windows版本支持
2. **DLL加载失败** - 检查依赖项和位数匹配
3. **服务获取失败** - 验证GUID和接口定义
4. **解码失败** - 检查文件格式支持和权限

### 调试输出
程序会输出详细的调试信息：
- `[INFO]` - 一般信息
- `[ERROR]` - 错误信息
- 组件加载状态
- 解码测试结果

## 📁 目录结构
```
fb2k_compat/
├── minihost.h              # 主机接口定义
├── minihost.cpp            # 主机实现
├── test_minihost.cpp       # 测试程序
├── CMakeLists.txt          # CMake配置
├── build_and_test.bat      # 一键构建脚本
└── quick_start.md          # 本文件
```

## 🤝 贡献指南

### 当前需要帮助的地方
1. **COM接口专家** - 完善COM实现
2. **foobar2000开发者** - 提供SDK细节
3. **音频处理专家** - 优化解码流程
4. **测试志愿者** - 测试不同组件

### 代码规范
- 使用C++17标准
- 遵循现有代码风格
- 添加详细注释
- 包含错误处理

## 📞 联系方式
项目基于开源协议，欢迎贡献代码和提出建议！

---
**状态**: 阶段1基础框架 ✅ 完成  
**下一步**: 运行首次测试并修复问题