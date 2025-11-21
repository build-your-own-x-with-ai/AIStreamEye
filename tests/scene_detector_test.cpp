#include <gtest/gtest.h>
#include "video_analyzer/scene_detector.h"
#include "video_analyzer/video_decoder.h"
#include <filesystem>

using namespace video_analyzer;

class SceneDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use test video with known scene cuts
        testVideoPath = "test_videos/test_h264_720p_60fps.mp4";
        
        // Check if test video exists
        if (!std::filesystem::exists(testVideoPath)) {
            GTEST_SKIP() << "Test video not found: " << testVideoPath;
        }
    }
    
    std::string testVideoPath;
};

// Test basic scene detection
TEST_F(SceneDetectorTest, BasicSceneDetection) {
    VideoDecoder decoder(testVideoPath);
    SceneDetector detector(decoder, 0.3);
    
    auto scenes = detector.analyze();
    
    // Should detect at least one scene
    EXPECT_GE(scenes.size(), 1);
    
    // Verify scene properties
    for (const auto& scene : scenes) {
        EXPECT_GE(scene.sceneIndex, 0);
        EXPECT_GE(scene.frameCount, 1);
        EXPECT_GE(scene.endFrameNumber, scene.startFrameNumber);
        EXPECT_GE(scene.endPts, scene.startPts);
        EXPECT_GE(scene.endTimestamp, scene.startTimestamp);
    }
}

// Test threshold configuration
TEST_F(SceneDetectorTest, ThresholdConfiguration) {
    VideoDecoder decoder1(testVideoPath);
    SceneDetector detector1(decoder1, 0.1);  // Low threshold - more sensitive
    
    VideoDecoder decoder2(testVideoPath);
    SceneDetector detector2(decoder2, 0.9);  // High threshold - less sensitive
    
    auto scenes1 = detector1.analyze();
    auto scenes2 = detector2.analyze();
    
    // Lower threshold should detect more or equal scenes
    EXPECT_GE(scenes1.size(), scenes2.size());
}

// Test threshold getter/setter
TEST_F(SceneDetectorTest, ThresholdGetterSetter) {
    VideoDecoder decoder(testVideoPath);
    SceneDetector detector(decoder, 0.3);
    
    EXPECT_DOUBLE_EQ(detector.getThreshold(), 0.3);
    
    detector.setThreshold(0.5);
    EXPECT_DOUBLE_EQ(detector.getThreshold(), 0.5);
}

// Test scene count
TEST_F(SceneDetectorTest, SceneCount) {
    VideoDecoder decoder(testVideoPath);
    SceneDetector detector(decoder, 0.3);
    
    auto scenes = detector.analyze();
    
    EXPECT_EQ(detector.getSceneCount(), scenes.size());
}

// Test average scene duration
TEST_F(SceneDetectorTest, AverageSceneDuration) {
    VideoDecoder decoder(testVideoPath);
    SceneDetector detector(decoder, 0.3);
    
    auto scenes = detector.analyze();
    double avgDuration = detector.getAverageSceneDuration();
    
    if (!scenes.empty()) {
        EXPECT_GT(avgDuration, 0.0);
        
        // Manually calculate average
        double totalDuration = 0.0;
        for (const auto& scene : scenes) {
            totalDuration += (scene.endTimestamp - scene.startTimestamp);
        }
        double expectedAvg = totalDuration / scenes.size();
        
        EXPECT_DOUBLE_EQ(avgDuration, expectedAvg);
    }
}

// Test scene boundary detection accuracy
TEST_F(SceneDetectorTest, SceneBoundaryAccuracy) {
    VideoDecoder decoder(testVideoPath);
    SceneDetector detector(decoder, 0.3);
    
    auto scenes = detector.analyze();
    
    // Verify scenes are contiguous (no gaps or overlaps)
    for (size_t i = 0; i < scenes.size() - 1; i++) {
        // Next scene should start right after current scene ends
        EXPECT_EQ(scenes[i + 1].startFrameNumber, scenes[i].endFrameNumber + 1);
    }
}

// Test scene info JSON serialization
TEST_F(SceneDetectorTest, SceneInfoJsonSerialization) {
    VideoDecoder decoder(testVideoPath);
    SceneDetector detector(decoder, 0.3);
    
    auto scenes = detector.analyze();
    
    if (!scenes.empty()) {
        auto json = scenes[0].toJson();
        
        EXPECT_TRUE(json.contains("sceneIndex"));
        EXPECT_TRUE(json.contains("startPts"));
        EXPECT_TRUE(json.contains("endPts"));
        EXPECT_TRUE(json.contains("startFrameNumber"));
        EXPECT_TRUE(json.contains("endFrameNumber"));
        EXPECT_TRUE(json.contains("startTimestamp"));
        EXPECT_TRUE(json.contains("endTimestamp"));
        EXPECT_TRUE(json.contains("frameCount"));
        EXPECT_TRUE(json.contains("averageBrightness"));
    }
}

// Test with different threshold values
TEST_F(SceneDetectorTest, DifferentThresholds) {
    std::vector<double> thresholds = {0.1, 0.3, 0.5, 0.7, 0.9};
    std::vector<size_t> sceneCounts;
    
    for (double threshold : thresholds) {
        VideoDecoder decoder(testVideoPath);
        SceneDetector detector(decoder, threshold);
        auto scenes = detector.analyze();
        sceneCounts.push_back(scenes.size());
    }
    
    // Generally, higher thresholds should result in fewer or equal scenes
    // (though not strictly monotonic due to algorithm specifics)
    EXPECT_GE(sceneCounts.front(), sceneCounts.back());
}

// Test empty video handling
TEST_F(SceneDetectorTest, EmptyVideoHandling) {
    // This test would require a zero-frame video, which is hard to create
    // Skip for now, but in production you'd want to test edge cases
    GTEST_SKIP() << "Empty video test not implemented";
}
