#pragma once

#include <string>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace video_analyzer {

/**
 * @brief Simple frame extractor for GUI display
 */
class FrameExtractor {
public:
    explicit FrameExtractor(const std::string& filepath);
    ~FrameExtractor();
    
    /**
     * @brief Seek to specific frame number
     * 
     * @param frame_number Frame number (0-based)
     * @return AVFrame* Frame data (owned by extractor, valid until next call)
     */
    AVFrame* getFrame(int frame_number);
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getFrameCount() const { return frame_count_; }
    
private:
    AVFormatContext* format_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    int video_stream_index_ = -1;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    
    int width_ = 0;
    int height_ = 0;
    int frame_count_ = 0;
    int current_frame_ = -1;
    
    bool decodeToFrame(int target_frame);
};

} // namespace video_analyzer
