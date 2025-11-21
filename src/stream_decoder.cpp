#include "video_analyzer/stream_decoder.h"
#include "video_analyzer/ffmpeg_error.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
}

#include <cstring>
#include <thread>
#include <algorithm>
#include <deque>
#include <mutex>

namespace video_analyzer {

struct StreamDecoder::Impl {
    FFmpegContext context;
    PacketPtr packet;
    FramePtr frame;
    int videoStreamIndex = -1;
    std::atomic<bool> streamActive{true};
    std::string streamUrl;
    int threadCount = 0;
    int lastPacketSize = 0;
    
    // Buffer management
    std::deque<FrameInfo> frameBuffer;
    std::mutex bufferMutex;
    size_t maxBufferSize = 100;  // Maximum frames to buffer
    
    Impl() = default;
};

StreamDecoder::StreamDecoder(const std::string& streamUrl, int threadCount)
    : pImpl_(std::make_unique<Impl>()) {
    
    pImpl_->streamUrl = streamUrl;
    
    // Auto-detect or limit thread count
    if (threadCount == 0) {
        pImpl_->threadCount = std::thread::hardware_concurrency();
        if (pImpl_->threadCount == 0) {
            pImpl_->threadCount = 1;
        }
    } else {
        unsigned int maxThreads = std::thread::hardware_concurrency();
        if (maxThreads == 0) {
            maxThreads = 1;
        }
        pImpl_->threadCount = std::min(static_cast<unsigned int>(threadCount), maxThreads);
    }
    
    // Open input stream
    AVFormatContext* fmtCtx = nullptr;
    
    // Set options for streaming
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);  // Use TCP for RTSP
    av_dict_set(&opts, "timeout", "5000000", 0);     // 5 second timeout
    
    int ret = avformat_open_input(&fmtCtx, streamUrl.c_str(), nullptr, &opts);
    av_dict_free(&opts);
    
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw FFmpegError(ret, std::string("Failed to open stream: ") + errbuf);
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

StreamDecoder::~StreamDecoder() {
    if (pImpl_) {
        pImpl_->streamActive = false;
    }
}

StreamDecoder::StreamDecoder(StreamDecoder&&) noexcept = default;

StreamDecoder& StreamDecoder::operator=(StreamDecoder&&) noexcept = default;

StreamInfo StreamDecoder::getStreamInfo() const {
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
    
    // Duration may not be available for live streams
    if (stream->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
    } else {
        info.duration = 0.0;  // Live stream
    }
    
    info.bitrate = codecCtx->bit_rate;
    const char* pixFmtName = av_get_pix_fmt_name(codecCtx->pix_fmt);
    info.pixelFormat = pixFmtName ? pixFmtName : "unknown";
    info.streamIndex = pImpl_->videoStreamIndex;
    
    return info;
}

std::optional<FrameInfo> StreamDecoder::readNextFrame() {
    if (!pImpl_->streamActive) {
        return std::nullopt;
    }
    
    AVFormatContext* fmtCtx = pImpl_->context.getFormatContext();
    AVCodecContext* codecCtx = pImpl_->context.getCodecContext();
    AVPacket* packet = pImpl_->packet.get();
    AVFrame* frame = pImpl_->frame.get();
    
    // Try to receive a frame first
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
        
        // Add to buffer
        {
            std::lock_guard<std::mutex> lock(pImpl_->bufferMutex);
            pImpl_->frameBuffer.push_back(info);
            
            // Limit buffer size
            if (pImpl_->frameBuffer.size() > pImpl_->maxBufferSize) {
                pImpl_->frameBuffer.pop_front();
            }
        }
        
        av_frame_unref(frame);
        return info;
    } else if (ret == AVERROR(EAGAIN)) {
        // Decoder needs more input
    } else if (ret == AVERROR_EOF) {
        pImpl_->streamActive = false;
        return std::nullopt;
    } else {
        // Error receiving frame - stream may have issues
        pImpl_->streamActive = false;
        return std::nullopt;
    }
    
    // Read packet
    ret = av_read_frame(fmtCtx, packet);
    if (ret < 0) {
        if (ret == AVERROR_EOF || ret == AVERROR(ETIMEDOUT)) {
            pImpl_->streamActive = false;
        }
        return std::nullopt;
    }
    
    // Skip non-video packets
    if (packet->stream_index != pImpl_->videoStreamIndex) {
        av_packet_unref(packet);
        return std::nullopt;
    }
    
    // Save packet size
    pImpl_->lastPacketSize = packet->size;
    
    // Send packet to decoder
    ret = avcodec_send_packet(codecCtx, packet);
    av_packet_unref(packet);
    
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        pImpl_->streamActive = false;
        return std::nullopt;
    }
    
    // Try to receive frame again
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
        
        // Add to buffer
        {
            std::lock_guard<std::mutex> lock(pImpl_->bufferMutex);
            pImpl_->frameBuffer.push_back(info);
            
            if (pImpl_->frameBuffer.size() > pImpl_->maxBufferSize) {
                pImpl_->frameBuffer.pop_front();
            }
        }
        
        av_frame_unref(frame);
        return info;
    }
    
    return std::nullopt;
}

bool StreamDecoder::isStreamActive() const {
    return pImpl_->streamActive;
}

BufferStatus StreamDecoder::getBufferStatus() const {
    std::lock_guard<std::mutex> lock(pImpl_->bufferMutex);
    
    BufferStatus status;
    status.bufferedFrames = pImpl_->frameBuffer.size();
    status.isBuffering = pImpl_->frameBuffer.size() < 10;  // Buffering if less than 10 frames
    
    // Calculate buffered duration
    if (pImpl_->frameBuffer.size() >= 2) {
        double firstTs = pImpl_->frameBuffer.front().timestamp;
        double lastTs = pImpl_->frameBuffer.back().timestamp;
        status.bufferedDuration = lastTs - firstTs;
    } else {
        status.bufferedDuration = 0.0;
    }
    
    return status;
}

void StreamDecoder::stop() {
    pImpl_->streamActive = false;
}

FrameType StreamDecoder::detectFrameType(const AVFrame* frame) const {
    if (!frame) {
        return FrameType::UNKNOWN;
    }
    
    AVCodecContext* codecCtx = pImpl_->context.getCodecContext();
    
    // For AV1, handle Key Frame vs Inter Frame
    if (codecCtx->codec_id == AV_CODEC_ID_AV1) {
        if (frame->flags & AV_FRAME_FLAG_KEY) {
            return FrameType::I_FRAME;
        } else {
            return FrameType::P_FRAME;
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

int StreamDecoder::extractQP(const AVFrame* frame) const {
    if (!frame) {
        return 0;
    }
    
    AVCodecContext* codecCtx = pImpl_->context.getCodecContext();
    
    if (codecCtx->codec_id == AV_CODEC_ID_AV1) {
        return 128;  // Placeholder for AV1
    }
    
    return 0;  // Placeholder
}

} // namespace video_analyzer
