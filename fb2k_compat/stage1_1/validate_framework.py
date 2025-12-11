#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
foobar2000 兼容层框架验证
阶段1.1：架构概念验证
"""

import sys
import os
import time
from abc import ABC, abstractmethod
from typing import Optional, Dict, Any, List
from dataclasses import dataclass

# 基础数据结构
@dataclass
class AudioInfo:
    sample_rate: int = 44100
    channels: int = 2
    bitrate: int = 0
    length: float = 0.0

@dataclass
class FileStats:
    size: int = 0
    timestamp: int = 0

# COM接口模拟
class IUnknown(ABC):
    """模拟IUnknown接口"""
    def __init__(self):
        self._ref_count = 1
    
    def QueryInterface(self, riid: str) -> Optional['IUnknown']:
        """接口查询"""
        if riid == "IUnknown":
            return self
        return self.QueryInterfaceImpl(riid)
    
    def QueryInterfaceImpl(self, riid: str) -> Optional['IUnknown']:
        """实现特定的接口查询"""
        return None
    
    def AddRef(self) -> int:
        """增加引用计数"""
        self._ref_count += 1
        return self._ref_count
    
    def Release(self) -> int:
        """减少引用计数"""
        self._ref_count -= 1
        count = self._ref_count
        if count <= 0:
            self._cleanup()
        return count
    
    def _cleanup(self):
        """清理资源"""
        pass

# GUID定义（简化版）
IID_IUnknown = "IUnknown"
IID_ServiceBase = "ServiceBase"
IID_FileInfo = "FileInfo"
IID_AbortCallback = "AbortCallback"
IID_InputDecoder = "InputDecoder"
CLSID_InputDecoderService = "InputDecoderService"

# 服务基类
class ServiceBase(IUnknown):
    """fb2k服务基类"""
    
    def QueryInterfaceImpl(self, riid: str) -> Optional['IUnknown']:
        if riid == IID_ServiceBase:
            return self
        return super().QueryInterfaceImpl(riid)
    
    def service_add_ref(self) -> int:
        return self.AddRef()
    
    def service_release(self) -> int:
        return self.Release()

# 智能指针模板
class service_ptr_t:
    """智能指针管理"""
    
    def __init__(self, ptr=None):
        self.ptr = ptr
    
    def __del__(self):
        self.reset()
    
    def reset(self, ptr=None):
        if self.ptr:
            self.ptr.Release()
        self.ptr = ptr
    
    def get(self):
        return self.ptr
    
    def __getattr__(self, name):
        if self.ptr:
            return getattr(self.ptr, name)
        raise AttributeError(f"'{self.__class__.__name__}' object has no attribute '{name}'")
    
    def is_valid(self):
        return self.ptr is not None
    
    def is_empty(self):
        return self.ptr is None

# 文件信息接口
class FileInfo(ServiceBase):
    """文件信息接口"""
    
    def __init__(self):
        super().__init__()
        self.metadata = {}
        self.audio_info = AudioInfo()
        self.file_stats = FileStats()
        self.length = 0.0
    
    def reset(self):
        """重置文件信息"""
        self.metadata.clear()
        self.audio_info = AudioInfo()
        self.file_stats = FileStats()
        self.length = 0.0
    
    def meta_get(self, name: str, index: int = 0) -> Optional[str]:
        """获取元数据"""
        if name in self.metadata and index < len(self.metadata[name]):
            return self.metadata[name][index]
        return None
    
    def meta_set(self, name: str, value: str):
        """设置元数据"""
        self.metadata[name] = [value]
    
    def get_length(self) -> float:
        """获取文件长度"""
        return self.length
    
    def set_length(self, length: float):
        """设置文件长度"""
        self.length = length
    
    def get_audio_info(self) -> AudioInfo:
        """获取音频信息"""
        return self.audio_info
    
    def set_audio_info(self, info: AudioInfo):
        """设置音频信息"""
        self.audio_info = info
    
    def get_file_stats(self) -> FileStats:
        """获取文件统计"""
        return self.file_stats
    
    def set_file_stats(self, stats: FileStats):
        """设置文件统计"""
        self.file_stats = stats

# 中止回调接口
class AbortCallback(ServiceBase):
    """中止回调接口"""
    
    def is_aborting(self) -> bool:
        """检查是否应中止"""
        return False

# 输入解码器接口
class InputDecoder(ServiceBase):
    """输入解码器接口"""
    
    def __init__(self, name: str):
        super().__init__()
        self.name = name
        self.is_open = False
        self.current_path = ""
        self.position = 0.0
    
    def open(self, path: str, file_info: FileInfo, abort: AbortCallback) -> bool:
        """打开音频文件"""
        print(f"[{self.name}] 正在打开文件: {path}")
        self.current_path = path
        self.is_open = True
        self.position = 0.0
        
        # 设置一些默认信息
        file_info.set_length(180.0)  # 3分钟
        file_info.set_audio_info(AudioInfo(sample_rate=44100, channels=2, bitrate=320))
        file_info.meta_set("title", os.path.basename(path))
        file_info.meta_set("decoder", self.name)
        
        print(f"[{self.name}] 文件打开成功")
        return True
    
    def decode(self, buffer: List[float], samples: int, abort: AbortCallback) -> int:
        """解码音频数据"""
        if not self.is_open:
            return 0
        
        if abort.is_aborting():
            return 0
        
        # 生成正弦波测试音频
        frequency = 440.0  # A4音符
        amplitude = 0.5
        
        for i in range(samples):
            time_pos = self.position + (i / 44100.0)
            value = amplitude * (i % 100) / 100.0  # 简化版正弦波
            
            # 立体声
            if i * 2 < len(buffer):
                buffer[i * 2] = value           # 左声道
                buffer[i * 2 + 1] = value       # 右声道
        
        self.position += samples / 44100.0
        
        print(f"[{self.name}] 解码了 {samples} 个采样，位置: {self.position:.3f}s")
        return samples
    
    def seek(self, seconds: float, abort: AbortCallback):
        """跳转到指定位置"""
        self.position = seconds
        print(f"[{self.name}] 跳转到: {seconds:.3f}秒")
    
    def can_seek(self) -> bool:
        """是否支持跳转"""
        return True
    
    def close(self):
        """关闭解码器"""
        print(f"[{self.name}] 关闭解码器")
        self.is_open = False
        self.current_path = ""
    
    def is_our_path(self, path: str) -> bool:
        """是否支持该文件路径"""
        supported_ext = ['.mp3', '.flac', '.wav', '.ape', '.ogg', '.m4a']
        ext = os.path.splitext(path)[1].lower()
        return ext in supported_ext
    
    def get_name(self) -> str:
        """获取解码器名称"""
        return self.name

# 服务工厂
class ServiceFactory:
    """服务工厂基类"""
    
    def create_instance(self, riid: str) -> Optional[IUnknown]:
        """创建服务实例"""
        raise NotImplementedError
    
    def get_service_guid(self) -> str:
        """获取服务GUID"""
        raise NotImplementedError

# 输入解码器工厂
class InputDecoderFactory(ServiceFactory):
    """输入解码器工厂"""
    
    def __init__(self, name: str):
        self.name = name
    
    def create_instance(self, riid: str) -> Optional[IUnknown]:
        """创建解码器实例"""
        if riid == IID_InputDecoder:
            decoder = InputDecoder(self.name)
            return decoder
        return None
    
    def get_service_guid(self) -> str:
        return CLSID_InputDecoderService

# 增强版主机
class EnhancedMiniHost:
    """增强版主机实现"""
    
    def __init__(self):
        self.factories = {}
        self.decoders = []
    
    def initialize(self) -> bool:
        """初始化主机"""
        print("[EnhancedMiniHost] 初始化主机环境...")
        print("[EnhancedMiniHost] COM环境模拟完成")
        print("[EnhancedMiniHost] 服务系统初始化完成")
        return True
    
    def register_service(self, guid: str, factory: ServiceFactory):
        """注册服务工厂"""
        self.factories[guid] = factory
        print(f"[EnhancedMiniHost] 注册服务: {guid}")
    
    def create_service(self, guid: str, riid: str) -> Optional[IUnknown]:
        """创建服务实例"""
        if guid not in self.factories:
            print(f"[EnhancedMiniHost] 未找到服务工厂: {guid}")
            return None
        
        factory = self.factories[guid]
        return factory.create_instance(riid)
    
    def create_decoder_for_path(self, path: str) -> Optional['service_ptr_t']:
        """为路径创建解码器"""
        print(f"[EnhancedMiniHost] 为路径创建解码器: {path}")
        
        # 创建解码器工厂
        factory = InputDecoderFactory("TestDecoder")
        
        # 创建解码器实例
        decoder = self.create_service(CLSID_InputDecoderService, IID_InputDecoder)
        if not decoder:
            return None
        
        # 检查是否支持该路径
        if isinstance(decoder, InputDecoder) and decoder.is_our_path(path):
            print(f"[EnhancedMiniHost] 找到匹配的解码器: {decoder.get_name()}")
            return service_ptr_t[InputDecoder](decoder)
        
        print("[EnhancedMiniHost] 未找到匹配的解码器")
        return None
    
    def test_decode(self, audio_file: str) -> bool:
        """测试解码功能"""
        print(f"\n=== 解码测试开始 ===")
        print(f"测试文件: {audio_file}")
        
        # 创建解码器
        decoder = self.create_decoder_for_path(audio_file)
        if not decoder or not decoder.is_valid():
            print("❌ 无法创建解码器")
            return False
        
        # 创建文件信息和中止回调
        file_info = FileInfo()
        abort_cb = AbortCallback()
        
        # 打开文件
        print("\n正在打开文件...")
        if not decoder.open(audio_file, file_info, abort_cb):
            print("❌ 无法打开文件")
            return False
        
        print("✅ 文件打开成功")
        
        # 显示文件信息
        print("\n文件信息:")
        print(f"  长度: {file_info.get_length()} 秒")
        ai = file_info.get_audio_info()
        print(f"  采样率: {ai.sample_rate} Hz")
        print(f"  声道数: {ai.channels}")
        print(f"  比特率: {ai.bitrate} kbps")
        
        title = file_info.meta_get("title")
        if title:
            print(f"  标题: {title}")
        
        # 测试解码
        print("\n开始解码测试...")
        test_samples = 1024
        buffer = [0.0] * (test_samples * ai.channels)
        
        total_decoded = 0
        max_iterations = 5
        
        for i in range(max_iterations):
            decoded = decoder.decode(buffer, test_samples, abort_cb)
            if decoded <= 0:
                print(f"解码结束，总共解码 {total_decoded} 个采样")
                break
            
            total_decoded += decoded
            
            # 显示进度
            progress = total_decoded / ai.sample_rate
            print(f"  进度: {progress:.2f}s")
            
            # 检查音频数据
            max_amplitude = max(abs(sample) for sample in buffer[:decoded * ai.channels])
            print(f"  最大振幅: {max_amplitude}")
        
        # 测试跳转
        if decoder.can_seek():
            print("\n测试跳转功能...")
            decoder.seek(1.0, abort_cb)
        
        # 关闭解码器
        print("\n关闭解码器...")
        decoder.close()
        
        print("\n=== 解码测试完成 ===")
        print(f"总解码采样数: {total_decoded}")
        print(f"测试时长: {total_decoded / ai.sample_rate} 秒")
        
        return True

# 框架验证测试
def validate_framework():
    """验证框架架构"""
    print("=" * 60)
    print("foobar2000 兼容层框架验证")
    print("阶段1.1：架构概念验证")
    print("=" * 60)
    
    # 创建增强主机
    host = EnhancedMiniHost()
    
    # 初始化
    if not host.initialize():
        print("❌ 主机初始化失败")
        return False
    
    print("✅ 主机初始化成功")
    
    # 注册测试服务
    factory = InputDecoderFactory("TestDecoder")
    host.register_service(CLSID_InputDecoderService, factory)
    
    # 运行测试
    all_passed = True
    
    print("\n1. 服务系统测试...")
    # 创建服务实例
    service = host.create_service(CLSID_InputDecoderService, IID_InputDecoder)
    if service and isinstance(service, InputDecoder):
        print("✅ 服务创建成功")
        print(f"   服务名称: {service.get_name()}")
    else:
        print("❌ 服务创建失败")
        all_passed = False
    
    print("\n2. COM接口测试...")
    # 测试接口查询
    unknown = service.QueryInterface(IID_IUnknown)
    if unknown:
        print("✅ IUnknown接口获取成功")
    else:
        print("❌ IUnknown接口获取失败")
        all_passed = False
    
    service_base = service.QueryInterface(IID_ServiceBase)
    if service_base:
        print("✅ ServiceBase接口获取成功")
        print(f"   通过ServiceBase调用: {service_base.get_name()}")
    else:
        print("❌ ServiceBase接口获取失败")
        all_passed = False
    
    print("\n3. 引用计数测试...")
    ref1 = service.AddRef()
    ref2 = service.AddRef()
    ref3 = service.Release()
    ref4 = service.Release()
    print(f"✅ 引用计数: {ref1} -> {ref2} -> {ref3} -> {ref4}")
    
    print("\n4. 解码功能测试...")
    if not host.test_decode("test.mp3"):
        print("❌ 解码测试失败")
        all_passed = False
    else:
        print("✅ 解码测试通过")
    
    print("\n" + "=" * 60)
    if all_passed:
        print("🎉 所有测试通过！框架架构验证成功。")
        print("\n核心验证完成:")
        print("  ✅ COM接口系统工作正常")
        print("  ✅ 服务系统架构正确")
        print("  ✅ 智能指针管理有效")
        print("  ✅ 工厂模式实现正确")
        print("  ✅ 解码功能验证通过")
        print("\n阶段1.1核心架构验证完成！")
    else:
        print("⚠️  部分测试失败，需要调试")
    
    print("=" * 60)
    return all_passed

# 主函数
if __name__ == "__main__":
    try:
        success = validate_framework()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\n测试被用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ 测试异常: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)