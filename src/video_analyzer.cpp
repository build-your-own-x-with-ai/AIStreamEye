#include "video_analyzer/video_analyzer.h"
#include "video_analyzer/ffmpeg_error.h"
#include <iostream>

namespace video_analyzer {

void VideoAnalyzer::analyze(const std::string& filepath) {
    // Create decoder
    VideoDecoder decoder(filepath);
    
    // Get stream info
    stream_info_ = decoder.getStreamInfo();
    
    // Decode all frames
    frames_.clear();
    
    while (auto frame_opt = decoder.readNextFrame()) {
        frames_.push_back(*frame_opt);
    }
    
    if (frames_.empty()) {
        throw std::runtime_error("No frames decoded from video");
    }
    
    // Reset decoder for GOP analysis
    decoder.reset();
    
    // Analyze GOPs
    GOPAnalyzer gop_analyzer(decoder);
    gops_ = gop_analyzer.analyze();
    
    // Calculate frame statistics
    frame_stats_ = FrameStatistics::compute(frames_);
    
    // Detect duplicate frames
    detectDuplicateFrames();
    
    std::cout << "Analyzed " << frames_.size() << " frames, "
              << gops_.size() << " GOPs" << std::endl;
}

void VideoAnalyzer::detectDuplicateFrames(float size_tolerance, 
                                           bool require_same_qp,
                                           bool require_same_type) {
    if (frames_.empty()) {
        return;
    }
    
    // Initialize all frames as non-duplicate
    for (auto& frame : frames_) {
        frame.isDuplicate = false;
        frame.duplicateGroupId = -1;
    }
    
    int currentGroupId = 0;
    
    // Compare consecutive frames
    for (size_t i = 1; i < frames_.size(); i++) {
        const auto& prev = frames_[i - 1];
        auto& current = frames_[i];
        
        // Check size similarity (configurable tolerance)
        bool sizeMatch = std::abs(current.size - prev.size) <= (prev.size * size_tolerance / 100.0f);
        
        // Check QP match (optional)
        bool qpMatch = !require_same_qp || (current.qp == prev.qp);
        
        // Check frame type match (optional)
        bool typeMatch = !require_same_type || (current.type == prev.type);
        
        if (sizeMatch && qpMatch && typeMatch) {
            // Mark as duplicate
            current.isDuplicate = true;
            
            // If previous frame is not in a group, create a new group
            if (prev.duplicateGroupId == -1) {
                frames_[i - 1].isDuplicate = true;
                frames_[i - 1].duplicateGroupId = currentGroupId;
            }
            
            // Add current frame to the same group
            current.duplicateGroupId = prev.duplicateGroupId != -1 ? 
                                      prev.duplicateGroupId : currentGroupId;
            
            // If we just created a new group, increment the ID for next group
            if (prev.duplicateGroupId == -1) {
                currentGroupId++;
            }
        }
    }
    
    // Count duplicate frames
    int duplicateCount = 0;
    for (const auto& frame : frames_) {
        if (frame.isDuplicate) {
            duplicateCount++;
        }
    }
    
    if (duplicateCount > 0) {
        std::cout << "Detected " << duplicateCount << " duplicate frames in " 
                  << currentGroupId << " groups" << std::endl;
    }
}

} // namespace video_analyzer
