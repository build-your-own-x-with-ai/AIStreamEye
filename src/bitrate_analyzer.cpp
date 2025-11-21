#include "video_analyzer/bitrate_analyzer.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace video_analyzer {

BitrateAnalyzer::BitrateAnalyzer(VideoDecoder& decoder, double windowSize)
    : decoder_(decoder), windowSize_(windowSize) {}

BitrateStatistics BitrateAnalyzer::analyze() {
    BitrateStatistics stats;
    
    // Collect all frames
    std::vector<FrameInfo> frames;
    decoder_.reset();
    
    while (auto frame = decoder_.readNextFrame()) {
        frames.push_back(*frame);
    }
    
    if (frames.empty()) {
        return stats;
    }
    
    // Calculate total size and duration
    int64_t totalSize = 0;
    for (const auto& frame : frames) {
        totalSize += frame.size;
    }
    
    double duration = frames.back().timestamp - frames.front().timestamp;
    if (duration <= 0) {
        duration = frames.size() / 30.0; // Assume 30fps if timestamps are invalid
    }
    
    // Calculate average bitrate
    stats.averageBitrate = (totalSize * 8.0) / duration; // bits per second
    
    // Calculate instantaneous bitrate using time windows
    stats.minBitrate = stats.averageBitrate;
    stats.maxBitrate = stats.averageBitrate;
    
    std::vector<double> bitrateValues;
    
    for (size_t i = 0; i < frames.size(); ) {
        double windowStart = frames[i].timestamp;
        double windowEnd = windowStart + windowSize_;
        
        // Find frames in this window
        size_t windowEndIdx = i;
        while (windowEndIdx < frames.size() && 
               frames[windowEndIdx].timestamp < windowEnd) {
            windowEndIdx++;
        }
        
        if (windowEndIdx > i) {
            double bitrate = calculateInstantaneousBitrate(frames, i, windowEndIdx);
            bitrateValues.push_back(bitrate);
            
            stats.timeSeriesData.push_back({
                windowStart,
                bitrate
            });
            
            stats.minBitrate = std::min(stats.minBitrate, bitrate);
            stats.maxBitrate = std::max(stats.maxBitrate, bitrate);
        }
        
        // Move to next window
        i = windowEndIdx > i ? windowEndIdx : i + 1;
    }
    
    // Calculate standard deviation
    if (!bitrateValues.empty()) {
        double mean = stats.averageBitrate;
        double variance = 0.0;
        
        for (double bitrate : bitrateValues) {
            double diff = bitrate - mean;
            variance += diff * diff;
        }
        
        variance /= bitrateValues.size();
        stats.stdDeviation = std::sqrt(variance);
    }
    
    return stats;
}

void BitrateAnalyzer::setWindowSize(double seconds) {
    windowSize_ = seconds;
}

double BitrateAnalyzer::calculateInstantaneousBitrate(
    const std::vector<FrameInfo>& frames,
    size_t startIdx,
    size_t endIdx
) const {
    if (startIdx >= endIdx || endIdx > frames.size()) {
        return 0.0;
    }
    
    int64_t totalSize = 0;
    for (size_t i = startIdx; i < endIdx; ++i) {
        totalSize += frames[i].size;
    }
    
    double duration = frames[endIdx - 1].timestamp - frames[startIdx].timestamp;
    if (duration <= 0) {
        duration = (endIdx - startIdx) / 30.0; // Assume 30fps
    }
    
    return (totalSize * 8.0) / duration; // bits per second
}

} // namespace video_analyzer
