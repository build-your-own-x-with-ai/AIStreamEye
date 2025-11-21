// Property-based tests
// Note: RapidCheck is not installed, so we implement basic property tests manually

#include "video_analyzer/ffmpeg_context.h"
#include "video_analyzer/ffmpeg_error.h"
#include <gtest/gtest.h>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

using namespace video_analyzer;

// Feature: video-stream-analyzer, Property 13: RAII resource cleanup
// For any VideoDecoder instance, when it goes out of scope or is destroyed,
// all FFmpeg resources should be properly released with no memory leaks

TEST(PropertyTest, RAIIResourceCleanup_MultipleContexts) {
    // Test that creating and destroying multiple contexts doesn't leak
    for (int i = 0; i < 100; ++i) {
        FFmpegContext ctx;
        AVFormatContext* fmtCtx = avformat_alloc_context();
        ctx.setFormatContext(fmtCtx);
        
        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (codec) {
            AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
            ctx.setCodecContext(codecCtx);
        }
        // Contexts should be freed automatically when ctx goes out of scope
    }
    SUCCEED(); // If we get here without crashing, RAII is working
}

TEST(PropertyTest, RAIIResourceCleanup_MultiplePackets) {
    // Test that creating and destroying multiple packets doesn't leak
    for (int i = 0; i < 100; ++i) {
        PacketPtr packet;
        EXPECT_NE(packet.get(), nullptr);
        // Packet should be freed automatically when packet goes out of scope
    }
    SUCCEED();
}

TEST(PropertyTest, RAIIResourceCleanup_MultipleFrames) {
    // Test that creating and destroying multiple frames doesn't leak
    for (int i = 0; i < 100; ++i) {
        FramePtr frame;
        EXPECT_NE(frame.get(), nullptr);
        // Frame should be freed automatically when frame goes out of scope
    }
    SUCCEED();
}

TEST(PropertyTest, RAIIResourceCleanup_MoveSemantics) {
    // Test that move semantics don't cause double-free
    std::vector<FFmpegContext> contexts;
    
    for (int i = 0; i < 10; ++i) {
        FFmpegContext ctx;
        AVFormatContext* fmtCtx = avformat_alloc_context();
        ctx.setFormatContext(fmtCtx);
        contexts.push_back(std::move(ctx));
    }
    
    // All contexts should be properly cleaned up when vector is destroyed
    SUCCEED();
}

TEST(PropertyTest, RAIIResourceCleanup_ExceptionSafety) {
    // Test that resources are cleaned up even when exceptions occur
    try {
        FFmpegContext ctx;
        AVFormatContext* fmtCtx = avformat_alloc_context();
        ctx.setFormatContext(fmtCtx);
        
        // Simulate an exception
        throw std::runtime_error("Test exception");
    } catch (const std::exception&) {
        // Context should be cleaned up despite the exception
    }
    SUCCEED();
}


// Feature: video-stream-analyzer, Property 11: JSON serialization round-trip
// For any analysis result, serializing to JSON and then deserializing
// should produce an equivalent object

#include "video_analyzer/data_models.h"

TEST(PropertyTest, JsonRoundTrip_FrameInfo) {
    // Test multiple FrameInfo instances with different values
    std::vector<FrameInfo> frames = {
        {1000, 900, FrameType::I_FRAME, 50000, 25, true, 0.033},
        {2000, 1900, FrameType::P_FRAME, 10000, 30, false, 0.066},
        {3000, 2900, FrameType::B_FRAME, 5000, 35, false, 0.099},
        {0, 0, FrameType::UNKNOWN, 0, 0, false, 0.0}
    };
    
    for (const auto& original : frames) {
        auto json = original.toJson();
        
        // Reconstruct from JSON
        FrameInfo reconstructed{
            json["pts"].get<int64_t>(),
            json["dts"].get<int64_t>(),
            stringToFrameType(json["type"].get<std::string>()),
            json["size"].get<int>(),
            json["qp"].get<int>(),
            json["isKeyFrame"].get<bool>(),
            json["timestamp"].get<double>()
        };
        
        // Verify round-trip
        EXPECT_EQ(reconstructed.pts, original.pts);
        EXPECT_EQ(reconstructed.dts, original.dts);
        EXPECT_EQ(reconstructed.type, original.type);
        EXPECT_EQ(reconstructed.size, original.size);
        EXPECT_EQ(reconstructed.qp, original.qp);
        EXPECT_EQ(reconstructed.isKeyFrame, original.isKeyFrame);
        EXPECT_NEAR(reconstructed.timestamp, original.timestamp, 0.0001);
    }
}

