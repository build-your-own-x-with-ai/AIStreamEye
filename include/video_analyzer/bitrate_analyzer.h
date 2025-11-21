#pragma once

#include "video_decoder.h"
#include "data_models.h"
#include <vector>

namespace video_analyzer {

/**
 * @brief Analyzer for video bitrate
 */
class BitrateAnalyzer {
public:
    /**
     * @brief Construct a BitrateAnalyzer
     * 
     * @param decoder VideoDecoder reference
     * @param windowSize Time window size in seconds (default: 1.0)
     */
    explicit BitrateAnalyzer(VideoDecoder& decoder, double windowSize = 1.0);
    
    /**
     * @brief Analyze bitrate
     * 
     * @return BitrateStatistics Bitrate statistics
     */
    BitrateStatistics analyze();
    
    /**
     * @brief Set the time window size
     * 
     * @param seconds Window size in seconds
     */
    void setWindowSize(double seconds);
    
private:
    VideoDecoder& decoder_;
    double windowSize_;
    
    double calculateInstantaneousBitrate(
        const std::vector<FrameInfo>& frames,
        size_t startIdx,
        size_t endIdx
    ) const;
};

} // namespace video_analyzer
