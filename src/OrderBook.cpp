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
    if (side == Side::BUY) {
        bids_[price].add_order(order_ptr);
    } else {
        asks_[price].add_order(order_ptr);
    }
    
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
    
    // Match based on order side
    if (incoming_order->side() == Side::BUY) {
        // Buy order matches against asks (lowest price first)
        while (!incoming_order->is_filled() && !asks_.empty()) {
            auto level_it = asks_.begin();
            PriceLevel& level = level_it->second;
            Order::Price level_price = level_it->first;
            
            // Check if prices cross (buy price must be >= ask price)
            if (incoming_order->type() != OrderType::MARKET && 
                incoming_order->price() < level_price) {
                break;  // No more matching possible
            }
            
            // Match against orders at this price level (FIFO)
            while (!incoming_order->is_filled() && !level.empty()) {
                Order* resting_order = level.front();
                
                Order::Quantity trade_qty = std::min(
                    incoming_order->remaining_quantity(),
                    resting_order->remaining_quantity()
                );
                
                Order::Price trade_price = resting_order->price();
                execute_trade(incoming_order, resting_order, trade_price, trade_qty);
                
                incoming_order->reduce_quantity(trade_qty);
                resting_order->reduce_quantity(trade_qty);
                
                if (resting_order->is_filled()) {
                    Order::OrderId resting_id = resting_order->id();
                    level.remove_order(resting_order);
                    order_map_.erase(resting_id);
                }
            }
            
            if (level.empty()) {
                asks_.erase(level_it);
            }
        }
    } else {
        // Sell order matches against bids (highest price first)
        while (!incoming_order->is_filled() && !bids_.empty()) {
            auto level_it = bids_.begin();
            PriceLevel& level = level_it->second;
            Order::Price level_price = level_it->first;
            
            // Check if prices cross (sell price must be <= bid price)
            if (incoming_order->type() != OrderType::MARKET && 
                incoming_order->price() > level_price) {
                break;  // No more matching possible
            }
            
            // Match against orders at this price level (FIFO)
            while (!incoming_order->is_filled() && !level.empty()) {
                Order* resting_order = level.front();
                
                Order::Quantity trade_qty = std::min(
                    incoming_order->remaining_quantity(),
                    resting_order->remaining_quantity()
                );
                
                Order::Price trade_price = resting_order->price();
                execute_trade(resting_order, incoming_order, trade_price, trade_qty);
                
                incoming_order->reduce_quantity(trade_qty);
                resting_order->reduce_quantity(trade_qty);
                
                if (resting_order->is_filled()) {
                    Order::OrderId resting_id = resting_order->id();
                    level.remove_order(resting_order);
                    order_map_.erase(resting_id);
                }
            }
            
            if (level.empty()) {
                bids_.erase(level_it);
            }
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
    if (order->side() == Side::BUY) {
        auto level_it = bids_.find(order->price());
        
        if (level_it != bids_.end()) {
            level_it->second.remove_order(order);
            
            // Remove empty price level
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(order->price());
        
        if (level_it != asks_.end()) {
            level_it->second.remove_order(order);
            
            // Remove empty price level
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
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
