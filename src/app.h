#pragma once

#include "audio_capture.h"
#include "audio_device.h"
#include "gui.h"
#include "ring_buffer.h"
#include "transcriber.h"
#include "whisper_engine.h"
#include <string>
#include <vector>

class App {
public:
    App();
    ~App();

    bool init();
    void run();
    void shutdown();

private:
    void refresh_devices();
    void start_capture(int device_index);
    void stop_capture();

    GUI gui_;
    AudioCapture capture_;
    WhisperEngine whisper_;
    Transcriber transcriber_;

    std::vector<AudioDeviceInfo> devices_;
    int selected_input_device_ = -1;
    int selected_output_device_ = -1;

    std::string model_path_ = "models/ggml-base.bin";
    std::string selected_language_ = "auto";
    bool translate_ = false;
    bool is_capturing_ = false;
    std::string status_message_;

    static constexpr int RING_BUFFER_SECONDS = 60;
    static constexpr int SAMPLE_RATE = 16000;
    RingBuffer<float> ring_buffer_;
};
