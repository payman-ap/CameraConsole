#pragma once

#include <alsa/asoundlib.h>

#include <cstdint>
#include <string>
#include <vector>

class AudioRecorder {
public:
    AudioRecorder(
        const std::string& device,
        unsigned int sample_rate,
        int channels,
        int frames_per_buffer
    );

    ~AudioRecorder();

    bool initialize();

    bool captureFrames(
        std::vector<int16_t>& out_buffer,
        int& frames_captured
    );

    unsigned int getSampleRate() const {
        return sample_rate_;
    }

    void setDevice(const std::string& device);
    std::string getDevice() const { return device_; }

private:
    std::string device_;
    unsigned int sample_rate_;
    int channels_;
    int frames_per_buffer_;
    snd_pcm_t* handle_;
};
