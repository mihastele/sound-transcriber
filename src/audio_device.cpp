#include "audio_device.h"
#include <portaudio.h>

bool AudioDevice::initialize() {
    return Pa_Initialize() == paNoError;
}

void AudioDevice::terminate() {
    Pa_Terminate();
}

std::vector<AudioDeviceInfo> AudioDevice::enumerate_devices() {
    std::vector<AudioDeviceInfo> devices;
    int count = Pa_GetDeviceCount();
    if (count < 0) return devices;

    int default_input = Pa_GetDefaultInputDevice();
    int default_output = Pa_GetDefaultOutputDevice();

    for (int i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;

        AudioDeviceInfo dev;
        dev.index = i;
        dev.name = info->name;
        dev.max_input_channels = info->maxInputChannels;
        dev.max_output_channels = info->maxOutputChannels;
        dev.default_sample_rate = info->defaultSampleRate;
        dev.is_default_input = (i == default_input);
        dev.is_default_output = (i == default_output);

        if (dev.max_input_channels > 0 || dev.max_output_channels > 0) {
            devices.push_back(dev);
        }
    }
    return devices;
}

AudioDeviceInfo AudioDevice::get_device(int index) {
    AudioDeviceInfo dev{};
    const PaDeviceInfo* info = Pa_GetDeviceInfo(index);
    if (!info) return dev;

    dev.index = index;
    dev.name = info->name;
    dev.max_input_channels = info->maxInputChannels;
    dev.max_output_channels = info->maxOutputChannels;
    dev.default_sample_rate = info->defaultSampleRate;
    dev.is_default_input = (index == Pa_GetDefaultInputDevice());
    dev.is_default_output = (index == Pa_GetDefaultOutputDevice());
    return dev;
}

int AudioDevice::get_default_input_device() {
    return Pa_GetDefaultInputDevice();
}

int AudioDevice::get_default_output_device() {
    return Pa_GetDefaultOutputDevice();
}
