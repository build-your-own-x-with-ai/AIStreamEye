#pragma once

#include "stream_decoder.h"
#include "data_models.h"
#include "frame_statistics.h"
#include "thread_pool.h"
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <fstream>

namespace video_analyzer {

/**
 * @brief Real-time stream analyzer
 * 
 * Analyzes streaming video in real-time with anomaly detection
 */
class StreamAnalyzer {
public:
    using FrameCallback = std::function<void(const FrameInfo&)>;
    using AnomalyCallback = std::function<void(const Anomaly&)>;
    
    /**
     * @brief Construct a StreamAnalyzer
     * 
     * @param streamUrl URL of the stream
     * @param threadCount Number of threads (0 = auto)
     */
    explicit StreamAnalyzer(const std::string& streamUrl, int threadCount = 0);
    
    /**
     * @brief Destructor
     */
    ~StreamAnalyzer();
    
    // Disable copy
    StreamAnalyzer(const StreamAnalyzer&) = delete;
    StreamAnalyzer& operator=(const StreamAnalyzer&) = delete;
    
    /**
     * @brief Start real-time analysis
     */
    void start();
    
    /**
     * @brief Stop analysis
     */
    void stop();
    
    /**
     * @brief Get current bitrate statistics (sliding window)
     * 
     * @param windowSize Window size in seconds
     * @return BitrateStatistics Current statistics
     */
    BitrateStatistics getCurrentBitrateStats(double windowSize = 5.0) const;
    
    /**
     * @brief Get current frame statistics (sliding window)
     * 
     * @param windowSize Window size in seconds
     * @return FrameStatistics Current statistics
     */
    FrameStatistics getCurrentFrameStats(double windowSize = 5.0) const;
    
    /**
     * @brief Get detected anomalies
     * 
     * @return std::vector<Anomaly> List of anomalies
     */
    std::vector<Anomaly> getDetectedAnomalies() const;
    
    /**
     * @brief Set frame callback
     * 
     * @param callback Callback function
     */
    void setFrameCallback(FrameCallback callback);
    
    /**
     * @brief Set anomaly callback
     * 
     * @param callback Callback function
     */
    void setAnomalyCallback(AnomalyCallback callback);
    
    /**
     * @brief Enable streaming export to JSON Lines format
     * 
     * @param outputPath Output file path
     */
    void enableStreamingExport(const std::string& outputPath);
    
private:
    std::unique_ptr<StreamDecoder> decoder_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::atomic<bool> running_{false};
    std::thread analysisThread_;
    
    // Sliding window data
    std::deque<FrameInfo> frameWindow_;
    std::deque<Anomaly> anomalies_;
    mutable std::mutex dataMutex_;
    
    // Callbacks
    FrameCallback frameCallback_;
    AnomalyCallback anomalyCallback_;
    
    // Streaming export
    std::ofstream streamingOutput_;
    bool exportEnabled_ = false;
    
    // Analysis loop
    void analysisLoop();
    
    // Anomaly detection
    void detectAnomalies(const FrameInfo& frame);
    
    // Previous frame for comparison
    std::optional<FrameInfo> previousFrame_;
    double averageBitrate_ = 0.0;
};

} // namespace video_analyzer
