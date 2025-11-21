#pragma once

#include "ffmpeg_context.h"
#include "data_models.h"
#include <string>
#include <optional>
#include <memory>
#include <atomic>

namespace video_analyzer {

/**
 * @brief Real-time stream decoder
 * 
 * Supports network streaming protocols (RTMP, HLS, RTSP)
 */
class StreamDecoder {
public:
    /**
     * @brief Construct a StreamDecoder and open the stream
     * 
     * @param streamUrl URL of the stream (rtmp://, http://, rtsp://)
     * @param threadCount Number of threads for decoding (0 = auto-detect)
     * @throws FFmpegError if stream cannot be opened
     */
    explicit StreamDecoder(const std::string& streamUrl, int threadCount = 0);
    
    /**
     * @brief Destructor
     */
    ~StreamDecoder();
    
    // Disable copy
    StreamDecoder(const StreamDecoder&) = delete;
    StreamDecoder& operator=(const StreamDecoder&) = delete;
    
    // Enable move
    StreamDecoder(StreamDecoder&&) noexcept;
    StreamDecoder& operator=(StreamDecoder&&) noexcept;
    
    /**
     * @brief Get stream information
     * 
     * @return StreamInfo Video stream metadata
     */
    StreamInfo getStreamInfo() const;
    
    /**
     * @brief Read the next frame (non-blocking)
     * 
     * @return std::optional<FrameInfo> Frame information, or nullopt if no frame available
     */
    std::optional<FrameInfo> readNextFrame();
    
    /**
     * @brief Check if stream is still active
     * 
     * @return true if stream is active
     */
    bool isStreamActive() const;
    
    /**
     * @brief Get buffer status
     * 
     * @return BufferStatus Current buffer status
     */
    BufferStatus getBufferStatus() const;
    
    /**
     * @brief Stop the stream
     */
    void stop();
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
    
    FrameType detectFrameType(const struct AVFrame* frame) const;
    int extractQP(const struct AVFrame* frame) const;
};

} // namespace video_analyzer
