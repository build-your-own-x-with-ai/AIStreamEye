cmake_minimum_required(VERSION 3.15)
project(VideoStreamAnalyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswscale
)

# nlohmann/json
find_package(nlohmann_json 3.9.0 REQUIRED)

# Google Test (optional)
find_package(GTest QUIET)

# RapidCheck (optional for property-based testing)
find_package(rapidcheck QUIET)

# Library
add_library(video_analyzer
    src/ffmpeg_context.cpp
    src/data_models.cpp
    src/video_decoder.cpp
    src/gop_analyzer.cpp
    src/bitrate_analyzer.cpp
    src/frame_statistics.cpp
    src/thread_pool.cpp
    src/scene_detector.cpp
    src/motion_vector_analyzer.cpp
    src/stream_decoder.cpp
    src/stream_analyzer.cpp
    src/gui_config.cpp
    src/video_analyzer.cpp
)

target_include_directories(video_analyzer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(video_analyzer
    PUBLIC 
        PkgConfig::FFMPEG 
        nlohmann_json::nlohmann_json
)

# Tests (only if GTest is available)
if(GTest_FOUND)
    enable_testing()
    add_executable(video_analyzer_tests
        tests/ffmpeg_context_test.cpp
        tests/data_models_test.cpp
        tests/video_decoder_test.cpp
        tests/gop_analyzer_test.cpp
        tests/bitrate_analyzer_test.cpp
        tests/frame_statistics_test.cpp
        tests/video_analyzer_test.cpp
        tests/thread_pool_test.cpp
        tests/scene_detector_test.cpp
        tests/motion_vector_analyzer_test.cpp
        tests/stream_decoder_test.cpp
        tests/property_tests.cpp
    )

    target_link_libraries(video_analyzer_tests
        video_analyzer
        GTest::gtest_main
    )

    if(rapidcheck_FOUND)
        target_sources(video_analyzer_tests PRIVATE tests/property_tests.cpp)
        target_link_libraries(video_analyzer_tests rapidcheck)
        target_compile_definitions(video_analyzer_tests PRIVATE HAVE_RAPIDCHECK)
    endif()

    # Disable automatic test discovery to avoid timeout issues
    # include(GoogleTest)
    # gtest_discover_tests(video_analyzer_tests)
    
    message(STATUS "GTest found - building tests")
else()
    message(STATUS "GTest not found - skipping tests (install with: brew install googletest)")
endif()

# CLI Application
add_executable(video_analyzer_cli
    src/main.cpp
)

target_link_libraries(video_analyzer_cli video_analyzer)

# GUI Application
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# GLEW (required on Windows/Linux, not on macOS)
if(NOT APPLE)
    find_package(GLEW REQUIRED)
endif()

# ImGui sources
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

# Windows icon resource
if(WIN32)
    set(ICON_RC ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.rc)
    if(EXISTS ${ICON_RC})
        set(APP_ICON ${ICON_RC})
    endif()
endif()

add_executable(AIStreamEye
    src/main_gui.cpp
    src/gui_application.cpp
    src/frame_extractor.cpp
    src/frame_renderer.cpp
    ${IMGUI_SOURCES}
    ${APP_ICON}
)

# Test frame extraction
add_executable(test_frame_extraction
    test_frame_extraction.cpp
    src/frame_extractor.cpp
    src/frame_renderer.cpp
)

target_include_directories(test_frame_extraction PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test_frame_extraction
    PkgConfig::FFMPEG
)

target_include_directories(AIStreamEye PRIVATE
    ${IMGUI_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)

target_link_libraries(AIStreamEye
    video_analyzer
    glfw
    ${OPENGL_LIBRARIES}
)

# Link GLEW on Windows/Linux
if(NOT APPLE)
    target_link_libraries(AIStreamEye GLEW::GLEW)
endif()

# macOS specific frameworks and icon
if(APPLE)
    target_link_libraries(AIStreamEye
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
    
    # Set macOS bundle properties
    set_target_properties(AIStreamEye PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "AIStreamEye"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.aidevlog.aistreameye"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_STRING "AIStreamEye - Video Stream Analyzer"
    )
    
    # Add icon if it exists
    set(MACOSX_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns)
    if(EXISTS ${MACOSX_ICON})
        set_target_properties(AIStreamEye PROPERTIES
            MACOSX_BUNDLE_ICON_FILE icon.icns
        )
        set_source_files_properties(${MACOSX_ICON} PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
        )
        target_sources(AIStreamEye PRIVATE ${MACOSX_ICON})
    endif()
endif()

# Installation
install(TARGETS video_analyzer video_analyzer_cli AIStreamEye
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

install(DIRECTORY include/ DESTINATION include)
