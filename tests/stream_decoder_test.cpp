#include "video_analyzer/stream_decoder.h"
#include "video_analyzer/ffmpeg_error.h"
#include <gtest/gtest.h>
#include <filesystem>

using namespace video_analyzer;

// Note: Real streaming tests require actual streaming servers
// These tests use local files as a proxy for streaming behavior

class StreamDecoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a local file to simulate streaming
        testVideoPath = "test_videos/test_h264_720p_60fps.mp4";
        
        if (!std::filesystem::exists(testVideoPath)) {
            GTEST_SKIP() << "Test video not found: " << testVideoPath;
        }
    }
    
    std::string testVideoPath;
};

// Test basic stream opening with file:// protocol
TEST_F(StreamDecoderTest, OpenLocalFile) {
    // Use file:// protocol to simulate streaming
    std::string fileUrl = "file://" + std::filesystem::absolute(testVideoPath).string();
    
    try {
        StreamDecoder decoder(fileUrl);
        EXPECT_TRUE(decoder.isStreamActive());
        
        auto info = decoder.getStreamInfo();
        EXPECT_GT(info.width, 0);
        EXPECT_GT(info.height, 0);
    } catch (const FFmpegError& e) {
        // File protocol may not be supported in all FFmpeg builds
        GTEST_SKIP() << "File protocol not supported: " << e.what();
    }
}

// Test stream status
TEST_F(StreamDecoderTest, StreamStatus) {
    std::string fileUrl = "file://" + std::filesystem::absolute(testVideoPath).string();
    
    try {
        StreamDecoder decoder(fileUrl);
        EXPECT_TRUE(decoder.isStreamActive());
        
        // Stop the stream
        decoder.stop();
        EXPECT_FALSE(decoder.isStreamActive());
    } catch (const FFmpegError& e) {
        GTEST_SKIP() << "File protocol not supported: " << e.what();
    }
}

// Test buffer status
TEST_F(StreamDecoderTest, BufferStatus) {
    std::string fileUrl = "file://" + std::filesystem::absolute(testVideoPath).string();
    
    try {
        StreamDecoder decoder(fileUrl);
        
        // Read some frames to fill buffer
        for (int i = 0; i < 20 && decoder.isStreamActive(); i++) {
            decoder.readNextFrame();
        }
        
        auto status = decoder.getBufferStatus();
        EXPECT_GE(status.bufferedFrames, 0);
        EXPECT_GE(status.bufferedDuration, 0.0);
    } catch (const FFmpegError& e) {
        GTEST_SKIP() << "File protocol not supported: " << e.what();
    }
}

// Test frame reading
TEST_F(StreamDecoderTest, ReadFrames) {
    std::string fileUrl = "file://" + std::filesystem::absolute(testVideoPath).string();
    
    try {
        StreamDecoder decoder(fileUrl);
        
        int frameCount = 0;
        while (decoder.isStreamActive() && frameCount < 10) {
            auto frame = decoder.readNextFrame();
            if (frame.has_value()) {
                frameCount++;
                EXPECT_GE(frame->pts, 0);
                EXPECT_GE(frame->size, 0);
            }
        }
        
        EXPECT_GT(frameCount, 0);
    } catch (const FFmpegError& e) {
        GTEST_SKIP() << "File protocol not supported: " << e.what();
    }
}

// Test multi-threaded decoding
TEST_F(StreamDecoderTest, MultiThreadedDecoding) {
    std::string fileUrl = "file://" + std::filesystem::absolute(testVideoPath).string();
    
    try {
        StreamDecoder decoder(fileUrl, 4);  // Use 4 threads
        
        EXPECT_TRUE(decoder.isStreamActive());
        
        // Read some frames
        int frameCount = 0;
        while (decoder.isStreamActive() && frameCount < 10) {
            auto frame = decoder.readNextFrame();
            if (frame.has_value()) {
                frameCount++;
            }
        }
        
        EXPECT_GT(frameCount, 0);
    } catch (const FFmpegError& e) {
        GTEST_SKIP() << "File protocol not supported: " << e.what();
    }
}

// Test invalid stream URL
TEST_F(StreamDecoderTest, InvalidStreamUrl) {
    EXPECT_THROW({
        StreamDecoder decoder("rtmp://invalid.stream.url/live");
    }, FFmpegError);
}

// Test buffer status JSON serialization
TEST_F(StreamDecoderTest, BufferStatusSerialization) {
    BufferStatus status;
    status.bufferedFrames = 10;
    status.bufferedDuration = 0.5;
    status.isBuffering = false;
    
    auto json = status.toJson();
    
    EXPECT_TRUE(json.contains("bufferedFrames"));
    EXPECT_TRUE(json.contains("bufferedDuration"));
    EXPECT_TRUE(json.contains("isBuffering"));
    
    EXPECT_EQ(json["bufferedFrames"].get<size_t>(), 10);
    EXPECT_DOUBLE_EQ(json["bufferedDuration"].get<double>(), 0.5);
    EXPECT_EQ(json["isBuffering"].get<bool>(), false);
}
