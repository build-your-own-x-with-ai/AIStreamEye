#include "video_analyzer/gui_config.h"
#include <fstream>

namespace video_analyzer {

nlohmann::json GuiConfig::toJson() const {
    return nlohmann::json{
        {"bitrateWindow", bitrateWindow},
        {"sceneThreshold", sceneThreshold},
        {"enableSceneDetection", enableSceneDetection},
        {"enableMotionAnalysis", enableMotionAnalysis},
        {"threadCount", threadCount},
        {"showBitrateChart", showBitrateChart},
        {"showGOPTimeline", showGOPTimeline},
        {"showSceneMarkers", showSceneMarkers},
        {"showMotionVectors", showMotionVectors},
        {"windowWidth", windowWidth},
        {"windowHeight", windowHeight}
    };
}

GuiConfig GuiConfig::fromJson(const nlohmann::json& j) {
    GuiConfig config;
    
    if (j.contains("bitrateWindow")) config.bitrateWindow = j["bitrateWindow"];
    if (j.contains("sceneThreshold")) config.sceneThreshold = j["sceneThreshold"];
    if (j.contains("enableSceneDetection")) config.enableSceneDetection = j["enableSceneDetection"];
    if (j.contains("enableMotionAnalysis")) config.enableMotionAnalysis = j["enableMotionAnalysis"];
    if (j.contains("threadCount")) config.threadCount = j["threadCount"];
    if (j.contains("showBitrateChart")) config.showBitrateChart = j["showBitrateChart"];
    if (j.contains("showGOPTimeline")) config.showGOPTimeline = j["showGOPTimeline"];
    if (j.contains("showSceneMarkers")) config.showSceneMarkers = j["showSceneMarkers"];
    if (j.contains("showMotionVectors")) config.showMotionVectors = j["showMotionVectors"];
    if (j.contains("windowWidth")) config.windowWidth = j["windowWidth"];
    if (j.contains("windowHeight")) config.windowHeight = j["windowHeight"];
    
    return config;
}

void GuiConfig::save(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << toJson().dump(4);
    }
}

GuiConfig GuiConfig::load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        return fromJson(j);
    }
    return GuiConfig();  // Return default config if file doesn't exist
}

} // namespace video_analyzer