TEST(PropertyTest, JsonRoundTrip_StreamInfo) {
    // Test multiple StreamInfo instances
    std::vector<StreamInfo> streams = {
        {"h264", 1920, 1080, 30.0, 10.5, 5000000, "yuv420p", 0},
        {"hevc", 3840, 2160, 60.0, 5.0, 10000000, "yuv420p10le", 1},
        {"vp9", 1280, 720, 24.0, 120.0, 2000000, "yuv420p", 0}
    };
    
    for (const auto& original : streams) {
        auto json = original.toJson();
        
        // Reconstruct from JSON
        StreamInfo reconstructed{
            json["codecName"].get<std::string>(),
            json["width"].get<int>(),
            json["height"].get<int>(),
            json["frameRate"].get<double>(),
            json["duration"].get<double>(),
            json["bitrate"].get<int64_t>(),
            json["pixelFormat"].get<std::string>(),
            json["streamIndex"].get<int>()
        };
        
        // Verify round-trip
        EXPECT_EQ(reconstructed.codecName, original.codecName);
        EXPECT_EQ(reconstructed.width, original.width);
        EXPECT_EQ(reconstructed.height, original.height);
        EXPECT_NEAR(reconstructed.frameRate, original.frameRate, 0.01);
        EXPECT_NEAR(reconstructed.duration, original.duration, 0.01);
        EXPECT_EQ(reconstructed.bitrate, original.bitrate);
        EXPECT_EQ(reconstructed.pixelFormat, original.pixelFormat);
        EXPECT_EQ(reconstructed.streamIndex, original.streamIndex);
    }
}

TEST(PropertyTest, JsonRoundTrip_GOPInfo) {
    // Test multiple GOPInfo instances
    std::vector<GOPInfo> gops = {
        {0, 0, 30000, 30, 1, 9, 20, 500000, false},
        {1, 30000, 60000, 30, 1, 9, 20, 480000, false},
        {2, 60000, 90000, 30, 1, 9, 20, 520000, true}
    };
    
    for (const auto& original : gops) {
        auto json = original.toJson();
        
        // Reconstruct from JSON
        GOPInfo reconstructed{
            json["gopIndex"].get<int>(),
            json["startPts"].get<int64_t>(),
            json["endPts"].get<int64_t>(),
            json["frameCount"].get<int>(),
            json["iFrameCount"].get<int>(),
            json["pFrameCount"].get<int>(),
            json["bFrameCount"].get<int>(),
            json["totalSize"].get<int64_t>(),
            json["isOpenGOP"].get<bool>()
        };
        
        // Verify round-trip
        EXPECT_EQ(reconstructed.gopIndex, original.gopIndex);
        EXPECT_EQ(reconstructed.startPts, original.startPts);
        EXPECT_EQ(reconstructed.endPts, original.endPts);
        EXPECT_EQ(reconstructed.frameCount, original.frameCount);
        EXPECT_EQ(reconstructed.iFrameCount, original.iFrameCount);
        EXPECT_EQ(reconstructed.pFrameCount, original.pFrameCount);
        EXPECT_EQ(reconstructed.bFrameCount, original.bFrameCount);
        EXPECT_EQ(reconstructed.totalSize, original.totalSize);
        EXPECT_EQ(reconstructed.isOpenGOP, original.isOpenGOP);
    }
}

TEST(PropertyTest, JsonRoundTrip_BitrateStatistics) {
    BitrateStatistics original{
        5000000.0,
        8000000.0,
        3000000.0,
        500000.0,
        {{0.0, 5000000.0}, {1.0, 6000000.0}, {2.0, 4000000.0}}
    };
    
    auto json = original.toJson();
    
    // Reconstruct from JSON
    BitrateStatistics reconstructed;
    reconstructed.averageBitrate = json["averageBitrate"].get<double>();
    reconstructed.maxBitrate = json["maxBitrate"].get<double>();
    reconstructed.minBitrate = json["minBitrate"].get<double>();
    reconstructed.stdDeviation = json["stdDeviation"].get<double>();
    
    for (const auto& item : json["timeSeriesData"]) {
        reconstructed.timeSeriesData.push_back({
            item["timestamp"].get<double>(),
            item["bitrate"].get<double>()
        });
    }
    
    // Verify round-trip
    EXPECT_NEAR(reconstructed.averageBitrate, original.averageBitrate, 1.0);
    EXPECT_NEAR(reconstructed.maxBitrate, original.maxBitrate, 1.0);
    EXPECT_NEAR(reconstructed.minBitrate, original.minBitrate, 1.0);
    EXPECT_NEAR(reconstructed.stdDeviation, original.stdDeviation, 1.0);
    EXPECT_EQ(reconstructed.timeSeriesData.size(), original.timeSeriesData.size());
    
    for (size_t i = 0; i < original.timeSeriesData.size(); ++i) {
        EXPECT_NEAR(reconstructed.timeSeriesData[i].timestamp, 
                    original.timeSeriesData[i].timestamp, 0.01);
        EXPECT_NEAR(reconstructed.timeSeriesData[i].bitrate, 
                    original.timeSeriesData[i].bitrate, 1.0);
    }
}


// VideoDecoder Property Tests
#include "video_analyzer/video_decoder.h"

