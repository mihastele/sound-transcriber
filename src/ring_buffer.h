#pragma once

#include <atomic>
#include <cstddef>
#include <cstring>
#include <vector>

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : capacity_(capacity), buffer_(capacity), read_pos_(0), write_pos_(0) {}

    bool write(const T* data, size_t count) {
        size_t available = write_available();
        if (count > available) return false;

        size_t wp = write_pos_.load(std::memory_order_relaxed);
        size_t first = std::min(count, capacity_ - wp);
        std::memcpy(buffer_.data() + wp, data, first * sizeof(T));
        if (count > first) {
            std::memcpy(buffer_.data(), data + first, (count - first) * sizeof(T));
        }
        write_pos_.store((wp + count) % capacity_, std::memory_order_release);
        return true;
    }

    size_t read(T* data, size_t count) {
        size_t avail = read_available();
        size_t to_read = std::min(count, avail);
        if (to_read == 0) return 0;

        size_t rp = read_pos_.load(std::memory_order_relaxed);
        size_t first = std::min(to_read, capacity_ - rp);
        std::memcpy(data, buffer_.data() + rp, first * sizeof(T));
        if (to_read > first) {
            std::memcpy(data + first, buffer_.data(), (to_read - first) * sizeof(T));
        }
        read_pos_.store((rp + to_read) % capacity_, std::memory_order_release);
        return to_read;
    }

    size_t read_available() const {
        size_t wp = write_pos_.load(std::memory_order_acquire);
        size_t rp = read_pos_.load(std::memory_order_relaxed);
        return (wp >= rp) ? (wp - rp) : (capacity_ - rp + wp);
    }

    size_t write_available() const {
        return capacity_ - 1 - read_available();
    }

    void reset() {
        read_pos_.store(0, std::memory_order_relaxed);
        write_pos_.store(0, std::memory_order_relaxed);
    }

private:
    size_t capacity_;
    std::vector<T> buffer_;
    std::atomic<size_t> read_pos_;
    std::atomic<size_t> write_pos_;
};
