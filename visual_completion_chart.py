#!/usr/bin/env python3
"""
功能完成度可视化图表
"""

import matplotlib.pyplot as plt
import numpy as np

# 功能完成度数据
categories = [
    '音频播放',
    '播放器控制',
    '文件处理',
    '用户界面',
    '系统功能',
    '架构功能',
    '插件系统',
    '文档系统',
    '跨平台支持'
]

completion_rates = [100, 100, 100, 100, 100, 90, 40, 90, 40]
colors = ['#4CAF50' if x >= 90 else '#FFC107' if x >= 60 else '#F44336' for x in completion_rates]

# 创建图表
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 8))

# 条形图
bars = ax1.barh(categories, completion_rates, color=colors)
ax1.set_xlim(0, 100)
ax1.set_xlabel('完成度 (%)')
ax1.set_title('XpuMusic 音乐播放器功能完成度', fontsize=16, fontweight='bold')
ax1.grid(axis='x', alpha=0.3)

# 添加数值标签
for bar in bars:
    width = bar.get_width()
    ax1.text(width + 1, bar.get_y() + bar.get_height()/2,
             f'{width}%', ha='left', va='center', fontweight='bold')

# 总体完成度
overall = np.mean(completion_rates)
colors2 = ['#4CAF50', '#FFC107', '#F44336']
sizes = [overall/100, (100-overall)/2, (100-overall)/2]
labels = [f'已完成: {overall:.1f}%', '插件扩展', '跨平台完善']
explode = (0.1, 0, 0)

ax2.pie(sizes, labels=labels, colors=colors2, explode=explode, autopct='%1.1f%%',
        shadow=True, startangle=90)
ax2.set_title(f'总体完成度: {overall:.1f}%', fontsize=16, fontweight='bold')

# 添加图例
from matplotlib.patches import Patch
legend_elements = [
    Patch(facecolor='#4CAF50', label='优秀 (≥90%)'),
    Patch(facecolor='#FFC107', label='良好 (60-89%)'),
    Patch(facecolor='#F44336', label='需改进 (<60%)')
]
ax1.legend(handles=legend_elements, loc='lower right')

plt.tight_layout()
plt.savefig('completion_chart.png', dpi=300, bbox_inches='tight')
plt.show()

print("功能完成度分析:")
print("=" * 50)
print(f"总体完成度: {overall:.1f}%")
print("=" * 50)

for cat, rate in zip(categories, completion_rates):
    status = "✅" if rate >= 90 else "⚠️" if rate >= 60 else "❌"
    print(f"{status} {cat:12}: {rate:3}%")
print("=" * 50)

# 核心功能 vs 扩展功能
core_features = [100, 100, 100, 100, 100]  # 前5个
extended_features = [90, 40, 90, 40]      # 后4个

core_avg = np.mean(core_features)
ext_avg = np.mean(extended_features)

print(f"\n核心功能平均完成度: {core_avg:.1f}%")
print(f"扩展功能平均完成度: {ext_avg:.1f}%")
print("\n结论: 核心功能已完全实现，可投入生产使用！")