#include "audio/audio_device_manager.hpp"
#include <alsa/asoundlib.h>
#include <iostream>
#include <memory>  // 1. was missing

static std::vector<AudioDevice> enumerateDevices(bool input)
{
    std::vector<AudioDevice> list;
    int card_num = -1;

    auto free_str = [](char* p) { if (p) free(p); };

    while (true)
    {
        int err = snd_card_next(&card_num);
        if (err < 0 || card_num < 0)
            break;
        char* card_name = nullptr;      // short name  → used in plughw:CARD=
        char* card_longname = nullptr;  // long name   → used in description only

        snd_card_get_name(card_num, &card_name);
        snd_card_get_longname(card_num, &card_longname);

        auto free_str = [](char* p) { if (p) free(p); };
        std::unique_ptr<char, decltype(free_str)> name_guard(card_name, free_str);
        std::unique_ptr<char, decltype(free_str)> longname_guard(card_longname, free_str);

        if (card_name && card_longname)
        {
            std::string ctl_name = "hw:" + std::to_string(card_num);
            snd_ctl_t* ctl_handle = nullptr;
            if (snd_ctl_open(&ctl_handle, ctl_name.c_str(), 0) >= 0)
            {
                int dev = -1;
                while (snd_ctl_pcm_next_device(ctl_handle, &dev) == 0 && dev >= 0)
                {
                    snd_pcm_info_t* pcm_info;
                    snd_pcm_info_alloca(&pcm_info);
                    snd_pcm_info_set_device(pcm_info, dev);
                    snd_pcm_info_set_subdevice(pcm_info, 0);
                    snd_pcm_info_set_stream(pcm_info, input ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK);
                    if (snd_ctl_pcm_info(ctl_handle, pcm_info) == 0)
                    {
                        AudioDevice dev_info;
                        dev_info.name = "plughw:" + std::to_string(card_num) + "," + std::to_string(dev);
                        dev_info.description = std::string(card_longname);  // longname for display only
                        list.push_back(dev_info);
                    }
                }
                snd_ctl_close(ctl_handle);
            }
        }
    }
    return list;
}

std::vector<AudioDevice> AudioDeviceManager::enumerateInputDevices()
{
    return enumerateDevices(true);
}

std::vector<AudioDevice> AudioDeviceManager::enumerateOutputDevices()
{
    return enumerateDevices(false);
}