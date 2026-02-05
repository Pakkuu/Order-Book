#pragma once

#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <string>

namespace orderbook {

// Lock-free metrics collection for minimal performance overhead
class Metrics {
public:
    Metrics() 
        : total_orders_(0)
        , total_cancels_(0)
        , total_matches_(0)
        , total_volume_traded_(0)
        , add_latencies_()
        , cancel_latencies_()
        , match_latencies_()
    {
        add_latencies_.reserve(100000);
        cancel_latencies_.reserve(100000);
        match_latencies_.reserve(100000);
    }

    // Record operations
    void record_add(int64_t latency_ns) {
        total_orders_.fetch_add(1, std::memory_order_relaxed);
        add_latencies_.push_back(latency_ns);
    }

    void record_cancel(int64_t latency_ns) {
        total_cancels_.fetch_add(1, std::memory_order_relaxed);
        cancel_latencies_.push_back(latency_ns);
    }

    void record_match(int64_t latency_ns, uint64_t volume) {
        total_matches_.fetch_add(1, std::memory_order_relaxed);
        total_volume_traded_.fetch_add(volume, std::memory_order_relaxed);
        match_latencies_.push_back(latency_ns);
    }

    // Getters
    [[nodiscard]] uint64_t total_orders() const { 
        return total_orders_.load(std::memory_order_relaxed); 
    }
    
    [[nodiscard]] uint64_t total_cancels() const { 
        return total_cancels_.load(std::memory_order_relaxed); 
    }
    
    [[nodiscard]] uint64_t total_matches() const { 
        return total_matches_.load(std::memory_order_relaxed); 
    }
    
    [[nodiscard]] uint64_t total_volume_traded() const { 
        return total_volume_traded_.load(std::memory_order_relaxed); 
    }

    // Calculate latency percentiles
    [[nodiscard]] int64_t get_add_percentile(double percentile) const {
        return calculate_percentile(add_latencies_, percentile);
    }

    [[nodiscard]] int64_t get_cancel_percentile(double percentile) const {
        return calculate_percentile(cancel_latencies_, percentile);
    }

    [[nodiscard]] int64_t get_match_percentile(double percentile) const {
        return calculate_percentile(match_latencies_, percentile);
    }

    // Get average latencies
    [[nodiscard]] double get_avg_add_latency() const {
        return calculate_average(add_latencies_);
    }

    [[nodiscard]] double get_avg_cancel_latency() const {
        return calculate_average(cancel_latencies_);
    }

    [[nodiscard]] double get_avg_match_latency() const {
        return calculate_average(match_latencies_);
    }

    // Reset all metrics
    void reset() {
        total_orders_.store(0, std::memory_order_relaxed);
        total_cancels_.store(0, std::memory_order_relaxed);
        total_matches_.store(0, std::memory_order_relaxed);
        total_volume_traded_.store(0, std::memory_order_relaxed);
        add_latencies_.clear();
        cancel_latencies_.clear();
        match_latencies_.clear();
    }

    // Print summary
    std::string get_summary() const;

private:
    std::atomic<uint64_t> total_orders_;
    std::atomic<uint64_t> total_cancels_;
    std::atomic<uint64_t> total_matches_;
    std::atomic<uint64_t> total_volume_traded_;

    // Latency tracking (note: not thread-safe for recording)
    // In production, use lock-free circular buffers
    std::vector<int64_t> add_latencies_;
    std::vector<int64_t> cancel_latencies_;
    std::vector<int64_t> match_latencies_;

    // Helper functions
    static int64_t calculate_percentile(const std::vector<int64_t>& data, double percentile);
    static double calculate_average(const std::vector<int64_t>& data);
};

} // namespace orderbook
