#pragma once

#include <alsa/asoundlib.h>

#include <cstdint>
#include <string>
#include <vector>

class AudioPlayer {
public:
    AudioPlayer(
        const std::string& device,
        unsigned int sample_rate,
        int channels,
        int frames_per_buffer
    );

    ~AudioPlayer();

    bool initialize();

    bool playFrames(
        const std::vector<int16_t>& input_buffer,
        int frames_to_play
    );

    unsigned int getSampleRate() const {
        return sample_rate_;
    }

private:
    std::string device_;

    unsigned int sample_rate_;
    int channels_;
    int frames_per_buffer_;

    snd_pcm_t* handle_;
};