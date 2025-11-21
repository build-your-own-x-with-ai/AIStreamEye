#include "video_analyzer/video_decoder.h"
#include "video_analyzer/ffmpeg_error.h"
#include <gtest/gtest.h>
#include <thread>

using namespace video_analyzer;

// Test opening a valid video file
TEST(VideoDecoderTest, OpenValidFile) {
    ASSERT_NO_THROW({
        VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
        auto info = decoder.getStreamInfo();
        EXPECT_EQ(info.width, 640);
        EXPECT_EQ(info.height, 480);
    });
}

// Test stream info extraction
TEST(VideoDecoderTest, GetStreamInfo) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    auto info = decoder.getStreamInfo();
    
    EXPECT_FALSE(info.codecName.empty());
    EXPECT_GT(info.width, 0);
    EXPECT_GT(info.height, 0);
    EXPECT_GT(info.frameRate, 0.0);
}

// Test frame reading
TEST(VideoDecoderTest, ReadFrames) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    
    int frameCount = 0;
    while (auto frame = decoder.readNextFrame()) {
        frameCount++;
        EXPECT_GE(frame->pts, 0);
        EXPECT_GT(frame->size, 0);
        
        if (frameCount >= 10) break; // Read first 10 frames
    }
    
    EXPECT_GT(frameCount, 0);
}

// Test invalid file
TEST(VideoDecoderTest, OpenInvalidFile) {
    EXPECT_THROW({
        VideoDecoder decoder("nonexistent_file.mp4");
    }, FFmpegError);
}

// Test multi-threaded decoding with different thread counts
TEST(VideoDecoderTest, MultiThreadedDecoding) {
    // Test with 1 thread
    {
        VideoDecoder decoder("test_videos/test_h264_720p_60fps.mp4", 1);
        auto info = decoder.getStreamInfo();
        EXPECT_GT(info.width, 0);
        EXPECT_GT(info.height, 0);
        
        int frameCount = 0;
        while (auto frame = decoder.readNextFrame()) {
            frameCount++;
            if (frameCount >= 20) break;
        }
        EXPECT_GT(frameCount, 0);
    }
    
    // Test with 2 threads
    {
        VideoDecoder decoder("test_videos/test_h264_720p_60fps.mp4", 2);
        auto info = decoder.getStreamInfo();
        EXPECT_GT(info.width, 0);
        
        int frameCount = 0;
        while (auto frame = decoder.readNextFrame()) {
            frameCount++;
            if (frameCount >= 20) break;
        }
        EXPECT_GT(frameCount, 0);
    }
    
    // Test with 4 threads
    {
        VideoDecoder decoder("test_videos/test_h264_720p_60fps.mp4", 4);
        auto info = decoder.getStreamInfo();
        EXPECT_GT(info.width, 0);
        
        int frameCount = 0;
        while (auto frame = decoder.readNextFrame()) {
            frameCount++;
            if (frameCount >= 20) break;
        }
        EXPECT_GT(frameCount, 0);
    }
}

// Test frame order preservation with multi-threading
TEST(VideoDecoderTest, MultiThreadedFrameOrderPreservation) {
    VideoDecoder decoder("test_videos/test_h264_720p_60fps.mp4", 4);
    
    int64_t lastPts = -1;
    int frameCount = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        // PTS should be monotonically increasing
        EXPECT_GT(frame->pts, lastPts) << "Frame " << frameCount << " has PTS " 
                                        << frame->pts << " which is not greater than previous PTS " 
                                        << lastPts;
        lastPts = frame->pts;
        frameCount++;
        
        if (frameCount >= 50) break; // Test first 50 frames
    }
    
    EXPECT_GT(frameCount, 0);
}

// Test thread count exceeding hardware cores
TEST(VideoDecoderTest, ThreadCountExceedsHardwareCores) {
    unsigned int hardwareCores = std::thread::hardware_concurrency();
    if (hardwareCores == 0) {
        hardwareCores = 1; // Fallback
    }
    
    // Request more threads than available cores
    unsigned int requestedThreads = hardwareCores * 2;
    
    VideoDecoder decoder("test_videos/test_h264_480p_24fps.mp4", requestedThreads);
    auto info = decoder.getStreamInfo();
    EXPECT_GT(info.width, 0);
    
    // Should still work correctly (limited to hardware cores internally)
    int frameCount = 0;
    int64_t lastPts = -1;
    
    while (auto frame = decoder.readNextFrame()) {
        EXPECT_GT(frame->pts, lastPts);
        lastPts = frame->pts;
        frameCount++;
        
        if (frameCount >= 20) break;
    }
    
    EXPECT_GT(frameCount, 0);
}

// Test auto-detect thread count (threadCount = 0)
TEST(VideoDecoderTest, AutoDetectThreadCount) {
    VideoDecoder decoder("test_videos/test_h264_480p_24fps.mp4", 0);
    auto info = decoder.getStreamInfo();
    EXPECT_GT(info.width, 0);
    
    int frameCount = 0;
    int64_t lastPts = -1;
    
    while (auto frame = decoder.readNextFrame()) {
        EXPECT_GT(frame->pts, lastPts);
        lastPts = frame->pts;
        frameCount++;
        
        if (frameCount >= 20) break;
    }
    
    EXPECT_GT(frameCount, 0);
}

// ============================================================================
// AV1 Support Tests (Requirements 12.1, 12.2, 12.3, 12.4)
// ============================================================================

