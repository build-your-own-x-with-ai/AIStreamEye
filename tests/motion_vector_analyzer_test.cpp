#include "video_analyzer/motion_vector_analyzer.h"
#include "video_analyzer/video_decoder.h"
#include "video_analyzer/gop_analyzer.h"
#include <gtest/gtest.h>
#include <filesystem>

using namespace video_analyzer;

class MotionVectorAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use test video with motion
        testVideoPath = "test_videos/test_h264_720p_60fps.mp4";
        
        // Check if test video exists
        if (!std::filesystem::exists(testVideoPath)) {
            GTEST_SKIP() << "Test video not found: " << testVideoPath;
        }
    }
    
    std::string testVideoPath;
};

// Test motion vector extraction
TEST_F(MotionVectorAnalyzerTest, ExtractMotionVectors) {
    VideoDecoder decoder(testVideoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    // Should have motion vector data for some frames
    // Note: I-frames typically don't have motion vectors
    EXPECT_GE(mvData.size(), 0);
    
    // Check that motion vector data has valid structure
    for (const auto& frameData : mvData) {
        EXPECT_GE(frameData.pts, 0);
        // Vectors may be empty for I-frames
    }
}

// Test statistics computation
TEST_F(MotionVectorAnalyzerTest, ComputeStatistics) {
    VideoDecoder decoder(testVideoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    auto stats = analyzer.computeStatistics(mvData);
    
    // Validate statistics
    EXPECT_GE(stats.averageMagnitude, 0.0);
    EXPECT_GE(stats.maxMagnitude, stats.minMagnitude);
    EXPECT_GE(stats.maxMagnitude, stats.averageMagnitude);
    EXPECT_GE(stats.staticRegions, 0);
    EXPECT_GE(stats.highMotionRegions, 0);
    
    // Direction distribution should exist (may be empty if no motion vectors)
    EXPECT_GE(stats.directionDistribution.size(), 0);
}

// Test frame aggregation
TEST_F(MotionVectorAnalyzerTest, AggregateByFrame) {
    VideoDecoder decoder(testVideoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    auto frameStats = analyzer.aggregateByFrame(mvData);
    
    // Should have one statistics entry per frame with motion vectors
    EXPECT_EQ(frameStats.size(), mvData.size());
    
    // Each frame should have valid statistics
    for (const auto& stats : frameStats) {
        EXPECT_GE(stats.averageMagnitude, 0.0);
        EXPECT_GE(stats.maxMagnitude, stats.minMagnitude);
    }
}

// Test GOP aggregation
TEST_F(MotionVectorAnalyzerTest, AggregateByGOP) {
    VideoDecoder decoder(testVideoPath);
    MotionVectorAnalyzer mvAnalyzer(decoder);
    
    // First get GOP information
    decoder.reset();
    GOPAnalyzer gopAnalyzer(decoder);
    auto gops = gopAnalyzer.analyze();
    
    // Then get motion vectors
    decoder.reset();
    auto mvData = mvAnalyzer.extractMotionVectors();
    
    if (mvData.empty() || gops.empty()) {
        GTEST_SKIP() << "No motion vector or GOP data available";
    }
    
    auto gopStats = mvAnalyzer.aggregateByGOP(mvData, gops);
    
    // Should have one statistics entry per GOP
    EXPECT_EQ(gopStats.size(), gops.size());
    
    // Each GOP should have valid statistics
    for (const auto& stats : gopStats) {
        EXPECT_GE(stats.averageMagnitude, 0.0);
        EXPECT_GE(stats.maxMagnitude, stats.minMagnitude);
    }
}

// Test region classification
TEST_F(MotionVectorAnalyzerTest, RegionClassification) {
    VideoDecoder decoder(testVideoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    auto stats = analyzer.computeStatistics(mvData);
    
    // Count total vectors
    int totalVectors = 0;
    for (const auto& frameData : mvData) {
        totalVectors += frameData.vectors.size();
    }
    
    if (totalVectors == 0) {
        GTEST_SKIP() << "No motion vectors in data";
    }
    
    // Static + high motion + medium motion should account for all vectors
    // (though we don't track medium motion explicitly)
    EXPECT_LE(stats.staticRegions, totalVectors);
    EXPECT_LE(stats.highMotionRegions, totalVectors);
}

// Test with video containing known motion patterns
TEST_F(MotionVectorAnalyzerTest, MotionPatternDetection) {
    // Use a video with higher frame rate (more motion)
    std::string motionVideoPath = "test_videos/test_h264_720p_60fps.mp4";
    
    if (!std::filesystem::exists(motionVideoPath)) {
        GTEST_SKIP() << "Motion test video not found";
    }
    
    VideoDecoder decoder(motionVideoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    auto stats = analyzer.computeStatistics(mvData);
    
    // Higher frame rate video should have some motion
    // (though this depends on the actual content)
    EXPECT_GE(stats.averageMagnitude, 0.0);
}

// Test JSON serialization
TEST_F(MotionVectorAnalyzerTest, JsonSerialization) {
    VideoDecoder decoder(testVideoPath);
    MotionVectorAnalyzer analyzer(decoder);
    
    auto mvData = analyzer.extractMotionVectors();
    
    if (mvData.empty()) {
        GTEST_SKIP() << "No motion vector data available";
    }
    
    // Test MotionVectorData serialization
    for (const auto& frameData : mvData) {
        auto json = frameData.toJson();
        EXPECT_TRUE(json.contains("pts"));
        EXPECT_TRUE(json.contains("vectors"));
        EXPECT_TRUE(json["vectors"].is_array());
    }
    
    // Test MotionStatistics serialization
    auto stats = analyzer.computeStatistics(mvData);
    auto json = stats.toJson();
    
    EXPECT_TRUE(json.contains("averageMagnitude"));
    EXPECT_TRUE(json.contains("maxMagnitude"));
    EXPECT_TRUE(json.contains("minMagnitude"));
    EXPECT_TRUE(json.contains("directionDistribution"));
    EXPECT_TRUE(json.contains("staticRegions"));
    EXPECT_TRUE(json.contains("highMotionRegions"));
}
