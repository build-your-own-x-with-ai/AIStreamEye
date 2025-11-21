#include "video_analyzer/data_models.h"
#include <gtest/gtest.h>

using namespace video_analyzer;

// Test FrameType conversion
TEST(FrameTypeTest, ToStringConversion) {
    EXPECT_EQ(frameTypeToString(FrameType::I_FRAME), "I");
    EXPECT_EQ(frameTypeToString(FrameType::P_FRAME), "P");
    EXPECT_EQ(frameTypeToString(FrameType::B_FRAME), "B");
    EXPECT_EQ(frameTypeToString(FrameType::UNKNOWN), "UNKNOWN");
}

TEST(FrameTypeTest, FromStringConversion) {
    EXPECT_EQ(stringToFrameType("I"), FrameType::I_FRAME);
    EXPECT_EQ(stringToFrameType("P"), FrameType::P_FRAME);
    EXPECT_EQ(stringToFrameType("B"), FrameType::B_FRAME);
    EXPECT_EQ(stringToFrameType("UNKNOWN"), FrameType::UNKNOWN);
    EXPECT_EQ(stringToFrameType("invalid"), FrameType::UNKNOWN);
}

// Test FrameInfo
TEST(FrameInfoTest, JsonSerialization) {
    FrameInfo frame{
        .pts = 1000,
        .dts = 900,
        .type = FrameType::I_FRAME,
        .size = 50000,
        .qp = 25,
        .isKeyFrame = true,
        .timestamp = 0.033
    };
    
    auto json = frame.toJson();
    EXPECT_EQ(json["pts"], 1000);
    EXPECT_EQ(json["dts"], 900);
    EXPECT_EQ(json["type"], "I");
    EXPECT_EQ(json["size"], 50000);
    EXPECT_EQ(json["qp"], 25);
    EXPECT_EQ(json["isKeyFrame"], true);
    EXPECT_NEAR(json["timestamp"].get<double>(), 0.033, 0.001);
}

TEST(FrameInfoTest, CsvSerialization) {
    FrameInfo frame{
        .pts = 1000,
        .dts = 900,
        .type = FrameType::P_FRAME,
        .size = 10000,
        .qp = 30,
        .isKeyFrame = false,
        .timestamp = 0.066
    };
    
    std::string csv = frame.toCsv();
    EXPECT_NE(csv.find("1000"), std::string::npos);
    EXPECT_NE(csv.find("900"), std::string::npos);
    EXPECT_NE(csv.find("P"), std::string::npos);
    EXPECT_NE(csv.find("10000"), std::string::npos);
    EXPECT_NE(csv.find("30"), std::string::npos);
    EXPECT_NE(csv.find("false"), std::string::npos);
}

// Test StreamInfo
TEST(StreamInfoTest, JsonSerialization) {
    StreamInfo stream{
        .codecName = "h264",
        .width = 1920,
        .height = 1080,
        .frameRate = 30.0,
        .duration = 10.5,
        .bitrate = 5000000,
        .pixelFormat = "yuv420p",
        .streamIndex = 0
    };
    
    auto json = stream.toJson();
    EXPECT_EQ(json["codecName"], "h264");
    EXPECT_EQ(json["width"], 1920);
    EXPECT_EQ(json["height"], 1080);
    EXPECT_NEAR(json["frameRate"].get<double>(), 30.0, 0.01);
    EXPECT_NEAR(json["duration"].get<double>(), 10.5, 0.01);
    EXPECT_EQ(json["bitrate"], 5000000);
    EXPECT_EQ(json["pixelFormat"], "yuv420p");
    EXPECT_EQ(json["streamIndex"], 0);
}

TEST(StreamInfoTest, CsvSerialization) {
    StreamInfo stream{
        .codecName = "hevc",
        .width = 3840,
        .height = 2160,
        .frameRate = 60.0,
        .duration = 5.0,
        .bitrate = 10000000,
        .pixelFormat = "yuv420p10le",
        .streamIndex = 1
    };
    
    std::string csv = stream.toCsv();
    EXPECT_NE(csv.find("hevc"), std::string::npos);
    EXPECT_NE(csv.find("3840"), std::string::npos);
    EXPECT_NE(csv.find("2160"), std::string::npos);
    EXPECT_NE(csv.find("60"), std::string::npos);
}

// Test GOPInfo
TEST(GOPInfoTest, JsonSerialization) {
    GOPInfo gop{
        .gopIndex = 0,
        .startPts = 0,
        .endPts = 30000,
        .frameCount = 30,
        .iFrameCount = 1,
        .pFrameCount = 9,
        .bFrameCount = 20,
        .totalSize = 500000,
        .isOpenGOP = false
    };
    
    auto json = gop.toJson();
    EXPECT_EQ(json["gopIndex"], 0);
    EXPECT_EQ(json["startPts"], 0);
    EXPECT_EQ(json["endPts"], 30000);
    EXPECT_EQ(json["frameCount"], 30);
    EXPECT_EQ(json["iFrameCount"], 1);
    EXPECT_EQ(json["pFrameCount"], 9);
    EXPECT_EQ(json["bFrameCount"], 20);
    EXPECT_EQ(json["totalSize"], 500000);
    EXPECT_EQ(json["isOpenGOP"], false);
}

// Test BitrateInfo
TEST(BitrateInfoTest, JsonSerialization) {
    BitrateInfo bitrate{
        .timestamp = 1.5,
        .bitrate = 5000000.0
    };
    
    auto json = bitrate.toJson();
    EXPECT_NEAR(json["timestamp"].get<double>(), 1.5, 0.01);
    EXPECT_NEAR(json["bitrate"].get<double>(), 5000000.0, 1.0);
}

// Test BitrateStatistics
TEST(BitrateStatisticsTest, JsonSerialization) {
    BitrateStatistics stats{
        .averageBitrate = 5000000.0,
        .maxBitrate = 8000000.0,
        .minBitrate = 3000000.0,
        .stdDeviation = 500000.0,
        .timeSeriesData = {
            {0.0, 5000000.0},
            {1.0, 6000000.0},
            {2.0, 4000000.0}
        }
    };
    
    auto json = stats.toJson();
    EXPECT_NEAR(json["averageBitrate"].get<double>(), 5000000.0, 1.0);
    EXPECT_NEAR(json["maxBitrate"].get<double>(), 8000000.0, 1.0);
    EXPECT_NEAR(json["minBitrate"].get<double>(), 3000000.0, 1.0);
    EXPECT_NEAR(json["stdDeviation"].get<double>(), 500000.0, 1.0);
    EXPECT_EQ(json["timeSeriesData"].size(), 3);
}
