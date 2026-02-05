#include "OrderBook.hpp"
#include <iostream>
#include <iomanip>

using namespace orderbook;

void print_order_book_state(const OrderBook& book) {
    std::cout << "\n--- Order Book State ---\n";
    
    auto bid = book.best_bid();
    auto ask = book.best_ask();
    auto spread_val = book.spread();
    
    if (bid) {
        std::cout << "Best Bid: $" << std::fixed << std::setprecision(2) 
                  << (*bid / 100.0) << "\n";
    } else {
        std::cout << "Best Bid: -\n";
    }
    
    if (ask) {
        std::cout << "Best Ask: $" << std::fixed << std::setprecision(2) 
                  << (*ask / 100.0) << "\n";
    } else {
        std::cout << "Best Ask: -\n";
    }
    
    if (spread_val) {
        std::cout << "Spread: $" << std::fixed << std::setprecision(2) 
                  << (*spread_val / 100.0) << "\n";
    }
    
    std::cout << "Bid Depth: " << book.bid_depth() << " levels\n";
    std::cout << "Ask Depth: " << book.ask_depth() << " levels\n";
    std::cout << "Total Orders: " << book.total_orders() << "\n";
    std::cout << "------------------------\n\n";
}

int main() {
    std::cout << "=== High-Performance Order Book Demo ===\n\n";
    
    // Create order book
    OrderBook book;
    
    // Set up trade callback
    book.set_trade_callback([](const Trade& trade) {
        std::cout << "TRADE EXECUTED:\n";
        std::cout << "  Buy Order ID: " << trade.buy_order_id << "\n";
        std::cout << "  Sell Order ID: " << trade.sell_order_id << "\n";
        std::cout << "  Price: $" << std::fixed << std::setprecision(2) 
                  << (trade.price / 100.0) << "\n";
        std::cout << "  Quantity: " << trade.quantity << "\n\n";
    });
    
    // Scenario 1: Build initial order book
    std::cout << "=== Scenario 1: Building Order Book ===\n";
    
    // Add buy orders
    book.add_limit_order(1, Side::BUY, 10000, 100);  // $100.00, qty 100
    book.add_limit_order(2, Side::BUY, 9950, 150);   // $99.50, qty 150
    book.add_limit_order(3, Side::BUY, 9900, 200);   // $99.00, qty 200
    
    // Add sell orders
    book.add_limit_order(4, Side::SELL, 10050, 100); // $100.50, qty 100
    book.add_limit_order(5, Side::SELL, 10100, 150); // $101.00, qty 150
    book.add_limit_order(6, Side::SELL, 10150, 200); // $101.50, qty 200
    
    print_order_book_state(book);
    
    // Scenario 2: Aggressive buy order crosses spread
    std::cout << "=== Scenario 2: Aggressive Buy Order ===\n";
    std::cout << "Adding buy order at $101.00 for 250 shares\n";
    std::cout << "(Should match against sells at $100.50 and $101.00)\n\n";
    
    book.add_limit_order(7, Side::BUY, 10100, 250);
    
    print_order_book_state(book);
    
    // Scenario 3: Market order
    std::cout << "=== Scenario 3: Market Order ===\n";
    std::cout << "Submitting market sell for 50 shares\n\n";
    
    auto filled = book.add_market_order(8, Side::SELL, 50);
    std::cout << "Market order filled: " << filled << " shares\n";
    
    print_order_book_state(book);
    
    // Scenario 4: Order cancellation
    std::cout << "=== Scenario 4: Order Cancellation ===\n";
    std::cout << "Cancelling order ID 3\n\n";
    
    if (book.cancel_order(3)) {
        std::cout << "Order 3 cancelled successfully\n";
    }
    
    print_order_book_state(book);
    
    // Scenario 5: High-volume stress test
    std::cout << "=== Scenario 5: Performance Test ===\n";
    std::cout << "Adding 10,000 orders to test performance...\n\n";
    
    const int NUM_ORDERS = 10000;
    for (int i = 100; i < NUM_ORDERS + 100; ++i) {
        // Alternate between buys and sells
        if (i % 2 == 0) {
            book.add_limit_order(i, Side::BUY, 9500 + (i % 500), 10);
        } else {
            book.add_limit_order(i, Side::SELL, 10500 + (i % 500), 10);
        }
    }
    
    print_order_book_state(book);
    
    // Display metrics
    std::cout << book.metrics().get_summary();
    
    // Calculate throughput
    auto total_ops = book.metrics().total_orders() + 
                     book.metrics().total_cancels() + 
                     book.metrics().total_matches();
    
    std::cout << "\n=== Performance Summary ===\n";
    std::cout << "Total Operations: " << total_ops << "\n";
    
    // Estimate throughput (operations per second)
    auto avg_latency_ns = book.metrics().get_avg_add_latency();
    if (avg_latency_ns > 0) {
        auto ops_per_sec = 1e9 / avg_latency_ns;
        std::cout << "Estimated Throughput: " << std::fixed << std::setprecision(0) 
                  << ops_per_sec << " operations/second\n";
    }
    
    std::cout << "\n=== Demo Complete ===\n";
    
    return 0;
}
