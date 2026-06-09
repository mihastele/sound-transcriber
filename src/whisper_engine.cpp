#include "whisper_engine.h"
#include <whisper.h>
#include <algorithm>
#include <filesystem>

WhisperEngine::WhisperEngine() : ctx_(nullptr) {}

WhisperEngine::~WhisperEngine() {
    if (ctx_) {
        whisper_free(ctx_);
        ctx_ = nullptr;
    }
}

bool WhisperEngine::load_model(const std::string& model_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (ctx_) {
        whisper_free(ctx_);
        ctx_ = nullptr;
    }

    ctx_ = whisper_init_from_file(model_path.c_str());
    if (!ctx_) return false;

    model_name_ = std::filesystem::path(model_path).stem().string();
    return true;
}

bool WhisperEngine::is_loaded() const {
    return ctx_ != nullptr;
}

std::vector<TranscriptionResult> WhisperEngine::transcribe(
    const std::vector<float>& audio_pcm,
    int sample_rate,
    const std::string& language,
    bool translate) {

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TranscriptionResult> results;

    if (!ctx_ || audio_pcm.empty()) return results;

    std::vector<float> audio = audio_pcm;
    if (sample_rate != 16000) {
        size_t new_size = audio.size() * 16000 / sample_rate;
        std::vector<float> resampled(new_size);
        for (size_t i = 0; i < new_size; i++) {
            float src_idx = static_cast<float>(i) * sample_rate / 16000.0f;
            size_t idx = static_cast<size_t>(src_idx);
            if (idx + 1 < audio.size()) {
                float frac = src_idx - static_cast<float>(idx);
                resampled[i] = audio[idx] * (1.0f - frac) + audio[idx + 1] * frac;
            } else if (idx < audio.size()) {
                resampled[i] = audio[idx];
            }
        }
        audio = std::move(resampled);
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_realtime = false;
    params.print_progress = false;
    params.print_timestamps = false;
    params.print_special = false;
    params.translate = translate;
    params.single_segment = false;
    params.no_timestamps = true;

    if (language == "auto") {
        params.language = "auto";
    } else {
        params.language = language.c_str();
    }

    params.n_threads = std::max(1u, std::thread::hardware_concurrency() / 2);

    int ret = whisper_full(ctx_, params, audio.data(), static_cast<int>(audio.size()));
    if (ret != 0) return results;

    int n_segments = whisper_full_n_segments(ctx_);
    for (int i = 0; i < n_segments; i++) {
        TranscriptionResult seg;
        seg.text = whisper_full_get_segment_text(ctx_, i);
        seg.start_time = static_cast<float>(whisper_full_get_segment_t0(ctx_, i)) / 100.0f;
        seg.end_time = static_cast<float>(whisper_full_get_segment_t1(ctx_, i)) / 100.0f;
        results.push_back(seg);
    }

    return results;
}

std::vector<std::string> WhisperEngine::supported_languages() {
    return {
        "auto", "en", "zh", "de", "es", "ru", "ko", "fr", "ja", "pt",
        "tr", "pl", "ca", "nl", "ar", "sv", "it", "id", "hi", "fi",
        "vi", "he", "uk", "el", "ms", "cs", "ro", "da", "hu", "ta",
        "no", "th", "ur", "hr", "bg", "lt", "la", "mi", "ml", "cy",
        "sk", "te", "fa", "lv", "bn", "sr", "az", "sl", "kn", "et",
        "mk", "br", "eu", "is", "hy", "ne", "mn", "bs", "kk", "sq",
        "sw", "gl", "mr", "pa", "si", "km", "sn", "yo", "so", "af",
        "oc", "ka", "be", "tg", "sd", "gu", "am", "yi", "lo", "uz",
        "fo", "ht", "ps", "tk", "nn", "mt", "sa", "lb", "my", "bo",
        "tl", "mg", "as", "tt", "haw", "ln", "ha", "ba", "jw", "su"
    };
}
