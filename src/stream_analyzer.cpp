#include "video_analyzer/stream_analyzer.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace video_analyzer {

StreamAnalyzer::StreamAnalyzer(const std::string& streamUrl, int threadCount)
    : decoder_(std::make_unique<StreamDecoder>(streamUrl, threadCount)),
      threadPool_(std::make_unique<ThreadPool>(threadCount)) {
}

StreamAnalyzer::~StreamAnalyzer() {
    stop();
}

void StreamAnalyzer::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    analysisThread_ = std::thread(&StreamAnalyzer::analysisLoop, this);
}

void StreamAnalyzer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    decoder_->stop();
    
    if (analysisThread_.joinable()) {
        analysisThread_.join();
    }
    
    if (streamingOutput_.is_open()) {
        streamingOutput_.close();
    }
}

BitrateStatistics StreamAnalyzer::getCurrentBitrateStats(double windowSize) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    BitrateStatistics stats;
    stats.averageBitrate = 0.0;
    stats.maxBitrate = 0.0;
    stats.minBitrate = 0.0;
    stats.stdDeviation = 0.0;
    
    if (frameWindow_.empty()) {
        return stats;
    }
    
    // Get frames within window
    double currentTime = frameWindow_.back().timestamp;
    double startTime = currentTime - windowSize;
    
    std::vector<FrameInfo> windowFrames;
    for (const auto& frame : frameWindow_) {
        if (frame.timestamp >= startTime) {
            windowFrames.push_back(frame);
        }
    }
    
    if (windowFrames.empty()) {
        return stats;
    }
    
    // Calculate bitrate
    int64_t totalSize = 0;
    for (const auto& frame : windowFrames) {
        totalSize += frame.size;
    }
    
    double duration = windowFrames.back().timestamp - windowFrames.front().timestamp;
    if (duration > 0) {
        stats.averageBitrate = (totalSize * 8.0) / duration;  // bits per second
        stats.maxBitrate = stats.averageBitrate * 1.5;  // Simplified
        stats.minBitrate = stats.averageBitrate * 0.5;
    }
    
    return stats;
}

FrameStatistics StreamAnalyzer::getCurrentFrameStats(double windowSize) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (frameWindow_.empty()) {
        return FrameStatistics();
    }
    
    // Get frames within window
    double currentTime = frameWindow_.back().timestamp;
    double startTime = currentTime - windowSize;
    
    std::vector<FrameInfo> windowFrames;
    for (const auto& frame : frameWindow_) {
        if (frame.timestamp >= startTime) {
            windowFrames.push_back(frame);
        }
    }
    
    return FrameStatistics::compute(windowFrames);
}

std::vector<Anomaly> StreamAnalyzer::getDetectedAnomalies() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return std::vector<Anomaly>(anomalies_.begin(), anomalies_.end());
}

void StreamAnalyzer::setFrameCallback(FrameCallback callback) {
    frameCallback_ = callback;
}

void StreamAnalyzer::setAnomalyCallback(AnomalyCallback callback) {
    anomalyCallback_ = callback;
}

void StreamAnalyzer::enableStreamingExport(const std::string& outputPath) {
    streamingOutput_.open(outputPath);
    exportEnabled_ = streamingOutput_.is_open();
}

void StreamAnalyzer::analysisLoop() {
    const size_t maxWindowSize = 300;  // Keep last 300 frames
    
    while (running_ && decoder_->isStreamActive()) {
        auto frame = decoder_->readNextFrame();
        
        if (!frame.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Add to window
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            frameWindow_.push_back(frame.value());
            
            // Limit window size
            if (frameWindow_.size() > maxWindowSize) {
                frameWindow_.pop_front();
            }
        }
        
        // Detect anomalies
        detectAnomalies(frame.value());
        
        // Call frame callback
        if (frameCallback_) {
            frameCallback_(frame.value());
        }
        
        // Export to JSON Lines
        if (exportEnabled_ && streamingOutput_.is_open()) {
            streamingOutput_ << frame->toJson().dump() << "\n";
            streamingOutput_.flush();
        }
        
        previousFrame_ = frame;
    }
}

void StreamAnalyzer::detectAnomalies(const FrameInfo& frame) {
    // Frame drop detection
    if (previousFrame_.has_value()) {
        double timeDiff = frame.timestamp - previousFrame_->timestamp;
        double expectedDiff = 1.0 / 30.0;  // Assume 30fps
        
        if (timeDiff > expectedDiff * 2.0) {
            Anomaly anomaly;
            anomaly.type = AnomalyType::FRAME_DROP;
            anomaly.timestamp = frame.timestamp;
            anomaly.description = "Frame drop detected: " + std::to_string(timeDiff) + "s gap";
            
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                anomalies_.push_back(anomaly);
                
                // Keep only recent anomalies
                if (anomalies_.size() > 100) {
                    anomalies_.pop_front();
                }
            }
            
            if (anomalyCallback_) {
                anomalyCallback_(anomaly);
            }
        }
    }
    
    // Bitrate spike detection
    if (previousFrame_.has_value()) {
        double currentBitrate = frame.size * 8.0;  // Simplified
        double previousBitrate = previousFrame_->size * 8.0;
        
        if (currentBitrate > previousBitrate * 3.0) {
            Anomaly anomaly;
            anomaly.type = AnomalyType::BITRATE_SPIKE;
            anomaly.timestamp = frame.timestamp;
            anomaly.description = "Bitrate spike detected";
            
            {
                std::lock_guard<std::mutex> lock(dataMutex_);
                anomalies_.push_back(anomaly);
                
                if (anomalies_.size() > 100) {
                    anomalies_.pop_front();
                }
            }
            
            if (anomalyCallback_) {
                anomalyCallback_(anomaly);
            }
        }
    }
    
    // Quality drop detection (simplified - based on QP)
    if (frame.qp > 40) {  // High QP indicates low quality
        Anomaly anomaly;
        anomaly.type = AnomalyType::QUALITY_DROP;
        anomaly.timestamp = frame.timestamp;
        anomaly.description = "Quality drop detected: QP=" + std::to_string(frame.qp);
        
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            anomalies_.push_back(anomaly);
            
            if (anomalies_.size() > 100) {
                anomalies_.pop_front();
            }
        }
        
        if (anomalyCallback_) {
            anomalyCallback_(anomaly);
        }
    }
}

} // namespace video_analyzer
