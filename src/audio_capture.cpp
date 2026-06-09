#include "audio_capture.h"

AudioCapture::AudioCapture()
    : stream_(nullptr), sample_rate_(16000), channels_(1),
      running_(false), ring_buffer_(nullptr) {}

AudioCapture::~AudioCapture() {
    stop();
}

bool AudioCapture::start(int device_index, int sample_rate, int channels,
                         RingBuffer<float>& ring_buffer) {
    if (running_) stop();

    sample_rate_ = sample_rate;
    channels_ = channels;
    ring_buffer_ = &ring_buffer;

    PaStreamParameters params{};
    params.device = device_index;
    params.channelCount = channels;
    params.sampleFormat = paFloat32;
    params.suggestedLatency =
        Pa_GetDeviceInfo(device_index)->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&stream_, &params, nullptr,
                                sample_rate, 256,
                                paClipOff, pa_callback, this);
    if (err != paNoError) return false;

    err = Pa_StartStream(stream_);
    if (err != paNoError) {
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        return false;
    }

    running_ = true;
    return true;
}

void AudioCapture::stop() {
    if (stream_ && running_) {
        running_ = false;
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
}

bool AudioCapture::is_running() const {
    return running_;
}

int AudioCapture::pa_callback(const void* input, void* /*output*/,
                              unsigned long frame_count,
                              const PaStreamCallbackTimeInfo* /*time_info*/,
                              PaStreamCallbackFlags /*status_flags*/,
                              void* user_data) {
    auto* self = static_cast<AudioCapture*>(user_data);
    if (!self->ring_buffer_ || !input) return paContinue;

    const float* samples = static_cast<const float*>(input);

    if (self->channels_ == 1) {
        self->ring_buffer_->write(samples, frame_count);
    } else {
        for (unsigned long i = 0; i < frame_count; i++) {
            float mono = 0.0f;
            for (int c = 0; c < self->channels_; c++) {
                mono += samples[i * self->channels_ + c];
            }
            mono /= static_cast<float>(self->channels_);
            self->ring_buffer_->write(&mono, 1);
        }
    }

    return self->running_ ? paContinue : paComplete;
}
