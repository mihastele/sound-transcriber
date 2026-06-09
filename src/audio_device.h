#pragma once

#include <string>
#include <vector>

struct AudioDeviceInfo {
    int index;
    std::string name;
    int max_input_channels;
    int max_output_channels;
    double default_sample_rate;
    bool is_default_input;
    bool is_default_output;
};

class AudioDevice {
public:
    static bool initialize();
    static void terminate();
    static std::vector<AudioDeviceInfo> enumerate_devices();
    static AudioDeviceInfo get_device(int index);
    static int get_default_input_device();
    static int get_default_output_device();
};
