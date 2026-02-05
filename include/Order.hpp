#pragma once

#include <cstdint>
#include <chrono>

namespace orderbook {

// Order side (buy or sell)
enum class Side : uint8_t {
    BUY,
    SELL
};

// Order type
enum class OrderType : uint8_t {
    LIMIT,   // Limit order with specified price
    MARKET   // Market order (execute at best available price)
};

// Order structure optimized for cache performance
// Fields are ordered by access frequency and alignment
class Order {
public:
    using OrderId = uint64_t;
    using Price = int64_t;      // Price in fixed-point (e.g., cents)
    using Quantity = uint64_t;
    using Timestamp = std::chrono::nanoseconds;

    // Constructor for limit orders
    Order(OrderId id, Side side, Price price, Quantity quantity)
        : id_(id)
        , price_(price)
        , quantity_(quantity)
        , remaining_quantity_(quantity)
        , side_(side)
        , type_(OrderType::LIMIT)
        , timestamp_(std::chrono::high_resolution_clock::now().time_since_epoch())
        , next_(nullptr)
        , prev_(nullptr)
    {}

    // Constructor for market orders
    Order(OrderId id, Side side, Quantity quantity)
        : id_(id)
        , price_(0)  // Market orders have no price
        , quantity_(quantity)
        , remaining_quantity_(quantity)
        , side_(side)
        , type_(OrderType::MARKET)
        , timestamp_(std::chrono::high_resolution_clock::now().time_since_epoch())
        , next_(nullptr)
        , prev_(nullptr)
    {}

    // Getters
    [[nodiscard]] OrderId id() const { return id_; }
    [[nodiscard]] Side side() const { return side_; }
    [[nodiscard]] OrderType type() const { return type_; }
    [[nodiscard]] Price price() const { return price_; }
    [[nodiscard]] Quantity quantity() const { return quantity_; }
    [[nodiscard]] Quantity remaining_quantity() const { return remaining_quantity_; }
    [[nodiscard]] Timestamp timestamp() const { return timestamp_; }
    [[nodiscard]] bool is_filled() const { return remaining_quantity_ == 0; }

    // Modify remaining quantity (for partial fills)
    void reduce_quantity(Quantity qty) {
        remaining_quantity_ -= qty;
    }

    // Intrusive list pointers (for price level)
    Order* next_;
    Order* prev_;

private:
    OrderId id_;
    Price price_;
    Quantity quantity_;
    Quantity remaining_quantity_;
    Side side_;
    OrderType type_;
    Timestamp timestamp_;
};

} // namespace orderbook
