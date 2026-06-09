#pragma once

#include <string>
#include <vector>
#include <mutex>

struct whisper_context;

struct TranscriptionResult {
    std::string text;
    float start_time;
    float end_time;
};

class WhisperEngine {
public:
    WhisperEngine();
    ~WhisperEngine();

    bool load_model(const std::string& model_path);
    bool is_loaded() const;

    std::vector<TranscriptionResult> transcribe(
        const std::vector<float>& audio_pcm,
        int sample_rate,
        const std::string& language = "auto",
        bool translate = false);

    std::string get_model_name() const { return model_name_; }

    static std::vector<std::string> supported_languages();

private:
    whisper_context* ctx_;
    std::string model_name_;
    std::mutex mutex_;
};
