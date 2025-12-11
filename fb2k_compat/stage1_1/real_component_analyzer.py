#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
真实foobar2000组件分析器
阶段1.1：分析真实组件结构和接口
"""

import os
import sys
import struct
from typing import Dict, List, Optional, Tuple
import json

class ComponentAnalyzer:
    """foobar2000组件DLL分析器"""
    
    def __init__(self, dll_path: str):
        self.dll_path = dll_path
        self.exports = {}
        self.imports = {}
        self.guids = {}
        self.services = {}
        
    def basic_analyze(self) -> Dict:
        """基础分析（不依赖pefile）"""
        print(f"正在分析组件: {os.path.basename(self.dll_path)}")
        
        try:
            # 基础文件信息
            file_size = os.path.getsize(self.dll_path)
            
            # 读取DOS头
            with open(self.dll_path, 'rb') as f:
                dos_header = f.read(64)
                if len(dos_header) < 64 or dos_header[:2] != b'MZ':
                    print("  错误: 不是有效的PE文件")
                    return None
                
                # 获取PE头偏移
                pe_offset = struct.unpack('<L', dos_header[60:64])[0]
                f.seek(pe_offset)
                
                # 读取PE签名和COFF头
                pe_sig = f.read(4)
                if pe_sig != b'PE\x00\x00':
                    print("  错误: 无效的PE签名")
                    return None
                
                coff_header = f.read(20)
                if len(coff_header) < 20:
                    print("  错误: COFF头不完整")
                    return None
                
                # 解析COFF头
                machine, num_sections, time_date_stamp, ptr_symbol_table, num_symbols, size_opt_header, characteristics = struct.unpack('<HHLLLHH', coff_header)
                
                print(f"  机器类型: 0x{machine:04X}")
                print(f"  节区数量: {num_sections}")
                print(f"  符号表指针: 0x{ptr_symbol_table:08X}")
                print(f"  符号数量: {num_symbols}")
                print(f"  可选头大小: {size_opt_header}")
                
                # 读取可选头（PE32或PE32+）
                if size_opt_header > 0:
                    opt_header = f.read(size_opt_header)
                    if len(opt_header) >= 96:  # PE32最小大小
                        self._parse_optional_header(opt_header[:96])
                
                # 基础字符串扫描
                f.seek(0)
                data = f.read(min(file_size, 1024 * 1024))  # 读取前1MB
                self._scan_strings(data)
                
                return {
                    "filename": os.path.basename(self.dll_path),
                    "file_size": file_size,
                    "machine_type": machine,
                    "num_sections": num_sections,
                    "characteristics": characteristics,
                    "analysis_complete": True,
                    "export_count": len(self.exports),
                    "services": list(self.services.keys()),
                    "guids": list(self.guids.keys())[:10]
                }
                
        except Exception as e:
            print(f"  分析失败: {e}")
            return None
    
    def _parse_optional_header(self, data: bytes):
        """解析可选头"""
        if len(data) < 96:
            return
        
        # 标准字段偏移
        magic = struct.unpack('<H', data[0:2])[0]
        major_linker_version = data[2]
        minor_linker_version = data[3]
        size_code = struct.unpack('<L', data[4:8])[0]
        size_initialized_data = struct.unpack('<L', data[8:12])[0]
        
        print(f"  Magic: 0x{magic:04X}")
        print(f"  链接器版本: {major_linker_version}.{minor_linker_version}")
        print(f"  代码大小: {size_code} 字节")
        print(f"  初始化数据大小: {size_initialized_data} 字节")
        
        if magic == 0x10B:  # PE32
            if len(data) >= 112:
                image_base = struct.unpack('<L', data[28:32])[0]
                section_alignment = struct.unpack('<L', data[32:36])[0]
                print(f"  映像基址: 0x{image_base:08X}")
                print(f"  节区对齐: {section_alignment}")
        
        elif magic == 0x20B:  # PE32+
            if len(data) >= 120:
                image_base = struct.unpack('<Q', data[24:32])[0]
                section_alignment = struct.unpack('<L', data[32:36])[0]
                print(f"  映像基址: 0x{image_base:016X}")
                print(f"  节区对齐: {section_alignment}")
    
    def _scan_strings(self, data: bytes):
        """扫描字符串模式"""
        print("  扫描字符串模式...")
        
        # 查找可打印字符串（最小长度4）
        current_string = b""
        string_count = 0
        
        for i, byte in enumerate(data):
            if 32 <= byte <= 126:  # 可打印ASCII字符
                current_string += bytes([byte])
            else:
                if len(current_string) >= 4:
                    try:
                        text = current_string.decode('utf-8', errors='ignore')
                        self._analyze_string(text)
                        string_count += 1
                        if string_count >= 20:  # 限制数量
                            break
                    except:
                        pass
                current_string = b""
        
        print(f"  扫描到 {string_count} 个潜在字符串")
    
    def _analyze_string(self, text: str):
        """分析单个字符串"""
        # 识别接口名称
        interface_patterns = [
            'input_decoder', 'file_info', 'abort_callback',
            'service_base', 'playback_control', 'metadb_handle',
            'cfg_var', 'dsp_preset', 'output_device',
            'audio_chunk', 'playable_location', 'titleformat_object'
        ]
        
        for pattern in interface_patterns:
            if pattern in text.lower():
                self.services[text] = text
                break
        
        # 识别GUID特征（包含连字符的32位十六进制）
        if '-' in text and len(text) >= 36:
            import re
            guid_pattern = r'[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}'
            if re.match(guid_pattern, text):
                self.guids[text] = "found_in_strings"

class RealComponentHost:
    """真实组件主机实现规划"""
    
    def __init__(self):
        self.required_interfaces = {
            "input_decoder": [
                "input_decoder", "file_info", "abort_callback"
            ],
            "dsp": [
                "dsp_preset", "dsp_chunk"
            ],
            "output": [
                "output_device", "audio_chunk"
            ]
        }
        
        self.known_guids = {
            # 这些是已知的fb2k相关GUID，需要验证
            "input_decoder": "{E92063D0-C149-4B31-BF37-5F5C9D013C6A}",
            "file_info": "{9A1D5E4F-3B7C-4A2E-8F5C-1D9E6B3A2C4D}",
            "abort_callback": "{12345678-1234-1234-1234-123456789ABC}",
        }
    
    def analyze_real_component(self, dll_path: str) -> Dict:
        """分析真实组件并给出实现建议"""
        analyzer = ComponentAnalyzer(dll_path)
        basic_report = analyzer.basic_analyze()
        
        if not basic_report:
            return None
        
        # 生成实现建议
        implementation_plan = self._generate_implementation_plan(basic_report)
        
        return {
            "basic_analysis": basic_report,
            "implementation_plan": implementation_plan,
            "recommendations": self._generate_recommendations(basic_report)
        }
    
    def _generate_implementation_plan(self, report: Dict) -> Dict:
        """生成实现计划"""
        plan = {
            "phase": "1.1",
            "priority": "high",
            "estimated_time": "1-2 weeks",
            "steps": [],
            "risks": [],
            "dependencies": []
        }
        
        # 基础实现步骤
        plan["steps"].extend([
            "1. 实现DLL加载和导出函数解析",
            "2. 创建服务工厂和GUID注册表", 
            "3. 实现标准COM接口查询",
            "4. 添加错误处理和日志系统",
            "5. 测试第一个真实组件加载"
        ])
        
        # 根据文件大小评估复杂度
        file_size = report.get("file_size", 0)
        if file_size > 1024 * 1024:  # 1MB
            plan["risks"].append("大体积组件可能需要更多内存")
            plan["estimated_time"] = "2-3 weeks"
        
        # 架构相关风险
        machine_type = report.get("machine_type", 0)
        if machine_type == 0x8664:  # x64
            plan["dependencies"].append("确保64位环境兼容性")
        elif machine_type == 0x014c:  # x86
            plan["dependencies"].append("考虑32位组件在64位环境的兼容")
        
        return plan
    
    def _generate_recommendations(self, report: Dict) -> List[str]:
        """生成技术建议"""
        recommendations = []
        
        # 基础建议
        recommendations.extend([
            "准备干净的Windows测试环境",
            "安装Visual C++运行库合集",
            "使用Dependency Walker分析依赖",
            "准备多个版本的foobar2000组件"
        ])
        
        # 基于分析结果的建议
        file_size = report.get("file_size", 0)
        if file_size > 500 * 1024:  # 500KB
            recommendations.append("大文件组件，注意加载时间优化")
        
        services = report.get("services", [])
        if len(services) > 5:
            recommendations.append("多功能组件，建议分步骤实现")
        
        return recommendations

def find_foobar2000_components():
    """查找系统中的foobar2000组件"""
    search_paths = [
        r"C:\Program Files (x86)\foobar2000\components",
        r"C:\Program Files\foobar2000\components", 
        os.path.expanduser(r"~\AppData\Roaming\foobar2000\user-components")
    ]
    
    components = []
    for base_path in search_paths:
        if os.path.exists(base_path):
            for root, dirs, files in os.walk(base_path):
                for file in files:
                    if file.endswith('.dll'):
                        full_path = os.path.join(root, file)
                        components.append(full_path)
    
    return components

def main():
    """主函数：分析真实foobar2000组件"""
    print("=" * 60)
    print("foobar2000 真实组件分析器")
    print("阶段1.1：组件结构分析")
    print("=" * 60)
    
    # 查找组件
    print("正在查找foobar2000组件...")
    components = find_foobar2000_components()
    
    if not components:
        print("未找到foobar2000组件!")
        print("请确保foobar2000已安装")
        print("\n将使用模拟数据进行演示...")
        # 创建模拟分析结果
        mock_result = {
            "basic_analysis": {
                "filename": "foo_input_std.dll",
                "file_size": 1024 * 1024,  # 1MB
                "machine_type": 0x8664,   # x64
                "component_type": "input_decoder",
                "analysis_complete": True
            },
            "implementation_plan": {
                "phase": "1.1",
                "priority": "high", 
                "estimated_time": "1-2 weeks",
                "steps": [
                    "1. 实现DLL加载和导出函数解析",
                    "2. 创建服务工厂和GUID注册表",
                    "3. 实现标准COM接口查询", 
                    "4. 测试foo_input_std组件加载",
                    "5. 验证解码功能正常工作"
                ],
                "risks": ["MSVC依赖", "架构兼容性"],
                "dependencies": ["Visual C++运行库", "Windows SDK"]
            },
            "recommendations": [
                "准备干净的Windows测试环境",
                "使用Dependency Walker分析依赖",
                "准备多个版本的foobar2000组件",
                "实现完整的错误处理机制"
            ]
        }
        results = [mock_result]
    else:
        # 分析真实组件
        host = RealComponentHost()
        results = []
        
        for component_path in components[:3]:  # 只分析前3个
            print(f"\n分析: {os.path.basename(component_path)}")
            
            result = host.analyze_real_component(component_path)
            if result:
                results.append(result)
                
                # 显示关键信息
                basic = result["basic_analysis"]
                print(f"  文件大小: {basic.get('file_size', 0) / 1024:.1f} KB")
                print(f"  架构: {'x64' if basic.get('machine_type') == 0x8664 else 'x86'}")
                print(f"  组件类型: {basic.get('component_type', 'unknown')}")
                
                # 显示实现计划
                plan = result["implementation_plan"]
                print(f"  预计时间: {plan['estimated_time']}")
                print(f"  主要步骤: {len(plan['steps'])} 个")
    
    # 生成汇总报告
    print(f"\n{'='*60}")
    print("阶段1.1分析汇总")
    print(f"{'='*60}")
    
    if results:
        # 保存详细报告
        report_file = "component_analysis_report.json"
        with open(report_file, 'w', encoding='utf-8') as f:
            json.dump(results, f, indent=2, ensure_ascii=False)
        
        print(f"详细报告已保存到: {report_file}")
        
        # 显示下一步计划
        print(f"\n🎯 阶段1.1实施计划:")
        for i, step in enumerate(results[0]["implementation_plan"]["steps"], 1):
            print(f"{i}. {step}")
        
        print(f"\n⚠️  主要风险:")
        for risk in results[0]["implementation_plan"]["risks"]:
            print(f"  - {risk}")
        
        print(f"\n📋 关键依赖:")
        for dep in results[0]["implementation_plan"]["dependencies"]:
            print(f"  - {dep}")
    
    print(f"\n阶段1.1分析完成！准备进入真实组件集成阶段。")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n分析被用户中断")
    except Exception as e:
        print(f"\n错误: {e}")
        import traceback
        traceback.print_exc()