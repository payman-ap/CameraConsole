#ifndef AUDIO_PIPELINE_HPP
#define AUDIO_PIPELINE_HPP

#include <atomic>
#include <thread>

struct AudioControl
{
    std::atomic<bool> running{false};
    std::atomic<bool> muted{false};
    std::atomic<int> gain_percent{50};
    std::atomic<int> level_left{0};
    std::atomic<int> level_right{0};
};

#include "audio_recorder.hpp"
#include "audio_player.hpp"
#include "capture_thread.hpp"
#include "playback_thread.hpp"
#include "ring_buffer.hpp"


class AudioPipeline
{
public:

    AudioPipeline();
    ~AudioPipeline();

    bool start();
    void stop();

    AudioControl& control();

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
};




#endif // AUDIO_PIPELINE_HPP
