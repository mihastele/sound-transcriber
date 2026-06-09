#pragma once

#include "ring_buffer.h"
#include "whisper_engine.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

enum class TranscriptionMode {
    RealTime,
    RecordThenTranscribe
};

class Transcriber {
public:
    Transcriber(WhisperEngine& engine);
    ~Transcriber();

    void set_mode(TranscriptionMode mode);
    TranscriptionMode mode() const { return mode_; }

    void set_language(const std::string& lang) { language_ = lang; }
    void set_translate(bool translate) { translate_ = translate; }

    void start_realtime(RingBuffer<float>& ring_buffer, int sample_rate);
    void stop_realtime();

    void start_recording(RingBuffer<float>& ring_buffer, int sample_rate);
    void stop_recording();
    void transcribe_recording();

    std::string get_transcript() const;
    void clear_transcript();

    bool is_transcribing() const { return transcribing_; }
    bool is_recording() const { return recording_; }
    float recording_progress() const;

private:
    void realtime_loop();

    WhisperEngine& engine_;
    TranscriptionMode mode_ = TranscriptionMode::RealTime;
    std::string language_ = "auto";
    bool translate_ = false;

    std::atomic<bool> running_{false};
    std::atomic<bool> transcribing_{false};
    std::atomic<bool> recording_{false};

    RingBuffer<float>* ring_buffer_ = nullptr;
    int sample_rate_ = 16000;

    std::thread realtime_thread_;
    std::vector<float> recording_buffer_;
    std::vector<float> realtime_scratch_;

    mutable std::mutex transcript_mutex_;
    std::string transcript_;

    static constexpr int CHUNK_SECONDS = 5;
    static constexpr int OVERLAP_SECONDS = 1;
};
