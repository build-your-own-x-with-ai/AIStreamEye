#include "video_analyzer/video_decoder.h"
#include "video_analyzer/gop_analyzer.h"
#include "video_analyzer/frame_statistics.h"
#include "video_analyzer/ffmpeg_error.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

using namespace video_analyzer;

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <video_file> [options]\n"
              << "\nOptions:\n"
              << "  --output <file>        Output file path (default: analysis_report.json)\n"
              << "  --format <json|csv>    Output format (default: json)\n"
              << "  --max-frames <n>       Maximum frames to analyze (default: all)\n"
              << "  --help                 Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Video Stream Analyzer CLI ===\n" << std::endl;
    
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string videoPath;
    std::string outputPath = "analysis_report.json";
    std::string format = "json";
    int maxFrames = -1;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--output" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "--format" && i + 1 < argc) {
            format = argv[++i];
        } else if (arg == "--max-frames" && i + 1 < argc) {
            maxFrames = std::stoi(argv[++i]);
        } else if (arg[0] != '-') {
            videoPath = arg;
        }
    }
    
    if (videoPath.empty()) {
        std::cerr << "Error: No video file specified\n" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        std::cout << "Analyzing video: " << videoPath << "\n" << std::endl;
        
        // Open video
        VideoDecoder decoder(videoPath);
        
        // Get stream info
        auto streamInfo = decoder.getStreamInfo();
        std::cout << "Stream Information:\n"
                  << "  Codec: " << streamInfo.codecName << "\n"
                  << "  Resolution: " << streamInfo.width << "x" << streamInfo.height << "\n"
                  << "  Frame Rate: " << std::fixed << std::setprecision(2) << streamInfo.frameRate << " fps\n"
                  << "  Duration: " << std::fixed << std::setprecision(2) << streamInfo.duration << " seconds\n"
                  << "  Bitrate: " << (streamInfo.bitrate / 1000) << " kbps\n"
                  << "  Pixel Format: " << streamInfo.pixelFormat << "\n"
                  << std::endl;
        
        // Collect frames
        std::cout << "Reading frames..." << std::flush;
        std::vector<FrameInfo> frames;
        int frameCount = 0;
        
        while (auto frame = decoder.readNextFrame()) {
            frames.push_back(*frame);
            frameCount++;
            
            if (frameCount % 100 == 0) {
                std::cout << "\rReading frames... " << frameCount << std::flush;
            }
            
            if (maxFrames > 0 && frameCount >= maxFrames) {
                break;
            }
        }
        std::cout << "\rReading frames... " << frameCount << " (done)\n" << std::endl;
        
        // Compute frame statistics
        auto frameStats = FrameStatistics::compute(frames);
        std::cout << "Frame Statistics:\n"
                  << "  Total Frames: " << frameStats.totalFrames << "\n"
                  << "  I-Frames: " << frameStats.iFrames << "\n"
                  << "  P-Frames: " << frameStats.pFrames << "\n"
                  << "  B-Frames: " << frameStats.bFrames << "\n"
                  << "  Average Frame Size: " << std::fixed << std::setprecision(2) 
                  << (frameStats.averageFrameSize / 1024.0) << " KB\n"
                  << "  Max Frame Size: " << (frameStats.maxFrameSize / 1024.0) << " KB\n"
                  << "  Min Frame Size: " << (frameStats.minFrameSize / 1024.0) << " KB\n"
                  << std::endl;
        
        // Analyze GOP structure
        std::cout << "Analyzing GOP structure..." << std::flush;
        decoder.reset();
        GOPAnalyzer gopAnalyzer(decoder);
        auto gops = gopAnalyzer.analyze();
        std::cout << " done\n" << std::endl;
        
        std::cout << "GOP Analysis:\n"
                  << "  Total GOPs: " << gops.size() << "\n"
                  << "  Average GOP Length: " << std::fixed << std::setprecision(2) 
                  << gopAnalyzer.getAverageGOPLength() << " frames\n"
                  << "  Max GOP Length: " << gopAnalyzer.getMaxGOPLength() << " frames\n"
                  << "  Min GOP Length: " << gopAnalyzer.getMinGOPLength() << " frames\n"
                  << std::endl;
        
        // Export results
        if (format == "json") {
            nlohmann::json report;
            report["streamInfo"] = streamInfo.toJson();
            report["frameStatistics"] = frameStats.toJson();
            
            nlohmann::json gopsJson = nlohmann::json::array();
            for (const auto& gop : gops) {
                gopsJson.push_back(gop.toJson());
            }
            report["gops"] = gopsJson;
            
            nlohmann::json framesJson = nlohmann::json::array();
            for (const auto& frame : frames) {
                framesJson.push_back(frame.toJson());
            }
            report["frames"] = framesJson;
            
            std::ofstream outFile(outputPath);
            outFile << report.dump(2);
            outFile.close();
            
            std::cout << "Analysis report saved to: " << outputPath << std::endl;
        } else if (format == "csv") {
            std::ofstream outFile(outputPath);
            outFile << "pts,dts,type,size,qp,isKeyFrame,timestamp\n";
            for (const auto& frame : frames) {
                outFile << frame.toCsv() << "\n";
            }
            outFile.close();
            
            std::cout << "Frame data saved to: " << outputPath << std::endl;
        }
        
        std::cout << "\nAnalysis complete!" << std::endl;
        
    } catch (const FFmpegError& e) {
        std::cerr << "FFmpeg Error: " << e.what() << " (code: " << e.getErrorCode() << ")" << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
