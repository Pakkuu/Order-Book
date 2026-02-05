#pragma once

#include <chrono>
#include <cstdint>

namespace orderbook {

// High-resolution timer for performance measurement
class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::nanoseconds;

    Timer() : start_(Clock::now()) {}

    // Reset the timer
    void reset() {
        start_ = Clock::now();
    }

    // Get elapsed time in nanoseconds
    [[nodiscard]] int64_t elapsed_ns() const {
        auto end = Clock::now();
        return std::chrono::duration_cast<Duration>(end - start_).count();
    }

    // Get elapsed time in microseconds
    [[nodiscard]] double elapsed_us() const {
        return elapsed_ns() / 1000.0;
    }

    // Get elapsed time in milliseconds
    [[nodiscard]] double elapsed_ms() const {
        return elapsed_ns() / 1000000.0;
    }

private:
    TimePoint start_;
};

// RAII-style scoped timer that automatically records duration
class ScopedTimer {
public:
    explicit ScopedTimer(int64_t& result_ns) 
        : result_(result_ns), timer_() {}

    ~ScopedTimer() {
        result_ = timer_.elapsed_ns();
    }

    // Non-copyable, non-movable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
    int64_t& result_;
    Timer timer_;
};

} // namespace orderbook
