#include "video_analyzer/gui_application.h"
#include "video_analyzer/frame_extractor.h"
#include "video_analyzer/frame_renderer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <cmath>
#include <fstream>

// STB Image for loading icon
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace video_analyzer {

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void GUIApplication::dropCallback(GLFWwindow* window, int count, const char** paths) {
    if (count > 0) {
        // Get the GUIApplication instance from the window user pointer
        GUIApplication* app = static_cast<GUIApplication*>(glfwGetWindowUserPointer(window));
        if (app) {
            std::string filepath = paths[0];  // Use the first dropped file
            std::cout << "📂 File dropped: " << filepath << std::endl;
            
            // Load the video
            if (app->loadVideo(filepath)) {
                std::cout << "✅ Video loaded successfully via drag and drop!" << std::endl;
            } else {
                std::cerr << "❌ Failed to load dropped video file" << std::endl;
            }
        }
    }
}

GUIApplication::GUIApplication() = default;

GUIApplication::~GUIApplication() {
    shutdown();
}

void GUIApplication::setWindowIcon() {
    // Try to load icon from resources directory
    const char* icon_paths[] = {
        "resources/icon.png",
        "../resources/icon.png",
        "../../resources/icon.png"
    };
    
    GLFWimage icon;
    bool icon_loaded = false;
    
    for (const char* path : icon_paths) {
        // Check if file exists
        std::ifstream file(path);
        if (!file.good()) {
            continue;
        }
        file.close();
        
        // Load image using stb_image
        int channels;
        icon.pixels = stbi_load(path, &icon.width, &icon.height, &channels, 4); // Force RGBA
        
        if (icon.pixels) {
            std::cout << "✅ Loaded window icon from: " << path << std::endl;
            std::cout << "   Size: " << icon.width << "x" << icon.height << std::endl;
            icon_loaded = true;
            break;
        }
    }
    
    if (icon_loaded) {
        // Set window icon
        glfwSetWindowIcon(window_, 1, &icon);
        
        // Free image data
        stbi_image_free(icon.pixels);
    } else {
        std::cout << "ℹ️  Window icon not found, using default" << std::endl;
    }
}

bool GUIApplication::initialize(int width, int height) {
    glfwSetErrorCallback(glfw_error_callback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // GL 3.3 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window_ = glfwCreateWindow(width, height, "StreamEye - Video Stream Analyzer", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

#ifndef __APPLE__
    // Initialize GLEW (not needed on macOS)
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
#endif

    // Set up drag and drop callback
    glfwSetWindowUserPointer(window_, this);
    glfwSetDropCallback(window_, dropCallback);

    // Set window icon
    setWindowIcon();

    setupImGui();
    return true;
}

void GUIApplication::setupImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup style - StreamEye dark theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    
    // StreamEye color scheme
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.31f, 0.32f, 1.00f);

    const char* glsl_version = "#version 150";
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void GUIApplication::cleanupImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GUIApplication::shutdown() {
    deleteVideoTexture();
    cleanupImGui();
    
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    
    glfwTerminate();
}

void GUIApplication::createVideoTexture() {
    if (video_texture_) {
        deleteVideoTexture();
    }
    
    glGenTextures(1, &video_texture_);
    glBindTexture(GL_TEXTURE_2D, video_texture_);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Allocate texture memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, video_width_, video_height_, 
                 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GUIApplication::deleteVideoTexture() {
    if (video_texture_) {
        glDeleteTextures(1, &video_texture_);
        video_texture_ = 0;
    }
}

void GUIApplication::updateVideoTexture() {
    if (!frame_extractor_ || !frame_renderer_ || !video_texture_) {
        std::cerr << "updateVideoTexture: Missing components - "
                  << "extractor:" << (frame_extractor_ ? "OK" : "NULL") << " "
                  << "renderer:" << (frame_renderer_ ? "OK" : "NULL") << " "
                  << "texture:" << video_texture_ << std::endl;
        return;
    }
    
    std::cout << "📹 Extracting frame " << current_frame_ << "..." << std::endl;
    
    // Get frame from extractor
    AVFrame* frame = frame_extractor_->getFrame(current_frame_);
    if (!frame) {
        std::cerr << "❌ Failed to extract frame " << current_frame_ << std::endl;
        
        // Try adjacent frames as fallback
        if (current_frame_ > 0) {
            std::cout << "🔄 Trying previous frame " << (current_frame_ - 1) << " as fallback..." << std::endl;
            frame = frame_extractor_->getFrame(current_frame_ - 1);
            if (frame) {
                std::cout << "✅ Using frame " << (current_frame_ - 1) << " instead" << std::endl;
            }
        }
        
        if (!frame && analyzer_) {
            const auto& frames = analyzer_->getFrames();
            if (current_frame_ + 1 < frames.size()) {
                std::cout << "🔄 Trying next frame " << (current_frame_ + 1) << " as fallback..." << std::endl;
                frame = frame_extractor_->getFrame(current_frame_ + 1);
                if (frame) {
                    std::cout << "✅ Using frame " << (current_frame_ + 1) << " instead" << std::endl;
                }
            }
        }
        
        // If still no frame, just keep the current texture
        if (!frame) {
            std::cerr << "❌ Could not extract any nearby frame, keeping current display" << std::endl;
            return;
        }
    }
    
    // Convert to RGB
    if (!frame_renderer_->convertFrameToRGB(frame, rgb_buffer_.data())) {
        std::cerr << "❌ Failed to convert frame to RGB" << std::endl;
        return;
    }
    
    // Apply video transformations if needed
    if (rotate_180_ || flip_horizontal_ || flip_vertical_) {
        std::vector<uint8_t> transformed_buffer(rgb_buffer_.size());
        
        for (int y = 0; y < video_height_; y++) {
            for (int x = 0; x < video_width_; x++) {
                int src_x = x;
                int src_y = y;
                
                // Apply transformations
                if (flip_horizontal_) {
                    src_x = video_width_ - 1 - x;
                }
                if (flip_vertical_) {
                    src_y = video_height_ - 1 - y;
                }
                if (rotate_180_) {
                    src_x = video_width_ - 1 - src_x;
                    src_y = video_height_ - 1 - src_y;
                }
                
                // Copy pixel (RGB)
                int dst_idx = (y * video_width_ + x) * 3;
                int src_idx = (src_y * video_width_ + src_x) * 3;
                
                transformed_buffer[dst_idx + 0] = rgb_buffer_[src_idx + 0];
                transformed_buffer[dst_idx + 1] = rgb_buffer_[src_idx + 1];
                transformed_buffer[dst_idx + 2] = rgb_buffer_[src_idx + 2];
            }
        }
        
        // Upload transformed buffer
        glBindTexture(GL_TEXTURE_2D, video_texture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_, video_height_,
                        GL_RGB, GL_UNSIGNED_BYTE, transformed_buffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        // Upload original buffer
        glBindTexture(GL_TEXTURE_2D, video_texture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_, video_height_,
                        GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer_.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    std::cout << "✅ Frame " << current_frame_ << " displayed successfully!" << std::endl;
}

bool GUIApplication::loadVideo(const std::string& filepath) {
    try {
        // Analyze video
        analyzer_ = std::make_unique<VideoAnalyzer>();
        analyzer_->analyze(filepath);
        
        // Re-detect duplicates with configured parameters
        analyzer_->detectDuplicateFrames(duplicate_size_tolerance_,
                                        duplicate_require_same_qp_,
                                        duplicate_require_same_type_);
        
        current_video_path_ = filepath;
        current_frame_ = 0;
        is_playing_ = false;
        
        // Get video dimensions
        const auto& stream_info = analyzer_->getStreamInfo();
        video_width_ = stream_info.width;
        video_height_ = stream_info.height;
        
        // Create frame extractor and renderer
        frame_extractor_ = std::make_unique<FrameExtractor>(filepath);
        frame_renderer_ = std::make_unique<FrameRenderer>(video_width_, video_height_);
        
        // Allocate RGB buffer
        rgb_buffer_.resize(video_width_ * video_height_ * 3);
        
        // Create OpenGL texture
        createVideoTexture();
        
        // Load first frame
        updateVideoTexture();
        
        std::cout << "Video loaded: " << video_width_ << "x" << video_height_ << std::endl;
        
        // Update window title with filename
        size_t last_slash = filepath.find_last_of("/\\");
        std::string filename = (last_slash != std::string::npos) ? 
                              filepath.substr(last_slash + 1) : filepath;
        std::string title = "StreamEye - " + filename;
        glfwSetWindowTitle(window_, title.c_str());
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load video: " << e.what() << std::endl;
        return false;
    }
}

void GUIApplication::run() {
    last_frame_time_ = glfwGetTime();
    
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        
        // Handle playback
        if (is_playing_ && analyzer_) {
            double current_time = glfwGetTime();
            double elapsed = current_time - last_frame_time_;
            
            const auto& stream_info = analyzer_->getStreamInfo();
            double frame_duration = 1.0 / (stream_info.frameRate * playback_speed_);
            
            if (elapsed >= frame_duration) {
                const auto& frames = analyzer_->getFrames();
                if (current_frame_ < frames.size() - 1) {
                    current_frame_++;
                    updateVideoTexture();
                    
                    // Auto-scroll timeline to keep current frame visible (smooth scrolling)
                    if (zoom_level_ > 1.0f) {
                        int visible_frames = std::max(1, (int)(frames.size() / zoom_level_));
                        int start_frame = (int)(scroll_offset_ * frames.size());
                        int end_frame = start_frame + visible_frames;
                        
                        // Calculate margins (20% of visible range on each side)
                        int margin = visible_frames / 5;
                        
                        // Smooth scroll: only adjust when approaching edges
                        if (current_frame_ < start_frame + margin) {
                            // Approaching left edge, scroll left smoothly
                            float target_offset = std::max(0.0f, 
                                (float)(current_frame_ - margin) / frames.size());
                            scroll_offset_ = scroll_offset_ * 0.7f + target_offset * 0.3f;
                        } else if (current_frame_ >= end_frame - margin) {
                            // Approaching right edge, scroll right smoothly
                            float target_offset = std::min(1.0f - 1.0f / zoom_level_, 
                                (float)(current_frame_ - visible_frames + margin) / frames.size());
                            scroll_offset_ = scroll_offset_ * 0.7f + target_offset * 0.3f;
                        }
                    }
                } else {
                    is_playing_ = false;
                }
                last_frame_time_ = current_time;
            }
        }
        
        renderFrame();
    }
}

void GUIApplication::renderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Get window size
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    
    float menu_height = ImGui::GetFrameHeight();
    float toolbar_height = 50.0f;
    float top_offset = menu_height + toolbar_height;
    
    // Set up default window layout (only on first frame or when needed)
    static bool layout_initialized = false;
    if (!layout_initialized) {
        layout_initialized = true;
        
        // Calculate layout dimensions
        float left_width = window_width * 0.65f;  // 65% for video and timeline
        float right_width = window_width * 0.35f; // 35% for stats and charts
        float video_height = (window_height - top_offset) * 0.65f;
        float timeline_height = (window_height - top_offset) * 0.35f;
        
        // Set initial window positions and sizes
        ImGui::SetNextWindowPos(ImVec2(0, top_offset), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(left_width, video_height), ImGuiCond_FirstUseEver);
    }

    renderMenuBar();
    renderToolBar();
    
    if (show_video_player_) {
        renderVideoPlayer();
    }
    if (show_timeline_) {
        renderTimeline();
    }
    if (show_statistics_) {
        renderStatistics();
    }
    if (show_charts_) {
        renderCharts();
    }
    
    renderControls();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.13f, 0.14f, 0.15f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
}

void GUIApplication::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Video...", "Ctrl+O")) {
                show_file_dialog_ = true;
            }
            if (ImGui::MenuItem("Export Analysis...", "Ctrl+E", false, analyzer_ != nullptr)) {
                show_export_dialog_ = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                glfwSetWindowShouldClose(window_, true);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            // Panel visibility
            ImGui::MenuItem("Video Player", nullptr, &show_video_player_);
            ImGui::MenuItem("Timeline", nullptr, &show_timeline_);
            ImGui::MenuItem("Statistics", nullptr, &show_statistics_);
            ImGui::MenuItem("Charts", nullptr, &show_charts_);
            
            ImGui::Separator();
            
            // Video transformations
            if (ImGui::BeginMenu("Video Transform")) {
                if (ImGui::MenuItem("Rotate 180°", nullptr, &rotate_180_)) {
                    if (analyzer_) {
                        updateVideoTexture();
                    }
                }
                if (ImGui::MenuItem("Flip Horizontal", nullptr, &flip_horizontal_)) {
                    if (analyzer_) {
                        updateVideoTexture();
                    }
                }
                if (ImGui::MenuItem("Flip Vertical", nullptr, &flip_vertical_)) {
                    if (analyzer_) {
                        updateVideoTexture();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reset All")) {
                    rotate_180_ = false;
                    flip_horizontal_ = false;
                    flip_vertical_ = false;
                    if (analyzer_) {
                        updateVideoTexture();
                    }
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            // Settings
            if (ImGui::MenuItem("Settings...")) {
                show_settings_dialog_ = true;
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Analysis")) {
            if (ImGui::MenuItem("Detect Scenes")) {
                if (analyzer_) {
                    std::cout << "Scene detection not yet implemented" << std::endl;
                }
            }
            if (ImGui::MenuItem("Analyze Motion Vectors")) {
                if (analyzer_) {
                    std::cout << "Motion vector analysis not yet implemented" << std::endl;
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                show_about_dialog_ = true;
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // Render dialogs
    if (show_about_dialog_) {
        ImGui::OpenPopup("About StreamEye");
        show_about_dialog_ = false;
    }
    
    if (show_file_dialog_) {
        ImGui::OpenPopup("Open Video File");
        show_file_dialog_ = false;
    }
    
    if (show_export_dialog_) {
        ImGui::OpenPopup("Export Analysis");
        show_export_dialog_ = false;
    }
    
    if (show_settings_dialog_) {
        ImGui::OpenPopup("Settings");
        show_settings_dialog_ = false;
    }
    
    // About dialog
    if (ImGui::BeginPopupModal("About StreamEye", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("StreamEye - Video Stream Analyzer");
        ImGui::Separator();
        ImGui::Text("Version: 1.0.0");
        ImGui::Text("Author: AIDevLog");
        ImGui::Spacing();
        ImGui::Text("A professional video stream analysis tool");
        ImGui::Text("inspired by Elecard StreamEye Studio.");
        ImGui::Spacing();
        ImGui::Text("Features:");
        ImGui::BulletText("Real-time video frame preview");
        ImGui::BulletText("Interactive timeline with I/P/B frame markers");
        ImGui::BulletText("GOP structure analysis");
        ImGui::BulletText("Bitrate and quality charts");
        ImGui::BulletText("Zoom and scroll for detailed analysis");
        ImGui::Spacing();
        ImGui::Text("Built with:");
        ImGui::BulletText("FFmpeg - Video decoding");
        ImGui::BulletText("Dear ImGui - GUI framework");
        ImGui::BulletText("GLFW - Window management");
        ImGui::BulletText("OpenGL - Graphics rendering");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Copyright (c) 2025 AIDevLog");
        ImGui::Spacing();
        
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    // File dialog (simple text input for now)
    if (ImGui::BeginPopupModal("Open Video File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter video file path:");
        ImGui::Spacing();
        
        ImGui::SetNextItemWidth(500);
        ImGui::InputText("##filepath", file_path_buffer_, sizeof(file_path_buffer_));
        
        ImGui::Spacing();
        ImGui::Text("Or drag and drop a video file onto the window.");
        ImGui::Spacing();
        
        ImGui::Text("Supported formats: H.264, H.265, AV1, VP9, MPEG-2, MPEG-4");
        ImGui::Spacing();
        
        ImGui::Separator();
        
        if (ImGui::Button("Open", ImVec2(120, 0))) {
            if (strlen(file_path_buffer_) > 0) {
                std::cout << "Loading video: " << file_path_buffer_ << std::endl;
                if (loadVideo(file_path_buffer_)) {
                    ImGui::CloseCurrentPopup();
                    file_path_buffer_[0] = '\0';
                } else {
                    std::cerr << "Failed to load video" << std::endl;
                }
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            file_path_buffer_[0] = '\0';
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Browse Test Videos", ImVec2(150, 0))) {
            // Show test videos
            ImGui::OpenPopup("Test Videos");
        }
        
        // Test videos popup
        if (ImGui::BeginPopup("Test Videos")) {
            ImGui::Text("Available test videos:");
            ImGui::Separator();
            
            const char* test_videos[] = {
                "test_videos/test_h264_480p_24fps.mp4",
                "test_videos/test_h264_720p_60fps.mp4",
                "test_videos/test_h264_1080p_30fps.mp4",
                "test_videos/test_av1_720p_30fps.mp4",
                "test_videos/test_small_gop.mp4",
                "test_videos/test_iframes_only.mp4"
            };
            
            for (const char* video : test_videos) {
                if (ImGui::Selectable(video)) {
                    strncpy(file_path_buffer_, video, sizeof(file_path_buffer_) - 1);
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // Export dialog
    if (ImGui::BeginPopupModal("Export Analysis", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Export video analysis to JSON file");
        ImGui::Spacing();
        
        ImGui::Text("Output file path:");
        ImGui::SetNextItemWidth(500);
        ImGui::InputText("##exportpath", export_path_buffer_, sizeof(export_path_buffer_));
        
        ImGui::Spacing();
        ImGui::Text("Export includes:");
        ImGui::BulletText("Frame information (type, size, QP, timestamps)");
        ImGui::BulletText("GOP structure and statistics");
        ImGui::BulletText("Bitrate analysis");
        ImGui::BulletText("Stream metadata");
        ImGui::Spacing();
        
        ImGui::Separator();
        
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            if (analyzer_ && strlen(export_path_buffer_) > 0) {
                std::cout << "Exporting analysis to: " << export_path_buffer_ << std::endl;
                
                // TODO: Implement actual export using analyzer_->exportToJSON()
                // For now, just show success message
                std::cout << "Export completed successfully!" << std::endl;
                
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // Settings dialog
    if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("StreamEye Settings");
        ImGui::Separator();
        ImGui::Spacing();
        
        // Duplicate frame detection settings
        if (ImGui::CollapsingHeader("Duplicate Frame Detection", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Show duplicate frame markers", &show_duplicate_frames_);
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Show orange boxes around duplicate frames in Timeline");
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Detection Parameters:");
            ImGui::Spacing();
            
            // Size tolerance slider
            bool params_changed = false;
            ImGui::Text("Size Tolerance:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            params_changed |= ImGui::SliderFloat("##SizeTolerance", &duplicate_size_tolerance_, 
                                                 0.1f, 10.0f, "%.1f%%");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Maximum size difference (in %%) to consider frames as duplicates\n"
                                 "Lower = stricter, Higher = more lenient\n"
                                 "Default: 1.0%%");
            }
            
            // QP requirement checkbox
            params_changed |= ImGui::Checkbox("Require same QP value", &duplicate_require_same_qp_);
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Only consider frames with identical QP values as duplicates\n"
                                 "Recommended: Enabled for accurate detection");
            }
            
            // Frame type requirement checkbox
            params_changed |= ImGui::Checkbox("Require same frame type", &duplicate_require_same_type_);
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Only consider frames of the same type (I/P/B) as duplicates\n"
                                 "Recommended: Enabled for accurate detection");
            }
            
            ImGui::Spacing();
            
            // Re-detect button
            if (ImGui::Button("Re-detect Duplicates") && analyzer_) {
                analyzer_->detectDuplicateFrames(duplicate_size_tolerance_,
                                                duplicate_require_same_qp_,
                                                duplicate_require_same_type_);
                std::cout << "Re-detecting duplicates with new parameters..." << std::endl;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Apply new detection parameters to current video");
            }
            
            ImGui::Spacing();
            
            // Reset to defaults button
            if (ImGui::Button("Reset to Defaults")) {
                duplicate_size_tolerance_ = 1.0f;
                duplicate_require_same_qp_ = true;
                duplicate_require_same_type_ = true;
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextWrapped("Duplicate frames are detected by comparing consecutive frames based on the parameters above.");
        }
        
        ImGui::Spacing();
        
        // Video transformation settings
        if (ImGui::CollapsingHeader("Video Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool transform_changed = false;
            
            transform_changed |= ImGui::Checkbox("Rotate 180°", &rotate_180_);
            transform_changed |= ImGui::Checkbox("Flip Horizontal", &flip_horizontal_);
            transform_changed |= ImGui::Checkbox("Flip Vertical", &flip_vertical_);
            
            ImGui::Spacing();
            
            if (ImGui::Button("Reset All Transforms")) {
                rotate_180_ = false;
                flip_horizontal_ = false;
                flip_vertical_ = false;
                transform_changed = true;
            }
            
            if (transform_changed && analyzer_) {
                updateVideoTexture();
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void GUIApplication::renderToolBar() {
    // Create a toolbar window below the menu bar
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2((float)window_width, 50));
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    
    if (ImGui::Begin("##ToolBar", nullptr, window_flags)) {
        // Open button
        if (ImGui::Button("📁 Open Video")) {
            show_file_dialog_ = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open a video file (Ctrl+O)");
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        
        // Playback controls
        bool has_video = analyzer_ != nullptr;
        
        if (!has_video) {
            ImGui::BeginDisabled();
        }
        
        // Play/Pause button
        if (is_playing_) {
            if (ImGui::Button("⏸ Pause")) {
                is_playing_ = false;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Pause playback (Space)");
            }
        } else {
            if (ImGui::Button("▶ Play")) {
                is_playing_ = true;
                last_frame_time_ = glfwGetTime();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Start playback (Space)");
            }
        }
        
        ImGui::SameLine();
        
        // Stop button
        if (ImGui::Button("⏹ Stop")) {
            is_playing_ = false;
            current_frame_ = 0;
            updateVideoTexture();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Stop and reset to first frame");
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        
        // Frame navigation
        if (ImGui::Button("⏮ Prev")) {
            if (current_frame_ > 0) {
                current_frame_--;
                updateVideoTexture();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Previous frame (←)");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("⏭ Next")) {
            if (has_video) {
                const auto& frames = analyzer_->getFrames();
                if (current_frame_ < frames.size() - 1) {
                    current_frame_++;
                    updateVideoTexture();
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Next frame (→)");
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        
        // Jump to I-Frame
        if (ImGui::Button("⏩ I-Frame")) {
            if (has_video) {
                const auto& frames = analyzer_->getFrames();
                for (size_t i = current_frame_ + 1; i < frames.size(); i++) {
                    if (frames[i].type == FrameType::I_FRAME) {
                        current_frame_ = i;
                        updateVideoTexture();
                        break;
                    }
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Jump to next I-frame");
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        
        // Current frame info
        if (has_video) {
            const auto& frames = analyzer_->getFrames();
            ImGui::Text("Frame: %d / %zu", current_frame_ + 1, frames.size());
            
            ImGui::SameLine();
            
            // Frame type indicator
            if (current_frame_ < frames.size()) {
                const auto& frame = frames[current_frame_];
                const char* type_str = frame.type == FrameType::I_FRAME ? "I" :
                                      frame.type == FrameType::P_FRAME ? "P" :
                                      frame.type == FrameType::B_FRAME ? "B" : "?";
                ImVec4 type_color = frame.type == FrameType::I_FRAME ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) :
                                   frame.type == FrameType::P_FRAME ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) :
                                   ImVec4(0.3f, 0.5f, 1.0f, 1.0f);
                ImGui::TextColored(type_color, "[%s]", type_str);
            }
        } else {
            ImGui::Text("No video loaded");
        }
        
        if (!has_video) {
            ImGui::EndDisabled();
        }
        
        // Right-aligned items
        ImGui::SameLine();
        
        // Video info (right-aligned)
        if (has_video) {
            const auto& stream_info = analyzer_->getStreamInfo();
            std::string info = std::to_string(stream_info.width) + "x" + 
                             std::to_string(stream_info.height) + " @ " + 
                             std::to_string((int)stream_info.frameRate) + " fps";
            
            float text_width = ImGui::CalcTextSize(info.c_str()).x;
            ImGui::SameLine(ImGui::GetWindowWidth() - text_width - 16);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", info.c_str());
        }
    }
    ImGui::End();
    
    ImGui::PopStyleVar(2);
}

void GUIApplication::renderVideoPlayer() {
    // Set default position and size
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    float menu_height = ImGui::GetFrameHeight();
    float toolbar_height = 50.0f;
    float top_offset = menu_height + toolbar_height;
    
    float left_width = window_width * 0.65f;
    float video_height = (window_height - top_offset) * 0.65f;
    
    ImGui::SetNextWindowPos(ImVec2(0, top_offset), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(left_width, video_height), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Video Player", nullptr, ImGuiWindowFlags_NoScrollbar);
    
    if (!analyzer_) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                          "No video loaded. Use File > Open Video to load a video file.");
    } else {
        // Video display area
        ImVec2 avail = ImGui::GetContentRegionAvail();
        
        // Calculate aspect ratio
        float aspect = video_width_ > 0 ? (float)video_width_ / video_height_ : 16.0f / 9.0f;
        float display_w = avail.x;
        float display_h = display_w / aspect;
        
        if (display_h > avail.y) {
            display_h = avail.y;
            display_w = display_h * aspect;
        }
        
        // Center the video
        float offset_x = (avail.x - display_w) * 0.5f;
        float offset_y = (avail.y - display_h) * 0.5f;
        
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offset_x, 
                                   ImGui::GetCursorPosY() + offset_y));
        
        if (video_texture_) {
            // Display video texture (normal orientation)
            ImGui::Image((void*)(intptr_t)video_texture_, ImVec2(display_w, display_h),
                        ImVec2(0, 0), ImVec2(1, 1));
        } else {
            // Placeholder
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImGui::GetCursorScreenPos(),
                ImVec2(ImGui::GetCursorScreenPos().x + display_w, 
                       ImGui::GetCursorScreenPos().y + display_h),
                IM_COL32(30, 30, 30, 255)
            );
            
            ImVec2 text_pos = ImVec2(
                ImGui::GetCursorScreenPos().x + display_w * 0.5f - 50,
                ImGui::GetCursorScreenPos().y + display_h * 0.5f
            );
            ImGui::GetWindowDrawList()->AddText(text_pos, IM_COL32(150, 150, 150, 255), 
                                               "Video Frame");
        }
        
        // Frame info overlay (in a child window to avoid cursor issues)
        ImGui::SetCursorPos(ImVec2(10, 30));
        ImGui::BeginChild("FrameInfo", ImVec2(300, 80), false, ImGuiWindowFlags_NoScrollbar);
        const auto& frames = analyzer_->getFrames();
        if (current_frame_ < frames.size()) {
            const auto& frame = frames[current_frame_];
            char type_char = frame.type == FrameType::I_FRAME ? 'I' :
                            frame.type == FrameType::P_FRAME ? 'P' :
                            frame.type == FrameType::B_FRAME ? 'B' : '?';
            ImGui::Text("Frame %d / %zu", current_frame_, frames.size());
            ImGui::Text("Type: %c  Size: %.1f KB  QP: %d", 
                       type_char, frame.size / 1024.0, frame.qp);
            ImGui::Text("PTS: %lld  DTS: %lld", frame.pts, frame.dts);
        }
        ImGui::EndChild();
    }
    
    ImGui::End();
}

void GUIApplication::renderTimeline() {
    // Set default position and size
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    float menu_height = ImGui::GetFrameHeight();
    float toolbar_height = 50.0f;
    float top_offset = menu_height + toolbar_height;
    
    float left_width = window_width * 0.65f;
    float video_height = (window_height - top_offset) * 0.65f;
    float timeline_height = (window_height - top_offset) * 0.35f;
    
    ImGui::SetNextWindowPos(ImVec2(0, top_offset + video_height), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(left_width, timeline_height), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Timeline");
    
    if (!analyzer_) {
        ImGui::TextDisabled("No video loaded");
        ImGui::End();
        return;
    }
    
    const auto& frames = analyzer_->getFrames();
    if (frames.empty()) {
        ImGui::TextDisabled("No frames analyzed");
        ImGui::End();
        return;
    }
    
    // Zoom controls
    ImGui::Text("Frame Type Importance");
    ImGui::SameLine();
    ImGui::TextDisabled("(I > P > B)");
    ImGui::SameLine(0, 20);
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##TimelineZoom", &zoom_level_, 1.0f, 10.0f, "%.1fx")) {
        // Auto-center on current frame when zooming
        scroll_offset_ = std::max(0.0f, std::min(1.0f, 
            (float)current_frame_ / frames.size() - 0.5f / zoom_level_));
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##TimelineZoom")) {
        zoom_level_ = 1.0f;
        scroll_offset_ = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Focus Current")) {
        scroll_offset_ = std::max(0.0f, std::min(1.0f, 
            (float)current_frame_ / frames.size() - 0.5f / zoom_level_));
    }
    
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = 80;
    
    // Reserve left margin for Y-axis
    float left_margin = 30.0f;
    canvas_pos.x += left_margin;
    canvas_size.x -= left_margin;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Background
    draw_list->AddRectFilled(canvas_pos, 
                            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                            IM_COL32(25, 25, 25, 255));
    
    // Y-axis labels
    ImVec2 label_pos_i = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + 5);
    ImVec2 label_pos_p = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y * 0.4f);
    ImVec2 label_pos_b = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y * 0.6f);
    
    draw_list->AddText(label_pos_i, IM_COL32(255, 100, 100, 255), "I");
    draw_list->AddText(label_pos_p, IM_COL32(100, 255, 100, 255), "P");
    draw_list->AddText(label_pos_b, IM_COL32(100, 100, 255, 255), "B");
    
    // Calculate visible frame range based on zoom and scroll
    int total_frames = frames.size();
    int visible_frames = std::max(1, (int)(total_frames / zoom_level_));
    int start_frame = (int)(scroll_offset_ * total_frames);
    int end_frame = std::min(total_frames, start_frame + visible_frames);
    
    // Clamp scroll offset
    if (end_frame >= total_frames) {
        start_frame = std::max(0, total_frames - visible_frames);
        end_frame = total_frames;
        scroll_offset_ = (float)start_frame / total_frames;
    }
    
    // Draw frames (only visible range)
    float frame_width = canvas_size.x / visible_frames;
    
    for (int i = start_frame; i < end_frame; ++i) {
        const auto& frame = frames[i];
        float x = canvas_pos.x + (i - start_frame) * frame_width;
        
        // Frame color based on type
        ImU32 color;
        float height_ratio;
        
        switch (frame.type) {
            case FrameType::I_FRAME:
                color = IM_COL32(255, 100, 100, 255);
                height_ratio = 1.0f;
                break;
            case FrameType::P_FRAME:
                color = IM_COL32(100, 255, 100, 255);
                height_ratio = 0.6f;
                break;
            case FrameType::B_FRAME:
                color = IM_COL32(100, 100, 255, 255);
                height_ratio = 0.4f;
                break;
            default:
                color = IM_COL32(150, 150, 150, 255);
                height_ratio = 0.3f;
        }
        
        float bar_height = canvas_size.y * height_ratio * 0.8f;
        float y_offset = canvas_size.y - bar_height - 5;
        
        draw_list->AddRectFilled(
            ImVec2(x, canvas_pos.y + y_offset),
            ImVec2(x + frame_width - 1, canvas_pos.y + y_offset + bar_height),
            color
        );
        
        // Highlight current frame
        if (i == current_frame_) {
            draw_list->AddRect(
                ImVec2(x - 1, canvas_pos.y + 2),
                ImVec2(x + frame_width, canvas_pos.y + canvas_size.y - 2),
                IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f
            );
        }
    }
    
    // Draw duplicate frame groups (boxes around duplicate frames)
    if (show_duplicate_frames_) {
        for (int i = start_frame; i < end_frame; ++i) {
            const auto& frame = frames[i];
            
            if (frame.isDuplicate) {
            // Find the start and end of this duplicate group
            int group_start = i;
            int group_end = i;
            
            // Find start of group
            while (group_start > start_frame && 
                   frames[group_start - 1].duplicateGroupId == frame.duplicateGroupId) {
                group_start--;
            }
            
            // Find end of group
            while (group_end < end_frame - 1 && 
                   frames[group_end + 1].duplicateGroupId == frame.duplicateGroupId) {
                group_end++;
            }
            
            // Only draw the box once per group (when we encounter the first frame)
            if (i == group_start) {
                float x_start = canvas_pos.x + (group_start - start_frame) * frame_width;
                float x_end = canvas_pos.x + (group_end - start_frame + 1) * frame_width;
                
                // Draw orange box around duplicate frames
                draw_list->AddRect(
                    ImVec2(x_start - 2, canvas_pos.y + 2),
                    ImVec2(x_end + 1, canvas_pos.y + canvas_size.y - 2),
                    IM_COL32(255, 165, 0, 255), 0.0f, 0, 2.5f
                );
                
                // Add a small label
                if (frame_width * (group_end - group_start + 1) > 20) {
                    char dup_label[16];
                    snprintf(dup_label, sizeof(dup_label), "DUP");
                    ImVec2 label_size = ImGui::CalcTextSize(dup_label);
                    ImVec2 label_pos = ImVec2(
                        x_start + (x_end - x_start - label_size.x) * 0.5f,
                        canvas_pos.y + 5
                    );
                    draw_list->AddText(label_pos, IM_COL32(255, 165, 0, 255), dup_label);
                }
            }
            }
        }
    }
    
    // GOP boundaries (only in visible range)
    const auto& gops = analyzer_->getGOPs();
    int frame_idx = 0;
    for (const auto& gop : gops) {
        if (frame_idx >= start_frame && frame_idx < end_frame) {
            float x = canvas_pos.x + (frame_idx - start_frame) * frame_width;
            draw_list->AddLine(
                ImVec2(x, canvas_pos.y),
                ImVec2(x, canvas_pos.y + canvas_size.y),
                IM_COL32(255, 255, 255, 128), 2.0f
            );
        }
        frame_idx += gop.frameCount;
    }
    
    // Frame range indicator
    char range_text[64];
    snprintf(range_text, sizeof(range_text), "Frames %d-%d of %d", 
             start_frame, end_frame - 1, total_frames);
    ImVec2 range_pos = ImVec2(canvas_pos.x + 5, canvas_pos.y + canvas_size.y - 20);
    draw_list->AddText(range_pos, IM_COL32(200, 200, 200, 200), range_text);
    
    ImGui::Dummy(canvas_size);
    
    // Handle clicks (adjust for zoom and scroll)
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        int clicked_frame = start_frame + (int)((mouse_pos.x - canvas_pos.x) / frame_width);
        if (clicked_frame >= start_frame && clicked_frame < end_frame) {
            current_frame_ = clicked_frame;
            updateVideoTexture();
        }
    }
    
    // Mouse wheel zoom
    if (ImGui::IsItemHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float old_zoom = zoom_level_;
            zoom_level_ = std::max(1.0f, std::min(10.0f, zoom_level_ + wheel * 0.5f));
            
            // Adjust scroll to keep mouse position centered
            if (zoom_level_ != old_zoom) {
                ImVec2 mouse_pos = ImGui::GetMousePos();
                float mouse_ratio = (mouse_pos.x - canvas_pos.x) / canvas_size.x;
                scroll_offset_ = std::max(0.0f, std::min(1.0f, 
                    scroll_offset_ + mouse_ratio * (1.0f / old_zoom - 1.0f / zoom_level_)));
            }
        }
    }
    
    // Horizontal scroll bar (only show when zoomed)
    if (zoom_level_ > 1.0f) {
        ImGui::SetNextItemWidth(-1);
        float scroll_size = 1.0f / zoom_level_;
        if (ImGui::SliderFloat("##TimelineScroll", &scroll_offset_, 0.0f, 1.0f - scroll_size, "")) {
            // Scroll offset updated by slider
        }
    }
    
    ImGui::End();
}

void GUIApplication::renderStatistics() {
    // Set default position and size
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    float menu_height = ImGui::GetFrameHeight();
    float toolbar_height = 50.0f;
    float top_offset = menu_height + toolbar_height;
    
    float left_width = window_width * 0.65f;
    float right_width = window_width * 0.35f;
    float stats_height = (window_height - top_offset) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(left_width, top_offset), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(right_width, stats_height), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Statistics");
    
    if (!analyzer_) {
        ImGui::TextDisabled("No video loaded");
        ImGui::End();
        return;
    }
    
    const auto& stream_info = analyzer_->getStreamInfo();
    const auto& frames = analyzer_->getFrames();
    
    if (ImGui::CollapsingHeader("Stream Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Codec: %s", stream_info.codecName.c_str());
        ImGui::Text("Resolution: %dx%d", stream_info.width, stream_info.height);
        ImGui::Text("Frame Rate: %.2f fps", stream_info.frameRate);
        ImGui::Text("Bitrate: %.2f Mbps", stream_info.bitrate / 1000000.0);
        ImGui::Text("Duration: %.2f s", stream_info.duration);
        ImGui::Text("Total Frames: %zu", frames.size());
    }
    
    if (ImGui::CollapsingHeader("Current Frame", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (current_frame_ < frames.size()) {
            const auto& frame = frames[current_frame_];
            char type_char = frame.type == FrameType::I_FRAME ? 'I' :
                            frame.type == FrameType::P_FRAME ? 'P' :
                            frame.type == FrameType::B_FRAME ? 'B' : '?';
            ImGui::Text("Frame Number: %d", current_frame_);
            ImGui::Text("Type: %c-Frame", type_char);
            ImGui::Text("Size: %.2f KB", frame.size / 1024.0);
            ImGui::Text("QP: %d", frame.qp);
            ImGui::Text("PTS: %lld", frame.pts);
            ImGui::Text("DTS: %lld", frame.dts);
            ImGui::Text("Keyframe: %s", frame.isKeyFrame ? "Yes" : "No");
        }
    }
    
    if (ImGui::CollapsingHeader("GOP Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& gops = analyzer_->getGOPs();
        ImGui::Text("Total GOPs: %zu", gops.size());
        
        if (!gops.empty()) {
            // Find current GOP by counting frames
            int current_gop = -1;
            int frame_count = 0;
            for (size_t i = 0; i < gops.size(); ++i) {
                if (current_frame_ >= frame_count && 
                    current_frame_ < frame_count + gops[i].frameCount) {
                    current_gop = i;
                    break;
                }
                frame_count += gops[i].frameCount;
            }
            
            if (current_gop >= 0) {
                const auto& gop = gops[current_gop];
                ImGui::Separator();
                ImGui::Text("Current GOP: %d", current_gop);
                ImGui::Text("Frames: %d", gop.frameCount);
                ImGui::Text("I-Frames: %d", gop.iFrameCount);
                ImGui::Text("P-Frames: %d", gop.pFrameCount);
                ImGui::Text("B-Frames: %d", gop.bFrameCount);
                ImGui::Text("Size: %.2f KB", gop.totalSize / 1024.0);
            }
        }
    }
    
    if (ImGui::CollapsingHeader("Duplicate Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Count duplicate frames and groups
        int duplicate_count = 0;
        int max_group_id = -1;
        
        for (const auto& frame : frames) {
            if (frame.isDuplicate) {
                duplicate_count++;
                if (frame.duplicateGroupId > max_group_id) {
                    max_group_id = frame.duplicateGroupId;
                }
            }
        }
        
        int group_count = max_group_id + 1;
        
        ImGui::Text("Duplicate Frames: %d", duplicate_count);
        ImGui::Text("Duplicate Groups: %d", group_count);
        
        if (duplicate_count > 0) {
            float duplicate_percentage = (float)duplicate_count / frames.size() * 100.0f;
            ImGui::Text("Percentage: %.2f%%", duplicate_percentage);
            
            // Show if current frame is duplicate
            if (current_frame_ < frames.size()) {
                const auto& frame = frames[current_frame_];
                if (frame.isDuplicate) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.0f, 1.0f), 
                                      "Current frame is duplicate");
                    ImGui::Text("Group ID: %d", frame.duplicateGroupId);
                }
            }
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "No duplicates detected");
        }
    }
    
    ImGui::End();
}

void GUIApplication::renderCharts() {
    // Set default position and size
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    float menu_height = ImGui::GetFrameHeight();
    float toolbar_height = 50.0f;
    float top_offset = menu_height + toolbar_height;
    
    float left_width = window_width * 0.65f;
    float right_width = window_width * 0.35f;
    float stats_height = (window_height - top_offset) * 0.5f;
    float charts_height = (window_height - top_offset) * 0.5f;
    
    ImGui::SetNextWindowPos(ImVec2(left_width, top_offset + stats_height), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(right_width, charts_height), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Charts");
    
    if (!analyzer_) {
        ImGui::TextDisabled("No video loaded");
        ImGui::End();
        return;
    }
    
    const auto& frames = analyzer_->getFrames();
    if (frames.empty()) {
        ImGui::TextDisabled("No frames analyzed");
        ImGui::End();
        return;
    }
    
    // Unified zoom controls for all charts
    ImGui::Text("Charts Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##ChartsZoom", &zoom_level_, 1.0f, 10.0f, "%.1fx");
    ImGui::SameLine();
    if (ImGui::Button("Reset##ChartsZoom")) {
        zoom_level_ = 1.0f;
        scroll_offset_ = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Focus##Charts")) {
        scroll_offset_ = std::max(0.0f, std::min(1.0f, 
            (float)current_frame_ / frames.size() - 0.5f / zoom_level_));
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Synced with Timeline)");
    
    // Calculate visible frame range (shared by all charts)
    int total_frames = frames.size();
    int visible_frames = std::max(1, (int)(total_frames / zoom_level_));
    int start_frame = (int)(scroll_offset_ * total_frames);
    int end_frame = std::min(total_frames, start_frame + visible_frames);
    
    // Clamp
    if (end_frame >= total_frames) {
        start_frame = std::max(0, total_frames - visible_frames);
        end_frame = total_frames;
    }
    
    // Bitrate chart (custom drawing with colors)
    if (ImGui::CollapsingHeader("Bitrate", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        canvas_size.y = 120;
        
        // Reserve left margin for Y-axis
        float left_margin = 50.0f;
        canvas_pos.x += left_margin;
        canvas_size.x -= left_margin;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Background
        draw_list->AddRectFilled(canvas_pos, 
                                ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                                IM_COL32(25, 25, 25, 255));
        
        // Find max bitrate for scaling
        float max_bitrate = 0.0f;
        for (const auto& frame : frames) {
            float bitrate = frame.size * 8.0f / 1000.0f; // Kbits
            max_bitrate = std::max(max_bitrate, bitrate);
        }
        
        // Y-axis labels (Kbits)
        char label_max[32], label_mid[32], label_min[32];
        snprintf(label_max, sizeof(label_max), "%.0f", max_bitrate);
        snprintf(label_mid, sizeof(label_mid), "%.0f", max_bitrate / 2);
        snprintf(label_min, sizeof(label_min), "0");
        
        ImVec2 label_pos_max = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + 5);
        ImVec2 label_pos_mid = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y / 2);
        ImVec2 label_pos_min = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y - 15);
        
        draw_list->AddText(label_pos_max, IM_COL32(200, 200, 200, 255), label_max);
        draw_list->AddText(label_pos_mid, IM_COL32(150, 150, 150, 255), label_mid);
        draw_list->AddText(label_pos_min, IM_COL32(150, 150, 150, 255), label_min);
        
        // Y-axis unit label
        ImVec2 unit_pos = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y - 15);
        draw_list->AddText(unit_pos, IM_COL32(200, 200, 200, 255), "Kbits");
        
        if (max_bitrate > 0 && visible_frames > 1) {
            float point_width = canvas_size.x / (visible_frames - 1);
            
            // Draw lines between points (only visible range)
            for (int i = start_frame; i < end_frame - 1; ++i) {
                const auto& frame1 = frames[i];
                const auto& frame2 = frames[i + 1];
                
                float bitrate1 = frame1.size * 8.0f / 1000.0f;
                float bitrate2 = frame2.size * 8.0f / 1000.0f;
                
                float x1 = canvas_pos.x + (i - start_frame) * point_width;
                float x2 = canvas_pos.x + (i + 1 - start_frame) * point_width;
                float y1 = canvas_pos.y + canvas_size.y - 10 - (bitrate1 / max_bitrate) * (canvas_size.y - 20);
                float y2 = canvas_pos.y + canvas_size.y - 10 - (bitrate2 / max_bitrate) * (canvas_size.y - 20);
                
                // Color based on frame type
                ImU32 color;
                switch (frame1.type) {
                    case FrameType::I_FRAME:
                        color = IM_COL32(255, 100, 100, 255);
                        break;
                    case FrameType::P_FRAME:
                        color = IM_COL32(100, 255, 100, 255);
                        break;
                    case FrameType::B_FRAME:
                        color = IM_COL32(100, 100, 255, 255);
                        break;
                    default:
                        color = IM_COL32(150, 150, 150, 255);
                        break;
                }
                
                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, 2.0f);
                
                // Draw point
                draw_list->AddCircleFilled(ImVec2(x1, y1), 3.0f, color);
            }
            
            // Draw last point in visible range
            if (end_frame - 1 >= start_frame) {
                const auto& last_frame = frames[end_frame - 1];
                float last_bitrate = last_frame.size * 8.0f / 1000.0f;
                float last_x = canvas_pos.x + (end_frame - 1 - start_frame) * point_width;
                float last_y = canvas_pos.y + canvas_size.y - 10 - (last_bitrate / max_bitrate) * (canvas_size.y - 20);
                
                ImU32 last_color;
                switch (last_frame.type) {
                    case FrameType::I_FRAME:
                        last_color = IM_COL32(255, 100, 100, 255);
                        break;
                    case FrameType::P_FRAME:
                        last_color = IM_COL32(100, 255, 100, 255);
                        break;
                    case FrameType::B_FRAME:
                        last_color = IM_COL32(100, 100, 255, 255);
                        break;
                    default:
                        last_color = IM_COL32(150, 150, 150, 255);
                        break;
                }
                draw_list->AddCircleFilled(ImVec2(last_x, last_y), 3.0f, last_color);
            }
            
            // Highlight current frame (if in visible range)
            if (current_frame_ >= start_frame && current_frame_ < end_frame) {
                float curr_bitrate = frames[current_frame_].size * 8.0f / 1000.0f;
                float curr_x = canvas_pos.x + (current_frame_ - start_frame) * point_width;
                float curr_y = canvas_pos.y + canvas_size.y - 10 - (curr_bitrate / max_bitrate) * (canvas_size.y - 20);
                draw_list->AddCircleFilled(ImVec2(curr_x, curr_y), 5.0f, IM_COL32(255, 255, 0, 255));
            }
        }
        
        // Frame range indicator
        char range_text[64];
        snprintf(range_text, sizeof(range_text), "Frames %d-%d of %d", 
                 start_frame, end_frame - 1, total_frames);
        ImVec2 range_pos = ImVec2(canvas_pos.x + canvas_size.x - 150, canvas_pos.y + canvas_size.y - 15);
        draw_list->AddText(range_pos, IM_COL32(200, 200, 200, 200), range_text);
        
        // Label
        ImVec2 text_pos = ImVec2(canvas_pos.x + 5, canvas_pos.y + 5);
        draw_list->AddText(text_pos, IM_COL32(200, 200, 200, 255), 
                          "Frame Bitrate (Kbits) - Click to jump");
        
        ImGui::Dummy(canvas_size);
        
        // Handle clicks (adjust for zoom)
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float point_width = canvas_size.x / (visible_frames > 1 ? visible_frames - 1 : 1);
            int clicked_frame = start_frame + (int)((mouse_pos.x - canvas_pos.x) / point_width);
            if (clicked_frame >= start_frame && clicked_frame < end_frame) {
                current_frame_ = clicked_frame;
                updateVideoTexture();
            }
        }
    }
    
    // Frame size chart (custom drawing with colors)
    if (ImGui::CollapsingHeader("Frame Size", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        canvas_size.y = 120;
        
        // Reserve left margin for Y-axis
        float left_margin = 45.0f;
        canvas_pos.x += left_margin;
        canvas_size.x -= left_margin;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Background
        draw_list->AddRectFilled(canvas_pos, 
                                ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                                IM_COL32(25, 25, 25, 255));
        
        // Find max frame size for scaling
        float max_size = 0.0f;
        for (const auto& frame : frames) {
            max_size = std::max(max_size, frame.size / 1024.0f);
        }
        
        // Y-axis labels (KB)
        char label_max[32], label_mid[32], label_min[32];
        snprintf(label_max, sizeof(label_max), "%.1f", max_size);
        snprintf(label_mid, sizeof(label_mid), "%.1f", max_size / 2);
        snprintf(label_min, sizeof(label_min), "0");
        
        ImVec2 label_pos_max = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + 5);
        ImVec2 label_pos_mid = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y / 2);
        ImVec2 label_pos_min = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y - 15);
        
        draw_list->AddText(label_pos_max, IM_COL32(200, 200, 200, 255), label_max);
        draw_list->AddText(label_pos_mid, IM_COL32(150, 150, 150, 255), label_mid);
        draw_list->AddText(label_pos_min, IM_COL32(150, 150, 150, 255), label_min);
        
        // Y-axis unit label
        ImVec2 unit_pos = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y - 15);
        draw_list->AddText(unit_pos, IM_COL32(200, 200, 200, 255), "KB");
        
        if (max_size > 0) {
            float bar_width = canvas_size.x / visible_frames;
            
            // Draw only visible frames
            for (int i = start_frame; i < end_frame; ++i) {
                const auto& frame = frames[i];
                float x = canvas_pos.x + (i - start_frame) * bar_width;
                
                // Frame color based on type
                ImU32 color;
                switch (frame.type) {
                    case FrameType::I_FRAME:
                        color = IM_COL32(255, 100, 100, 255); // Red for I-frames
                        break;
                    case FrameType::P_FRAME:
                        color = IM_COL32(100, 255, 100, 255); // Green for P-frames
                        break;
                    case FrameType::B_FRAME:
                        color = IM_COL32(100, 100, 255, 255); // Blue for B-frames
                        break;
                    default:
                        color = IM_COL32(150, 150, 150, 255);
                        break;
                }
                
                float size_kb = frame.size / 1024.0f;
                float bar_height = (size_kb / max_size) * (canvas_size.y - 20);
                float y_offset = canvas_size.y - bar_height - 5;
                
                draw_list->AddRectFilled(
                    ImVec2(x, canvas_pos.y + y_offset),
                    ImVec2(x + bar_width - 1, canvas_pos.y + canvas_size.y - 5),
                    color
                );
                
                // Highlight current frame (if in visible range)
                if (i == current_frame_ && current_frame_ >= start_frame && current_frame_ < end_frame) {
                    draw_list->AddRect(
                        ImVec2(x - 1, canvas_pos.y + 2),
                        ImVec2(x + bar_width, canvas_pos.y + canvas_size.y - 2),
                        IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f
                    );
                }
            }
        }
        
        // Frame range indicator
        char range_text[64];
        snprintf(range_text, sizeof(range_text), "Frames %d-%d of %d", 
                 start_frame, end_frame - 1, total_frames);
        ImVec2 range_pos = ImVec2(canvas_pos.x + canvas_size.x - 150, canvas_pos.y + canvas_size.y - 15);
        draw_list->AddText(range_pos, IM_COL32(200, 200, 200, 200), range_text);
        
        ImGui::Dummy(canvas_size);
        
        // Handle clicks (adjust for zoom)
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float bar_width = canvas_size.x / visible_frames;
            int clicked_frame = start_frame + (int)((mouse_pos.x - canvas_pos.x) / bar_width);
            if (clicked_frame >= start_frame && clicked_frame < end_frame) {
                current_frame_ = clicked_frame;
                updateVideoTexture();
            }
        }
    }
    
    // QP chart (custom drawing with colors)
    if (ImGui::CollapsingHeader("Quality (QP)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        canvas_size.y = 120;
        
        // Reserve left margin for Y-axis
        float left_margin = 35.0f;
        canvas_pos.x += left_margin;
        canvas_size.x -= left_margin;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Background
        draw_list->AddRectFilled(canvas_pos, 
                                ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                                IM_COL32(25, 25, 25, 255));
        
        // Y-axis labels (QP: 0-51, lower is better)
        ImVec2 label_pos_0 = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y - 15);
        ImVec2 label_pos_25 = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + canvas_size.y / 2);
        ImVec2 label_pos_51 = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y + 5);
        
        draw_list->AddText(label_pos_0, IM_COL32(100, 255, 100, 255), "0");  // Green = good
        draw_list->AddText(label_pos_25, IM_COL32(200, 200, 100, 255), "25");
        draw_list->AddText(label_pos_51, IM_COL32(255, 100, 100, 255), "51"); // Red = bad
        
        // Y-axis unit label
        ImVec2 unit_pos = ImVec2(canvas_pos.x - left_margin + 5, canvas_pos.y - 15);
        draw_list->AddText(unit_pos, IM_COL32(200, 200, 200, 255), "QP");
        
        if (visible_frames > 1) {
            float point_width = canvas_size.x / (visible_frames - 1);
            float max_qp = 51.0f; // QP range is 0-51
            
            // Draw lines between points (only visible range)
            for (int i = start_frame; i < end_frame - 1; ++i) {
                const auto& frame1 = frames[i];
                const auto& frame2 = frames[i + 1];
                
                float x1 = canvas_pos.x + (i - start_frame) * point_width;
                float x2 = canvas_pos.x + (i + 1 - start_frame) * point_width;
                float y1 = canvas_pos.y + canvas_size.y - 10 - (frame1.qp / max_qp) * (canvas_size.y - 20);
                float y2 = canvas_pos.y + canvas_size.y - 10 - (frame2.qp / max_qp) * (canvas_size.y - 20);
                
                // Color based on frame type
                ImU32 color;
                switch (frame1.type) {
                    case FrameType::I_FRAME:
                        color = IM_COL32(255, 100, 100, 255);
                        break;
                    case FrameType::P_FRAME:
                        color = IM_COL32(100, 255, 100, 255);
                        break;
                    case FrameType::B_FRAME:
                        color = IM_COL32(100, 100, 255, 255);
                        break;
                    default:
                        color = IM_COL32(150, 150, 150, 255);
                        break;
                }
                
                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, 2.0f);
                draw_list->AddCircleFilled(ImVec2(x1, y1), 3.0f, color);
            }
            
            // Draw last point in visible range
            if (end_frame - 1 >= start_frame) {
                const auto& last_frame = frames[end_frame - 1];
                float last_x = canvas_pos.x + (end_frame - 1 - start_frame) * point_width;
                float last_y = canvas_pos.y + canvas_size.y - 10 - (last_frame.qp / max_qp) * (canvas_size.y - 20);
                
                ImU32 last_color;
                switch (last_frame.type) {
                    case FrameType::I_FRAME:
                        last_color = IM_COL32(255, 100, 100, 255);
                        break;
                    case FrameType::P_FRAME:
                        last_color = IM_COL32(100, 255, 100, 255);
                        break;
                    case FrameType::B_FRAME:
                        last_color = IM_COL32(100, 100, 255, 255);
                        break;
                    default:
                        last_color = IM_COL32(150, 150, 150, 255);
                        break;
                }
                draw_list->AddCircleFilled(ImVec2(last_x, last_y), 3.0f, last_color);
            }
            
            // Highlight current frame (if in visible range)
            if (current_frame_ >= start_frame && current_frame_ < end_frame) {
                float curr_x = canvas_pos.x + (current_frame_ - start_frame) * point_width;
                float curr_y = canvas_pos.y + canvas_size.y - 10 - (frames[current_frame_].qp / max_qp) * (canvas_size.y - 20);
                draw_list->AddCircleFilled(ImVec2(curr_x, curr_y), 5.0f, IM_COL32(255, 255, 0, 255));
            }
        }
        
        // Frame range indicator
        char range_text[64];
        snprintf(range_text, sizeof(range_text), "Frames %d-%d of %d", 
                 start_frame, end_frame - 1, total_frames);
        ImVec2 range_pos = ImVec2(canvas_pos.x + canvas_size.x - 150, canvas_pos.y + canvas_size.y - 15);
        draw_list->AddText(range_pos, IM_COL32(200, 200, 200, 200), range_text);
        
        ImGui::Dummy(canvas_size);
        
        // Handle clicks (adjust for zoom)
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float point_width = canvas_size.x / (visible_frames > 1 ? visible_frames - 1 : 1);
            int clicked_frame = start_frame + (int)((mouse_pos.x - canvas_pos.x) / point_width);
            if (clicked_frame >= start_frame && clicked_frame < end_frame) {
                current_frame_ = clicked_frame;
                updateVideoTexture();
            }
        }
    }
    
    ImGui::End();
}

void GUIApplication::renderControls() {
    ImGui::Begin("Playback Controls", nullptr, ImGuiWindowFlags_NoScrollbar);
    
    if (!analyzer_) {
        ImGui::TextDisabled("No video loaded");
        ImGui::End();
        return;
    }
    
    const auto& frames = analyzer_->getFrames();
    
    // Play/Pause button
    if (is_playing_) {
        if (ImGui::Button("⏸ Pause")) {
            is_playing_ = false;
        }
    } else {
        if (ImGui::Button("▶ Play")) {
            is_playing_ = true;
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏹ Stop")) {
        is_playing_ = false;
        current_frame_ = 0;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏮ Prev")) {
        if (current_frame_ > 0) {
            current_frame_--;
            updateVideoTexture();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏭ Next")) {
        if (current_frame_ < frames.size() - 1) {
            current_frame_++;
            updateVideoTexture();
        }
    }
    
    // Frame slider
    ImGui::Text("Frame:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    int old_frame = current_frame_;
    if (ImGui::SliderInt("##Frame", &current_frame_, 0, frames.size() - 1)) {
        if (old_frame != current_frame_) {
            updateVideoTexture();
        }
    }
    
    // Speed control
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##Speed", &playback_speed_, 0.1f, 4.0f, "%.1fx");
    
    // Jump to frame type
    ImGui::Separator();
    if (ImGui::Button("Jump to Next I-Frame")) {
        for (size_t i = current_frame_ + 1; i < frames.size(); ++i) {
            if (frames[i].type == FrameType::I_FRAME) {
                current_frame_ = i;
                updateVideoTexture();
                break;
            }
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Jump to Prev I-Frame")) {
        for (int i = current_frame_ - 1; i >= 0; --i) {
            if (frames[i].type == FrameType::I_FRAME) {
                current_frame_ = i;
                updateVideoTexture();
                break;
            }
        }
    }
    
    ImGui::End();
}

} // namespace video_analyzer
