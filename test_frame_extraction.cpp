#include "video_analyzer/frame_extractor.h"
#include "video_analyzer/frame_renderer.h"
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return 1;
    }
    
    try {
        std::cout << "Opening video: " << argv[1] << std::endl;
        
        // Create frame extractor
        video_analyzer::FrameExtractor extractor(argv[1]);
        
        std::cout << "Video info:" << std::endl;
        std::cout << "  Width: " << extractor.getWidth() << std::endl;
        std::cout << "  Height: " << extractor.getHeight() << std::endl;
        std::cout << "  Frame count: " << extractor.getFrameCount() << std::endl;
        
        // Create frame renderer
        video_analyzer::FrameRenderer renderer(extractor.getWidth(), extractor.getHeight());
        
        // Extract first frame
        std::cout << "\nExtracting frame 0..." << std::endl;
        AVFrame* frame = extractor.getFrame(0);
        
        if (!frame) {
            std::cerr << "Failed to extract frame 0" << std::endl;
            return 1;
        }
        
        std::cout << "Frame extracted successfully!" << std::endl;
        std::cout << "  Format: " << frame->format << std::endl;
        std::cout << "  Width: " << frame->width << std::endl;
        std::cout << "  Height: " << frame->height << std::endl;
        
        // Convert to RGB
        std::vector<uint8_t> rgb_buffer(extractor.getWidth() * extractor.getHeight() * 3);
        
        std::cout << "\nConverting to RGB..." << std::endl;
        if (!renderer.convertFrameToRGB(frame, rgb_buffer.data())) {
            std::cerr << "Failed to convert to RGB" << std::endl;
            return 1;
        }
        
        std::cout << "Conversion successful!" << std::endl;
        
        // Save as PPM for verification
        std::string output_file = "frame_0.ppm";
        std::ofstream out(output_file, std::ios::binary);
        out << "P6\n" << extractor.getWidth() << " " << extractor.getHeight() << "\n255\n";
        out.write(reinterpret_cast<const char*>(rgb_buffer.data()), rgb_buffer.size());
        out.close();
        
        std::cout << "Saved frame to: " << output_file << std::endl;
        std::cout << "\nTest PASSED!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
