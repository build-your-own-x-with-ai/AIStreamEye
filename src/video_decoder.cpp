#include "video_analyzer/video_decoder.h"
#include "video_analyzer/ffmpeg_error.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/motion_vector.h>
}

#include <cstring>
#include <thread>
#include <algorithm>

namespace video_analyzer {

struct VideoDecoder::Impl {
    FFmpegContext context;
    PacketPtr packet;
    FramePtr frame;
    int videoStreamIndex = -1;
    bool endOfStream = false;
    std::string filePath;
    int threadCount = 0;
    int lastPacketSize = 0;  // Track last packet size for frame info
    FramePtr lastDecodedFrame;  // Keep last frame for motion vector extraction
    
    Impl() = default;
};

VideoDecoder::VideoDecoder(const std::string& filePath, int threadCount)
    : pImpl_(std::make_unique<Impl>()) {
    
    pImpl_->filePath = filePath;
    
    // Auto-detect or limit thread count to hardware cores
    if (threadCount == 0) {
        pImpl_->threadCount = std::thread::hardware_concurrency();
        if (pImpl_->threadCount == 0) {
            pImpl_->threadCount = 1; // Fallback if detection fails
        }
    } else {
        // Limit to hardware cores
        unsigned int maxThreads = std::thread::hardware_concurrency();
        if (maxThreads == 0) {
            maxThreads = 1;
        }
        pImpl_->threadCount = std::min(static_cast<unsigned int>(threadCount), maxThreads);
    }
    
    // Open input file
    AVFormatContext* fmtCtx = nullptr;
    int ret = avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw FFmpegError(ret, std::string("Failed to open file: ") + errbuf);
    }
    pImpl_->context.setFormatContext(fmtCtx);
    
    // Retrieve stream information
    ret = avformat_find_stream_info(fmtCtx, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw FFmpegError(ret, std::string("Failed to find stream info: ") + errbuf);
    }
    
    // Find the first video stream
    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            pImpl_->videoStreamIndex = i;
            break;
        }
    }
    
    if (pImpl_->videoStreamIndex == -1) {
        throw FFmpegError(AVERROR_STREAM_NOT_FOUND, "No video stream found");
    }
    
    // Get codec parameters
    AVCodecParameters* codecpar = fmtCtx->streams[pImpl_->videoStreamIndex]->codecpar;
    
    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        throw FFmpegError(AVERROR_DECODER_NOT_FOUND, "Codec not found");
    }
    
    // Allocate codec context
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        throw FFmpegError(AVERROR(ENOMEM), "Failed to allocate codec context");
    }
    
    // Copy codec parameters to context
    ret = avcodec_parameters_to_context(codecCtx, codecpar);
    if (ret < 0) {
        avcodec_free_context(&codecCtx);
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw FFmpegError(ret, std::string("Failed to copy codec parameters: ") + errbuf);
    }
    
    // Configure multi-threading
    codecCtx->thread_count = pImpl_->threadCount;
    // Use frame-level threading for better frame order preservation
    // FF_THREAD_FRAME ensures frames are output in presentation order
    codecCtx->thread_type = FF_THREAD_FRAME;
    
    // Open codec
    ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        avcodec_free_context(&codecCtx);
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw FFmpegError(ret, std::string("Failed to open codec: ") + errbuf);
    }
    
    pImpl_->context.setCodecContext(codecCtx);
}

VideoDecoder::~VideoDecoder() = default;

VideoDecoder::VideoDecoder(VideoDecoder&&) noexcept = default;

VideoDecoder& VideoDecoder::operator=(VideoDecoder&&) noexcept = default;

StreamInfo VideoDecoder::getStreamInfo() const {
    AVFormatContext* fmtCtx = pImpl_->context.getFormatContext();
    AVCodecContext* codecCtx = pImpl_->context.getCodecContext();
    AVStream* stream = fmtCtx->streams[pImpl_->videoStreamIndex];
    
    StreamInfo info;
    info.codecName = avcodec_get_name(codecCtx->codec_id);
    info.width = codecCtx->width;
    info.height = codecCtx->height;
    
    // Calculate frame rate
    if (stream->avg_frame_rate.den != 0) {
        info.frameRate = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
    } else {
        info.frameRate = 0.0;
    }
    
    // Calculate duration
    if (stream->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
    } else if (fmtCtx->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(fmtCtx->duration) / AV_TIME_BASE;
    } else {
        info.duration = 0.0;
    }
    
    info.bitrate = codecCtx->bit_rate;
    const char* pixFmtName = av_get_pix_fmt_name(codecCtx->pix_fmt);
    info.pixelFormat = pixFmtName ? pixFmtName : "unknown";
    info.streamIndex = pImpl_->videoStreamIndex;
    
    // Extract AV1-specific tile information
    if (codecCtx->codec_id == AV_CODEC_ID_AV1) {
        AV1TileInfo tileInfo;
        
        // Try to get tile information from codec parameters
        AVCodecParameters* codecpar = stream->codecpar;
        if (codecpar->extradata_size > 0) {
            // AV1 tile info is typically in the sequence header
            // For simplicity, we'll set default values
            // In a full implementation, you would parse the AV1 sequence header
            tileInfo.tileColumns = 1;
            tileInfo.tileRows = 1;
        } else {
            tileInfo.tileColumns = 1;
            tileInfo.tileRows = 1;
        }
        
        info.av1TileInfo = tileInfo;
    }
    
    return info;
}

