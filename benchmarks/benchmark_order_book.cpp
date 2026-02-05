#include "OrderBook.hpp"
#include <benchmark/benchmark.h>
#include <random>

using namespace orderbook;

// Benchmark: Add limit order
static void BM_AddLimitOrder(benchmark::State& state) {
    OrderBook book;
    uint64_t order_id = 1;
    
    for (auto _ : state) {
        book.add_limit_order(order_id++, Side::BUY, 10000, 100);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddLimitOrder);

// Benchmark: Cancel order
static void BM_CancelOrder(benchmark::State& state) {
    OrderBook book;
    
    // Pre-populate with orders
    for (uint64_t i = 0; i < 10000; ++i) {
        book.add_limit_order(i, Side::BUY, 9000 + (i % 1000), 100);
    }
    
    uint64_t order_id = 0;
    for (auto _ : state) {
        state.PauseTiming();
        book.add_limit_order(order_id, Side::BUY, 10000, 100);
        state.ResumeTiming();
        
        book.cancel_order(order_id);
        order_id++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder);

// Benchmark: Order matching (immediate fill)
static void BM_OrderMatching(benchmark::State& state) {
    OrderBook book;
    uint64_t order_id = 1;
    
    // Pre-populate with resting sell orders
    for (uint64_t i = 0; i < 1000; ++i) {
        book.add_limit_order(order_id++, Side::SELL, 10000 + i, 100);
    }
    
    for (auto _ : state) {
        // Add aggressive buy order that will match
        book.add_limit_order(order_id++, Side::BUY, 10500, 50);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderMatching);

// Benchmark: Market order execution
static void BM_MarketOrder(benchmark::State& state) {
    OrderBook book;
    uint64_t order_id = 1;
    
    for (auto _ : state) {
        state.PauseTiming();
        // Add liquidity
        for (int i = 0; i < 10; ++i) {
            book.add_limit_order(order_id++, Side::SELL, 10000 + i * 10, 100);
        }
        state.ResumeTiming();
        
        // Execute market order
        book.add_market_order(order_id++, Side::BUY, 500);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MarketOrder);

// Benchmark: Mixed operations (realistic workload)
static void BM_MixedOperations(benchmark::State& state) {
    OrderBook book;
    uint64_t order_id = 1;
    std::mt19937 rng(12345);
    std::uniform_int_distribution<> op_dist(0, 99);
    std::uniform_int_distribution<> price_dist(9900, 10100);
    std::uniform_int_distribution<> qty_dist(1, 1000);
    
    // Pre-populate
    for (int i = 0; i < 100; ++i) {
        book.add_limit_order(order_id++, Side::BUY, price_dist(rng), qty_dist(rng));
        book.add_limit_order(order_id++, Side::SELL, price_dist(rng), qty_dist(rng));
    }
    
    for (auto _ : state) {
        int op = op_dist(rng);
        
        if (op < 70) {  // 70% add order
            auto side = (op % 2 == 0) ? Side::BUY : Side::SELL;
            book.add_limit_order(order_id++, side, price_dist(rng), qty_dist(rng));
        } else if (op < 90) {  // 20% cancel
            if (order_id > 100) {
                book.cancel_order(order_id - (rng() % 100));
            }
        } else {  // 10% market order
            auto side = (op % 2 == 0) ? Side::BUY : Side::SELL;
            book.add_market_order(order_id++, side, qty_dist(rng));
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MixedOperations);

// Benchmark: Throughput with varying depth
static void BM_VaryingDepth(benchmark::State& state) {
    OrderBook book;
    uint64_t order_id = 1;
    
    // Create specified depth
    int depth = state.range(0);
    for (int i = 0; i < depth; ++i) {
        book.add_limit_order(order_id++, Side::BUY, 9000 + i, 100);
        book.add_limit_order(order_id++, Side::SELL, 11000 + i, 100);
    }
    
    for (auto _ : state) {
        book.add_limit_order(order_id++, Side::BUY, 10000, 100);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VaryingDepth)->Range(8, 8<<10);

// Benchmark: Best bid/ask lookup
static void BM_BestBidAsk(benchmark::State& state) {
    OrderBook book;
    
    // Populate book
    for (uint64_t i = 0; i < 1000; ++i) {
        book.add_limit_order(i, Side::BUY, 9000 + (i % 100), 100);
        book.add_limit_order(i + 1000, Side::SELL, 10000 + (i % 100), 100);
    }
    
    for (auto _ : state) {
        auto bid = book.best_bid();
        auto ask = book.best_ask();
        benchmark::DoNotOptimize(bid);
        benchmark::DoNotOptimize(ask);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BestBidAsk);

BENCHMARK_MAIN();
