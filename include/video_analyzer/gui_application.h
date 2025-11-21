#pragma once

// GLEW must be included before any OpenGL headers (including GLFW)
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include "video_analyzer/video_analyzer.h"

namespace video_analyzer {

class GUIApplication {
public:
    GUIApplication();
    ~GUIApplication();

    bool initialize(int width = 1920, int height = 1080);
    void run();
    void shutdown();

    bool loadVideo(const std::string& filepath);

private:
    // GLFW callbacks
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
    
    // Window icon
    void setWindowIcon();
    
    void setupImGui();
    void cleanupImGui();
    void renderFrame();
    void renderMenuBar();
    void renderToolBar();
    void renderVideoPlayer();
    void renderTimeline();
    void renderStatistics();
    void renderCharts();
    void renderControls();
    
    // Video frame rendering
    void updateVideoTexture();
    void createVideoTexture();
    void deleteVideoTexture();

    GLFWwindow* window_ = nullptr;
    std::unique_ptr<VideoAnalyzer> analyzer_;
    std::string current_video_path_;
    
    // Playback state
    bool is_playing_ = false;
    int current_frame_ = 0;
    float playback_speed_ = 1.0f;
    double last_frame_time_ = 0.0;
    
    // Video texture
    GLuint video_texture_ = 0;
    int video_width_ = 0;
    int video_height_ = 0;
    
    // Video frame extraction and rendering
    std::unique_ptr<class FrameExtractor> frame_extractor_;
    std::unique_ptr<class FrameRenderer> frame_renderer_;
    std::vector<uint8_t> rgb_buffer_;
    
    // Zoom and scroll state
    float zoom_level_ = 1.0f;        // 1.0 = show all frames, 2.0 = show half, etc.
    float scroll_offset_ = 0.0f;     // Horizontal scroll position (0.0 - 1.0)
    
    // Dialog state
    bool show_about_dialog_ = false;
    bool show_file_dialog_ = false;
    bool show_export_dialog_ = false;
    char file_path_buffer_[512] = "";
    char export_path_buffer_[512] = "analysis_export.json";
    
    // Panel visibility state
    bool show_video_player_ = true;
    bool show_timeline_ = true;
    bool show_statistics_ = true;
    bool show_charts_ = true;
    
    // Video transformation state
    bool rotate_180_ = false;
    bool flip_horizontal_ = false;
    bool flip_vertical_ = false;
    
    // Duplicate frame detection settings
    bool show_duplicate_frames_ = true;
    bool show_settings_dialog_ = false;
    float duplicate_size_tolerance_ = 1.0f;  // Size tolerance in percentage (default 1%)
    bool duplicate_require_same_qp_ = true;   // Require same QP value
    bool duplicate_require_same_type_ = true; // Require same frame type
};

} // namespace video_analyzer
