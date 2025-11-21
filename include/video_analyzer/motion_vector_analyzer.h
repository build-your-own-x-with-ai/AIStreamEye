#pragma once

#include "video_decoder.h"
#include "data_models.h"
#include "gop_analyzer.h"
#include <vector>
#include <memory>

namespace video_analyzer {

/**
 * @brief Motion vector analyzer
 * 
 * Analyzes motion vectors from video frames to compute statistics
 * and identify motion patterns.
 */
class MotionVectorAnalyzer {
public:
    /**
     * @brief Construct a MotionVectorAnalyzer
     * 
     * @param decoder Reference to VideoDecoder
     */
    explicit MotionVectorAnalyzer(VideoDecoder& decoder);
    
    /**
     * @brief Extract motion vectors from all frames
     * 
     * @return std::vector<MotionVectorData> Motion vector data for all frames
     */
    std::vector<MotionVectorData> extractMotionVectors();
    
    /**
     * @brief Compute statistics from motion vector data
     * 
     * @param mvData Motion vector data
     * @return MotionStatistics Computed statistics
     */
    MotionStatistics computeStatistics(const std::vector<MotionVectorData>& mvData);
    
    /**
     * @brief Aggregate motion vectors by frame
     * 
     * @param mvData Motion vector data
     * @return std::vector<MotionStatistics> Statistics for each frame
     */
    std::vector<MotionStatistics> aggregateByFrame(const std::vector<MotionVectorData>& mvData);
    
    /**
     * @brief Aggregate motion vectors by GOP
     * 
     * @param mvData Motion vector data
     * @param gops GOP information
     * @return std::vector<MotionStatistics> Statistics for each GOP
     */
    std::vector<MotionStatistics> aggregateByGOP(
        const std::vector<MotionVectorData>& mvData,
        const std::vector<GOPInfo>& gops
    );
    
private:
    VideoDecoder& decoder_;
    
    /**
     * @brief Check if a motion vector represents a static region
     * 
     * @param mv Motion vector
     * @param threshold Magnitude threshold (default 1.0)
     * @return true if static
     */
    bool isStaticRegion(const MotionVector& mv, double threshold = 1.0) const;
    
    /**
     * @brief Check if a motion vector represents a high motion region
     * 
     * @param mv Motion vector
     * @param threshold Magnitude threshold (default 10.0)
     * @return true if high motion
     */
    bool isHighMotionRegion(const MotionVector& mv, double threshold = 10.0) const;
    
    /**
     * @brief Compute statistics for a single set of motion vectors
     * 
     * @param vectors Motion vectors
     * @return MotionStatistics Computed statistics
     */
    MotionStatistics computeStatisticsForVectors(const std::vector<MotionVector>& vectors);
};

} // namespace video_analyzer
