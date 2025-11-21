#pragma once

#include <stdexcept>
#include <string>

namespace video_analyzer {

/**
 * @brief Custom exception class for FFmpeg errors
 * 
 * Wraps FFmpeg error codes and provides human-readable error messages.
 */
class FFmpegError : public std::runtime_error {
public:
    /**
     * @brief Construct a new FFmpegError
     * 
     * @param errorCode FFmpeg error code (negative value)
     * @param message Human-readable error message
     */
    explicit FFmpegError(int errorCode, const std::string& message);
    
    /**
     * @brief Get the FFmpeg error code
     * 
     * @return int The error code
     */
    int getErrorCode() const noexcept;
    
private:
    int errorCode_;
};

} // namespace video_analyzer
