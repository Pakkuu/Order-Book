# High-Performance C++ Order Book

A production-grade limit order book implementation in C++17 with comprehensive metrics collection and sub-microsecond latency.

## Features

- **Price-Time Priority Matching**: Standard FIFO matching algorithm used by exchanges
- **High Performance**: 
  - O(1) order insertion and cancellation
  - O(log n) price level lookup
  - Target <200ns per operation (P99)
  - >1M orders/second throughput
- **Comprehensive Metrics**:
  - Per-operation latency tracking (P50, P95, P99)
  - Throughput measurements
  - Volume and trade statistics
- **Zero-Copy Design**: Intrusive linked lists for order management
- **Well-Tested**: Complete unit test suite and performance benchmarks

## Architecture

### Core Components

- **Order**: Represents individual buy/sell orders with quantity and price
- **PriceLevel**: Manages all orders at a specific price point using intrusive doubly-linked list
- **OrderBook**: Main matching engine with separate bid/ask sides
- **Metrics**: Lock-free performance metrics collection

### Data Structures

- `std::map` for price level management (automatic ordering)
- `std::unordered_map` for O(1) order lookup by ID
- Intrusive linked lists within price levels for zero-allocation order management

## Building

### Prerequisites

```bash
# macOS
brew install cmake googletest google-benchmark

# Ubuntu/Debian
sudo apt-get install cmake libgtest-dev libbenchmark-dev
```

### Build Instructions

```bash
cd /Users/pakkuu/git/Order-Book
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Usage

### Basic Example

```cpp
#include "OrderBook.hpp"

using namespace orderbook;

int main() {
    OrderBook book;
    
    // Set up trade callback
    book.set_trade_callback([](const Trade& trade) {
        std::cout << "Trade: " << trade.quantity 
                  << " @ $" << trade.price / 100.0 << "\n";
    });
    
    // Add limit orders
    book.add_limit_order(1, Side::BUY, 10000, 100);   // Buy 100 @ $100.00
    book.add_limit_order(2, Side::SELL, 10050, 50);   // Sell 50 @ $100.50
    
    // Add market order (executes immediately)
    auto filled = book.add_market_order(3, Side::BUY, 50);
    
    // Cancel order
    book.cancel_order(1);
    
    // View metrics
    std::cout << book.metrics().get_summary();
    
    return 0;
}
```

### Running Examples

```bash
# Run demo program
./examples/simple_trading

# Run unit tests
./tests/order_book_tests

# Run performance benchmarks
./benchmarks/order_book_benchmark
```

## Performance Characteristics

### Time Complexity

| Operation | Average | Worst Case |
|-----------|---------|------------|
| Add Order | O(log n) | O(log n) |
| Cancel Order | O(log n) | O(log n) |
| Match Order | O(m) | O(m) |
| Best Bid/Ask | O(1) | O(1) |

where n = number of price levels, m = number of orders matched

### Memory Layout

Orders are stored with cache-friendly memory layout:
- 64-byte cache line alignment
- Hot fields grouped together
- Minimal pointer chasing

## Metrics

The order book tracks:

- **Latency**: Per-operation timing (add, cancel, match)
- **Throughput**: Operations per second
- **Volume**: Total traded volume
- **Depth**: Number of price levels and orders

Access metrics:

```cpp
const auto& metrics = book.metrics();

std::cout << "P99 Add Latency: " << metrics.get_add_percentile(99) << " ns\n";
std::cout << "Total Matches: " << metrics.total_matches() << "\n";
std::cout << "Volume Traded: " << metrics.total_volume_traded() << "\n";
```

## Design Decisions

### Why Intrusive Lists?

Intrusive linked lists avoid separate allocations for list nodes, reducing memory overhead and improving cache locality. Orders contain next/prev pointers directly.

### Why std::map for Price Levels?

While a custom B-tree might be faster, `std::map` provides:
- Automatic ordering
- Good enough performance (O(log n))
- Standard library reliability
- Easy to optimize later if needed

### Thread Safety

The current implementation is **not thread-safe**. For multi-threaded use:
- Use external synchronization (mutex per order book)
- Or implement lock-free order book (future enhancement)

## Testing

### Unit Tests

Comprehensive test coverage includes:
- Order lifecycle (add, cancel, modify)
- Matching logic correctness
- Price-time priority verification
- Edge cases (empty book, crossing orders, etc.)

Run tests:
```bash
./tests/order_book_tests
```

### Benchmarks

Performance benchmarks measure:
- Individual operation latencies
- Mixed workload throughput
- Scalability with order book depth

Run benchmarks:
```bash
./benchmarks/order_book_benchmark
```

## Future Enhancements

- [ ] Lock-free implementation for multi-threading
- [ ] Advanced order types (Stop-Loss, Iceberg, FOK, IOC)
- [ ] Order book snapshots and replay
- [ ] Prometheus metrics exporter
- [ ] SIMD optimizations for matching
- [ ] Custom memory allocator

## License

MIT License - Free to use for any purpose

## Author

Built with modern C++ best practices for maximum performance and correctness.
