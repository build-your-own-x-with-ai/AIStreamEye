#include "video_analyzer/frame_statistics.h"
#include <gtest/gtest.h>

using namespace video_analyzer;

TEST(FrameStatisticsTest, ComputeStatistics) {
    std::vector<FrameInfo> frames = {
        {1000, 900, FrameType::I_FRAME, 50000, 25, true, 0.033},
        {2000, 1900, FrameType::P_FRAME, 10000, 30, false, 0.066},
        {3000, 2900, FrameType::B_FRAME, 5000, 35, false, 0.099}
    };
    
    auto stats = FrameStatistics::compute(frames);
    
    EXPECT_EQ(stats.totalFrames, 3);
    EXPECT_EQ(stats.iFrames, 1);
    EXPECT_EQ(stats.pFrames, 1);
    EXPECT_EQ(stats.bFrames, 1);
    EXPECT_GT(stats.averageFrameSize, 0.0);
    EXPECT_EQ(stats.maxFrameSize, 50000);
    EXPECT_EQ(stats.minFrameSize, 5000);
}

TEST(FrameStatisticsTest, JsonSerialization) {
    FrameStatistics stats;
    stats.totalFrames = 100;
    stats.iFrames = 10;
    stats.pFrames = 30;
    stats.bFrames = 60;
    
    auto json = stats.toJson();
    EXPECT_EQ(json["totalFrames"], 100);
    EXPECT_EQ(json["iFrames"], 10);
}
