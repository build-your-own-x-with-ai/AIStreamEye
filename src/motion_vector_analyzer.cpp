#include "video_analyzer/motion_vector_analyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace video_analyzer {

MotionVectorAnalyzer::MotionVectorAnalyzer(VideoDecoder& decoder)
    : decoder_(decoder) {
}

std::vector<MotionVectorData> MotionVectorAnalyzer::extractMotionVectors() {
    std::vector<MotionVectorData> result;
    
    // Reset decoder to start
    decoder_.reset();
    
    // Read all frames and extract motion vectors
    while (auto frameInfo = decoder_.readNextFrame()) {
        // Get motion vectors for this frame
        auto mvData = decoder_.getMotionVectors();
        if (mvData.has_value()) {
            result.push_back(mvData.value());
        }
    }
    
    return result;
}

MotionStatistics MotionVectorAnalyzer::computeStatistics(
    const std::vector<MotionVectorData>& mvData) {
    
    // Collect all motion vectors from all frames
    std::vector<MotionVector> allVectors;
    for (const auto& frameData : mvData) {
        allVectors.insert(allVectors.end(), 
                         frameData.vectors.begin(), 
                         frameData.vectors.end());
    }
    
    return computeStatisticsForVectors(allVectors);
}

std::vector<MotionStatistics> MotionVectorAnalyzer::aggregateByFrame(
    const std::vector<MotionVectorData>& mvData) {
    
    std::vector<MotionStatistics> result;
    
    for (const auto& frameData : mvData) {
        result.push_back(computeStatisticsForVectors(frameData.vectors));
    }
    
    return result;
}

std::vector<MotionStatistics> MotionVectorAnalyzer::aggregateByGOP(
    const std::vector<MotionVectorData>& mvData,
    const std::vector<GOPInfo>& gops) {
    
    std::vector<MotionStatistics> result;
    
    for (const auto& gop : gops) {
        // Collect all motion vectors within this GOP
        std::vector<MotionVector> gopVectors;
        
        for (const auto& frameData : mvData) {
            // Check if this frame is within the GOP
            if (frameData.pts >= gop.startPts && frameData.pts <= gop.endPts) {
                gopVectors.insert(gopVectors.end(),
                                 frameData.vectors.begin(),
                                 frameData.vectors.end());
            }
        }
        
        result.push_back(computeStatisticsForVectors(gopVectors));
    }
    
    return result;
}

bool MotionVectorAnalyzer::isStaticRegion(const MotionVector& mv, double threshold) const {
    return mv.magnitude < threshold;
}

bool MotionVectorAnalyzer::isHighMotionRegion(const MotionVector& mv, double threshold) const {
    return mv.magnitude > threshold;
}

MotionStatistics MotionVectorAnalyzer::computeStatisticsForVectors(
    const std::vector<MotionVector>& vectors) {
    
    MotionStatistics stats;
    
    if (vectors.empty()) {
        stats.averageMagnitude = 0.0;
        stats.maxMagnitude = 0.0;
        stats.minMagnitude = 0.0;
        stats.staticRegions = 0;
        stats.highMotionRegions = 0;
        return stats;
    }
    
    // Calculate magnitude statistics
    double sumMagnitude = 0.0;
    double maxMag = vectors[0].magnitude;
    double minMag = vectors[0].magnitude;
    
    int staticCount = 0;
    int highMotionCount = 0;
    
    // Direction bins: 8 directions (N, NE, E, SE, S, SW, W, NW)
    std::map<std::string, int> directionDist = {
        {"N", 0}, {"NE", 0}, {"E", 0}, {"SE", 0},
        {"S", 0}, {"SW", 0}, {"W", 0}, {"NW", 0}
    };
    
    for (const auto& vec : vectors) {
        sumMagnitude += vec.magnitude;
        maxMag = std::max(maxMag, static_cast<double>(vec.magnitude));
        minMag = std::min(minMag, static_cast<double>(vec.magnitude));
        
        // Count static and high motion regions
        if (isStaticRegion(vec)) {
            staticCount++;
        }
        if (isHighMotionRegion(vec)) {
            highMotionCount++;
        }
        
        // Classify direction (only for non-static vectors)
        if (vec.magnitude > 1.0) {
            // Convert direction from radians to degrees
            double degrees = vec.direction * 180.0 / M_PI;
            if (degrees < 0) degrees += 360.0;
            
            // Classify into 8 directions
            if (degrees >= 337.5 || degrees < 22.5) {
                directionDist["E"]++;
            } else if (degrees >= 22.5 && degrees < 67.5) {
                directionDist["NE"]++;
            } else if (degrees >= 67.5 && degrees < 112.5) {
                directionDist["N"]++;
            } else if (degrees >= 112.5 && degrees < 157.5) {
                directionDist["NW"]++;
            } else if (degrees >= 157.5 && degrees < 202.5) {
                directionDist["W"]++;
            } else if (degrees >= 202.5 && degrees < 247.5) {
                directionDist["SW"]++;
            } else if (degrees >= 247.5 && degrees < 292.5) {
                directionDist["S"]++;
            } else {
                directionDist["SE"]++;
            }
        }
    }
    
    stats.averageMagnitude = sumMagnitude / vectors.size();
    stats.maxMagnitude = maxMag;
    stats.minMagnitude = minMag;
    stats.staticRegions = staticCount;
    stats.highMotionRegions = highMotionCount;
    stats.directionDistribution = directionDist;
    
    return stats;
}

} // namespace video_analyzer
