#include "audio/audio_recorder.hpp"

#include <iostream>

AudioRecorder::AudioRecorder(
    const std::string& device,
    unsigned int sample_rate,
    int channels,
    int frames_per_buffer
)
    : device_(device),
      sample_rate_(sample_rate),
      channels_(channels),
      frames_per_buffer_(frames_per_buffer),
      handle_(nullptr) {}

AudioRecorder::~AudioRecorder() {
    if (handle_) {
        snd_pcm_close(handle_);
    }
}

bool AudioRecorder::initialize() {
    if (handle_) {
        return true;
    }

    int err = snd_pcm_open(
        &handle_,
        device_.c_str(),
        SND_PCM_STREAM_CAPTURE,
        0
    );
    if (err < 0) {
        std::cerr << "Cannot open device: " << snd_strerror(err) << std::endl;
        return false;
    }

    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_malloc(&params);
    if (!params) {
        std::cerr << "Cannot allocate hw_params" << std::endl;
        return false;
    }

    snd_pcm_hw_params_any(handle_, params);

    snd_pcm_hw_params_set_access(handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle_, params, SND_PCM_FORMAT_S16_LE);

    unsigned int requested_rate = sample_rate_;
    snd_pcm_hw_params_set_rate_near(handle_, params, &requested_rate, 0);

    snd_pcm_hw_params_set_channels(handle_, params, channels_);

    // CRITICAL: pin the period size to exactly frames_per_buffer_
    // Without this, ALSA picks its own period size and snd_pcm_readi
    // may write more samples than our buffer can hold
    snd_pcm_uframes_t period = static_cast<snd_pcm_uframes_t>(frames_per_buffer_);
    snd_pcm_hw_params_set_period_size_near(handle_, params, &period, 0);

    err = snd_pcm_hw_params(handle_, params);
    if (err < 0) {
        std::cerr << "Cannot set parameters: " << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(params);
        return false;
    }

    snd_pcm_prepare(handle_);

    // Read back actual negotiated values
    unsigned int actual_rate = 0;
    snd_pcm_hw_params_get_rate(params, &actual_rate, 0);

    snd_pcm_uframes_t actual_period = 0;
    snd_pcm_hw_params_get_period_size(params, &actual_period, 0);

    snd_pcm_hw_params_free(params);

    std::cout << "Actual sample rate: " << actual_rate << std::endl;
    std::cout << "Actual period size: " << actual_period << " frames" << std::endl;

    sample_rate_ = actual_rate;

    // Sync frames_per_buffer_ to what ALSA actually negotiated
    frames_per_buffer_ = static_cast<int>(actual_period);

    return true;
}

bool AudioRecorder::captureFrames(
    std::vector<int16_t>& out_buffer,
    int& frames_captured
) {
    // Buffer must hold frames × channels samples
    const size_t required = static_cast<size_t>(frames_per_buffer_ * channels_);

    if (out_buffer.size() != required) {
        out_buffer.resize(required);
    }

    int rc = snd_pcm_readi(
        handle_,
        out_buffer.data(),
        frames_per_buffer_   // still in frames, as ALSA expects
    );

    if (rc == -EPIPE) {

        std::cerr << "Overrun occurred" << std::endl;
        snd_pcm_prepare(handle_);
        frames_captured = 0;
        return false;

    } else if (rc < 0) {

        std::cerr << "Read error: "
                  << snd_strerror(rc)
                  << std::endl;

        frames_captured = 0;
        return false;
    }

    frames_captured = rc;

    return true;
}

void AudioRecorder::setDevice(const std::string& device)
{
    if (device_ != device)
    {
        device_ = device;
        if (handle_)
        {
            snd_pcm_close(handle_);
            handle_ = nullptr;
        }
    }
}