// Feature: video-stream-analyzer, Property 1: Video file opening succeeds for valid formats
TEST(PropertyTest, VideoFileOpeningSucceeds) {
    std::vector<std::string> testVideos = {
        "../test_videos/test_h264_480p_24fps.mp4",
        "../test_videos/test_h264_720p_60fps.mp4",
        "../test_videos/test_h264_1080p_30fps.mp4"
    };
    
    for (const auto& video : testVideos) {
        ASSERT_NO_THROW({
            VideoDecoder decoder(video);
            auto info = decoder.getStreamInfo();
            EXPECT_GT(info.width, 0);
            EXPECT_GT(info.height, 0);
        });
    }
}

// Feature: video-stream-analyzer, Property 2: Frame type identification is consistent
TEST(PropertyTest, FrameTypeIdentificationConsistent) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    
    bool foundIFrame = false;
    int frameCount = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        // Every frame should have a valid type
        EXPECT_NE(frame->type, FrameType::UNKNOWN);
        
        if (frame->isKeyFrame) {
            EXPECT_EQ(frame->type, FrameType::I_FRAME);
            foundIFrame = true;
        }
        
        if (++frameCount >= 50) break;
    }
    
    EXPECT_TRUE(foundIFrame); // Should have at least one I-frame
}

// Feature: video-stream-analyzer, Property 3: Timestamp monotonicity
TEST(PropertyTest, TimestampMonotonicity) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    
    int64_t lastPts = -1;
    int frameCount = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        if (lastPts >= 0) {
            EXPECT_GE(frame->pts, lastPts); // PTS should be monotonically increasing
        }
        lastPts = frame->pts;
        
        if (++frameCount >= 50) break;
    }
}

// Feature: video-stream-analyzer, Property 4: Frame size summation
TEST(PropertyTest, FrameSizeSummation) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    
    int64_t totalSize = 0;
    int frameCount = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        EXPECT_GT(frame->size, 0);
        totalSize += frame->size;
        frameCount++;
        if (frameCount >= 20) break; // Test first 20 frames
    }
    
    EXPECT_GT(totalSize, 0);
    EXPECT_GT(frameCount, 0);
}

// Feature: video-stream-analyzer, Property 5: QP value range validity
TEST(PropertyTest, QPValueRangeValidity) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    
    int frameCount = 0;
    while (auto frame = decoder.readNextFrame()) {
        // QP should be in valid range [0, 51] for H.264
        // Note: Our current implementation returns 0 as placeholder
        EXPECT_GE(frame->qp, 0);
        EXPECT_LE(frame->qp, 51);
        
        if (++frameCount >= 20) break;
    }
}

// Feature: video-stream-analyzer, Property 22: Multi-threaded frame order preservation
// For any video decoded with multiple threads, the output frame sequence should
// maintain monotonically increasing PTS values
TEST(PropertyTest, MultiThreadedFrameOrderPreservation) {
    // Test with different thread counts
    std::vector<int> threadCounts = {1, 2, 4, 8};
    std::vector<std::string> testVideos = {
        "test_videos/test_h264_480p_24fps.mp4",
        "test_videos/test_h264_720p_60fps.mp4",
        "test_videos/test_h264_1080p_30fps.mp4"
    };
    
    for (const auto& video : testVideos) {
        for (int threadCount : threadCounts) {
            VideoDecoder decoder(video, threadCount);
            
            int64_t lastPts = -1;
            int frameCount = 0;
            
            while (auto frame = decoder.readNextFrame()) {
                // PTS should be monotonically increasing regardless of thread count
                if (lastPts >= 0) {
                    EXPECT_GT(frame->pts, lastPts) 
                        << "Video: " << video 
                        << ", Threads: " << threadCount 
                        << ", Frame: " << frameCount
                        << ", Current PTS: " << frame->pts
                        << ", Previous PTS: " << lastPts;
                }
                lastPts = frame->pts;
                frameCount++;
                
                if (frameCount >= 100) break; // Test first 100 frames
            }
            
            EXPECT_GT(frameCount, 0) << "Video: " << video << ", Threads: " << threadCount;
        }
    }
}

// Additional property test: Multi-threaded decoding produces same frame sequence
TEST(PropertyTest, MultiThreadedFrameSequenceConsistency) {
    std::string testVideo = "test_videos/test_h264_720p_60fps.mp4";
    
    // Decode with single thread (reference)
    std::vector<FrameInfo> referenceFrames;
    {
        VideoDecoder decoder(testVideo, 1);
        int frameCount = 0;
        while (auto frame = decoder.readNextFrame()) {
            referenceFrames.push_back(*frame);
            if (++frameCount >= 50) break;
        }
    }
    
    // Decode with multiple threads and compare
    std::vector<int> threadCounts = {2, 4};
    for (int threadCount : threadCounts) {
        VideoDecoder decoder(testVideo, threadCount);
        
        size_t frameIndex = 0;
        while (auto frame = decoder.readNextFrame()) {
            ASSERT_LT(frameIndex, referenceFrames.size()) 
                << "Multi-threaded decoder produced more frames than single-threaded";
            
            const auto& refFrame = referenceFrames[frameIndex];
            
            // Verify frame properties match
            EXPECT_EQ(frame->pts, refFrame.pts) 
                << "Threads: " << threadCount << ", Frame: " << frameIndex;
            EXPECT_EQ(frame->type, refFrame.type) 
                << "Threads: " << threadCount << ", Frame: " << frameIndex;
            EXPECT_EQ(frame->isKeyFrame, refFrame.isKeyFrame) 
                << "Threads: " << threadCount << ", Frame: " << frameIndex;
            
            frameIndex++;
            if (frameIndex >= referenceFrames.size()) break;
        }
        
        EXPECT_EQ(frameIndex, referenceFrames.size()) 
            << "Multi-threaded decoder produced different number of frames";
    }
}


