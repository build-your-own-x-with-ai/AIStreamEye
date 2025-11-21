#include "video_analyzer/frame_statistics.h"
#include <algorithm>
#include <numeric>

namespace video_analyzer {

nlohmann::json FrameStatistics::toJson() const {
    return nlohmann::json{
        {"totalFrames", totalFrames},
        {"iFrames", iFrames},
        {"pFrames", pFrames},
        {"bFrames", bFrames},
        {"averageFrameSize", averageFrameSize},
        {"maxFrameSize", maxFrameSize},
        {"minFrameSize", minFrameSize},
        {"averageQP", averageQP}
    };
}

FrameStatistics FrameStatistics::compute(const std::vector<FrameInfo>& frames) {
    FrameStatistics stats;
    
    if (frames.empty()) {
        return stats;
    }
    
    stats.totalFrames = static_cast<int>(frames.size());
    
    int64_t totalSize = 0;
    int totalQP = 0;
    stats.minFrameSize = frames[0].size;
    stats.maxFrameSize = frames[0].size;
    
    for (const auto& frame : frames) {
        // Count frame types
        switch (frame.type) {
            case FrameType::I_FRAME:
                stats.iFrames++;
                break;
            case FrameType::P_FRAME:
                stats.pFrames++;
                break;
            case FrameType::B_FRAME:
                stats.bFrames++;
                break;
            default:
                break;
        }
        
        // Accumulate sizes
        totalSize += frame.size;
        stats.minFrameSize = std::min(stats.minFrameSize, frame.size);
        stats.maxFrameSize = std::max(stats.maxFrameSize, frame.size);
        
        // Accumulate QP
        totalQP += frame.qp;
    }
    
    stats.averageFrameSize = static_cast<double>(totalSize) / frames.size();
    stats.averageQP = static_cast<double>(totalQP) / frames.size();
    
    return stats;
}

} // namespace video_analyzer
