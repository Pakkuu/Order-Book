#include "Metrics.hpp"
#include <sstream>
#include <iomanip>
#include <numeric>

namespace orderbook {

int64_t Metrics::calculate_percentile(const std::vector<int64_t>& data, double percentile) {
    if (data.empty()) return 0;
    
    // Create a sorted copy
    auto sorted = data;
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>((percentile / 100.0) * sorted.size());
    if (index >= sorted.size()) index = sorted.size() - 1;
    
    return sorted[index];
}

double Metrics::calculate_average(const std::vector<int64_t>& data) {
    if (data.empty()) return 0.0;
    
    int64_t sum = std::accumulate(data.begin(), data.end(), 0LL);
    return static_cast<double>(sum) / data.size();
}

std::string Metrics::get_summary() const {
    std::ostringstream oss;
    
    oss << "\n=== Order Book Metrics ===\n\n";
    
    // Counters
    oss << "Operations:\n";
    oss << "  Total Orders Added: " << total_orders() << "\n";
    oss << "  Total Cancellations: " << total_cancels() << "\n";
    oss << "  Total Matches: " << total_matches() << "\n";
    oss << "  Total Volume Traded: " << total_volume_traded() << "\n\n";
    
    // Add order latencies
    if (!add_latencies_.empty()) {
        oss << "Add Order Latency (nanoseconds):\n";
        oss << "  Average: " << std::fixed << std::setprecision(2) << get_avg_add_latency() << " ns\n";
        oss << "  P50: " << get_add_percentile(50) << " ns\n";
        oss << "  P95: " << get_add_percentile(95) << " ns\n";
        oss << "  P99: " << get_add_percentile(99) << " ns\n";
        oss << "  Max: " << get_add_percentile(100) << " ns\n\n";
    }
    
    // Cancel order latencies
    if (!cancel_latencies_.empty()) {
        oss << "Cancel Order Latency (nanoseconds):\n";
        oss << "  Average: " << std::fixed << std::setprecision(2) << get_avg_cancel_latency() << " ns\n";
        oss << "  P50: " << get_cancel_percentile(50) << " ns\n";
        oss << "  P95: " << get_cancel_percentile(95) << " ns\n";
        oss << "  P99: " << get_cancel_percentile(99) << " ns\n";
        oss << "  Max: " << get_cancel_percentile(100) << " ns\n\n";
    }
    
    // Match order latencies
    if (!match_latencies_.empty()) {
        oss << "Match Order Latency (nanoseconds):\n";
        oss << "  Average: " << std::fixed << std::setprecision(2) << get_avg_match_latency() << " ns\n";
        oss << "  P50: " << get_match_percentile(50) << " ns\n";
        oss << "  P95: " << get_match_percentile(95) << " ns\n";
        oss << "  P99: " << get_match_percentile(99) << " ns\n";
        oss << "  Max: " << get_match_percentile(100) << " ns\n\n";
    }
    
    oss << "==========================\n";
    
    return oss.str();
}

} // namespace orderbook
