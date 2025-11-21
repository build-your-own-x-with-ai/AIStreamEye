#include "video_analyzer/scene_detector.h"
#include "video_analyzer/ffmpeg_error.h"
#include "video_analyzer/ffmpeg_context.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#include <cmath>
#include <algorithm>
#include <vector>

namespace video_analyzer {

// SceneInfo implementation
nlohmann::json SceneInfo::toJson() const {
    return nlohmann::json{
        {"sceneIndex", sceneIndex},
        {"startPts", startPts},
        {"endPts", endPts},
        {"startFrameNumber", startFrameNumber},
        {"endFrameNumber", endFrameNumber},
        {"startTimestamp", startTimestamp},
        {"endTimestamp", endTimestamp},
        {"frameCount", frameCount},
        {"averageBrightness", averageBrightness}
    };
}

// SceneDetector implementation
struct SceneDetector::Impl {
    VideoDecoder& decoder;
    double threshold;
    std::vector<SceneInfo> scenes;
    
    explicit Impl(VideoDecoder& dec, double thresh)
        : decoder(dec), threshold(thresh) {}
};

SceneDetector::SceneDetector(VideoDecoder& decoder, double threshold)
    : pImpl_(std::make_unique<Impl>(decoder, threshold)) {
}

SceneDetector::~SceneDetector() = default;

SceneDetector::SceneDetector(SceneDetector&&) noexcept = default;

SceneDetector& SceneDetector::operator=(SceneDetector&&) noexcept = default;

std::vector<SceneInfo> SceneDetector::analyze() {
    pImpl_->scenes.clear();
    pImpl_->decoder.reset();
    
    std::vector<FrameInfo> frames;
    std::vector<FramePtr> frameBuffers;
    
    // Read all frames and store frame info
    while (auto frameInfo = pImpl_->decoder.readNextFrame()) {
        frames.push_back(*frameInfo);
    }
    
    if (frames.empty()) {
        return pImpl_->scenes;
    }
    
    // Reset decoder to read frames again for pixel analysis
    pImpl_->decoder.reset();
    
    // Allocate frame buffers for comparison
    FramePtr prevFrame;
    FramePtr currFrame;
    
    // Track scene boundaries
    std::vector<int> sceneBoundaries;
    sceneBoundaries.push_back(0); // First frame is always a scene boundary
    
    int frameIndex = 0;
    double prevBrightness = 0.0;
    double totalBrightness = 0.0;
    int brightnessCount = 0;
    
    // Read frames and detect scene changes
    while (auto frameInfo = pImpl_->decoder.readNextFrame()) {
        // For scene detection, we need actual frame data
        // Since VideoDecoder doesn't expose AVFrame directly, we'll use a simplified approach
        // based on frame size changes and keyframe detection
        
        // A scene change is likely when:
        // 1. Frame is a keyframe (I-frame)
        // 2. Frame size changes significantly
        // 3. Frame type pattern changes
        
        if (frameIndex > 0) {
            const auto& prevFrameInfo = frames[frameIndex - 1];
            const auto& currFrameInfo = frames[frameIndex];
            
            // Calculate size difference ratio
            double sizeDiff = 0.0;
            if (prevFrameInfo.size > 0) {
                sizeDiff = std::abs(static_cast<double>(currFrameInfo.size - prevFrameInfo.size)) / 
                          static_cast<double>(prevFrameInfo.size);
            }
            
            // Detect scene boundary based on:
            // 1. Keyframe with significant size change
            // 2. Size difference exceeds threshold
            bool isSceneBoundary = false;
            
            if (currFrameInfo.isKeyFrame && sizeDiff > pImpl_->threshold) {
                isSceneBoundary = true;
            } else if (sizeDiff > pImpl_->threshold * 2.0) {
                // Very large size change even without keyframe
                isSceneBoundary = true;
            }
            
            if (isSceneBoundary) {
                sceneBoundaries.push_back(frameIndex);
            }
        }
        
        frameIndex++;
    }
    
    // Add final boundary
    if (sceneBoundaries.back() != static_cast<int>(frames.size()) - 1) {
        sceneBoundaries.push_back(frames.size());
    }
    
    // Create SceneInfo objects
    for (size_t i = 0; i < sceneBoundaries.size() - 1; i++) {
        int startIdx = sceneBoundaries[i];
        int endIdx = sceneBoundaries[i + 1] - 1;
        
        if (endIdx >= static_cast<int>(frames.size())) {
            endIdx = frames.size() - 1;
        }
        
        SceneInfo scene;
        scene.sceneIndex = i;
        scene.startFrameNumber = startIdx;
        scene.endFrameNumber = endIdx;
        scene.startPts = frames[startIdx].pts;
        scene.endPts = frames[endIdx].pts;
        scene.startTimestamp = frames[startIdx].timestamp;
        scene.endTimestamp = frames[endIdx].timestamp;
        scene.frameCount = endIdx - startIdx + 1;
        
        // Calculate average brightness (approximated by average frame size)
        double totalSize = 0.0;
        for (int j = startIdx; j <= endIdx; j++) {
            totalSize += frames[j].size;
        }
        scene.averageBrightness = totalSize / scene.frameCount;
        
        pImpl_->scenes.push_back(scene);
    }
    
    return pImpl_->scenes;
}

void SceneDetector::setThreshold(double threshold) {
    pImpl_->threshold = threshold;
}

double SceneDetector::getThreshold() const {
    return pImpl_->threshold;
}

int SceneDetector::getSceneCount() const {
    return pImpl_->scenes.size();
}

double SceneDetector::getAverageSceneDuration() const {
    if (pImpl_->scenes.empty()) {
        return 0.0;
    }
    
    double totalDuration = 0.0;
    for (const auto& scene : pImpl_->scenes) {
        totalDuration += (scene.endTimestamp - scene.startTimestamp);
    }
    
    return totalDuration / pImpl_->scenes.size();
}

double SceneDetector::calculateFrameDifference(const AVFrame* frame1, const AVFrame* frame2) const {
    if (!frame1 || !frame2) {
        return 0.0;
    }
    
    // Simplified pixel difference calculation
    // In a full implementation, this would:
    // 1. Convert frames to same format if needed
    // 2. Calculate pixel-wise difference
    // 3. Normalize by frame size
    
    // For now, return a placeholder
    return 0.0;
}

double SceneDetector::calculateHistogramDifference(const AVFrame* frame1, const AVFrame* frame2) const {
    if (!frame1 || !frame2) {
        return 0.0;
    }
    
    // Simplified histogram difference calculation
    // In a full implementation, this would:
    // 1. Calculate histograms for both frames
    // 2. Compare histograms using chi-square or correlation
    // 3. Return normalized difference
    
    // For now, return a placeholder
    return 0.0;
}

} // namespace video_analyzer
