#pragma once

#include "ring_buffer.h"
#include <portaudio.h>
#include <atomic>
#include <functional>
#include <memory>

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    bool start(int device_index, int sample_rate, int channels,
               RingBuffer<float>& ring_buffer);
    void stop();
    bool is_running() const;

    int sample_rate() const { return sample_rate_; }
    int channels() const { return channels_; }

private:
    static int pa_callback(const void* input, void* output,
                           unsigned long frame_count,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void* user_data);

    PaStream* stream_;
    int sample_rate_;
    int channels_;
    std::atomic<bool> running_;
    RingBuffer<float>* ring_buffer_;
};
