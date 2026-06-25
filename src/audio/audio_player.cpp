#include "audio/audio_player.hpp"

#include <iostream>
#include <cstring>

AudioPlayer::AudioPlayer(
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

AudioPlayer::~AudioPlayer() {
    if (handle_) {
        snd_pcm_drain(handle_);
        snd_pcm_close(handle_);
    }
}

bool AudioPlayer::initialize() {
    if (handle_) {
        return true;
    }

    int err = snd_pcm_open(
        &handle_,
        device_.c_str(),
        SND_PCM_STREAM_PLAYBACK,
        0
    );

    if (err < 0) {
        std::cerr << "Cannot open playback device: "
                  << snd_strerror(err)
                  << std::endl;
        return false;
    }

    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_malloc(&params);

    if (!params) {
        std::cerr << "Cannot allocate hw_params (player)" << std::endl;
        return false;
    }

    snd_pcm_hw_params_any(handle_, params);

    snd_pcm_hw_params_set_access(
        handle_,
        params,
        SND_PCM_ACCESS_RW_INTERLEAVED
    );

    snd_pcm_hw_params_set_format(
        handle_,
        params,
        SND_PCM_FORMAT_S16_LE
    );

    unsigned int requested_rate = sample_rate_;

    snd_pcm_hw_params_set_rate_near(
        handle_,
        params,
        &requested_rate,
        0
    );

    snd_pcm_hw_params_set_channels(
        handle_,
        params,
        channels_
    );

    snd_pcm_uframes_t period =
        static_cast<snd_pcm_uframes_t>(frames_per_buffer_);

    snd_pcm_hw_params_set_period_size_near(
        handle_,
        params,
        &period,
        0
    );

    // Temp: Set buffer size to 4× the period to give the hardware more headroom
    // snd_pcm_uframes_t buffer_size = static_cast<snd_pcm_uframes_t>(frames_per_buffer_ * 4);
    // snd_pcm_hw_params_set_buffer_size_near(handle_, params, &buffer_size);

    err = snd_pcm_hw_params(handle_, params);

    if (err < 0) {
        std::cerr << "Cannot set playback parameters: "
                  << snd_strerror(err)
                  << std::endl;

        snd_pcm_hw_params_free(params);
        return false;
    }

    snd_pcm_prepare(handle_);

    unsigned int actual_rate = 0;
    snd_pcm_hw_params_get_rate(params, &actual_rate, 0);

    snd_pcm_uframes_t actual_period = 0;
    snd_pcm_hw_params_get_period_size(params, &actual_period, 0);

    snd_pcm_hw_params_free(params);

    sample_rate_ = actual_rate;
    frames_per_buffer_ = static_cast<int>(actual_period);

    std::cout << "Playback sample rate: "
              << sample_rate_ << std::endl;

    std::cout << "Playback period size: "
              << frames_per_buffer_ << " frames" << std::endl;

    return true;
}

bool AudioPlayer::playFrames(
    const std::vector<int16_t>& input_buffer,
    int frames_to_play
) {
    if (!handle_) return false;

    int rc = snd_pcm_writei(
        handle_,
        input_buffer.data(),
        frames_to_play
    );

    if (rc == -EPIPE) {

        std::cerr << "Playback underrun occurred" << std::endl;
        snd_pcm_prepare(handle_);
        return false;

    } else if (rc < 0) {

        std::cerr << "Playback error: "
                  << snd_strerror(rc)
                  << std::endl;
        return false;
    }

    return true;
}

void AudioPlayer::setDevice(const std::string& device)
{
    if (device_ != device)
    {
        device_ = device;
        if (handle_)
        {
            snd_pcm_drain(handle_);
            snd_pcm_close(handle_);
            handle_ = nullptr;
        }
    }
}

