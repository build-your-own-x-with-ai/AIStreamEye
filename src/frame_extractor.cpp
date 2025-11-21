#include "video_analyzer/frame_extractor.h"
#include <stdexcept>
#include <iostream>

namespace video_analyzer {

FrameExtractor::FrameExtractor(const std::string& filepath) {
    // Open video file
    if (avformat_open_input(&format_ctx_, filepath.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Failed to open video file");
    }
    
    // Retrieve stream information
    if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Failed to find stream information");
    }
    
    // Find video stream
    for (unsigned i = 0; i < format_ctx_->nb_streams; ++i) {
        if (format_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            break;
        }
    }
    
    if (video_stream_index_ == -1) {
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("No video stream found");
    }
    
    // Get codec parameters
    AVCodecParameters* codec_params = format_ctx_->streams[video_stream_index_]->codecpar;
    
    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Codec not found");
    }
    
    // Allocate codec context
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Failed to allocate codec context");
    }
    
    // Copy codec parameters to context
    if (avcodec_parameters_to_context(codec_ctx_, codec_params) < 0) {
        avcodec_free_context(&codec_ctx_);
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Failed to copy codec parameters");
    }
    
    // Open codec
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx_);
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Failed to open codec");
    }
    
    // Allocate frame and packet
    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
    
    if (!frame_ || !packet_) {
        if (frame_) av_frame_free(&frame_);
        if (packet_) av_packet_free(&packet_);
        avcodec_free_context(&codec_ctx_);
        avformat_close_input(&format_ctx_);
        throw std::runtime_error("Failed to allocate frame/packet");
    }
    
    // Store dimensions
    width_ = codec_ctx_->width;
    height_ = codec_ctx_->height;
    
    // Estimate frame count
    AVStream* stream = format_ctx_->streams[video_stream_index_];
    if (stream->nb_frames > 0) {
        frame_count_ = stream->nb_frames;
    } else {
        // Estimate from duration and frame rate
        double duration = stream->duration * av_q2d(stream->time_base);
        double fps = av_q2d(stream->avg_frame_rate);
        frame_count_ = static_cast<int>(duration * fps);
    }
}

FrameExtractor::~FrameExtractor() {
    if (frame_) av_frame_free(&frame_);
    if (packet_) av_packet_free(&packet_);
    if (codec_ctx_) avcodec_free_context(&codec_ctx_);
    if (format_ctx_) avformat_close_input(&format_ctx_);
}

AVFrame* FrameExtractor::getFrame(int frame_number) {
    if (frame_number < 0 || frame_number >= frame_count_) {
        return nullptr;
    }
    
    // If we need to seek backwards or jump far forward, reset
    if (frame_number < current_frame_ || frame_number > current_frame_ + 100) {
        // Seek to keyframe before target
        AVStream* stream = format_ctx_->streams[video_stream_index_];
        int64_t timestamp = frame_number * stream->time_base.den / 
                           (stream->time_base.num * av_q2d(stream->avg_frame_rate));
        
        int seek_ret = av_seek_frame(format_ctx_, video_stream_index_, timestamp, AVSEEK_FLAG_BACKWARD);
        if (seek_ret < 0) {
            std::cerr << "Seek failed for frame " << frame_number << ", trying from beginning" << std::endl;
            // Try seeking to beginning
            av_seek_frame(format_ctx_, video_stream_index_, 0, AVSEEK_FLAG_BACKWARD);
        }
        avcodec_flush_buffers(codec_ctx_);
        current_frame_ = -1;
    }
    
    // Decode until we reach target frame
    bool success = decodeToFrame(frame_number);
    
    // If failed and we haven't tried from beginning, try once more
    if (!success && current_frame_ != -1) {
        std::cout << "⚠️  First attempt failed for frame " << frame_number << ", retrying from beginning..." << std::endl;
        av_seek_frame(format_ctx_, video_stream_index_, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codec_ctx_);
        current_frame_ = -1;
        success = decodeToFrame(frame_number);
        
        if (success) {
            std::cout << "✅ Retry successful for frame " << frame_number << std::endl;
        } else {
            std::cerr << "❌ Failed to decode frame " << frame_number << " even after retry" << std::endl;
        }
    }
    
    return success ? frame_ : nullptr;
}

bool FrameExtractor::decodeToFrame(int target_frame) {
    while (current_frame_ < target_frame) {
        // Read packet
        int ret = av_read_frame(format_ctx_, packet_);
        if (ret < 0) {
            return false; // End of file or error
        }
        
        // Skip non-video packets
        if (packet_->stream_index != video_stream_index_) {
            av_packet_unref(packet_);
            continue;
        }
        
        // Send packet to decoder
        ret = avcodec_send_packet(codec_ctx_, packet_);
        av_packet_unref(packet_);
        
        if (ret < 0) {
            return false;
        }
        
        // Receive frame from decoder
        while (ret >= 0) {
            ret = avcodec_receive_frame(codec_ctx_, frame_);
            
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                return false;
            }
            
            // We got a frame
            current_frame_++;
            
            if (current_frame_ == target_frame) {
                return true;
            }
            
            // Not the target frame yet, continue
            av_frame_unref(frame_);
        }
    }
    
    return current_frame_ == target_frame;
}

} // namespace video_analyzer