// Test opening AV1 video file (Requirement 12.1)
TEST(VideoDecoderTest, AV1_OpenFile) {
    ASSERT_NO_THROW({
        VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
        auto info = decoder.getStreamInfo();
        EXPECT_EQ(info.codecName, "av1");
        EXPECT_EQ(info.width, 1280);
        EXPECT_EQ(info.height, 720);
    });
}

// Test AV1 stream info extraction (Requirement 12.1)
TEST(VideoDecoderTest, AV1_GetStreamInfo) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    auto info = decoder.getStreamInfo();
    
    EXPECT_EQ(info.codecName, "av1");
    EXPECT_GT(info.width, 0);
    EXPECT_GT(info.height, 0);
    EXPECT_GT(info.frameRate, 0.0);
    EXPECT_GT(info.duration, 0.0);
}

// Test AV1 frame type extraction (Requirement 12.2)
TEST(VideoDecoderTest, AV1_FrameTypeExtraction) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    
    bool foundKeyFrame = false;
    bool foundInterFrame = false;
    int frameCount = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        // Every frame should have a valid type
        EXPECT_NE(frame->type, FrameType::UNKNOWN);
        
        // AV1 has Key Frames (I_FRAME) and Inter Frames (P_FRAME)
        if (frame->isKeyFrame) {
            EXPECT_EQ(frame->type, FrameType::I_FRAME);
            foundKeyFrame = true;
        } else {
            // Inter frames should be P_FRAME
            EXPECT_EQ(frame->type, FrameType::P_FRAME);
            foundInterFrame = true;
        }
        
        frameCount++;
        if (frameCount >= 50) break;
    }
    
    EXPECT_TRUE(foundKeyFrame) << "Should have at least one Key Frame";
    EXPECT_TRUE(foundInterFrame) << "Should have at least one Inter Frame";
}

// Test AV1 tile information extraction (Requirement 12.3)
TEST(VideoDecoderTest, AV1_TileInformationExtraction) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    auto info = decoder.getStreamInfo();
    
    // Verify AV1 tile info is present
    ASSERT_TRUE(info.av1TileInfo.has_value()) << "AV1 stream should have tile information";
    
    // Verify tile info has valid values
    EXPECT_GT(info.av1TileInfo->tileColumns, 0);
    EXPECT_GT(info.av1TileInfo->tileRows, 0);
}

// Test AV1 QP range validity (Requirement 12.4)
TEST(VideoDecoderTest, AV1_QPRangeValidity) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    
    int frameCount = 0;
    while (auto frame = decoder.readNextFrame()) {
        // AV1 QP should be in valid range [0, 255]
        EXPECT_GE(frame->qp, 0) << "Frame " << frameCount << " QP is below 0";
        EXPECT_LE(frame->qp, 255) << "Frame " << frameCount << " QP is above 255";
        
        frameCount++;
        if (frameCount >= 30) break;
    }
    
    EXPECT_GT(frameCount, 0);
}

// Test AV1 frame reading
TEST(VideoDecoderTest, AV1_ReadFrames) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    
    int frameCount = 0;
    int64_t lastPts = -1;
    
    while (auto frame = decoder.readNextFrame()) {
        frameCount++;
        
        // Verify basic frame properties
        EXPECT_GE(frame->pts, 0);
        EXPECT_GT(frame->size, 0);
        EXPECT_GE(frame->timestamp, 0.0);
        
        // Verify PTS monotonicity
        if (lastPts >= 0) {
            EXPECT_GT(frame->pts, lastPts);
        }
        lastPts = frame->pts;
        
        if (frameCount >= 20) break;
    }
    
    EXPECT_GT(frameCount, 0);
}

// Test AV1 with multi-threaded decoding
TEST(VideoDecoderTest, AV1_MultiThreadedDecoding) {
    std::vector<int> threadCounts = {1, 2, 4};
    
    for (int threadCount : threadCounts) {
        VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4", threadCount);
        auto info = decoder.getStreamInfo();
        
        EXPECT_EQ(info.codecName, "av1");
        EXPECT_GT(info.width, 0);
        
        int frameCount = 0;
        int64_t lastPts = -1;
        
        while (auto frame = decoder.readNextFrame()) {
            // Verify PTS monotonicity with multi-threading
            if (lastPts >= 0) {
                EXPECT_GT(frame->pts, lastPts) 
                    << "Threads: " << threadCount << ", Frame: " << frameCount;
            }
            lastPts = frame->pts;
            
            frameCount++;
            if (frameCount >= 30) break;
        }
        
        EXPECT_GT(frameCount, 0) << "Threads: " << threadCount;
    }
}

// Test AV1 tile info JSON serialization
TEST(VideoDecoderTest, AV1_TileInfoSerialization) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    auto info = decoder.getStreamInfo();
    
    // Serialize to JSON
    auto json = info.toJson();
    
    // Verify AV1 tile info is in JSON
    ASSERT_TRUE(json.contains("av1TileInfo"));
    EXPECT_TRUE(json["av1TileInfo"].contains("tileColumns"));
    EXPECT_TRUE(json["av1TileInfo"].contains("tileRows"));
    
    // Verify values
    EXPECT_GT(json["av1TileInfo"]["tileColumns"].get<int>(), 0);
    EXPECT_GT(json["av1TileInfo"]["tileRows"].get<int>(), 0);
}

// Test that non-AV1 streams don't have AV1 tile info
TEST(VideoDecoderTest, NonAV1_NoTileInfo) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    auto info = decoder.getStreamInfo();
    
    // H.264 stream should not have AV1 tile info
    EXPECT_FALSE(info.av1TileInfo.has_value());
    
    // JSON should not contain av1TileInfo
    auto json = info.toJson();
    EXPECT_FALSE(json.contains("av1TileInfo"));
}
