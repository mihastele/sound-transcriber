#pragma once

#include "audio_device.h"
#include "transcriber.h"
#include "whisper_engine.h"
#include <string>
#include <vector>

struct SDL_Window;
typedef void* SDL_GLContext;

class GUI {
public:
    GUI();
    ~GUI();

    bool init(const std::string& title, int width, int height);
    void shutdown();
    bool should_close() const;

    void begin_frame();
    void end_frame();

    void render(
        const std::vector<AudioDeviceInfo>& devices,
        int& selected_input_device,
        int& selected_output_device,
        WhisperEngine& whisper,
        Transcriber& transcriber,
        std::string& model_path,
        std::string& selected_language,
        bool& translate,
        bool& is_capturing,
        std::string& status_message
    );

    SDL_Window* window() const { return window_; }

private:
    void render_device_panel(
        const std::vector<AudioDeviceInfo>& devices,
        int& selected_input,
        int& selected_output);

    void render_controls_panel(
        WhisperEngine& whisper,
        Transcriber& transcriber,
        std::string& model_path,
        std::string& selected_language,
        bool& translate,
        bool& is_capturing,
        std::string& status_message);

    void render_transcript_panel(Transcriber& transcriber);

    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;

    std::vector<std::string> input_device_names_;
    std::vector<std::string> output_device_names_;
    std::vector<int> input_device_indices_;
    std::vector<int> output_device_indices_;
    int current_input_selection_ = 0;
    int current_output_selection_ = 0;

    std::vector<std::string> languages_;
    int language_selection_ = 0;
};
