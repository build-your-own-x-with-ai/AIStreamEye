#include "video_analyzer/ffmpeg_context.h"
#include "video_analyzer/ffmpeg_error.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
}

namespace video_analyzer {

// FFmpegError implementation
FFmpegError::FFmpegError(int errorCode, const std::string& message)
    : std::runtime_error(message), errorCode_(errorCode) {}

int FFmpegError::getErrorCode() const noexcept {
    return errorCode_;
}

// FFmpegContext::Impl
struct FFmpegContext::Impl {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    
    ~Impl() {
        if (codecContext) {
            avcodec_free_context(&codecContext);
        }
        if (formatContext) {
            avformat_close_input(&formatContext);
        }
    }
};

// FFmpegContext implementation
FFmpegContext::FFmpegContext() : pImpl_(std::make_unique<Impl>()) {}

FFmpegContext::~FFmpegContext() = default;

FFmpegContext::FFmpegContext(FFmpegContext&& other) noexcept 
    : pImpl_(std::move(other.pImpl_)) {
    // Ensure other has a valid pImpl after move
    other.pImpl_ = std::make_unique<Impl>();
}

FFmpegContext& FFmpegContext::operator=(FFmpegContext&& other) noexcept {
    if (this != &other) {
        pImpl_ = std::move(other.pImpl_);
        // Ensure other has a valid pImpl after move
        other.pImpl_ = std::make_unique<Impl>();
    }
    return *this;
}

AVFormatContext* FFmpegContext::getFormatContext() const {
    return pImpl_->formatContext;
}

AVCodecContext* FFmpegContext::getCodecContext() const {
    return pImpl_->codecContext;
}

void FFmpegContext::setFormatContext(AVFormatContext* ctx) {
    if (pImpl_->formatContext) {
        avformat_close_input(&pImpl_->formatContext);
    }
    pImpl_->formatContext = ctx;
}

void FFmpegContext::setCodecContext(AVCodecContext* ctx) {
    if (pImpl_->codecContext) {
        avcodec_free_context(&pImpl_->codecContext);
    }
    pImpl_->codecContext = ctx;
}

// PacketPtr implementation
PacketPtr::PacketPtr() : packet_(av_packet_alloc()) {
    if (!packet_) {
        throw FFmpegError(AVERROR(ENOMEM), "Failed to allocate AVPacket");
    }
}

PacketPtr::~PacketPtr() {
    if (packet_) {
        av_packet_free(&packet_);
    }
}

PacketPtr::PacketPtr(PacketPtr&& other) noexcept : packet_(other.packet_) {
    other.packet_ = nullptr;
}

PacketPtr& PacketPtr::operator=(PacketPtr&& other) noexcept {
    if (this != &other) {
        if (packet_) {
            av_packet_free(&packet_);
        }
        packet_ = other.packet_;
        other.packet_ = nullptr;
    }
    return *this;
}

AVPacket* PacketPtr::get() const {
    return packet_;
}

AVPacket* PacketPtr::operator->() const {
    return packet_;
}

// FramePtr implementation
FramePtr::FramePtr() : frame_(av_frame_alloc()) {
    if (!frame_) {
        throw FFmpegError(AVERROR(ENOMEM), "Failed to allocate AVFrame");
    }
}

FramePtr::~FramePtr() {
    if (frame_) {
        av_frame_free(&frame_);
    }
}

FramePtr::FramePtr(FramePtr&& other) noexcept : frame_(other.frame_) {
    other.frame_ = nullptr;
}

FramePtr& FramePtr::operator=(FramePtr&& other) noexcept {
    if (this != &other) {
        if (frame_) {
            av_frame_free(&frame_);
        }
        frame_ = other.frame_;
        other.frame_ = nullptr;
    }
    return *this;
}

AVFrame* FramePtr::get() const {
    return frame_;
}

AVFrame* FramePtr::operator->() const {
    return frame_;
}

} // namespace video_analyzer
