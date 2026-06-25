#ifndef AUDIO_PIPELINE_HPP
#define AUDIO_PIPELINE_HPP

#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>   // std::mutex
#include <vector>  // std::vector
#include <memory>  // std::unique_ptr

struct AudioControl
{
    std::atomic<bool> running{false};
    std::atomic<bool> muted{false};
    std::atomic<int> gain_percent{50};
    std::atomic<int> level_left{0};
    std::atomic<int> level_right{0};
};

#include "audio/audio_recorder.hpp"
#include "audio/audio_player.hpp"
#include "video/capture_thread.hpp"
#include "audio/playback_thread.hpp"
#include "audio/ring_buffer.hpp"
#include "audio/audio_device_manager.hpp"


class AudioPipeline
{
public:

    AudioPipeline();
    ~AudioPipeline();

    bool start();
    void stop();

    void setInputDevice(const std::string& device);
    void setOutputDevice(const std::string& device);
    std::string getInputDevice() const;
    std::string getOutputDevice() const;

    AudioControl& control();

    // For visualization
    std::vector<int16_t> latestSamples();

private:
    static constexpr unsigned int sample_rate_ = 48000;
    static constexpr int channels_ = 1;
    static constexpr int frames_per_buffer_ = 512;

    AudioControl control_;
    AudioRecorder recorder_;

    RingBuffer<std::vector<int16_t>, CAPTURE_RING_CAP> ring_; // later: constexpr size_t AUDIO_RING_CAP=64;
    std::unique_ptr<CaptureThread> capture_thread_;

    AudioPlayer player_;
    std::unique_ptr<PlaybackThread> playback_thread_;

    // For visulization
    void updateVisualizationSamples(const std::vector<int16_t>& samples);
    std::mutex viz_mutex_;
    std::vector<int16_t> latest_samples_;
};




#endif // AUDIO_PIPELINE_HPP
