#include "OrderBook.hpp"
#include <gtest/gtest.h>

using namespace orderbook;

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book;
};

TEST_F(OrderBookTest, AddLimitOrder) {
    EXPECT_TRUE(book.add_limit_order(1, Side::BUY, 10000, 100));
    EXPECT_EQ(book.total_orders(), 1);
    
    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(*bid, 10000);
}

TEST_F(OrderBookTest, BestBidAsk) {
    book.add_limit_order(1, Side::BUY, 10000, 100);
    book.add_limit_order(2, Side::SELL, 10100, 100);
    
    auto bid = book.best_bid();
    auto ask = book.best_ask();
    
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(*bid, 10000);
    EXPECT_EQ(*ask, 10100);
}

TEST_F(OrderBookTest, Spread) {
    book.add_limit_order(1, Side::BUY, 10000, 100);
    book.add_limit_order(2, Side::SELL, 10100, 100);
    
    auto spread = book.spread();
    ASSERT_TRUE(spread.has_value());
    EXPECT_EQ(*spread, 100);
}

TEST_F(OrderBookTest, OrderCancellation) {
    book.add_limit_order(1, Side::BUY, 10000, 100);
    EXPECT_EQ(book.total_orders(), 1);
    
    EXPECT_TRUE(book.cancel_order(1));
    EXPECT_EQ(book.total_orders(), 0);
    
    // Try to cancel again
    EXPECT_FALSE(book.cancel_order(1));
}

TEST_F(OrderBookTest, SimpleMatch) {
    int trades = 0;
    book.set_trade_callback([&trades](const Trade& trade) {
        trades++;
        EXPECT_EQ(trade.quantity, 50);
        EXPECT_EQ(trade.price, 10000);
    });
    
    // Add resting sell order
    book.add_limit_order(1, Side::SELL, 10000, 50);
    
    // Add aggressive buy order (should match)
    book.add_limit_order(2, Side::BUY, 10000, 50);
    
    EXPECT_EQ(trades, 1);
    EXPECT_EQ(book.total_orders(), 0);  // Both fully filled
}

TEST_F(OrderBookTest, PartialFill) {
    int trades = 0;
    book.set_trade_callback([&trades](const Trade& trade) {
        trades++;
    });
    
    // Add resting sell order
    book.add_limit_order(1, Side::SELL, 10000, 100);
    
    // Add smaller buy order
    book.add_limit_order(2, Side::BUY, 10000, 50);
    
    EXPECT_EQ(trades, 1);
    EXPECT_EQ(book.total_orders(), 1);  // Sell order partially filled
    EXPECT_EQ(book.get_ask_volume(10000), 50);  // Remaining volume
}

TEST_F(OrderBookTest, PriceTimePriority) {
    std::vector<Order::OrderId> execution_order;
    
    book.set_trade_callback([&execution_order](const Trade& trade) {
        execution_order.push_back(trade.sell_order_id);
    });
    
    // Add multiple sell orders at same price
    book.add_limit_order(1, Side::SELL, 10000, 50);
    book.add_limit_order(2, Side::SELL, 10000, 50);
    book.add_limit_order(3, Side::SELL, 10000, 50);
    
    // Add buy order to match all
    book.add_limit_order(4, Side::BUY, 10000, 150);
    
    // Should match in FIFO order
    ASSERT_EQ(execution_order.size(), 3);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
}

TEST_F(OrderBookTest, MarketOrder) {
    book.add_limit_order(1, Side::SELL, 10000, 50);
    book.add_limit_order(2, Side::SELL, 10100, 50);
    
    // Market buy should fill at best available prices
    auto filled = book.add_market_order(3, Side::BUY, 75);
    
    EXPECT_EQ(filled, 75);
    EXPECT_EQ(book.get_ask_volume(10000), 0);
    EXPECT_EQ(book.get_ask_volume(10100), 25);
}

TEST_F(OrderBookTest, EmptyBookMarketOrder) {
    // Market order on empty book should not fill
    auto filled = book.add_market_order(1, Side::BUY, 100);
    EXPECT_EQ(filled, 0);
}

TEST_F(OrderBookTest, MultiplePriceLevels) {
    // Add orders at different price levels
    book.add_limit_order(1, Side::BUY, 10000, 100);
    book.add_limit_order(2, Side::BUY, 9900, 100);
    book.add_limit_order(3, Side::BUY, 9800, 100);
    
    EXPECT_EQ(book.bid_depth(), 3);
    
    auto best = book.best_bid();
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(*best, 10000);  // Highest bid
}

TEST_F(OrderBookTest, CrossingOrders) {
    int trades = 0;
    book.set_trade_callback([&trades](const Trade&) {
        trades++;
    });
    
    // Add sell order
    book.add_limit_order(1, Side::SELL, 10000, 100);
    
    // Add buy order at higher price (should match)
    book.add_limit_order(2, Side::BUY, 10100, 100);
    
    EXPECT_EQ(trades, 1);
    EXPECT_EQ(book.total_orders(), 0);
}

TEST_F(OrderBookTest, MetricsTracking) {
    book.add_limit_order(1, Side::BUY, 10000, 100);
    book.add_limit_order(2, Side::SELL, 10000, 50);
    book.cancel_order(1);
    
    EXPECT_GT(book.metrics().total_orders(), 0);
    EXPECT_GT(book.metrics().total_matches(), 0);
    EXPECT_GT(book.metrics().total_cancels(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