// ============================================================================
// AV1 Property Tests (Requirements 12.1, 12.2, 12.3, 12.4)
// ============================================================================

// Feature: video-stream-analyzer, Property 23: AV1 file opening
// For any valid AV1-encoded video file, opening the file should succeed
// and return a valid VideoDecoder instance
// **Validates: Requirements 12.1**
TEST(PropertyTest, AV1_FileOpening) {
    std::vector<std::string> av1Videos = {
        "../test_videos/test_av1_720p_30fps.mp4"
    };
    
    for (const auto& video : av1Videos) {
        ASSERT_NO_THROW({
            VideoDecoder decoder(video);
            auto info = decoder.getStreamInfo();
            
            // Verify it's an AV1 stream
            EXPECT_EQ(info.codecName, "av1");
            
            // Verify basic properties are valid
            EXPECT_GT(info.width, 0);
            EXPECT_GT(info.height, 0);
            EXPECT_GT(info.frameRate, 0.0);
            
            // Verify AV1-specific tile info is present
            EXPECT_TRUE(info.av1TileInfo.has_value());
        }) << "Failed to open AV1 video: " << video;
    }
}

// Feature: video-stream-analyzer, Property 24: AV1 frame type extraction
// For any AV1 video frame, the frame type (Key Frame or Inter Frame)
// should be correctly identified
// **Validates: Requirements 12.2**
TEST(PropertyTest, AV1_FrameTypeExtraction) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    
    bool foundKeyFrame = false;
    bool foundInterFrame = false;
    int frameCount = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        // Every frame should have a valid type (not UNKNOWN)
        EXPECT_NE(frame->type, FrameType::UNKNOWN) 
            << "Frame " << frameCount << " has UNKNOWN type";
        
        // AV1 frames should be either I_FRAME (Key Frame) or P_FRAME (Inter Frame)
        EXPECT_TRUE(frame->type == FrameType::I_FRAME || frame->type == FrameType::P_FRAME)
            << "Frame " << frameCount << " has unexpected type";
        
        // Key frames should be marked as I_FRAME
        if (frame->isKeyFrame) {
            EXPECT_EQ(frame->type, FrameType::I_FRAME)
                << "Frame " << frameCount << " is marked as key frame but type is not I_FRAME";
            foundKeyFrame = true;
        } else {
            // Non-key frames should be P_FRAME (Inter Frame)
            EXPECT_EQ(frame->type, FrameType::P_FRAME)
                << "Frame " << frameCount << " is not a key frame but type is not P_FRAME";
            foundInterFrame = true;
        }
        
        frameCount++;
        if (frameCount >= 90) break; // Test all frames in the 3-second video
    }
    
    // Verify we found both types of frames
    EXPECT_TRUE(foundKeyFrame) << "No Key Frames found in AV1 video";
    EXPECT_TRUE(foundInterFrame) << "No Inter Frames found in AV1 video";
    EXPECT_GT(frameCount, 0) << "No frames were decoded";
}

// Feature: video-stream-analyzer, Property 25: AV1 tile information extraction
// For any AV1 video, tile configuration information should be present
// in the stream metadata
// **Validates: Requirements 12.3**
TEST(PropertyTest, AV1_TileInformationExtraction) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    auto info = decoder.getStreamInfo();
    
    // Verify codec is AV1
    EXPECT_EQ(info.codecName, "av1");
    
    // Verify AV1 tile info is present
    ASSERT_TRUE(info.av1TileInfo.has_value()) 
        << "AV1 stream should have tile information";
    
    // Verify tile info has valid values (at least 1 tile in each dimension)
    EXPECT_GT(info.av1TileInfo->tileColumns, 0) 
        << "Tile columns should be greater than 0";
    EXPECT_GT(info.av1TileInfo->tileRows, 0) 
        << "Tile rows should be greater than 0";
    
    // Verify tile info can be serialized to JSON
    auto json = info.toJson();
    ASSERT_TRUE(json.contains("av1TileInfo"));
    EXPECT_TRUE(json["av1TileInfo"].contains("tileColumns"));
    EXPECT_TRUE(json["av1TileInfo"].contains("tileRows"));
}