std::optional<FrameInfo> VideoDecoder::readNextFrame() {
    if (pImpl_->endOfStream) {
        return std::nullopt;
    }
    
    AVFormatContext* fmtCtx = pImpl_->context.getFormatContext();
    AVCodecContext* codecCtx = pImpl_->context.getCodecContext();
    AVPacket* packet = pImpl_->packet.get();
    AVFrame* frame = pImpl_->frame.get();
    
    while (true) {
        // Try to receive a frame first (in case decoder has buffered frames)
        int ret = avcodec_receive_frame(codecCtx, frame);
        
        if (ret == 0) {
            // Successfully received a frame
            FrameInfo info;
            info.pts = frame->pts;
            info.dts = frame->pkt_dts;
            info.type = detectFrameType(frame);
            info.size = pImpl_->lastPacketSize;
            info.qp = extractQP(frame);
            info.isKeyFrame = (frame->flags & AV_FRAME_FLAG_KEY) != 0;
            
            AVStream* stream = fmtCtx->streams[pImpl_->videoStreamIndex];
            info.timestamp = frame->pts * av_q2d(stream->time_base);
            
            // Store a reference to the frame for motion vector extraction
            if (!pImpl_->lastDecodedFrame.get()) {
                pImpl_->lastDecodedFrame = FramePtr();
            }
            av_frame_unref(pImpl_->lastDecodedFrame.get());
            av_frame_ref(pImpl_->lastDecodedFrame.get(), frame);
            
            av_frame_unref(frame);
            return info;
        } else if (ret == AVERROR(EAGAIN)) {
            // Decoder needs more input, read a packet
        } else if (ret == AVERROR_EOF) {
            pImpl_->endOfStream = true;
            return std::nullopt;
        } else {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            throw FFmpegError(ret, std::string("Error receiving frame: ") + errbuf);
        }
        
        // Read packet
        ret = av_read_frame(fmtCtx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                // Send NULL packet to flush decoder
                avcodec_send_packet(codecCtx, nullptr);
                pImpl_->endOfStream = true;
                
                // Try to receive remaining frames
                ret = avcodec_receive_frame(codecCtx, frame);
                if (ret == 0) {
                    FrameInfo info;
                    info.pts = frame->pts;
                    info.dts = frame->pkt_dts;
                    info.type = detectFrameType(frame);
                    info.size = pImpl_->lastPacketSize;
                    info.qp = extractQP(frame);
                    info.isKeyFrame = (frame->flags & AV_FRAME_FLAG_KEY) != 0;
                    
                    AVStream* stream = fmtCtx->streams[pImpl_->videoStreamIndex];
                    info.timestamp = frame->pts * av_q2d(stream->time_base);
                    
                    // Store a reference to the frame for motion vector extraction
                    if (!pImpl_->lastDecodedFrame.get()) {
                        pImpl_->lastDecodedFrame = FramePtr();
                    }
                    av_frame_unref(pImpl_->lastDecodedFrame.get());
                    av_frame_ref(pImpl_->lastDecodedFrame.get(), frame);
                    
                    av_frame_unref(frame);
                    return info;
                }
                return std::nullopt;
            }
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            throw FFmpegError(ret, std::string("Error reading frame: ") + errbuf);
        }
        
        // Skip non-video packets
        if (packet->stream_index != pImpl_->videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }
        
        // Save packet size
        pImpl_->lastPacketSize = packet->size;
        
        // Send packet to decoder
        ret = avcodec_send_packet(codecCtx, packet);
        av_packet_unref(packet);
        
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            throw FFmpegError(ret, std::string("Error sending packet: ") + errbuf);
        }
        
        // Loop back to try receiving a frame
    }
}

