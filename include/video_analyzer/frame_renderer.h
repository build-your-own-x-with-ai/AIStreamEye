#pragma once

#include <memory>
#include <vector>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace video_analyzer {

/**
 * @brief Helper class to render video frames to RGB buffer
 */
class FrameRenderer {
public:
    FrameRenderer(int width, int height);
    ~FrameRenderer();
    
    /**
     * @brief Convert AVFrame to RGB buffer
     * 
     * @param frame AVFrame to convert
     * @param rgb_buffer Output RGB buffer (must be width * height * 3 bytes)
     * @return true if successful
     */
    bool convertFrameToRGB(AVFrame* frame, uint8_t* rgb_buffer);
    
    /**
     * @brief Get RGB buffer size
     */
    size_t getRGBBufferSize() const { return width_ * height_ * 3; }
    
private:
    int width_;
    int height_;
    SwsContext* sws_context_ = nullptr;
    AVFrame* rgb_frame_ = nullptr;
};

} // namespace video_analyzer
