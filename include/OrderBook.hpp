#pragma once

#include "Order.hpp"
#include "PriceLevel.hpp"
#include "Metrics.hpp"
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>

namespace orderbook {

// Trade information
struct Trade {
    Order::OrderId buy_order_id;
    Order::OrderId sell_order_id;
    Order::Price price;
    Order::Quantity quantity;
    Order::Timestamp timestamp;
};

// Order book with price-time priority matching
class OrderBook {
public:
    using TradeCallback = std::function<void(const Trade&)>;

    OrderBook() 
        : metrics_()
        , trade_callback_(nullptr)
    {}

    // Set callback for trade notifications
    void set_trade_callback(TradeCallback callback) {
        trade_callback_ = std::move(callback);
    }

    // Add a limit order to the book
    // Returns true if order was added, false if immediately matched
    bool add_limit_order(Order::OrderId id, Side side, Order::Price price, Order::Quantity quantity);

    // Add a market order (executes immediately at best available price)
    // Returns quantity that was filled
    Order::Quantity add_market_order(Order::OrderId id, Side side, Order::Quantity quantity);

    // Cancel an existing order
    bool cancel_order(Order::OrderId id);

    // Get best bid price (highest buy price)
    [[nodiscard]] std::optional<Order::Price> best_bid() const;

    // Get best ask price (lowest sell price)
    [[nodiscard]] std::optional<Order::Price> best_ask() const;

    // Get spread (ask - bid)
    [[nodiscard]] std::optional<Order::Price> spread() const;

    // Get total volume at a price level
    [[nodiscard]] Order::Quantity get_bid_volume(Order::Price price) const;
    [[nodiscard]] Order::Quantity get_ask_volume(Order::Price price) const;

    // Get number of price levels
    [[nodiscard]] size_t bid_depth() const { return bids_.size(); }
    [[nodiscard]] size_t ask_depth() const { return asks_.size(); }

    // Get total number of orders
    [[nodiscard]] size_t total_orders() const { return order_map_.size(); }

    // Get metrics
    [[nodiscard]] const Metrics& metrics() const { return metrics_; }
    [[nodiscard]] Metrics& metrics() { return metrics_; }

private:
    // Price levels ordered by price
    // Bids: highest price first (descending)
    std::map<Order::Price, PriceLevel, std::greater<Order::Price>> bids_;
    
    // Asks: lowest price first (ascending)
    std::map<Order::Price, PriceLevel, std::less<Order::Price>> asks_;

    // Order lookup by ID for O(1) cancellation
    std::unordered_map<Order::OrderId, std::unique_ptr<Order>> order_map_;

    // Metrics collection
    Metrics metrics_;

    // Trade callback
    TradeCallback trade_callback_;

    // Internal matching logic
    void match_order(Order* incoming_order);
    
    // Execute a trade between two orders
    void execute_trade(Order* buy_order, Order* sell_order, Order::Price price, Order::Quantity quantity);

    // Remove order from price level and order map
    void remove_order_internal(Order* order);
};

} // namespace orderbook
