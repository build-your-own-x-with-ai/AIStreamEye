#include "video_analyzer/gui_application.h"
#include <iostream>

int main(int argc, char* argv[]) {
    video_analyzer::GUIApplication app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize GUI application" << std::endl;
        return 1;
    }
    
    // Load video if provided as argument
    if (argc > 1) {
        std::string video_path = argv[1];
        std::cout << "Loading video: " << video_path << std::endl;
        
        if (!app.loadVideo(video_path)) {
            std::cerr << "Failed to load video: " << video_path << std::endl;
            // Continue anyway to show the GUI
        }
    }
    
    app.run();
    app.shutdown();
    
    return 0;
}
