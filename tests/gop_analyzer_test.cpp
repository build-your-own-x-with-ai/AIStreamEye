#include "video_analyzer/gop_analyzer.h"
#include "video_analyzer/video_decoder.h"
#include <gtest/gtest.h>

using namespace video_analyzer;

TEST(GOPAnalyzerTest, AnalyzeGOPStructure) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    GOPAnalyzer analyzer(decoder);
    
    auto gops = analyzer.analyze();
    
    EXPECT_GT(gops.size(), 0);
    
    // Each GOP should have at least one I-frame
    for (const auto& gop : gops) {
        EXPECT_GE(gop.iFrameCount, 1);
        EXPECT_EQ(gop.frameCount, gop.iFrameCount + gop.pFrameCount + gop.bFrameCount);
    }
}

TEST(GOPAnalyzerTest, GOPStatistics) {
    VideoDecoder decoder("../test_videos/test_h264_480p_24fps.mp4");
    GOPAnalyzer analyzer(decoder);
    
    analyzer.analyze();
    
    EXPECT_GT(analyzer.getAverageGOPLength(), 0.0);
    EXPECT_GT(analyzer.getMaxGOPLength(), 0);
    EXPECT_GT(analyzer.getMinGOPLength(), 0);
}
