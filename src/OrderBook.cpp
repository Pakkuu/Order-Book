#include "OrderBook.hpp"
#include "Timer.hpp"
#include <chrono>

namespace orderbook {

bool OrderBook::add_limit_order(Order::OrderId id, Side side, Order::Price price, Order::Quantity quantity) {
    Timer timer;
    
    // Create new order
    auto order = std::make_unique<Order>(id, side, price, quantity);
    Order* order_ptr = order.get();
    
    // Store in order map for O(1) lookup
    order_map_[id] = std::move(order);
    
    // Try to match against opposite side
    match_order(order_ptr);
    
    // If order is completely filled, remove it
    if (order_ptr->is_filled()) {
        order_map_.erase(id);
        metrics_.record_add(timer.elapsed_ns());
        return false;  // Order was fully matched
    }
    
    // Add remaining quantity to appropriate side
    auto& levels = (side == Side::BUY) ? bids_ : asks_;
    levels[price].add_order(order_ptr);
    
    metrics_.record_add(timer.elapsed_ns());
    return true;  // Order was added to book
}

Order::Quantity OrderBook::add_market_order(Order::OrderId id, Side side, Order::Quantity quantity) {
    Timer timer;
    
    // Create market order
    auto order = std::make_unique<Order>(id, side, quantity);
    Order* order_ptr = order.get();
    
    // Store temporarily for matching
    order_map_[id] = std::move(order);
    
    // Match immediately
    match_order(order_ptr);
    
    Order::Quantity filled = quantity - order_ptr->remaining_quantity();
    
    // Remove order (market orders don't rest in the book)
    order_map_.erase(id);
    
    metrics_.record_add(timer.elapsed_ns());
    return filled;
}

bool OrderBook::cancel_order(Order::OrderId id) {
    Timer timer;
    
    auto it = order_map_.find(id);
    if (it == order_map_.end()) {
        return false;  // Order not found
    }
    
    Order* order = it->second.get();
    remove_order_internal(order);
    
    metrics_.record_cancel(timer.elapsed_ns());
    return true;
}

void OrderBook::match_order(Order* incoming_order) {
    Timer timer;
    
    if (!incoming_order || incoming_order->is_filled()) {
        return;
    }
    
    // Get opposite side levels
    auto& opposite_levels = (incoming_order->side() == Side::BUY) ? asks_ : bids_;
    
    // Iterate through price levels until order is filled or no more matches
    while (!incoming_order->is_filled() && !opposite_levels.empty()) {
        auto level_it = opposite_levels.begin();
        PriceLevel& level = level_it->second;
        Order::Price level_price = level_it->first;
        
        // Check if prices cross
        bool can_match = false;
        if (incoming_order->type() == OrderType::MARKET) {
            can_match = true;  // Market orders match at any price
        } else if (incoming_order->side() == Side::BUY) {
            can_match = (incoming_order->price() >= level_price);
        } else {  // SELL
            can_match = (incoming_order->price() <= level_price);
        }
        
        if (!can_match) {
            break;  // No more matching possible
        }
        
        // Match against orders at this price level (FIFO)
        while (!incoming_order->is_filled() && !level.empty()) {
            Order* resting_order = level.front();
            
            // Determine trade quantity
            Order::Quantity trade_qty = std::min(
                incoming_order->remaining_quantity(),
                resting_order->remaining_quantity()
            );
            
            // Determine trade price (resting order's price has priority)
            Order::Price trade_price = resting_order->price();
            
            // Execute the trade
            if (incoming_order->side() == Side::BUY) {
                execute_trade(incoming_order, resting_order, trade_price, trade_qty);
            } else {
                execute_trade(resting_order, incoming_order, trade_price, trade_qty);
            }
            
            // Update quantities
            incoming_order->reduce_quantity(trade_qty);
            resting_order->reduce_quantity(trade_qty);
            
            // Remove filled resting order
            if (resting_order->is_filled()) {
                Order::OrderId resting_id = resting_order->id();
                level.remove_order(resting_order);
                order_map_.erase(resting_id);
            }
        }
        
        // Remove empty price level
        if (level.empty()) {
            opposite_levels.erase(level_it);
        }
    }
    
    // Record match latency if any matching occurred
    if (incoming_order->quantity() > incoming_order->remaining_quantity()) {
        metrics_.record_match(timer.elapsed_ns(), 
                             incoming_order->quantity() - incoming_order->remaining_quantity());
    }
}

void OrderBook::execute_trade(Order* buy_order, Order* sell_order, 
                              Order::Price price, Order::Quantity quantity) {
    // Create trade record
    Trade trade{
        buy_order->id(),
        sell_order->id(),
        price,
        quantity,
        std::chrono::high_resolution_clock::now().time_since_epoch()
    };
    
    // Notify via callback if set
    if (trade_callback_) {
        trade_callback_(trade);
    }
}

void OrderBook::remove_order_internal(Order* order) {
    if (!order) return;
    
    // Remove from price level
    auto& levels = (order->side() == Side::BUY) ? bids_ : asks_;
    auto level_it = levels.find(order->price());
    
    if (level_it != levels.end()) {
        level_it->second.remove_order(order);
        
        // Remove empty price level
        if (level_it->second.empty()) {
            levels.erase(level_it);
        }
    }
    
    // Remove from order map
    order_map_.erase(order->id());
}

std::optional<Order::Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Order::Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<Order::Price> OrderBook::spread() const {
    auto bid = best_bid();
    auto ask = best_ask();
    
    if (!bid || !ask) return std::nullopt;
    return *ask - *bid;
}

Order::Quantity OrderBook::get_bid_volume(Order::Price price) const {
    auto it = bids_.find(price);
    if (it == bids_.end()) return 0;
    return it->second.total_volume();
}

Order::Quantity OrderBook::get_ask_volume(Order::Price price) const {
    auto it = asks_.find(price);
    if (it == asks_.end()) return 0;
    return it->second.total_volume();
}

} // namespace orderbook
