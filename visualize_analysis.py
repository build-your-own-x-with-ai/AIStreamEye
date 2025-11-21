#!/usr/bin/env python3
"""
Video Stream Analyzer - Visualization Tool
可视化视频分析结果
"""

import json
import sys
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from pathlib import Path

def load_analysis(json_file):
    """加载分析结果"""
    with open(json_file, 'r') as f:
        return json.load(f)

def plot_bitrate(data, ax):
    """绘制码率图表"""
    if 'bitrateStats' not in data or 'timeSeriesData' not in data['bitrateStats']:
        ax.text(0.5, 0.5, 'No bitrate data available', 
                ha='center', va='center', transform=ax.transAxes)
        return
    
    bitrate_data = data['bitrateStats']['timeSeriesData']
    if not bitrate_data:
        ax.text(0.5, 0.5, 'No bitrate data available', 
                ha='center', va='center', transform=ax.transAxes)
        return
    
    timestamps = [b['timestamp'] for b in bitrate_data]
    bitrates = [b['bitrate'] / 1_000_000 for b in bitrate_data]  # Convert to Mbps
    
    ax.plot(timestamps, bitrates, 'b-', linewidth=1.5, label='Instantaneous Bitrate')
    
    # Add average line
    avg_bitrate = data['bitrateStats']['averageBitrate'] / 1_000_000
    ax.axhline(y=avg_bitrate, color='r', linestyle='--', 
               linewidth=1, label=f'Average: {avg_bitrate:.2f} Mbps')
    
    ax.set_xlabel('Time (seconds)', fontsize=10)
    ax.set_ylabel('Bitrate (Mbps)', fontsize=10)
    ax.set_title('Video Bitrate Over Time', fontsize=12, fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.legend(loc='upper right')

def plot_frame_types(data, ax):
    """绘制帧类型分布"""
    if 'frameStats' not in data:
        ax.text(0.5, 0.5, 'No frame statistics available', 
                ha='center', va='center', transform=ax.transAxes)
        return
    
    stats = data['frameStats']
    frame_types = ['I-Frames', 'P-Frames', 'B-Frames']
    counts = [stats.get('iFrames', 0), stats.get('pFrames', 0), stats.get('bFrames', 0)]
    colors = ['#ff6b6b', '#4ecdc4', '#45b7d1']
    
    wedges, texts, autotexts = ax.pie(counts, labels=frame_types, colors=colors,
                                        autopct='%1.1f%%', startangle=90)
    
    for autotext in autotexts:
        autotext.set_color('white')
        autotext.set_fontweight('bold')
    
    ax.set_title('Frame Type Distribution', fontsize=12, fontweight='bold')

def plot_gop_structure(data, ax):
    """绘制 GOP 结构"""
    if 'gops' not in data or not data['gops']:
        ax.text(0.5, 0.5, 'No GOP data available', 
                ha='center', va='center', transform=ax.transAxes)
        return
    
    gops = data['gops']
    gop_indices = [g['gopIndex'] for g in gops]
    gop_lengths = [g['frameCount'] for g in gops]
    
    bars = ax.bar(gop_indices, gop_lengths, color='#95e1d3', edgecolor='#38ada9', linewidth=1.5)
    
    # Highlight GOPs with different colors based on size
    avg_length = sum(gop_lengths) / len(gop_lengths)
    for i, (bar, length) in enumerate(zip(bars, gop_lengths)):
        if length > avg_length * 1.2:
            bar.set_color('#ff6b6b')
        elif length < avg_length * 0.8:
            bar.set_color('#4ecdc4')
    
    ax.set_xlabel('GOP Index', fontsize=10)
    ax.set_ylabel('Frame Count', fontsize=10)
    ax.set_title('GOP Structure', fontsize=12, fontweight='bold')
    ax.grid(True, alpha=0.3, axis='y')
    
    # Add legend
    red_patch = mpatches.Patch(color='#ff6b6b', label='Long GOP')
    green_patch = mpatches.Patch(color='#95e1d3', label='Normal GOP')
    blue_patch = mpatches.Patch(color='#4ecdc4', label='Short GOP')
    ax.legend(handles=[red_patch, green_patch, blue_patch], loc='upper right')

def plot_frame_sizes(data, ax):
    """绘制帧大小分布"""
    if 'frames' not in data or not data['frames']:
        ax.text(0.5, 0.5, 'No frame data available', 
                ha='center', va='center', transform=ax.transAxes)
        return
    
    frames = data['frames']
    timestamps = [f['timestamp'] for f in frames]
    sizes = [f['size'] / 1024 for f in frames]  # Convert to KB
    types = [f['type'] for f in frames]
    
    # Color by frame type
    colors = {'I': '#ff6b6b', 'P': '#4ecdc4', 'B': '#45b7d1', 'UNKNOWN': '#gray'}
    frame_colors = [colors.get(t, '#gray') for t in types]
    
    ax.scatter(timestamps, sizes, c=frame_colors, alpha=0.6, s=10)
    
    ax.set_xlabel('Time (seconds)', fontsize=10)
    ax.set_ylabel('Frame Size (KB)', fontsize=10)
    ax.set_title('Frame Sizes Over Time', fontsize=12, fontweight='bold')
    ax.grid(True, alpha=0.3)
    
    # Add legend
    i_patch = mpatches.Patch(color='#ff6b6b', label='I-Frame')
    p_patch = mpatches.Patch(color='#4ecdc4', label='P-Frame')
    b_patch = mpatches.Patch(color='#45b7d1', label='B-Frame')
    ax.legend(handles=[i_patch, p_patch, b_patch], loc='upper right')

def print_summary(data):
    """打印分析摘要"""
    print("\n" + "="*60)
    print("VIDEO ANALYSIS SUMMARY")
    print("="*60)
    
    if 'streamInfo' in data:
        info = data['streamInfo']
        print(f"\n📹 Stream Information:")
        print(f"  Codec: {info.get('codecName', 'N/A')}")
        print(f"  Resolution: {info.get('width', 0)}x{info.get('height', 0)}")
        print(f"  Frame Rate: {info.get('frameRate', 0):.2f} fps")
        print(f"  Duration: {info.get('duration', 0):.2f} seconds")
        print(f"  Bitrate: {info.get('bitrate', 0) / 1_000_000:.2f} Mbps")
    
    if 'frameStats' in data:
        stats = data['frameStats']
        print(f"\n📊 Frame Statistics:")
        print(f"  Total Frames: {stats.get('totalFrames', 0)}")
        print(f"  I-Frames: {stats.get('iFrames', 0)}")
        print(f"  P-Frames: {stats.get('pFrames', 0)}")
        print(f"  B-Frames: {stats.get('bFrames', 0)}")
        print(f"  Average Frame Size: {stats.get('averageFrameSize', 0) / 1024:.2f} KB")
    
    if 'bitrateStats' in data:
        stats = data['bitrateStats']
        print(f"\n📈 Bitrate Statistics:")
        print(f"  Average: {stats.get('averageBitrate', 0) / 1_000_000:.2f} Mbps")
        print(f"  Maximum: {stats.get('maxBitrate', 0) / 1_000_000:.2f} Mbps")
        print(f"  Minimum: {stats.get('minBitrate', 0) / 1_000_000:.2f} Mbps")
    
    if 'gops' in data:
        gops = data['gops']
        if gops:
            avg_length = sum(g['frameCount'] for g in gops) / len(gops)
            print(f"\n🎬 GOP Statistics:")
            print(f"  Total GOPs: {len(gops)}")
            print(f"  Average GOP Length: {avg_length:.1f} frames")
            print(f"  Max GOP Length: {max(g['frameCount'] for g in gops)} frames")
            print(f"  Min GOP Length: {min(g['frameCount'] for g in gops)} frames")
    
    print("\n" + "="*60 + "\n")

def visualize(json_file):
    """主可视化函数"""
    # Load data
    print(f"Loading analysis from: {json_file}")
    data = load_analysis(json_file)
    
    # Print summary
    print_summary(data)
    
    # Create figure with subplots
    fig = plt.figure(figsize=(16, 10))
    fig.suptitle('Video Stream Analysis Results', fontsize=16, fontweight='bold')
    
    # Create 2x2 grid
    ax1 = plt.subplot(2, 2, 1)
    ax2 = plt.subplot(2, 2, 2)
    ax3 = plt.subplot(2, 2, 3)
    ax4 = plt.subplot(2, 2, 4)
    
    # Plot different aspects
    plot_bitrate(data, ax1)
    plot_frame_types(data, ax2)
    plot_gop_structure(data, ax3)
    plot_frame_sizes(data, ax4)
    
    plt.tight_layout()
    
    # Save figure
    output_file = Path(json_file).stem + '_visualization.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"✅ Visualization saved to: {output_file}")
    
    # Show interactive plot
    print("📊 Displaying interactive plot...")
    plt.show()

def main():
    if len(sys.argv) < 2:
        print("Usage: python visualize_analysis.py <analysis.json>")
        print("\nExample:")
        print("  ./video_analyzer_cli input.mp4 --output analysis.json")
        print("  python visualize_analysis.py analysis.json")
        sys.exit(1)
    
    json_file = sys.argv[1]
    
    if not Path(json_file).exists():
        print(f"Error: File not found: {json_file}")
        sys.exit(1)
    
    try:
        visualize(json_file)
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
