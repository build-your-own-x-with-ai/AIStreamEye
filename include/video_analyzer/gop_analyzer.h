#pragma once

#include "video_decoder.h"
#include "data_models.h"
#include <vector>

namespace video_analyzer {

/**
 * @brief Analyzer for GOP (Group of Pictures) structure
 */
class GOPAnalyzer {
public:
    /**
     * @brief Construct a GOPAnalyzer
     * 
     * @param decoder VideoDecoder reference
     */
    explicit GOPAnalyzer(VideoDecoder& decoder);
    
    /**
     * @brief Analyze GOP structure
     * 
     * @return std::vector<GOPInfo> List of GOP information
     */
    std::vector<GOPInfo> analyze();
    
    /**
     * @brief Get average GOP length
     */
    double getAverageGOPLength() const;
    
    /**
     * @brief Get maximum GOP length
     */
    int getMaxGOPLength() const;
    
    /**
     * @brief Get minimum GOP length
     */
    int getMinGOPLength() const;
    
private:
    VideoDecoder& decoder_;
    std::vector<GOPInfo> gops_;
    
    void detectGOPBoundaries(const std::vector<FrameInfo>& frames);
};

} // namespace video_analyzer
