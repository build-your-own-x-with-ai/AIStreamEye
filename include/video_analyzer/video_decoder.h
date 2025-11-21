#pragma once

#include "ffmpeg_context.h"
#include "data_models.h"
#include <string>
#include <optional>
#include <memory>

namespace video_analyzer {

/**
 * @brief Core video decoder class
 * 
 * Responsible for opening video files and decoding frames.
 */
class VideoDecoder {
public:
    /**
     * @brief Construct a VideoDecoder and open the video file
     * 
     * @param filePath Path to the video file
     * @param threadCount Number of threads for decoding (0 = auto-detect, limited to hardware cores)
     * @throws FFmpegError if file cannot be opened or decoded
     */
    explicit VideoDecoder(const std::string& filePath, int threadCount = 0);
    
    /**
     * @brief Destructor
     */
    ~VideoDecoder();
    
    // Disable copy
    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;
    
    // Enable move
    VideoDecoder(VideoDecoder&&) noexcept;
    VideoDecoder& operator=(VideoDecoder&&) noexcept;
    
    /**
     * @brief Get stream information
     * 
     * @return StreamInfo Video stream metadata
     */
    StreamInfo getStreamInfo() const;
    
    /**
     * @brief Read the next frame
     * 
     * @return std::optional<FrameInfo> Frame information, or nullopt if end of stream
     */
    std::optional<FrameInfo> readNextFrame();
    
    /**
     * @brief Seek to a specific time
     * 
     * @param seconds Time in seconds
     */
    void seekToTime(double seconds);
    
    /**
     * @brief Reset to the beginning of the stream
     */
    void reset();
    
    /**
     * @brief Check if there are more frames to read
     * 
     * @return true if more frames available
     */
    bool hasMoreFrames() const;
    
    /**
     * @brief Get motion vectors from the last decoded frame (if available)
     * 
     * @return std::optional<MotionVectorData> Motion vector data, or nullopt if not available
     */
    std::optional<MotionVectorData> getMotionVectors() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
    
    FrameType detectFrameType(const struct AVFrame* frame) const;
    int extractQP(const struct AVFrame* frame) const;
    MotionVectorData extractMotionVectors(const struct AVFrame* frame) const;
};

} // namespace video_analyzer