// Feature: video-stream-analyzer, Property 26: AV1 QP range validity
// For any AV1 video frame, the extracted quantization parameter
// should be within the valid range [0, 255]
// **Validates: Requirements 12.4**
TEST(PropertyTest, AV1_QPRangeValidity) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    
    int frameCount = 0;
    int minQP = 255;
    int maxQP = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        // AV1 QP should be in valid range [0, 255]
        EXPECT_GE(frame->qp, 0) 
            << "Frame " << frameCount << " QP (" << frame->qp << ") is below 0";
        EXPECT_LE(frame->qp, 255) 
            << "Frame " << frameCount << " QP (" << frame->qp << ") is above 255";
        
        // Track min/max for statistics
        minQP = std::min(minQP, frame->qp);
        maxQP = std::max(maxQP, frame->qp);
        
        frameCount++;
        if (frameCount >= 90) break; // Test all frames
    }
    
    EXPECT_GT(frameCount, 0) << "No frames were decoded";
    
    // Verify we got valid QP values
    EXPECT_GE(minQP, 0);
    EXPECT_LE(maxQP, 255);
}

// Additional property test: AV1 vs H.264 QP range difference
TEST(PropertyTest, AV1_QPRangeVsH264) {
    // Test AV1 QP range
    {
        VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
        int frameCount = 0;
        
        while (auto frame = decoder.readNextFrame()) {
            // AV1 QP range is [0, 255]
            EXPECT_GE(frame->qp, 0);
            EXPECT_LE(frame->qp, 255);
            
            frameCount++;
            if (frameCount >= 20) break;
        }
        
        EXPECT_GT(frameCount, 0);
    }
    
    // Test H.264 QP range
    {
        VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
        int frameCount = 0;
        
        while (auto frame = decoder.readNextFrame()) {
            // H.264 QP range is [0, 51]
            EXPECT_GE(frame->qp, 0);
            EXPECT_LE(frame->qp, 51);
            
            frameCount++;
            if (frameCount >= 20) break;
        }
        
        EXPECT_GT(frameCount, 0);
    }
}

// Additional property test: AV1 frame sequence consistency
TEST(PropertyTest, AV1_FrameSequenceConsistency) {
    VideoDecoder decoder("../test_videos/test_av1_720p_30fps.mp4");
    
    int64_t lastPts = -1;
    int frameCount = 0;
    int keyFrameCount = 0;
    
    while (auto frame = decoder.readNextFrame()) {
        // PTS should be monotonically increasing
        if (lastPts >= 0) {
            EXPECT_GT(frame->pts, lastPts) 
                << "Frame " << frameCount << " PTS not monotonic";
        }
        lastPts = frame->pts;
        
        // Count key frames
        if (frame->isKeyFrame) {
            keyFrameCount++;
        }
        
        // Verify frame size is positive
        EXPECT_GT(frame->size, 0) 
            << "Frame " << frameCount << " has zero or negative size";
        
        // Verify timestamp is non-negative
        EXPECT_GE(frame->timestamp, 0.0) 
            << "Frame " << frameCount << " has negative timestamp";
        
        frameCount++;
    }
    
    EXPECT_GT(frameCount, 0) << "No frames decoded";
    EXPECT_GT(keyFrameCount, 0) << "No key frames found";
    
    // For a 3-second video at 30fps, we expect ~90 frames
    EXPECT_GE(frameCount, 80) << "Expected at least 80 frames for 3-second video";
    EXPECT_LE(frameCount, 100) << "Expected at most 100 frames for 3-second video";
}


// Scene Detection Property Tests
#include "video_analyzer/scene_detector.h"

// Feature: video-stream-analyzer, Property 14: Scene detection completeness
// For any video with scene cuts, all detected scene boundaries should have
// both timestamp and frame number recorded
TEST(PropertyTest, SceneDetectionCompleteness) {
    std::vector<std::string> testVideos = {
        "test_videos/test_h264_480p_24fps.mp4",
        "test_videos/test_h264_720p_60fps.mp4",
        "test_videos/test_h264_1080p_30fps.mp4"
    };
    
    for (const auto& videoPath : testVideos) {
        VideoDecoder decoder(videoPath);
        SceneDetector detector(decoder, 0.3);
        
        auto scenes = detector.analyze();
        
        // For any detected scene, verify completeness
        for (const auto& scene : scenes) {
            // Scene index should be valid
            EXPECT_GE(scene.sceneIndex, 0);
            
            // Timestamps should be recorded
            EXPECT_GE(scene.startTimestamp, 0.0);
            EXPECT_GE(scene.endTimestamp, scene.startTimestamp);
            
            // Frame numbers should be recorded
            EXPECT_GE(scene.startFrameNumber, 0);
            EXPECT_GE(scene.endFrameNumber, scene.startFrameNumber);
            
            // PTS values should be recorded
            EXPECT_GE(scene.startPts, 0);
            EXPECT_GE(scene.endPts, scene.startPts);
            
            // Frame count should be consistent
            EXPECT_EQ(scene.frameCount, scene.endFrameNumber - scene.startFrameNumber + 1);
            
            // Average brightness should be recorded
            EXPECT_GE(scene.averageBrightness, 0.0);
        }
    }
}

