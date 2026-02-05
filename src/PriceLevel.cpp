#include "PriceLevel.hpp"

namespace orderbook {

PriceLevel::~PriceLevel() {
    // Note: Orders are owned by OrderBook's order_map_
    // We just clear the links, not delete the orders
    head_ = nullptr;
    tail_ = nullptr;
}

void PriceLevel::add_order(Order* order) {
    if (!order) return;

    // Add to tail (FIFO - new orders go to the back)
    order->prev_ = tail_;
    order->next_ = nullptr;

    if (tail_) {
        tail_->next_ = order;
    } else {
        // List was empty
        head_ = order;
    }
    
    tail_ = order;
    total_volume_ += order->remaining_quantity();
    ++order_count_;
}

void PriceLevel::remove_order(Order* order) {
    if (!order) return;

    // Update links
    if (order->prev_) {
        order->prev_->next_ = order->next_;
    } else {
        // Removing head
        head_ = order->next_;
    }

    if (order->next_) {
        order->next_->prev_ = order->prev_;
    } else {
        // Removing tail
        tail_ = order->prev_;
    }

    total_volume_ -= order->remaining_quantity();
    --order_count_;

    // Clear order's links
    order->prev_ = nullptr;
    order->next_ = nullptr;
}

} // namespace orderbook
