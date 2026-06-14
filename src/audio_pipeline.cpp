#include "audio_pipeline.hpp"
#include <QDebug>


AudioPipeline::AudioPipeline() :
    recorder_("plughw:CARD=Audio,DEV=0", sample_rate_, channels_, frames_per_buffer_),
    player_("default", sample_rate_, channels_, frames_per_buffer_)
{
    //
}

AudioPipeline::~AudioPipeline()
{
    stop();
}

bool AudioPipeline::start()
{
    if(control_.running)
    {
        qDebug() << "Audio already running.";
        return true;
    }

    qDebug() << "Initializing recorder...";
    if(!recorder_.initialize())
    {
        qDebug() << "Recorder initialization failed.";
        return false;
    }

    capture_thread_ = std::make_unique<CaptureThread>(recorder_, ring_, channels_);
    capture_thread_->start();

    playback_thread_ = std::make_unique<PlaybackThread>(player_, ring_, channels_);
    playback_thread_->start();

    control_.running = true;
    qDebug() << "Audio capture started.";

    return true;
}


void AudioPipeline::stop()
{
    if(!control_.running)
        return;

    qDebug() << "Stopping audio capture...";

    if(playback_thread_)
    {
        playback_thread_->stop();
        playback_thread_->join();
        playback_thread_.reset();
    }

    if(capture_thread_)
    {
        capture_thread_->stop();
        capture_thread_->join();
        capture_thread_.reset();
    }

    control_.running = false;
    qDebug() << "Audio stopped.";
}

AudioControl& AudioPipeline::control()
{
    return control_;
}