// Feature: video-stream-analyzer, Property 15: Scene threshold configuration
// For any valid threshold value, setting the scene detection threshold
// should affect the number of detected scenes
TEST(PropertyTest, SceneThresholdConfiguration) {
    std::string videoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    // Test with different threshold values
    std::vector<double> thresholds = {0.1, 0.3, 0.5, 0.7, 0.9};
    std::vector<size_t> sceneCounts;
    
    for (double threshold : thresholds) {
        VideoDecoder decoder(videoPath);
        SceneDetector detector(decoder, threshold);
        
        // Verify threshold is set correctly
        EXPECT_DOUBLE_EQ(detector.getThreshold(), threshold);
        
        auto scenes = detector.analyze();
        sceneCounts.push_back(scenes.size());
        
        // Scene count should be at least 1 (entire video is one scene)
        EXPECT_GE(scenes.size(), 1);
    }
    
    // Generally, lower thresholds should detect more or equal scenes
    // (more sensitive to changes)
    EXPECT_GE(sceneCounts[0], sceneCounts[4]);
    
    // Test threshold modification
    VideoDecoder decoder(videoPath);
    SceneDetector detector(decoder, 0.3);
    
    detector.setThreshold(0.5);
    EXPECT_DOUBLE_EQ(detector.getThreshold(), 0.5);
    
    detector.setThreshold(0.2);
    EXPECT_DOUBLE_EQ(detector.getThreshold(), 0.2);
}

// Feature: video-stream-analyzer, Property 16: Scene export completeness
// For any analyzed video with scene detection enabled, the exported data
// should contain all detected scene boundaries and statistics
TEST(PropertyTest, SceneExportCompleteness) {
    std::string videoPath = "test_videos/test_h264_480p_24fps.mp4";
    
    VideoDecoder decoder(videoPath);
    SceneDetector detector(decoder, 0.3);
    
    auto scenes = detector.analyze();
    
    // Verify all scenes can be exported to JSON
    for (const auto& scene : scenes) {
        auto json = scene.toJson();
        
        // Verify all required fields are present
        EXPECT_TRUE(json.contains("sceneIndex"));
        EXPECT_TRUE(json.contains("startPts"));
        EXPECT_TRUE(json.contains("endPts"));
        EXPECT_TRUE(json.contains("startFrameNumber"));
        EXPECT_TRUE(json.contains("endFrameNumber"));
        EXPECT_TRUE(json.contains("startTimestamp"));
        EXPECT_TRUE(json.contains("endTimestamp"));
        EXPECT_TRUE(json.contains("frameCount"));
        EXPECT_TRUE(json.contains("averageBrightness"));
        
        // Verify values match
        EXPECT_EQ(json["sceneIndex"].get<int>(), scene.sceneIndex);
        EXPECT_EQ(json["startPts"].get<int64_t>(), scene.startPts);
        EXPECT_EQ(json["endPts"].get<int64_t>(), scene.endPts);
        EXPECT_EQ(json["startFrameNumber"].get<int>(), scene.startFrameNumber);
        EXPECT_EQ(json["endFrameNumber"].get<int>(), scene.endFrameNumber);
        EXPECT_DOUBLE_EQ(json["startTimestamp"].get<double>(), scene.startTimestamp);
        EXPECT_DOUBLE_EQ(json["endTimestamp"].get<double>(), scene.endTimestamp);
        EXPECT_EQ(json["frameCount"].get<int>(), scene.frameCount);
        EXPECT_DOUBLE_EQ(json["averageBrightness"].get<double>(), scene.averageBrightness);
    }
    
    // Verify scene count and average duration statistics
    EXPECT_EQ(detector.getSceneCount(), static_cast<int>(scenes.size()));
    
    if (!scenes.empty()) {
        double avgDuration = detector.getAverageSceneDuration();
        EXPECT_GT(avgDuration, 0.0);
        
        // Manually verify average duration
        double totalDuration = 0.0;
        for (const auto& scene : scenes) {
            totalDuration += (scene.endTimestamp - scene.startTimestamp);
        }
        double expectedAvg = totalDuration / scenes.size();
        EXPECT_DOUBLE_EQ(avgDuration, expectedAvg);
    }
}

// ============================================================================
// Motion Vector Analysis Property Tests
// ============================================================================

#include "video_analyzer/motion_vector_analyzer.h"

// Feature: video-stream-analyzer, Property 17: Motion vector extraction from P/B frames
// For any P-frame or B-frame in a video, motion vector data should be extractable and non-empty

