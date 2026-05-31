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
```

### 2. The Inactive Network (Memory Pool)
The OS heap is bypassed entirely. On system boot, a massive contiguous array (`Order pool[MAX_ORDERS]`) is pre-allocated. 
* **The Free List:** The empty slots are wired into an $O(1)$ Intrusive Singly-Linked Stack.
* **Allocation:** `allocate_order()` grabs the index at the top of the stack in $O(1)$ time. 
* **Deallocation:** `free_order(index)` pushes the empty index back to the top of the stack in $O(1)$ time. Zero data copying occurs.

### 3. The Active Network (The OrderBook)
Price levels are tracked using a Direct Addressing Array (`BuyHead[MAX_PRICE_TICKS]` and `BuyTail[MAX_PRICE_TICKS]`). Each price level operates as an $O(1)$ Array-Backed Doubly-Linked FIFO Queue. 
* **Global Trackers:** Store only the absolute front (locomotive) and back (caboose) of the queue.
* **Implicit Linking:** Middle orders are tracked entirely by the `next` and `prev` intrusive indices inside the memory pool.

---

## ⚙️ Implemented Operations

All operations execute in strict **$O(1)$ Time Complexity** with zero memory shifting.

* `init()`: Wires the initial contiguous memory pool into the Free List.
* `allocate_order()` / `free_order()`: Custom bare-metal memory management engine.
* `add_buy_order(price_tick, new_index)`: Appends an order to the Tail of a price queue while maintaining strict doubly-linked integrity.
* `cancel_buy_order(price_tick, target_index)`: Safely extracts an order from the Head, Tail, or Middle of the queue and returns its memory to the Free List by bypassing its neighbors, avoiding $O(N)$ array shifting.

---

## 🚀 Next Steps (Roadmap)
* **Execution Engine:** Implement `match_order()` to cross incoming aggressive orders against the resting passive book.
* **Lock-Free Bridge:** Engineer a Single-Producer/Single-Consumer (SPSC) lock-free Ring Buffer to feed network packets into the single-threaded matching engine.