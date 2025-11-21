#pragma once

#include "video_decoder.h"
#include "data_models.h"
#include <vector>
#include <memory>

namespace video_analyzer {

/**
 * @brief Information about a detected scene
 */
struct SceneInfo {
    int sceneIndex;              // Scene index (0-based)
    int64_t startPts;            // PTS of first frame
    int64_t endPts;              // PTS of last frame
    int startFrameNumber;        // Frame number of first frame
    int endFrameNumber;          // Frame number of last frame
    double startTimestamp;       // Start timestamp in seconds
    double endTimestamp;         // End timestamp in seconds
    int frameCount;              // Number of frames in scene
    double averageBrightness;    // Average brightness of scene
    
    nlohmann::json toJson() const;
};

/**
 * @brief Scene detection analyzer
 * 
 * Detects scene boundaries using frame difference metrics
 */
class SceneDetector {
public:
    /**
     * @brief Construct a SceneDetector
     * 
     * @param decoder Reference to VideoDecoder
     * @param threshold Scene detection threshold (0.0-1.0, default 0.3)
     */
    explicit SceneDetector(VideoDecoder& decoder, double threshold = 0.3);
    
    /**
     * @brief Destructor
     */
    ~SceneDetector();
    
    // Disable copy
    SceneDetector(const SceneDetector&) = delete;
    SceneDetector& operator=(const SceneDetector&) = delete;
    
    // Enable move
    SceneDetector(SceneDetector&&) noexcept;
    SceneDetector& operator=(SceneDetector&&) noexcept;
    
    /**
     * @brief Analyze video and detect scenes
     * 
     * @return std::vector<SceneInfo> Detected scenes
     */
    std::vector<SceneInfo> analyze();
    
    /**
     * @brief Set scene detection threshold
     * 
     * @param threshold Threshold value (0.0-1.0)
     */
    void setThreshold(double threshold);
    
    /**
     * @brief Get current threshold
     * 
     * @return double Current threshold value
     */
    double getThreshold() const;
    
    /**
     * @brief Get number of detected scenes
     * 
     * @return int Scene count
     */
    int getSceneCount() const;
    
    /**
     * @brief Get average scene duration
     * 
     * @return double Average duration in seconds
     */
    double getAverageSceneDuration() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
    
    /**
     * @brief Calculate frame difference using pixel difference
     * 
     * @param frame1 First frame
     * @param frame2 Second frame
     * @return double Difference metric (0.0-1.0)
     */
    double calculateFrameDifference(const struct AVFrame* frame1, const struct AVFrame* frame2) const;
    
    /**
     * @brief Calculate frame difference using histogram difference
     * 
     * @param frame1 First frame
     * @param frame2 Second frame
     * @return double Difference metric (0.0-1.0)
     */
    double calculateHistogramDifference(const struct AVFrame* frame1, const struct AVFrame* frame2) const;
};

} // namespace video_analyzer