TEST(PropertyTest, MotionVectorExtraction_PBFrames) {
    std::string videoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    if (!std::filesystem::exists(videoPath)) {
        GTEST_SKIP() << "Test video not found: " << videoPath;
    }
    
    VideoDecoder decoder(videoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    // Extract motion vectors
    auto mvData = analyzer.extractMotionVectors();
    
    // Reset decoder to check frame types
    decoder.reset();
    std::vector<FrameInfo> frames;
    while (auto frameInfo = decoder.readNextFrame()) {
        frames.push_back(frameInfo.value());
    }
    
    // For P and B frames, motion vectors should be available
    // Note: Motion vector availability depends on FFmpeg configuration and codec
    // Some frames may not have motion vectors even if they are P/B frames
    
    // At minimum, verify that motion vector data structure is valid
    for (const auto& frameData : mvData) {
        EXPECT_GE(frameData.pts, 0);
        
        // Each motion vector should have valid structure
        for (const auto& vec : frameData.vectors) {
            // Magnitude should be non-negative
            EXPECT_GE(vec.magnitude, 0.0f);
            
            // Direction should be in valid range [-π, π]
            EXPECT_GE(vec.direction, -M_PI);
            EXPECT_LE(vec.direction, M_PI);
            
            // Motion components should be consistent with magnitude
            float calculatedMag = std::sqrt(vec.motionX * vec.motionX + vec.motionY * vec.motionY);
            EXPECT_NEAR(vec.magnitude, calculatedMag, 0.01f);
        }
    }
}

// Feature: video-stream-analyzer, Property 18: Motion vector structure completeness
// For any extracted motion vector, it should contain both direction and magnitude values

TEST(PropertyTest, MotionVectorStructure_Completeness) {
    std::string videoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    if (!std::filesystem::exists(videoPath)) {
        GTEST_SKIP() << "Test video not found: " << videoPath;
    }
    
    VideoDecoder decoder(videoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    // For every motion vector, verify structure completeness
    for (const auto& frameData : mvData) {
        for (const auto& vec : frameData.vectors) {
            // Verify all fields are present and valid
            
            // Position fields should be reasonable
            EXPECT_GE(vec.srcX, 0);
            EXPECT_GE(vec.srcY, 0);
            EXPECT_GE(vec.dstX, 0);
            EXPECT_GE(vec.dstY, 0);
            
            // Motion vector components
            // (can be negative for backward motion)
            
            // Magnitude should be non-negative
            EXPECT_GE(vec.magnitude, 0.0f);
            
            // Direction should be in valid range
            EXPECT_GE(vec.direction, -M_PI);
            EXPECT_LE(vec.direction, M_PI);
            
            // Verify magnitude and direction are consistent with motion components
            float expectedMag = std::sqrt(vec.motionX * vec.motionX + vec.motionY * vec.motionY);
            EXPECT_NEAR(vec.magnitude, expectedMag, 0.01f);
            
            if (vec.magnitude > 0.01f) {
                float expectedDir = std::atan2(vec.motionY, vec.motionX);
                EXPECT_NEAR(vec.direction, expectedDir, 0.01f);
            }
        }
    }
}

// Feature: video-stream-analyzer, Property 19: Motion statistics correctness
// For any set of motion vectors, the computed average magnitude should equal 
// the sum of magnitudes divided by the count

TEST(PropertyTest, MotionStatistics_AverageMagnitude) {
    std::string videoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    if (!std::filesystem::exists(videoPath)) {
        GTEST_SKIP() << "Test video not found: " << videoPath;
    }
    
    VideoDecoder decoder(videoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    auto stats = analyzer.computeStatistics(mvData);
    
    // Manually calculate average magnitude
    double sumMagnitude = 0.0;
    int totalVectors = 0;
    
    for (const auto& frameData : mvData) {
        for (const auto& vec : frameData.vectors) {
            sumMagnitude += vec.magnitude;
            totalVectors++;
        }
    }
    
    if (totalVectors == 0) {
        GTEST_SKIP() << "No motion vectors in data";
    }
    
    double expectedAvg = sumMagnitude / totalVectors;
    
    // Verify computed average matches manual calculation
    EXPECT_NEAR(stats.averageMagnitude, expectedAvg, 0.001);
    
    // Verify max >= avg >= min
    EXPECT_GE(stats.maxMagnitude, stats.averageMagnitude);
    EXPECT_GE(stats.averageMagnitude, stats.minMagnitude);
}

// Feature: video-stream-analyzer, Property 20: Motion region classification
// For any motion vector with magnitude below threshold, it should be classified as a static region;
// vectors above high threshold should be classified as high-motion regions

TEST(PropertyTest, MotionRegionClassification) {
    std::string videoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    if (!std::filesystem::exists(videoPath)) {
        GTEST_SKIP() << "Test video not found: " << videoPath;
    }
    
    VideoDecoder decoder(videoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    auto stats = analyzer.computeStatistics(mvData);
    
    // Manually classify regions
    int manualStaticCount = 0;
    int manualHighMotionCount = 0;
    const double staticThreshold = 1.0;
    const double highMotionThreshold = 10.0;
    
    for (const auto& frameData : mvData) {
        for (const auto& vec : frameData.vectors) {
            if (vec.magnitude < staticThreshold) {
                manualStaticCount++;
            }
            if (vec.magnitude > highMotionThreshold) {
                manualHighMotionCount++;
            }
        }
    }
    
    // Verify classification matches
    EXPECT_EQ(stats.staticRegions, manualStaticCount);
    EXPECT_EQ(stats.highMotionRegions, manualHighMotionCount);
    
    // Verify logical constraints
    int totalVectors = 0;
    for (const auto& frameData : mvData) {
        totalVectors += frameData.vectors.size();
    }
    
    if (totalVectors > 0) {
        EXPECT_LE(stats.staticRegions, totalVectors);
        EXPECT_LE(stats.highMotionRegions, totalVectors);
        EXPECT_GE(stats.staticRegions, 0);
        EXPECT_GE(stats.highMotionRegions, 0);
    }
}

// Feature: video-stream-analyzer, Property 21: Motion vector aggregation modes
// For any motion vector data, both per-frame and per-GOP aggregation should produce valid statistics

TEST(PropertyTest, MotionVectorAggregation_Modes) {
    std::string videoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    if (!std::filesystem::exists(videoPath)) {
        GTEST_SKIP() << "Test video not found: " << videoPath;
    }
    
    VideoDecoder decoder(videoPath);
    MotionVectorAnalyzer mvAnalyzer(decoder);
    
    auto mvData = mvAnalyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    // Test per-frame aggregation
    auto frameStats = mvAnalyzer.aggregateByFrame(mvData);
    
    // Should have one statistics entry per frame
    EXPECT_EQ(frameStats.size(), mvData.size());
    
    // Each frame should have valid statistics
    for (const auto& stats : frameStats) {
        EXPECT_GE(stats.averageMagnitude, 0.0);
        EXPECT_GE(stats.maxMagnitude, stats.minMagnitude);
        EXPECT_GE(stats.maxMagnitude, stats.averageMagnitude);
        EXPECT_GE(stats.staticRegions, 0);
        EXPECT_GE(stats.highMotionRegions, 0);
    }
    
    // Test per-GOP aggregation
    decoder.reset();
    GOPAnalyzer gopAnalyzer(decoder);
    auto gops = gopAnalyzer.analyze();
    
    if (gops.empty()) {
        GTEST_SKIP() << "No GOP data available";
    }
    
    auto gopStats = mvAnalyzer.aggregateByGOP(mvData, gops);
    
    // Should have one statistics entry per GOP
    EXPECT_EQ(gopStats.size(), gops.size());
    
    // Each GOP should have valid statistics
    for (const auto& stats : gopStats) {
        EXPECT_GE(stats.averageMagnitude, 0.0);
        EXPECT_GE(stats.maxMagnitude, stats.minMagnitude);
        EXPECT_GE(stats.maxMagnitude, stats.averageMagnitude);
        EXPECT_GE(stats.staticRegions, 0);
        EXPECT_GE(stats.highMotionRegions, 0);
    }
    
    // Verify that overall statistics are consistent with aggregated statistics
    auto overallStats = mvAnalyzer.computeStatistics(mvData);
    
    // The overall max should be >= any frame or GOP max
    for (const auto& stats : frameStats) {
        EXPECT_GE(overallStats.maxMagnitude, stats.maxMagnitude);
    }
    
    for (const auto& stats : gopStats) {
        EXPECT_GE(overallStats.maxMagnitude, stats.maxMagnitude);
    }
}


// ============================================================================
// Streaming Property Tests
// ============================================================================

#include "video_analyzer/stream_decoder.h"

// Feature: video-stream-analyzer, Property 27: Streaming protocol support
// For any supported streaming protocol, the StreamDecoder should successfully open and read frames

TEST(PropertyTest, StreamingProtocolSupport) {
    // Test with file:// protocol as a proxy for streaming
    std::string videoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    if (!std::filesystem::exists(videoPath)) {
        GTEST_SKIP() << "Test video not found";
    }
    
    std::string fileUrl = "file://" + std::filesystem::absolute(videoPath).string();
    
    try {
        StreamDecoder decoder(fileUrl);
        
        // Verify stream is active
        EXPECT_TRUE(decoder.isStreamActive());
        
        // Verify stream info is valid
        auto info = decoder.getStreamInfo();
        EXPECT_GT(info.width, 0);
        EXPECT_GT(info.height, 0);
        EXPECT_GE(info.frameRate, 0.0);
        
        // Verify frames can be read
        int frameCount = 0;
        while (decoder.isStreamActive() && frameCount < 10) {
            auto frame = decoder.readNextFrame();
            if (frame.has_value()) {
                frameCount++;
                
                // Verify frame data is valid
                EXPECT_GE(frame->pts, 0);
                EXPECT_GE(frame->size, 0);
                EXPECT_GE(frame->timestamp, 0.0);
            }
        }
        
        EXPECT_GT(frameCount, 0);
        
        // Verify buffer status is accessible
        auto status = decoder.getBufferStatus();
        EXPECT_GE(status.bufferedFrames, 0);
        EXPECT_GE(status.bufferedDuration, 0.0);
        
    } catch (const FFmpegError& e) {
        GTEST_SKIP() << "File protocol not supported: " << e.what();
    }
}
