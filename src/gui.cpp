#include "gui.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <SDL.h>
#include <SDL_opengl.h>

#ifdef _WIN32
#include <windows.h>
#endif

GUI::GUI() {
    languages_ = WhisperEngine::supported_languages();
}

GUI::~GUI() {
    shutdown();
}

bool GUI::init(const std::string& title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window_) return false;

    gl_context_ = SDL_GL_CreateContext(window_);
    SDL_GL_MakeCurrent(window_, gl_context_);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window_, gl_context_);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
}

void GUI::shutdown() {
    if (gl_context_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

bool GUI::should_close() const {
    return window_ == nullptr;
}

void GUI::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void GUI::end_frame() {
    ImGui::Render();
    glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x),
               static_cast<int>(ImGui::GetIO().DisplaySize.y));
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
}

void GUI::render(
    const std::vector<AudioDeviceInfo>& devices,
    int& selected_input_device,
    int& selected_output_device,
    WhisperEngine& whisper,
    Transcriber& transcriber,
    std::string& model_path,
    std::string& selected_language,
    bool& translate,
    bool& is_capturing,
    std::string& status_message) {

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Main", nullptr, flags);

    render_device_panel(devices, selected_input_device, selected_output_device);
    ImGui::Spacing();
    render_controls_panel(whisper, transcriber, model_path, selected_language,
                          translate, is_capturing, status_message);
    ImGui::Spacing();
    render_transcript_panel(transcriber);

    ImGui::End();
}

void GUI::render_device_panel(
    const std::vector<AudioDeviceInfo>& devices,
    int& selected_input,
    int& selected_output) {

    if (!ImGui::CollapsingHeader("Audio Devices", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    input_device_names_.clear();
    input_device_indices_.clear();
    output_device_names_.clear();
    output_device_indices_.clear();

    for (const auto& dev : devices) {
        if (dev.max_input_channels > 0) {
            input_device_names_.push_back(dev.name + (dev.is_default_input ? " [Default]" : ""));
            input_device_indices_.push_back(dev.index);
        }
        if (dev.max_output_channels > 0) {
            output_device_names_.push_back(dev.name + (dev.is_default_output ? " [Default]" : ""));
            output_device_indices_.push_back(dev.index);
        }
    }

    if (!input_device_names_.empty()) {
        if (current_input_selection_ >= static_cast<int>(input_device_names_.size()))
            current_input_selection_ = 0;

        if (ImGui::Combo("Input Device", &current_input_selection_,
                         [](void* data, int idx) -> const char* {
                             auto& names = *static_cast<std::vector<std::string>*>(data);
                             return names[idx].c_str();
                         }, &input_device_names_, static_cast<int>(input_device_names_.size()))) {
            selected_input = input_device_indices_[current_input_selection_];
        }
    } else {
        ImGui::TextDisabled("No input devices found");
    }

    if (!output_device_names_.empty()) {
        if (current_output_selection_ >= static_cast<int>(output_device_names_.size()))
            current_output_selection_ = 0;

        if (ImGui::Combo("Output Device (loopback)", &current_output_selection_,
                         [](void* data, int idx) -> const char* {
                             auto& names = *static_cast<std::vector<std::string>*>(data);
                             return names[idx].c_str();
                         }, &output_device_names_, static_cast<int>(output_device_names_.size()))) {
            selected_output = output_device_indices_[current_output_selection_];
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Output capture requires:\n"
                "  Windows: WASAPI loopback support\n"
                "  Linux: PulseAudio/PipeWire monitor\n"
                "  macOS: Virtual audio device (e.g. BlackHole)");
        }
    } else {
        ImGui::TextDisabled("No output devices found");
    }
}

void GUI::render_controls_panel(
    WhisperEngine& whisper,
    Transcriber& transcriber,
    std::string& model_path,
    std::string& selected_language,
    bool& translate,
    bool& is_capturing,
    std::string& status_message) {

    if (!ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Text("Model:");
    ImGui::SameLine();
    char path_buf[512];
    strncpy(path_buf, model_path.c_str(), sizeof(path_buf) - 1);
    path_buf[sizeof(path_buf) - 1] = '\0';
    if (ImGui::InputText("##model_path", path_buf, sizeof(path_buf))) {
        model_path = path_buf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Model")) {
        if (whisper.load_model(model_path)) {
            status_message = "Model loaded: " + whisper.get_model_name();
        } else {
            status_message = "Failed to load model!";
        }
    }
    if (whisper.is_loaded()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "[%s]",
                           whisper.get_model_name().c_str());
    }

    ImGui::Separator();

    int lang_idx = 0;
    for (int i = 0; i < static_cast<int>(languages_.size()); i++) {
        if (languages_[i] == selected_language) {
            lang_idx = i;
            break;
        }
    }

    if (ImGui::Combo("Language", &lang_idx,
                     [](void* data, int idx) -> const char* {
                         auto& langs = *static_cast<std::vector<std::string>*>(data);
                         return langs[idx].c_str();
                     }, &languages_, static_cast<int>(languages_.size()))) {
        selected_language = languages_[lang_idx];
        transcriber.set_language(selected_language);
    }

    if (ImGui::Checkbox("Translate to English", &translate)) {
        transcriber.set_translate(translate);
    }

    ImGui::Separator();

    int mode = (transcriber.mode() == TranscriptionMode::RealTime) ? 0 : 1;
    if (ImGui::RadioButton("Real-time", &mode, 0)) {
        transcriber.set_mode(TranscriptionMode::RealTime);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Record then Transcribe", &mode, 1)) {
        transcriber.set_mode(TranscriptionMode::RecordThenTranscribe);
    }

    ImGui::Separator();

    if (transcriber.mode() == TranscriptionMode::RealTime) {
        if (!is_capturing) {
            if (ImGui::Button("Start Listening", ImVec2(200, 40))) {
                is_capturing = true;
                status_message = "Listening...";
            }
        } else {
            if (ImGui::Button("Stop Listening", ImVec2(200, 40))) {
                is_capturing = false;
                status_message = "Stopped.";
            }
        }
        if (transcriber.is_transcribing()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "Transcribing...");
        }
    } else {
        if (!transcriber.is_recording()) {
            if (ImGui::Button("Record", ImVec2(120, 40))) {
                is_capturing = true;
                status_message = "Recording...";
            }
        } else {
            if (ImGui::Button("Stop Recording", ImVec2(120, 40))) {
                is_capturing = false;
                status_message = "Recording stopped. Click Transcribe.";
            }
            ImGui::SameLine();
            ImGui::Text("Duration: %.1fs", transcriber.recording_progress());
        }

        if (!transcriber.is_recording() && transcriber.recording_progress() > 0.0f) {
            if (ImGui::Button("Transcribe", ImVec2(120, 40))) {
                transcriber.transcribe_recording();
                status_message = "Transcribing recording...";
            }
        }
        if (transcriber.is_transcribing()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "Processing...");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Transcript", ImVec2(140, 40))) {
        transcriber.clear_transcript();
    }

    if (!status_message.empty()) {
        ImGui::Text("%s", status_message.c_str());
    }
}

void GUI::render_transcript_panel(Transcriber& transcriber) {
    if (!ImGui::CollapsingHeader("Transcript", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    std::string text = transcriber.get_transcript();

    ImGui::BeginChild("transcript_scroll", ImVec2(0, 0), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextWrapped("%s", text.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}
