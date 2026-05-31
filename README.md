# Bare-Metal HFT Matching Engine

A single-threaded, highly deterministic OrderBook engineered for strict zero-allocation runtime execution and optimal L1/L3 cache locality. Designed to mimic the core architecture of proprietary trading matching engines.

## ⚡ Core Philosophy
In High-Frequency Trading (HFT), memory shifting and OS-level heap allocations are fatal latency leaks. This engine abandons `std::deque`, `std::map`, and dynamic `new`/`malloc` allocations. Instead, it relies on a pre-allocated flat array and intrusive pointers to achieve absolute $O(1)$ time complexity for all critical path operations (Insertion, Execution, and Middle Queue Cancellations).

---

## 🏗️ Architecture

### 1. The Cache-Aligned Struct
To prevent L1 cache-line splits and maximize memory bandwidth, the `Order` node is strictly constrained to **32 bytes** (allowing exactly two orders to fit into a standard 64-byte L1 cache line). Physical 64-bit memory addresses are replaced with 32-bit array indices. The `price` variable is intentionally omitted, as the order's location in the global tracking array implicitly provides the price context.

```cpp
struct Order {
    uint64_t timestamp;  // 8 bytes 
    uint64_t order_id;   // 8 bytes 
    uint32_t quantity;   // 4 bytes 
    uint32_t next;       // 4 bytes (Index of next order in the pool)
    uint32_t prev;       // 4 bytes (Index of previous order in the pool)
    uint8_t  side;       // 1 byte  (0 for Buy, 1 for Sell)
    uint8_t  _pad[3];    // 3 bytes (Padding to hit exactly 32 bytes)
};