void VideoDecoder::seekToTime(double seconds) {
    AVFormatContext* fmtCtx = pImpl_->context.getFormatContext();
    AVStream* stream = fmtCtx->streams[pImpl_->videoStreamIndex];
    
    int64_t timestamp = static_cast<int64_t>(seconds / av_q2d(stream->time_base));
    
    int ret = av_seek_frame(fmtCtx, pImpl_->videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw FFmpegError(ret, std::string("Error seeking: ") + errbuf);
    }
    
    avcodec_flush_buffers(pImpl_->context.getCodecContext());
    pImpl_->endOfStream = false;
}

void VideoDecoder::reset() {
    seekToTime(0.0);
}

bool VideoDecoder::hasMoreFrames() const {
    return !pImpl_->endOfStream;
}

FrameType VideoDecoder::detectFrameType(const AVFrame* frame) const {
    if (!frame) {
        return FrameType::UNKNOWN;
    }
    
    AVCodecContext* codecCtx = pImpl_->context.getCodecContext();
    
    // For AV1, handle Key Frame vs Inter Frame
    if (codecCtx->codec_id == AV_CODEC_ID_AV1) {
        // AV1 uses key_frame flag to distinguish Key Frames from Inter Frames
        if (frame->flags & AV_FRAME_FLAG_KEY) {
            return FrameType::I_FRAME;  // Key Frame maps to I_FRAME
        } else {
            return FrameType::P_FRAME;  // Inter Frame maps to P_FRAME
        }
    }
    
    // For other codecs, use pict_type
    switch (frame->pict_type) {
        case AV_PICTURE_TYPE_I:
            return FrameType::I_FRAME;
        case AV_PICTURE_TYPE_P:
            return FrameType::P_FRAME;
        case AV_PICTURE_TYPE_B:
            return FrameType::B_FRAME;
        default:
            return FrameType::UNKNOWN;
    }
}

int VideoDecoder::extractQP(const AVFrame* frame) const {
    if (!frame) {
        return 0;
    }
    
    AVCodecContext* codecCtx = pImpl_->context.getCodecContext();
    
    // QP extraction is codec-specific and complex
    // For AV1, we return a placeholder value in the valid range (0-255)
    // For H.264/H.265, we return a placeholder value in the valid range (0-51)
    
    if (codecCtx->codec_id == AV_CODEC_ID_AV1) {
        // AV1 QP range is 0-255
        // Return a mid-range placeholder value
        return 128;
    }
    
    // For other codecs (H.264/H.265), return 0 as placeholder
    // In a full implementation, you would:
    // 1. Access frame side data for QP table
    // 2. Calculate average QP from the table
    // 3. Validate against codec-specific range
    return 0;
}

std::optional<MotionVectorData> VideoDecoder::getMotionVectors() const {
    if (!pImpl_->lastDecodedFrame.get()) {
        return std::nullopt;
    }
    
    return extractMotionVectors(pImpl_->lastDecodedFrame.get());
}

MotionVectorData VideoDecoder::extractMotionVectors(const AVFrame* frame) const {
    MotionVectorData mvData;
    mvData.pts = frame->pts;
    
    if (!frame) {
        return mvData;
    }
    
    // Extract motion vectors from frame side data
    AVFrameSideData* sd = av_frame_get_side_data(frame, AV_FRAME_DATA_MOTION_VECTORS);
    if (!sd) {
        // No motion vectors available (e.g., I-frames don't have motion vectors)
        return mvData;
    }
    
    // Parse motion vector data
    const AVMotionVector* mvs = reinterpret_cast<const AVMotionVector*>(sd->data);
    int mv_count = sd->size / sizeof(AVMotionVector);
    
    for (int i = 0; i < mv_count; i++) {
        const AVMotionVector* mv = &mvs[i];
        
        MotionVector vec;
        vec.srcX = mv->src_x;
        vec.srcY = mv->src_y;
        vec.dstX = mv->dst_x;
        vec.dstY = mv->dst_y;
        vec.motionX = mv->motion_x;
        vec.motionY = mv->motion_y;
        
        // Calculate magnitude and direction
        vec.magnitude = std::sqrt(static_cast<float>(mv->motion_x * mv->motion_x + 
                                                      mv->motion_y * mv->motion_y));
        vec.direction = std::atan2(static_cast<float>(mv->motion_y), 
                                   static_cast<float>(mv->motion_x));
        
        mvData.vectors.push_back(vec);
    }
    
    return mvData;
}

} // namespace video_analyzer
