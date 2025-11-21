#!/bin/bash

echo "================================================"
echo "视频分析可视化工具 - 安装脚本"
echo "================================================"
echo ""

# 检查 Python3
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 未安装"
    echo "请先安装 Python3: brew install python3"
    exit 1
fi

echo "✅ Python3 已安装: $(python3 --version)"
echo ""

# 安装 Python 依赖
echo "📦 安装 Python 依赖..."
pip3 install matplotlib numpy

echo ""
echo "================================================"
echo "✅ 安装完成！"
echo "================================================"
echo ""
echo "使用方法："
echo "  1. 分析视频:"
echo "     ./build/video_analyzer_cli input.mp4 --output analysis.json"
echo ""
echo "  2. 可视化结果:"
echo "     python3 visualize_analysis.py analysis.json"
echo ""
echo "================================================"
