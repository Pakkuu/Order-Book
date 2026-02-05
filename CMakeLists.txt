cmake_minimum_required(VERSION 3.15)
project(OrderBook VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for performance and warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/Order.cpp
    src/PriceLevel.cpp
    src/OrderBook.cpp
    src/Metrics.cpp
)

# Create library
add_library(orderbook ${SOURCES})
target_include_directories(orderbook PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Example executable
add_executable(simple_trading examples/simple_trading.cpp)
target_link_libraries(simple_trading orderbook)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest QUIET)
if(GTest_FOUND)
    add_executable(order_book_tests tests/test_order_book.cpp)
    target_link_libraries(order_book_tests orderbook GTest::GTest GTest::Main)
    add_test(NAME order_book_tests COMMAND order_book_tests)
else()
    message(STATUS "Google Test not found. Unit tests will not be built.")
    message(STATUS "To install: brew install googletest (macOS) or apt-get install libgtest-dev (Linux)")
endif()

# Find Google Benchmark
find_package(benchmark QUIET)
if(benchmark_FOUND)
    add_executable(order_book_benchmark benchmarks/benchmark_order_book.cpp)
    target_link_libraries(order_book_benchmark orderbook benchmark::benchmark)
else()
    message(STATUS "Google Benchmark not found. Benchmarks will not be built.")
    message(STATUS "To install: brew install google-benchmark (macOS)")
endif()

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")
