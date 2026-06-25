#ifndef AUDIO_DEVICE_MANAGER_HPP
#define AUDIO_DEVICE_MANAGER_HPP

#include <vector>
#include <string>

struct AudioDevice {
    std::string name;        // e.g. "plughw:CARD=Audio,DEV=0"
    std::string description; // e.g. "Baseus USB Audio (Audio)"
};

class AudioDeviceManager {
public:
    static std::vector<AudioDevice> enumerateInputDevices();
    static std::vector<AudioDevice> enumerateOutputDevices();
};

#endif // AUDIO_DEVICE_MANAGER_HPP