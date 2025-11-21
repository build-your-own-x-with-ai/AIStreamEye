#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace video_analyzer {

/**
 * @brief GUI configuration
 */
struct GuiConfig {
    // Analysis configuration
    double bitrateWindow = 1.0;
    double sceneThreshold = 0.3;
    bool enableSceneDetection = true;
    bool enableMotionAnalysis = true;
    int threadCount = 0;  // 0 = auto
    
    // Display configuration
    bool showBitrateChart = true;
    bool showGOPTimeline = true;
    bool showSceneMarkers = true;
    bool showMotionVectors = false;
    
    // Window configuration
    int windowWidth = 1920;
    int windowHeight = 1080;
    
    // Serialization
    nlohmann::json toJson() const;
    static GuiConfig fromJson(const nlohmann::json& j);
    
    // Save/load
    void save(const std::string& filePath) const;
    static GuiConfig load(const std::string& filePath);
};

} // namespace video_analyzer
