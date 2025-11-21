#include "video_analyzer/data_models.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace video_analyzer {

// Helper functions
std::string frameTypeToString(FrameType type) {
    switch (type) {
        case FrameType::I_FRAME: return "I";
        case FrameType::P_FRAME: return "P";
        case FrameType::B_FRAME: return "B";
        case FrameType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

FrameType stringToFrameType(const std::string& str) {
    if (str == "I") return FrameType::I_FRAME;
    if (str == "P") return FrameType::P_FRAME;
    if (str == "B") return FrameType::B_FRAME;
    return FrameType::UNKNOWN;
}

// FrameInfo implementation
nlohmann::json FrameInfo::toJson() const {
    return nlohmann::json{
        {"pts", pts},
        {"dts", dts},
        {"type", frameTypeToString(type)},
        {"size", size},
        {"qp", qp},
        {"isKeyFrame", isKeyFrame},
        {"timestamp", timestamp},
        {"isDuplicate", isDuplicate},
        {"duplicateGroupId", duplicateGroupId}
    };
}

std::string FrameInfo::toCsv() const {
    std::ostringstream oss;
    oss << pts << ","
        << dts << ","
        << frameTypeToString(type) << ","
        << size << ","
        << qp << ","
        << (isKeyFrame ? "true" : "false") << ","
        << std::fixed << std::setprecision(6) << timestamp;
    return oss.str();
}

// AV1TileInfo implementation
nlohmann::json AV1TileInfo::toJson() const {
    return nlohmann::json{
        {"tileColumns", tileColumns},
        {"tileRows", tileRows}
    };
}

// StreamInfo implementation
nlohmann::json StreamInfo::toJson() const {
    nlohmann::json j = {
        {"codecName", codecName},
        {"width", width},
        {"height", height},
        {"frameRate", frameRate},
        {"duration", duration},
        {"bitrate", bitrate},
        {"pixelFormat", pixelFormat},
        {"streamIndex", streamIndex}
    };
    
    if (av1TileInfo.has_value()) {
        j["av1TileInfo"] = av1TileInfo->toJson();
    }
    
    return j;
}

std::string StreamInfo::toCsv() const {
    std::ostringstream oss;
    oss << codecName << ","
        << width << ","
        << height << ","
        << std::fixed << std::setprecision(2) << frameRate << ","
        << std::fixed << std::setprecision(2) << duration << ","
        << bitrate << ","
        << pixelFormat << ","
        << streamIndex;
    return oss.str();
}

// GOPInfo implementation
nlohmann::json GOPInfo::toJson() const {
    return nlohmann::json{
        {"gopIndex", gopIndex},
        {"startPts", startPts},
        {"endPts", endPts},
        {"frameCount", frameCount},
        {"iFrameCount", iFrameCount},
        {"pFrameCount", pFrameCount},
        {"bFrameCount", bFrameCount},
        {"totalSize", totalSize},
        {"isOpenGOP", isOpenGOP}
    };
}

// BitrateInfo implementation
nlohmann::json BitrateInfo::toJson() const {
    return nlohmann::json{
        {"timestamp", timestamp},
        {"bitrate", bitrate}
    };
}

// BitrateStatistics implementation
nlohmann::json BitrateStatistics::toJson() const {
    nlohmann::json timeSeriesJson = nlohmann::json::array();
    for (const auto& data : timeSeriesData) {
        timeSeriesJson.push_back(data.toJson());
    }
    
    return nlohmann::json{
        {"averageBitrate", averageBitrate},
        {"maxBitrate", maxBitrate},
        {"minBitrate", minBitrate},
        {"stdDeviation", stdDeviation},
        {"timeSeriesData", timeSeriesJson}
    };
}

// MotionVector implementation
nlohmann::json MotionVector::toJson() const {
    return nlohmann::json{
        {"srcX", srcX},
        {"srcY", srcY},
        {"dstX", dstX},
        {"dstY", dstY},
        {"motionX", motionX},
        {"motionY", motionY},
        {"magnitude", magnitude},
        {"direction", direction}
    };
}

// MotionVectorData implementation
nlohmann::json MotionVectorData::toJson() const {
    nlohmann::json vectorsJson = nlohmann::json::array();
    for (const auto& vec : vectors) {
        vectorsJson.push_back(vec.toJson());
    }
    
    return nlohmann::json{
        {"pts", pts},
        {"vectors", vectorsJson}
    };
}

// MotionStatistics implementation
nlohmann::json MotionStatistics::toJson() const {
    return nlohmann::json{
        {"averageMagnitude", averageMagnitude},
        {"maxMagnitude", maxMagnitude},
        {"minMagnitude", minMagnitude},
        {"directionDistribution", directionDistribution},
        {"staticRegions", staticRegions},
        {"highMotionRegions", highMotionRegions}
    };
}

// BufferStatus implementation
nlohmann::json BufferStatus::toJson() const {
    return nlohmann::json{
        {"bufferedFrames", bufferedFrames},
        {"bufferedDuration", bufferedDuration},
        {"isBuffering", isBuffering}
    };
}

// Helper function for AnomalyType
std::string anomalyTypeToString(AnomalyType type) {
    switch (type) {
        case AnomalyType::FRAME_DROP: return "FRAME_DROP";
        case AnomalyType::BITRATE_SPIKE: return "BITRATE_SPIKE";
        case AnomalyType::QUALITY_DROP: return "QUALITY_DROP";
        default: return "UNKNOWN";
    }
}

// Anomaly implementation
nlohmann::json Anomaly::toJson() const {
    return nlohmann::json{
        {"type", anomalyTypeToString(type)},
        {"timestamp", timestamp},
        {"description", description}
    };
}

} // namespace video_analyzer
