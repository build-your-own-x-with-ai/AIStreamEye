#pragma once

#include "video_analyzer/video_decoder.h"
#include "video_analyzer/gop_analyzer.h"
#include "video_analyzer/frame_statistics.h"
#include "video_analyzer/data_models.h"
#include <string>
#include <vector>
#include <memory>

namespace video_analyzer {

/**
 * @brief High-level video analyzer for GUI
 * 
 * Wraps video decoding and analysis functionality
 */
class VideoAnalyzer {
public:
    VideoAnalyzer() = default;
    ~VideoAnalyzer() = default;
    
    /**
     * @brief Analyze a video file
     * 
     * @param filepath Path to video file
     */
    void analyze(const std::string& filepath);
    
    /**
     * @brief Get stream information
     * 
     * @return const StreamInfo& Stream information
     */
    const StreamInfo& getStreamInfo() const { return stream_info_; }
    
    /**
     * @brief Get all frames
     * 
     * @return const std::vector<FrameInfo>& Frame list
     */
    const std::vector<FrameInfo>& getFrames() const { return frames_; }
    
    /**
     * @brief Get all GOPs
     * 
     * @return const std::vector<GOPInfo>& GOP list
     */
    const std::vector<GOPInfo>& getGOPs() const { return gops_; }
    
    /**
     * @brief Get frame statistics
     * 
     * @return const FrameStatistics& Frame statistics
     */
    const FrameStatistics& getFrameStatistics() const { return frame_stats_; }
    
    /**
     * @brief Detect duplicate frames
     * 
     * Analyzes frames and marks duplicates based on size and QP similarity
     * 
     * @param size_tolerance Size tolerance in percentage (default 1.0%)
     * @param require_same_qp Require same QP value (default true)
     * @param require_same_type Require same frame type (default true)
     */
    void detectDuplicateFrames(float size_tolerance = 1.0f, 
                               bool require_same_qp = true,
                               bool require_same_type = true);

private:
    StreamInfo stream_info_;
    std::vector<FrameInfo> frames_;
    std::vector<GOPInfo> gops_;
    FrameStatistics frame_stats_;
};

} // namespace video_analyzer
