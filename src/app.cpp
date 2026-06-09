#include "app.h"
#include <SDL.h>
#include <imgui_impl_sdl2.h>

App::App()
    : transcriber_(whisper_),
      ring_buffer_(SAMPLE_RATE * RING_BUFFER_SECONDS) {}

App::~App() {
    shutdown();
}

bool App::init() {
    if (!AudioDevice::initialize()) {
        status_message_ = "Failed to initialize PortAudio!";
        return false;
    }

    if (!gui_.init("MicTranscriber - Whisper Voice Transcription", 900, 700)) {
        status_message_ = "Failed to initialize GUI!";
        return false;
    }

    refresh_devices();

    if (!devices_.empty()) {
        for (const auto& dev : devices_) {
            if (dev.is_default_input && dev.max_input_channels > 0) {
                selected_input_device_ = dev.index;
                break;
            }
        }
        if (selected_input_device_ < 0) {
            for (const auto& dev : devices_) {
                if (dev.max_input_channels > 0) {
                    selected_input_device_ = dev.index;
                    break;
                }
            }
        }
    }

    status_message_ = "Ready. Load a Whisper model to begin.";
    return true;
}

void App::run() {
    bool quit = false;

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) quit = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(gui_.window())) {
                quit = true;
            }
        }

        bool prev_capturing = is_capturing_;

        gui_.begin_frame();
        gui_.render(devices_, selected_input_device_, selected_output_device_,
                    whisper_, transcriber_, model_path_, selected_language_,
                    translate_, is_capturing_, status_message_);
        gui_.end_frame();

        if (is_capturing_ && !prev_capturing) {
            if (transcriber_.mode() == TranscriptionMode::RealTime) {
                ring_buffer_.reset();
                start_capture(selected_input_device_);
                if (capture_.is_running()) {
                    transcriber_.start_realtime(ring_buffer_, SAMPLE_RATE);
                } else {
                    is_capturing_ = false;
                    status_message_ = "Failed to start audio capture!";
                }
            } else {
                ring_buffer_.reset();
                start_capture(selected_input_device_);
                if (capture_.is_running()) {
                    transcriber_.start_recording(ring_buffer_, SAMPLE_RATE);
                } else {
                    is_capturing_ = false;
                    status_message_ = "Failed to start audio capture!";
                }
            }
        } else if (!is_capturing_ && prev_capturing) {
            if (transcriber_.mode() == TranscriptionMode::RealTime) {
                transcriber_.stop_realtime();
                stop_capture();
                status_message_ = "Stopped listening.";
            } else {
                transcriber_.stop_recording();
                stop_capture();
                status_message_ = "Recording complete. Click Transcribe.";
            }
        }
    }

    if (is_capturing_) {
        if (transcriber_.mode() == TranscriptionMode::RealTime) {
            transcriber_.stop_realtime();
        } else {
            transcriber_.stop_recording();
        }
        stop_capture();
    }
}

void App::shutdown() {
    stop_capture();
    transcriber_.stop_realtime();
    gui_.shutdown();
    AudioDevice::terminate();
}

void App::refresh_devices() {
    devices_ = AudioDevice::enumerate_devices();
}

void App::start_capture(int device_index) {
    if (device_index < 0) return;
    capture_.start(device_index, SAMPLE_RATE, 1, ring_buffer_);
}

void App::stop_capture() {
    capture_.stop();
}
