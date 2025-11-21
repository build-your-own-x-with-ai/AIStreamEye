#include "video_analyzer/ffmpeg_context.h"
#include "video_analyzer/ffmpeg_error.h"
#include <gtest/gtest.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

using namespace video_analyzer;

// Test FFmpegError
TEST(FFmpegErrorTest, ConstructorAndGetters) {
    FFmpegError error(AVERROR(ENOMEM), "Out of memory");
    EXPECT_EQ(error.getErrorCode(), AVERROR(ENOMEM));
    EXPECT_STREQ(error.what(), "Out of memory");
}

// Test FFmpegContext
TEST(FFmpegContextTest, DefaultConstruction) {
    FFmpegContext ctx;
    EXPECT_EQ(ctx.getFormatContext(), nullptr);
    EXPECT_EQ(ctx.getCodecContext(), nullptr);
}

TEST(FFmpegContextTest, SetAndGetFormatContext) {
    FFmpegContext ctx;
    AVFormatContext* fmtCtx = avformat_alloc_context();
    ASSERT_NE(fmtCtx, nullptr);
    
    ctx.setFormatContext(fmtCtx);
    EXPECT_EQ(ctx.getFormatContext(), fmtCtx);
    
    // Context will be freed by FFmpegContext destructor
}

TEST(FFmpegContextTest, SetAndGetCodecContext) {
    FFmpegContext ctx;
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    ASSERT_NE(codec, nullptr);
    
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    ASSERT_NE(codecCtx, nullptr);
    
    ctx.setCodecContext(codecCtx);
    EXPECT_EQ(ctx.getCodecContext(), codecCtx);
    
    // Context will be freed by FFmpegContext destructor
}

TEST(FFmpegContextTest, MoveConstruction) {
    FFmpegContext ctx1;
    AVFormatContext* fmtCtx = avformat_alloc_context();
    ctx1.setFormatContext(fmtCtx);
    
    FFmpegContext ctx2(std::move(ctx1));
    EXPECT_EQ(ctx2.getFormatContext(), fmtCtx);
    EXPECT_EQ(ctx1.getFormatContext(), nullptr); // Moved from
}

TEST(FFmpegContextTest, MoveAssignment) {
    FFmpegContext ctx1;
    AVFormatContext* fmtCtx = avformat_alloc_context();
    ctx1.setFormatContext(fmtCtx);
    
    FFmpegContext ctx2;
    ctx2 = std::move(ctx1);
    EXPECT_EQ(ctx2.getFormatContext(), fmtCtx);
    EXPECT_EQ(ctx1.getFormatContext(), nullptr); // Moved from
}

TEST(FFmpegContextTest, ReplacementFreesOldContext) {
    FFmpegContext ctx;
    
    // Set first context
    AVFormatContext* fmtCtx1 = avformat_alloc_context();
    ctx.setFormatContext(fmtCtx1);
    
    // Set second context (should free first)
    AVFormatContext* fmtCtx2 = avformat_alloc_context();
    ctx.setFormatContext(fmtCtx2);
    
    EXPECT_EQ(ctx.getFormatContext(), fmtCtx2);
    // fmtCtx1 should have been freed automatically
}

// Test PacketPtr
TEST(PacketPtrTest, DefaultConstruction) {
    PacketPtr packet;
    EXPECT_NE(packet.get(), nullptr);
    EXPECT_NE(packet.operator->(), nullptr);
}

TEST(PacketPtrTest, MoveConstruction) {
    PacketPtr packet1;
    AVPacket* ptr = packet1.get();
    
    PacketPtr packet2(std::move(packet1));
    EXPECT_EQ(packet2.get(), ptr);
    EXPECT_EQ(packet1.get(), nullptr);
}

TEST(PacketPtrTest, MoveAssignment) {
    PacketPtr packet1;
    AVPacket* ptr = packet1.get();
    
    PacketPtr packet2;
    packet2 = std::move(packet1);
    EXPECT_EQ(packet2.get(), ptr);
    EXPECT_EQ(packet1.get(), nullptr);
}

// Test FramePtr
TEST(FramePtrTest, DefaultConstruction) {
    FramePtr frame;
    EXPECT_NE(frame.get(), nullptr);
    EXPECT_NE(frame.operator->(), nullptr);
}

TEST(FramePtrTest, MoveConstruction) {
    FramePtr frame1;
    AVFrame* ptr = frame1.get();
    
    FramePtr frame2(std::move(frame1));
    EXPECT_EQ(frame2.get(), ptr);
    EXPECT_EQ(frame1.get(), nullptr);
}

TEST(FramePtrTest, MoveAssignment) {
    FramePtr frame1;
    AVFrame* ptr = frame1.get();
    
    FramePtr frame2;
    frame2 = std::move(frame1);
    EXPECT_EQ(frame2.get(), ptr);
    EXPECT_EQ(frame1.get(), nullptr);
}
