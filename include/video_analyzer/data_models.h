#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <nlohmann/json.hpp>

namespace video_analyzer {

/**
 * @brief Frame type enumeration
 */
enum class FrameType {
    I_FRAME,  // Intra-coded frame (keyframe)
    P_FRAME,  // Predicted frame
    B_FRAME,  // Bi-directional predicted frame
    UNKNOWN   // Unknown or unsupported frame type
};

/**
 * @brief Information about a single video frame
 */
struct FrameInfo {
    int64_t pts;           // Presentation timestamp
    int64_t dts;           // Decode timestamp
    FrameType type;        // Frame type (I/P/B)
    int size;              // Frame size in bytes
    int qp;                // Quantization parameter
    bool isKeyFrame;       // Whether this is a keyframe
    double timestamp;      // Timestamp in seconds
    bool isDuplicate;      // Whether this frame is a duplicate of the previous frame
    int duplicateGroupId;  // ID of the duplicate group (-1 if not duplicate)
    
    nlohmann::json toJson() const;
    std::string toCsv() const;
};

/**
 * @brief AV1-specific tile information
 */
struct AV1TileInfo {
    int tileColumns = 0;        // Number of tile columns
    int tileRows = 0;           // Number of tile rows
    
    nlohmann::json toJson() const;
};

/**
 * @brief Information about a video stream
 */
struct StreamInfo {
    std::string codecName;      // Codec name (e.g., "h264", "hevc", "av1")
    int width;                  // Video width in pixels
    int height;                 // Video height in pixels
    double frameRate;           // Frame rate (fps)
    double duration;            // Duration in seconds
    int64_t bitrate;            // Bitrate in bits per second
    std::string pixelFormat;    // Pixel format (e.g., "yuv420p")
    int streamIndex;            // Stream index in the container
    
    // AV1-specific metadata (optional, only populated for AV1 streams)
    std::optional<AV1TileInfo> av1TileInfo;
    
    nlohmann::json toJson() const;
    std::string toCsv() const;
};

/**
 * @brief Information about a GOP (Group of Pictures)
 */
struct GOPInfo {
    int gopIndex;          // GOP index (0-based)
    int64_t startPts;      // PTS of first frame
    int64_t endPts;        // PTS of last frame
    int frameCount;        // Total number of frames
    int iFrameCount;       // Number of I-frames
    int pFrameCount;       // Number of P-frames
    int bFrameCount;       // Number of B-frames
    int64_t totalSize;     // Total size in bytes
    bool isOpenGOP;        // Whether this is an open GOP
    
    nlohmann::json toJson() const;
};

/**
 * @brief Bitrate information at a specific timestamp
 */
struct BitrateInfo {
    double timestamp;      // Timestamp in seconds
    double bitrate;        // Bitrate in bits per second
    
    nlohmann::json toJson() const;
};

/**
 * @brief Bitrate statistics for a video
 */
struct BitrateStatistics {
    double averageBitrate;     // Average bitrate
    double maxBitrate;         // Maximum bitrate
    double minBitrate;         // Minimum bitrate
    double stdDeviation;       // Standard deviation
    std::vector<BitrateInfo> timeSeriesData;  // Time series data
    
    nlohmann::json toJson() const;
};

/**
 * @brief Motion vector information
 */
struct MotionVector {
    int srcX, srcY;      // Source position
    int dstX, dstY;      // Destination position
    int motionX, motionY; // Motion vector
    float magnitude;     // Magnitude
    float direction;     // Direction in radians
    
    nlohmann::json toJson() const;
};

/**
 * @brief Motion vector data for a frame
 */
struct MotionVectorData {
    int64_t pts;                        // Presentation timestamp
    std::vector<MotionVector> vectors;  // Motion vectors
    
    nlohmann::json toJson() const;
};

/**
 * @brief Motion vector statistics
 */
struct MotionStatistics {
    double averageMagnitude;                        // Average magnitude
    double maxMagnitude;                            // Maximum magnitude
    double minMagnitude;                            // Minimum magnitude
    std::map<std::string, int> directionDistribution;  // Direction range -> count
    int staticRegions;                              // Number of static regions
    int highMotionRegions;                          // Number of high motion regions
    
    nlohmann::json toJson() const;
};

/**
 * @brief Buffer status for streaming
 */
struct BufferStatus {
    size_t bufferedFrames;      // Number of buffered frames
    double bufferedDuration;    // Buffered duration in seconds
    bool isBuffering;           // Whether currently buffering
    
    nlohmann::json toJson() const;
};

/**
 * @brief Anomaly types for stream analysis
 */
enum class AnomalyType {
    FRAME_DROP,
    BITRATE_SPIKE,
    QUALITY_DROP
};

/**
 * @brief Anomaly detected in stream
 */
struct Anomaly {
    AnomalyType type;
    double timestamp;
    std::string description;
    
    nlohmann::json toJson() const;
};

// Helper function to convert FrameType to string
std::string frameTypeToString(FrameType type);

// Helper function to convert AnomalyType to string
std::string anomalyTypeToString(AnomalyType type);

// Helper function to convert string to FrameType
FrameType stringToFrameType(const std::string& str);

} // namespace video_analyzer
