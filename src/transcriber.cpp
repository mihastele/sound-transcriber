#include "transcriber.h"
#include <algorithm>
#include <chrono>

Transcriber::Transcriber(WhisperEngine& engine) : engine_(engine) {}

Transcriber::~Transcriber() {
    stop_realtime();
    stop_recording();
}

void Transcriber::set_mode(TranscriptionMode mode) {
    if (mode_ == mode) return;
    if (mode_ == TranscriptionMode::RealTime) stop_realtime();
    if (recording_) stop_recording();
    mode_ = mode;
}

void Transcriber::start_realtime(RingBuffer<float>& ring_buffer, int sample_rate) {
    if (running_) return;
    ring_buffer_ = &ring_buffer;
    sample_rate_ = sample_rate;
    running_ = true;
    realtime_thread_ = std::thread(&Transcriber::realtime_loop, this);
}

void Transcriber::stop_realtime() {
    running_ = false;
    if (realtime_thread_.joinable()) {
        realtime_thread_.join();
    }
}

void Transcriber::start_recording(RingBuffer<float>& ring_buffer, int sample_rate) {
    ring_buffer_ = &ring_buffer;
    sample_rate_ = sample_rate;
    recording_buffer_.clear();
    recording_ = true;
}

void Transcriber::stop_recording() {
    recording_ = false;
}

void Transcriber::transcribe_recording() {
    if (recording_buffer_.empty() || !engine_.is_loaded()) return;

    transcribing_ = true;
    auto results = engine_.transcribe(recording_buffer_, sample_rate_, language_, translate_);

    std::lock_guard<std::mutex> lock(transcript_mutex_);
    for (const auto& r : results) {
        transcript_ += r.text;
    }
    transcribing_ = false;
}

std::string Transcriber::get_transcript() const {
    std::lock_guard<std::mutex> lock(transcript_mutex_);
    return transcript_;
}

void Transcriber::clear_transcript() {
    std::lock_guard<std::mutex> lock(transcript_mutex_);
    transcript_.clear();
}

float Transcriber::recording_progress() const {
    if (sample_rate_ <= 0) return 0.0f;
    return static_cast<float>(recording_buffer_.size()) /
           static_cast<float>(sample_rate_);
}

void Transcriber::realtime_loop() {
    int chunk_size = CHUNK_SECONDS * sample_rate_;
    int overlap_size = OVERLAP_SECONDS * sample_rate_;
    int step_size = chunk_size - overlap_size;

    std::vector<float> buffer;
    buffer.reserve(chunk_size * 2);
    realtime_scratch_.resize(chunk_size);

    while (running_) {
        if (ring_buffer_) {
            size_t avail = ring_buffer_->read_available();
            if (avail > 0) {
                size_t to_read = std::min(avail, realtime_scratch_.size());
                size_t got = ring_buffer_->read(realtime_scratch_.data(), to_read);
                buffer.insert(buffer.end(), realtime_scratch_.begin(),
                              realtime_scratch_.begin() + got);
            }
        }

        if (recording_) {
            recording_buffer_.insert(recording_buffer_.end(),
                                     buffer.begin(), buffer.end());
        }

        if (static_cast<int>(buffer.size()) >= chunk_size && engine_.is_loaded()) {
            std::vector<float> chunk(buffer.end() - chunk_size, buffer.end());

            transcribing_ = true;
            auto results = engine_.transcribe(chunk, sample_rate_, language_, translate_);
            transcribing_ = false;

            if (!results.empty()) {
                std::lock_guard<std::mutex> lock(transcript_mutex_);
                for (const auto& r : results) {
                    if (!r.text.empty() && r.text != " " && r.text != "[BLANK_AUDIO]") {
                        transcript_ += r.text + " ";
                    }
                }
            }

            if (static_cast<int>(buffer.size()) > step_size) {
                buffer.erase(buffer.begin(), buffer.begin() + step_size);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
