#pragma once

#include "data_models.h"
#include <vector>

namespace video_analyzer {

/**
 * @brief Frame statistics
 */
struct FrameStatistics {
    int totalFrames = 0;
    int iFrames = 0;
    int pFrames = 0;
    int bFrames = 0;
    double averageFrameSize = 0.0;
    int maxFrameSize = 0;
    int minFrameSize = 0;
    double averageQP = 0.0;
    
    nlohmann::json toJson() const;
    
    /**
     * @brief Compute statistics from frame sequence
     */
    static FrameStatistics compute(const std::vector<FrameInfo>& frames);
};

} // namespace video_analyzer
