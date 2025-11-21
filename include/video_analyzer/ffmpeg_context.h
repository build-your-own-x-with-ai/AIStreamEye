#pragma once

#include <memory>

// Forward declarations for FFmpeg types
struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace video_analyzer {

/**
 * @brief RAII wrapper for FFmpeg resources
 * 
 * Manages the lifecycle of AVFormatContext and AVCodecContext.
 * Ensures proper cleanup even when exceptions are thrown.
 */
class FFmpegContext {
public:
    /**
     * @brief Construct a new FFmpegContext
     */
    FFmpegContext();
    
    /**
     * @brief Destroy the FFmpegContext and cleanup resources
     */
    ~FFmpegContext();
    
    // Disable copy
    FFmpegContext(const FFmpegContext&) = delete;
    FFmpegContext& operator=(const FFmpegContext&) = delete;
    
    // Enable move
    FFmpegContext(FFmpegContext&&) noexcept;
    FFmpegContext& operator=(FFmpegContext&&) noexcept;
    
    /**
     * @brief Get the format context
     * 
     * @return AVFormatContext* Pointer to format context (may be null)
     */
    AVFormatContext* getFormatContext() const;
    
    /**
     * @brief Get the codec context
     * 
     * @return AVCodecContext* Pointer to codec context (may be null)
     */
    AVCodecContext* getCodecContext() const;
    
    /**
     * @brief Set the format context (takes ownership)
     * 
     * @param ctx Format context to manage
     */
    void setFormatContext(AVFormatContext* ctx);
    
    /**
     * @brief Set the codec context (takes ownership)
     * 
     * @param ctx Codec context to manage
     */
    void setCodecContext(AVCodecContext* ctx);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * @brief RAII wrapper for AVPacket
 */
class PacketPtr {
public:
    PacketPtr();
    ~PacketPtr();
    
    PacketPtr(const PacketPtr&) = delete;
    PacketPtr& operator=(const PacketPtr&) = delete;
    
    PacketPtr(PacketPtr&&) noexcept;
    PacketPtr& operator=(PacketPtr&&) noexcept;
    
    AVPacket* get() const;
    AVPacket* operator->() const;
    
private:
    AVPacket* packet_;
};

/**
 * @brief RAII wrapper for AVFrame
 */
class FramePtr {
public:
    FramePtr();
    ~FramePtr();
    
    FramePtr(const FramePtr&) = delete;
    FramePtr& operator=(const FramePtr&) = delete;
    
    FramePtr(FramePtr&&) noexcept;
    FramePtr& operator=(FramePtr&&) noexcept;
    
    AVFrame* get() const;
    AVFrame* operator->() const;
    
private:
    AVFrame* frame_;
};

} // namespace video_analyzer
