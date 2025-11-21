#include "video_analyzer/frame_renderer.h"
#include <stdexcept>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace video_analyzer {

FrameRenderer::FrameRenderer(int width, int height)
    : width_(width), height_(height) {
    
    // Allocate RGB frame
    rgb_frame_ = av_frame_alloc();
    if (!rgb_frame_) {
        throw std::runtime_error("Failed to allocate RGB frame");
    }
    
    rgb_frame_->format = AV_PIX_FMT_RGB24;
    rgb_frame_->width = width;
    rgb_frame_->height = height;
    
    int ret = av_frame_get_buffer(rgb_frame_, 0);
    if (ret < 0) {
        av_frame_free(&rgb_frame_);
        throw std::runtime_error("Failed to allocate RGB frame buffer");
    }
}

FrameRenderer::~FrameRenderer() {
    if (sws_context_) {
        sws_freeContext(sws_context_);
    }
    if (rgb_frame_) {
        av_frame_free(&rgb_frame_);
    }
}

bool FrameRenderer::convertFrameToRGB(AVFrame* frame, uint8_t* rgb_buffer) {
    if (!frame || !rgb_buffer) {
        return false;
    }
    
    // Create swscale context if needed
    if (!sws_context_) {
        sws_context_ = sws_getContext(
            frame->width, frame->height, (AVPixelFormat)frame->format,
            width_, height_, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
        
        if (!sws_context_) {
            return false;
        }
    }
    
    // Convert frame to RGB
    uint8_t* dest[1] = { rgb_buffer };
    int dest_linesize[1] = { width_ * 3 };
    
    sws_scale(sws_context_,
              frame->data, frame->linesize, 0, frame->height,
              dest, dest_linesize);
    
    return true;
}

} // namespace video_analyzer
