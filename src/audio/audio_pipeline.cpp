#include "audio/audio_pipeline.hpp"
#include <QDebug>


AudioPipeline::AudioPipeline() :
    recorder_("default", sample_rate_, channels_, frames_per_buffer_),
    player_("default", sample_rate_, channels_, frames_per_buffer_)
{
    std::string default_input = "plughw:CARD=Audio,DEV=0"; // preferred default input
    std::string default_output = "plughw:CARD=Device,DEV=0"; // preferred default output

    auto inputs = AudioDeviceManager::enumerateInputDevices();
    if (!inputs.empty())
    {
        bool found = false;
        for (const auto& dev : inputs)
        {
            if (dev.name == default_input)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            default_input = inputs[0].name; // fallback to first available
        }
    }
    else
    {
        default_input = "default"; // absolute fallback
    }

    auto outputs = AudioDeviceManager::enumerateOutputDevices();
    if (!outputs.empty())
    {
        bool found = false;
        for (const auto& dev : outputs)
        {
            if (dev.name == default_output)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            default_output = outputs[0].name; // fallback to first available
        }
    }
    else
    {
        default_output = "default"; // absolute fallback
    }

    recorder_.setDevice(default_input);
    player_.setDevice(default_output);

    latest_samples_.resize(frames_per_buffer_ * channels_);
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

    qDebug() << "Initializing player...";
    if(!player_.initialize())
    {
        qDebug() << "Player initialization failed.";
        return false;
    }

    // capture_thread_ = std::make_unique<CaptureThread>(recorder_, ring_, channels_,
    //                                                       [this](const std::vector<int16_t>& samples){
    //                                                           std::scoped_lock lock(viz_mutex_);
    //                                                           if (latest_samples_.size() == samples.size()) {
    //                                                               std::copy(samples.begin(), samples.end(), latest_samples_.begin());
    //                                                           }
    //                                                       }
    //                                                   );
    capture_thread_ = std::make_unique<CaptureThread>(
            recorder_, ring_, channels_,
            std::bind(&AudioPipeline::updateVisualizationSamples, this, std::placeholders::_1)
            );
    capture_thread_->start();
    // Give capture a head start — fill ~4 frames before playback begins
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    playback_thread_ = std::make_unique<PlaybackThread>(player_, ring_, channels_, control_);
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

    ring_.clear(); // flush stale frames so playback starts clean after restart/ device change

    control_.running = false;
    qDebug() << "Audio stopped.";
}

AudioControl& AudioPipeline::control()
{
    return control_;
}


std::vector<int16_t> AudioPipeline::latestSamples()
{
    std::scoped_lock lock(viz_mutex_);
    return latest_samples_;
}


void AudioPipeline::updateVisualizationSamples(const std::vector<int16_t>& samples)
{
    std::scoped_lock lock(viz_mutex_);

    // Safely copy data into the pre-allocated vector without causing heap allocations
    if (latest_samples_.size() != samples.size()) {
        latest_samples_.resize(samples.size());
    }
    std::copy(samples.begin(), samples.end(), latest_samples_.begin());
}


// For dropdown menu
void AudioPipeline::setInputDevice(const std::string& device)
{
    recorder_.setDevice(device);
}

void AudioPipeline::setOutputDevice(const std::string& device)
{
    player_.setDevice(device);
}

std::string AudioPipeline::getInputDevice() const
{
    return recorder_.getDevice();
}

std::string AudioPipeline::getOutputDevice() const
{
    return player_.getDevice();
}
