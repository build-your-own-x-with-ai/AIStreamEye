#!/bin/bash

# Script to generate test videos for Video Stream Analyzer

set -e

# Create test_videos directory
mkdir -p test_videos

echo "Generating test videos..."

# H.264, 1920x1080, 30fps, 5-second, GOP=30
echo "1. Generating H.264 test video..."
ffmpeg -f lavfi -i testsrc=duration=5:size=1920x1080:rate=30 \
       -c:v libx264 -g 30 -keyint_min 30 -pix_fmt yuv420p \
       -y test_videos/test_h264_1080p_30fps.mp4

# H.264, 1280x720, 60fps, 3-second, GOP=60
echo "2. Generating H.264 720p 60fps test video..."
ffmpeg -f lavfi -i testsrc=duration=3:size=1280x720:rate=60 \
       -c:v libx264 -g 60 -keyint_min 60 -pix_fmt yuv420p \
       -y test_videos/test_h264_720p_60fps.mp4

# H.264, 640x480, 24fps, 2-second, GOP=24
echo "3. Generating H.264 480p 24fps test video..."
ffmpeg -f lavfi -i testsrc=duration=2:size=640x480:rate=24 \
       -c:v libx264 -g 24 -keyint_min 24 -pix_fmt yuv420p \
       -y test_videos/test_h264_480p_24fps.mp4

# Single frame video
echo "4. Generating single frame video..."
ffmpeg -f lavfi -i testsrc=duration=0.04:size=640x480:rate=25 \
       -c:v libx264 -frames:v 1 -pix_fmt yuv420p \
       -y test_videos/test_single_frame.mp4

# Only I-frames (GOP=1)
echo "5. Generating I-frame only video..."
ffmpeg -f lavfi -i testsrc=duration=2:size=640x480:rate=25 \
       -c:v libx264 -g 1 -keyint_min 1 -pix_fmt yuv420p \
       -y test_videos/test_iframes_only.mp4

# Small GOP (GOP=5)
echo "6. Generating small GOP video..."
ffmpeg -f lavfi -i testsrc=duration=2:size=640x480:rate=25 \
       -c:v libx264 -g 5 -keyint_min 5 -pix_fmt yuv420p \
       -y test_videos/test_small_gop.mp4

# AV1, 1280x720, 30fps, 3-second, GOP=30
echo "7. Generating AV1 test video..."
ffmpeg -f lavfi -i testsrc=duration=3:size=1280x720:rate=30 \
       -c:v libaom-av1 -g 30 -cpu-used 8 -pix_fmt yuv420p \
       -y test_videos/test_av1_720p_30fps.mp4 2>/dev/null || \
       echo "Warning: AV1 encoder not available, skipping AV1 test video"

echo "Test videos generated successfully in test_videos/"
ls -lh test_videos/
