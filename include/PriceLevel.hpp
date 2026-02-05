#pragma once

#include "Order.hpp"
#include <cstddef>

namespace orderbook {

// Price level manages all orders at a specific price point
// Uses intrusive doubly-linked list for O(1) insertion and removal
class PriceLevel {
public:
    PriceLevel() 
        : head_(nullptr)
        , tail_(nullptr)
        , total_volume_(0)
        , order_count_(0)
    {}

    ~PriceLevel();

    // Add order to the end of the queue (FIFO)
    void add_order(Order* order);

    // Remove specific order from the queue
    void remove_order(Order* order);

    // Get the first order in the queue (oldest)
    [[nodiscard]] Order* front() const { return head_; }

    // Check if price level is empty
    [[nodiscard]] bool empty() const { return head_ == nullptr; }

    // Get total volume at this price level
    [[nodiscard]] Order::Quantity total_volume() const { return total_volume_; }

    // Get number of orders at this price level
    [[nodiscard]] size_t order_count() const { return order_count_; }

private:
    Order* head_;
    Order* tail_;
    Order::Quantity total_volume_;
    size_t order_count_;
};

} // namespace orderbook
