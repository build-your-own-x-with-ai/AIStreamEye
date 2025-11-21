#include "video_analyzer/gop_analyzer.h"
#include <algorithm>

namespace video_analyzer {

GOPAnalyzer::GOPAnalyzer(VideoDecoder& decoder) : decoder_(decoder) {}

std::vector<GOPInfo> GOPAnalyzer::analyze() {
    gops_.clear();
    
    // Collect all frames
    std::vector<FrameInfo> frames;
    decoder_.reset();
    
    while (auto frame = decoder_.readNextFrame()) {
        frames.push_back(*frame);
    }
    
    if (frames.empty()) {
        return gops_;
    }
    
    // Detect GOP boundaries
    detectGOPBoundaries(frames);
    
    return gops_;
}

void GOPAnalyzer::detectGOPBoundaries(const std::vector<FrameInfo>& frames) {
    if (frames.empty()) return;
    
    int gopIndex = 0;
    size_t gopStart = 0;
    
    for (size_t i = 1; i < frames.size(); ++i) {
        // New GOP starts at I-frame
        if (frames[i].type == FrameType::I_FRAME && frames[i].isKeyFrame) {
            // Process previous GOP
            GOPInfo gop;
            gop.gopIndex = gopIndex++;
            gop.startPts = frames[gopStart].pts;
            gop.endPts = frames[i - 1].pts;
            gop.frameCount = static_cast<int>(i - gopStart);
            gop.iFrameCount = 0;
            gop.pFrameCount = 0;
            gop.bFrameCount = 0;
            gop.totalSize = 0;
            gop.isOpenGOP = false;
            
            // Count frame types and sizes
            for (size_t j = gopStart; j < i; ++j) {
                switch (frames[j].type) {
                    case FrameType::I_FRAME:
                        gop.iFrameCount++;
                        break;
                    case FrameType::P_FRAME:
                        gop.pFrameCount++;
                        break;
                    case FrameType::B_FRAME:
                        gop.bFrameCount++;
                        break;
                    default:
                        break;
                }
                gop.totalSize += frames[j].size;
            }
            
            gops_.push_back(gop);
            gopStart = i;
        }
    }
    
    // Process last GOP
    if (gopStart < frames.size()) {
        GOPInfo gop;
        gop.gopIndex = gopIndex;
        gop.startPts = frames[gopStart].pts;
        gop.endPts = frames.back().pts;
        gop.frameCount = static_cast<int>(frames.size() - gopStart);
        gop.iFrameCount = 0;
        gop.pFrameCount = 0;
        gop.bFrameCount = 0;
        gop.totalSize = 0;
        gop.isOpenGOP = false;
        
        for (size_t j = gopStart; j < frames.size(); ++j) {
            switch (frames[j].type) {
                case FrameType::I_FRAME:
                    gop.iFrameCount++;
                    break;
                case FrameType::P_FRAME:
                    gop.pFrameCount++;
                    break;
                case FrameType::B_FRAME:
                    gop.bFrameCount++;
                    break;
                default:
                    break;
            }
            gop.totalSize += frames[j].size;
        }
        
        gops_.push_back(gop);
    }
}

double GOPAnalyzer::getAverageGOPLength() const {
    if (gops_.empty()) return 0.0;
    
    int total = 0;
    for (const auto& gop : gops_) {
        total += gop.frameCount;
    }
    
    return static_cast<double>(total) / gops_.size();
}

int GOPAnalyzer::getMaxGOPLength() const {
    if (gops_.empty()) return 0;
    
    int maxLen = 0;
    for (const auto& gop : gops_) {
        maxLen = std::max(maxLen, gop.frameCount);
    }
    
    return maxLen;
}

int GOPAnalyzer::getMinGOPLength() const {
    if (gops_.empty()) return 0;
    
    int minLen = gops_[0].frameCount;
    for (const auto& gop : gops_) {
        minLen = std::min(minLen, gop.frameCount);
    }
    
    return minLen;
}

} // namespace video_analyzer
