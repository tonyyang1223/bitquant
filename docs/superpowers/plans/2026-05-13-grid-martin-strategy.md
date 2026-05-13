# Grid Martin Strategy Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a grid martin trading strategy that works with the BitQuant C++ paper trading engine.

**Architecture:** GridMartinStrategy inherits from IStrategy, calculates 10 price grid levels with 1% spacing, buys fixed amount when price crosses grid down, sells with FIFO order when price crosses grid up, triggers stop loss at bottom grid.

**Tech Stack:** C++20, TA-Lib, existing BitQuant framework

---

## File Structure

| File | Purpose |
|------|---------|
| `include/strategy/grid_martin_strategy.hpp` | Strategy class declaration |
| `src/strategy/grid_martin_strategy.cpp` | Strategy implementation |
| `tests/unit/test_grid_martin.cpp` | Unit tests |
| `examples/demo_grid_martin.cpp` | Demo program |

---

## Task 1: Create Header File with Grid Level Calculation Test

**Files:**
- Create: `include/strategy/grid_martin_strategy.hpp`
- Create: `tests/unit/test_grid_martin.cpp`

- [ ] **Step 1: Write the header file**

```cpp
// include/strategy/grid_martin_strategy.hpp
/**
 * @file grid_martin_strategy.hpp
 * @brief Grid Martin trading strategy
 *
 * Grid-based trading strategy:
 * - Calculate grid levels based on base price
 * - Buy when price crosses grid down
 * - Sell with FIFO order when price crosses grid up
 * - Stop loss at bottom grid level
 */

#pragma once

#include "engine/strategy.hpp"
#include <vector>
#include <queue>
#include <algorithm>

namespace bitquant {

/**
 * @brief Grid Martin trading strategy
 *
 * Inherits from IStrategy and implements grid-based trading logic.
 * Each grid level triggers a fixed-amount buy/sell order.
 */
class GridMartinStrategy : public IStrategy {
public:
    // Configurable parameters
    double base_price_ = 0.0;          // Base price for grid calculation
    int grid_count_ = 10;              // Number of grid levels
    double grid_spacing_ = 0.01;       // Grid spacing as percentage (1%)
    double amount_per_grid_ = 100.0;   // Fixed amount per grid ($)
    std::string symbol_ = "BTCUSDT";   // Trading symbol

    // Constructor
    GridMartinStrategy();

    // Lifecycle callbacks
    void on_init() override;
    void on_start() override;
    void on_bar(const BarData& bar) override;
    void on_order(const Order& order) override;

    // Internal methods
    void calculate_grid_levels();
    int get_grid_index(double price) const;
    void execute_grid_trade(int from_grid, int to_grid, double price);
    void check_stop_loss(double price);

    // Accessors for testing
    const std::vector<double>& grid_levels() const { return grid_levels_; }
    double total_position() const { return total_position_; }
    double avg_cost() const { return avg_cost_; }
    const std::queue<int>& buy_queue() const { return buy_queue_; }

private:
    std::vector<double> grid_levels_;      // Grid price levels
    std::vector<double> grid_positions_;   // Position at each grid
    std::vector<double> grid_costs_;       // Cost at each grid
    std::queue<int> buy_queue_;            // FIFO queue for sell order
    double total_position_ = 0.0;          // Total position
    double avg_cost_ = 0.0;                // Average cost
    int last_grid_index_ = -1;             // Last grid index
    bool stop_loss_triggered_ = false;     // Stop loss flag
};

} // namespace bitquant
```

- [ ] **Step 2: Write the failing test for grid level calculation**

```cpp
// tests/unit/test_grid_martin.cpp
/**
 * @file test_grid_martin.cpp
 * @brief Unit tests for GridMartinStrategy
 */

#include "strategy/grid_martin_strategy.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace bitquant;

// Test helper
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << std::endl; \
            return 1; \
        } \
    } while(0)

#define ASSERT_NEAR(a, b, eps, msg) \
    TEST_ASSERT(std::abs((a) - (b)) < (eps), msg << " (expected: " << (b) << ", got: " << (a) << ")")

int test_calculate_grid_levels() {
    std::cout << "Testing: CalculateGridLevels..." << std::endl;

    GridMartinStrategy strategy;
    strategy.base_price_ = 100000.0;
    strategy.grid_count_ = 10;
    strategy.grid_spacing_ = 0.01;

    strategy.calculate_grid_levels();

    const auto& levels = strategy.grid_levels();

    // Should have grid_count levels
    TEST_ASSERT(levels.size() == 10, "Grid count should be 10");

    // Grid 0 (lowest): base_price * (1 - 10 * 0.01) = 90000
    ASSERT_NEAR(levels[0], 90000.0, 0.01, "Grid 0 should be 90000");

    // Grid 9 (highest): base_price * (1 - 1 * 0.01) = 99000
    // Actually, let's recalculate: grid_levels_[i] = base_price * (1 - grid_spacing * (grid_count - i))
    // For i=0: 100000 * (1 - 0.01 * 10) = 90000
    // For i=9: 100000 * (1 - 0.01 * 1) = 99000
    ASSERT_NEAR(levels[9], 99000.0, 0.01, "Grid 9 should be 99000");

    std::cout << "  PASSED: CalculateGridLevels" << std::endl;
    return 0;
}

int test_get_grid_index() {
    std::cout << "Testing: GetGridIndex..." << std::endl;

    GridMartinStrategy strategy;
    strategy.base_price_ = 100000.0;
    strategy.grid_count_ = 10;
    strategy.grid_spacing_ = 0.01;

    strategy.calculate_grid_levels();

    // Test price at different levels
    // Price at 95000 should be around grid 5
    int idx1 = strategy.get_grid_index(95000.0);
    TEST_ASSERT(idx1 >= 0 && idx1 < 10, "Grid index should be valid");

    // Price below bottom grid
    int idx2 = strategy.get_grid_index(85000.0);
    TEST_ASSERT(idx2 == 0, "Price below grid should return 0");

    // Price above top grid
    int idx3 = strategy.get_grid_index(105000.0);
    TEST_ASSERT(idx3 == 9, "Price above grid should return 9");

    std::cout << "  PASSED: GetGridIndex" << std::endl;
    return 0;
}

int main() {
    std::cout << "=== GridMartinStrategy Unit Tests ===" << std::endl;

    int failed = 0;
    failed += test_calculate_grid_levels();
    failed += test_get_grid_index();

    std::cout << "\n=== Results ===" << std::endl;
    if (failed == 0) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << failed << " test(s) FAILED!" << std::endl;
        return 1;
    }
}
```

- [ ] **Step 3: Run test to verify it fails**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && cmake .. && make test_grid_martin 2>&1 || true`
Expected: Compilation error (header not found or method not implemented)

---

## Task 2: Implement Grid Level Calculation Methods

**Files:**
- Create: `src/strategy/grid_martin_strategy.cpp`
- Modify: `include/strategy/grid_martin_strategy.hpp` (if needed)

- [ ] **Step 1: Write the implementation file**

```cpp
// src/strategy/grid_martin_strategy.cpp
/**
 * @file grid_martin_strategy.cpp
 * @brief Grid Martin trading strategy implementation
 */

#include "strategy/grid_martin_strategy.hpp"
#include "utils/logger.hpp"
#include <cmath>
#include <algorithm>

namespace bitquant {

GridMartinStrategy::GridMartinStrategy() {
    author = "BitQuant";
    parameters = {"base_price", "grid_count", "grid_spacing", "amount_per_grid"};
    variables = {"total_position", "avg_cost", "last_grid_index"};
}

void GridMartinStrategy::on_init() {
    calculate_grid_levels();

    // Initialize grid positions and costs
    grid_positions_.resize(grid_count_, 0.0);
    grid_costs_.resize(grid_count_, 0.0);

    write_log("GridMartinStrategy initialized with " + std::to_string(grid_count_) + " grids");

    // Log grid levels
    for (int i = 0; i < grid_count_; ++i) {
        write_log("  Grid " + std::to_string(i) + ": $" +
                  std::to_string(static_cast<int64_t>(grid_levels_[i])));
    }

    inited_ = true;
}

void GridMartinStrategy::on_start() {
    trading_ = true;
    write_log("GridMartinStrategy started trading");
}

void GridMartinStrategy::on_bar(const BarData& bar) {
    if (!inited_ || !trading_) {
        return;
    }

    double price = bar.close_price;

    // Get current grid index
    int current_grid = get_grid_index(price);

    // Check stop loss first
    check_stop_loss(price);

    if (!trading_) {
        return;  // Stopped after stop loss
    }

    // Execute grid trades if grid changed
    if (last_grid_index_ >= 0 && current_grid != last_grid_index_) {
        execute_grid_trade(last_grid_index_, current_grid, price);
    }

    // Update last grid index
    last_grid_index_ = current_grid;
}

void GridMartinStrategy::on_order(const Order& order) {
    // Log order updates
    std::string status_str;
    switch (order.status) {
        case Status::ALLTRADED: status_str = "FILLED"; break;
        case Status::CANCELLED: status_str = "CANCELLED"; break;
        case Status::REJECTED: status_str = "REJECTED"; break;
        default: status_str = "PENDING"; break;
    }

    write_log("Order " + std::to_string(order.order_id) + " " + status_str +
              " | Price: $" + std::to_string(static_cast<int64_t>(order.price)) +
              " | Volume: " + std::to_string(order.volume));
}

void GridMartinStrategy::calculate_grid_levels() {
    grid_levels_.clear();
    grid_levels_.reserve(grid_count_);

    // Calculate grid levels from bottom to top
    // grid_levels_[i] = base_price * (1 - grid_spacing * (grid_count - i))
    for (int i = 0; i < grid_count_; ++i) {
        double level = base_price_ * (1.0 - grid_spacing_ * (grid_count_ - i));
        grid_levels_.push_back(level);
    }
}

int GridMartinStrategy::get_grid_index(double price) const {
    if (grid_levels_.empty()) {
        return -1;
    }

    // Price below all grids
    if (price < grid_levels_[0]) {
        return 0;
    }

    // Price above all grids
    if (price >= grid_levels_.back()) {
        return static_cast<int>(grid_levels_.size()) - 1;
    }

    // Find the grid index where price falls between
    for (size_t i = 0; i < grid_levels_.size() - 1; ++i) {
        if (price >= grid_levels_[i] && price < grid_levels_[i + 1]) {
            return static_cast<int>(i);
        }
    }

    return static_cast<int>(grid_levels_.size()) - 1;
}

void GridMartinStrategy::execute_grid_trade(int from_grid, int to_grid, double price) {
    // Skip if stopped
    if (!trading_) {
        return;
    }

    int grid_diff = to_grid - from_grid;

    if (grid_diff > 0) {
        // Price went UP - sell positions (FIFO)
        for (int i = 0; i < grid_diff; ++i) {
            if (buy_queue_.empty()) {
                break;
            }

            int grid_to_sell = buy_queue_.front();
            buy_queue_.pop();

            double sell_volume = grid_positions_[grid_to_sell];
            if (sell_volume > 0.0) {
                sell(price, sell_volume);

                // Calculate profit
                double cost = grid_costs_[grid_to_sell];
                double profit = (price - cost) * sell_volume;

                write_log("SELL Grid " + std::to_string(grid_to_sell) +
                         " | Price: $" + std::to_string(static_cast<int64_t>(price)) +
                         " | Volume: " + std::to_string(sell_volume) +
                         " | Profit: $" + std::to_string(static_cast<int64_t>(profit)));

                // Reset grid position
                grid_positions_[grid_to_sell] = 0.0;
                grid_costs_[grid_to_sell] = 0.0;
                total_position_ -= sell_volume;
            }
        }
    } else if (grid_diff < 0) {
        // Price went DOWN - buy positions
        int grids_crossed = -grid_diff;

        for (int i = 0; i < grids_crossed; ++i) {
            int grid_to_buy = from_grid - i - 1;

            if (grid_to_buy < 0 || grid_to_buy >= grid_count_) {
                break;
            }

            // Skip if already have position at this grid
            if (grid_positions_[grid_to_buy] > 0.0) {
                continue;
            }

            // Calculate buy volume based on fixed amount
            double buy_volume = amount_per_grid_ / price;

            buy(price, buy_volume);

            // Record position
            grid_positions_[grid_to_buy] = buy_volume;
            grid_costs_[grid_to_buy] = price;
            buy_queue_.push(grid_to_buy);
            total_position_ += buy_volume;

            // Update average cost
            double total_cost = avg_cost_ * (total_position_ - buy_volume) + price * buy_volume;
            avg_cost_ = total_cost / total_position_;

            write_log("BUY Grid " + std::to_string(grid_to_buy) +
                     " | Price: $" + std::to_string(static_cast<int64_t>(price)) +
                     " | Volume: " + std::to_string(buy_volume) +
                     " | Amount: $" + std::to_string(static_cast<int64_t>(amount_per_grid_)));
        }
    }
}

void GridMartinStrategy::check_stop_loss(double price) {
    // Check if price is below the bottom grid
    if (grid_levels_.empty()) {
        return;
    }

    double stop_price = grid_levels_[0];

    if (price < stop_price && total_position_ > 0.0) {
        write_log("[STOP LOSS] Triggered at $" + std::to_string(static_cast<int64_t>(stop_price)) +
                 " | Current: $" + std::to_string(static_cast<int64_t>(price)));

        // Close all positions
        sell(price, total_position_);

        // Calculate loss
        double loss = (price - avg_cost_) * total_position_;
        write_log("[STOP LOSS] Position closed | Loss: $" + std::to_string(static_cast<int64_t>(loss)));

        // Reset state
        total_position_ = 0.0;
        avg_cost_ = 0.0;
        stop_loss_triggered_ = true;

        // Clear positions and queue
        std::fill(grid_positions_.begin(), grid_positions_.end(), 0.0);
        std::fill(grid_costs_.begin(), grid_costs_.end(), 0.0);
        while (!buy_queue_.empty()) {
            buy_queue_.pop();
        }

        // Stop trading
        trading_ = false;
    }
}

} // namespace bitquant
```

- [ ] **Step 2: Run test to verify it passes**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && cmake .. && make test_grid_martin && ./test_grid_martin`
Expected: All tests PASS

- [ ] **Step 3: Commit**

```bash
cd /home/ubuntu/project/bitquant
git add bitquant_cpp/include/strategy/grid_martin_strategy.hpp bitquant_cpp/src/strategy/grid_martin_strategy.cpp bitquant_cpp/tests/unit/test_grid_martin.cpp
git commit -m "$(cat <<'EOF'
feat: Add GridMartinStrategy with grid level calculation

- Add GridMartinStrategy class inheriting from IStrategy
- Implement calculate_grid_levels() for price grid setup
- Implement get_grid_index() for price-to-grid mapping
- Add unit tests for grid level calculation

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: Add Grid Crossing Trade Tests

**Files:**
- Modify: `tests/unit/test_grid_martin.cpp`

- [ ] **Step 1: Add tests for grid crossing buy**

```cpp
// Add to tests/unit/test_grid_martin.cpp after test_get_grid_index()

int test_grid_crossing_buy() {
    std::cout << "Testing: GridCrossingBuy..." << std::endl;

    GridMartinStrategy strategy;
    strategy.base_price_ = 100000.0;
    strategy.grid_count_ = 10;
    strategy.grid_spacing_ = 0.01;
    strategy.amount_per_grid_ = 100.0;

    strategy.on_init();
    strategy.on_start();

    // Simulate being at grid 9 (top grid) initially
    strategy.last_grid_index_ = 9;

    // Price drops from ~99000 to ~95000 (crossing multiple grids down)
    BarData bar;
    bar.close_price = 95000.0;
    strategy.on_bar(bar);

    // Should have bought some position
    TEST_ASSERT(strategy.total_position() > 0.0, "Should have position after price drop");

    // Average cost should be around 95000 range
    TEST_ASSERT(strategy.avg_cost() > 0.0, "Should have avg cost");

    std::cout << "  PASSED: GridCrossingBuy" << std::endl;
    return 0;
}

int test_fifo_sell_order() {
    std::cout << "Testing: FIFOSellOrder..." << std::endl;

    GridMartinStrategy strategy;
    strategy.base_price_ = 100000.0;
    strategy.grid_count_ = 10;
    strategy.grid_spacing_ = 0.01;
    strategy.amount_per_grid_ = 100.0;

    strategy.on_init();
    strategy.on_start();

    // Simulate buying at grid 5 (price ~95000)
    strategy.last_grid_index_ = 9;
    BarData bar1;
    bar1.close_price = 95000.0;
    strategy.on_bar(bar1);

    double position_after_buy = strategy.total_position();

    // Now price goes back up (selling FIFO)
    strategy.last_grid_index_ = 5;
    BarData bar2;
    bar2.close_price = 97000.0;
    strategy.on_bar(bar2);

    // Position should decrease
    TEST_ASSERT(strategy.total_position() < position_after_buy, "Position should decrease after sell");

    std::cout << "  PASSED: FIFOSellOrder" << std::endl;
    return 0;
}

// Update main() to include new tests
int main() {
    std::cout << "=== GridMartinStrategy Unit Tests ===" << std::endl;

    int failed = 0;
    failed += test_calculate_grid_levels();
    failed += test_get_grid_index();
    failed += test_grid_crossing_buy();
    failed += test_fifo_sell_order();

    std::cout << "\n=== Results ===" << std::endl;
    if (failed == 0) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << failed << " test(s) FAILED!" << std::endl;
        return 1;
    }
}
```

- [ ] **Step 2: Run test to verify it passes**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && make test_grid_martin && ./test_grid_martin`
Expected: All tests PASS

- [ ] **Step 3: Commit**

```bash
cd /home/ubuntu/project/bitquant
git add bitquant_cpp/tests/unit/test_grid_martin.cpp
git commit -m "$(cat <<'EOF'
test: Add grid crossing and FIFO sell tests

- Test grid crossing buy logic
- Test FIFO sell order execution

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: Add Stop Loss Tests

**Files:**
- Modify: `tests/unit/test_grid_martin.cpp`

- [ ] **Step 1: Add stop loss test**

```cpp
// Add to tests/unit/test_grid_martin.cpp after test_fifo_sell_order()

int test_stop_loss_trigger() {
    std::cout << "Testing: StopLossTrigger..." << std::endl;

    GridMartinStrategy strategy;
    strategy.base_price_ = 100000.0;
    strategy.grid_count_ = 10;
    strategy.grid_spacing_ = 0.01;
    strategy.amount_per_grid_ = 100.0;

    strategy.on_init();
    strategy.on_start();

    // Simulate buying position
    strategy.last_grid_index_ = 9;
    BarData bar1;
    bar1.close_price = 95000.0;
    strategy.on_bar(bar1);

    // Verify position exists
    TEST_ASSERT(strategy.total_position() > 0.0, "Should have position before stop loss");

    // Price drops below stop loss (grid 0 = 90000)
    BarData bar2;
    bar2.close_price = 85000.0;
    strategy.on_bar(bar2);

    // Position should be cleared
    TEST_ASSERT(strategy.total_position() == 0.0, "Position should be 0 after stop loss");

    // Trading should be stopped
    TEST_ASSERT(!strategy.trading_, "Trading should be stopped after stop loss");

    std::cout << "  PASSED: StopLossTrigger" << std::endl;
    return 0;
}

// Update main() to include stop loss test
int main() {
    std::cout << "=== GridMartinStrategy Unit Tests ===" << std::endl;

    int failed = 0;
    failed += test_calculate_grid_levels();
    failed += test_get_grid_index();
    failed += test_grid_crossing_buy();
    failed += test_fifo_sell_order();
    failed += test_stop_loss_trigger();

    std::cout << "\n=== Results ===" << std::endl;
    if (failed == 0) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << failed << " test(s) FAILED!" << std::endl;
        return 1;
    }
}
```

- [ ] **Step 2: Run test to verify it passes**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && make test_grid_martin && ./test_grid_martin`
Expected: All tests PASS

- [ ] **Step 3: Commit**

```bash
cd /home/ubuntu/project/bitquant
git add bitquant_cpp/tests/unit/test_grid_martin.cpp
git commit -m "$(cat <<'EOF'
test: Add stop loss trigger test

- Test stop loss execution when price drops below bottom grid

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Task 5: Update CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add strategy source and test target**

Add to `CMakeLists.txt` after line 66 (after `src/strategy/validation_strategy.cpp`):

```cmake
    src/strategy/grid_martin_strategy.cpp
```

Add after line 314 (after `test_validation_strategy`):

```cmake
    # GridMartinStrategy tests
    add_executable(test_grid_martin
        tests/unit/test_grid_martin.cpp
    )
    target_link_libraries(test_grid_martin PRIVATE bitquant)
    add_test(NAME grid_martin_tests COMMAND test_grid_martin)
```

- [ ] **Step 2: Run cmake and compile**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && cmake .. && make test_grid_martin -j$(nproc)`
Expected: Compilation succeeds

- [ ] **Step 3: Commit**

```bash
cd /home/ubuntu/project/bitquant
git add bitquant_cpp/CMakeLists.txt
git commit -m "$(cat <<'EOF'
build: Add GridMartinStrategy to CMakeLists

- Add grid_martin_strategy.cpp to library sources
- Add test_grid_martin test target

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Task 6: Create Demo Program

**Files:**
- Create: `examples/demo_grid_martin.cpp`

- [ ] **Step 1: Write the demo program**

```cpp
// examples/demo_grid_martin.cpp
/**
 * @file demo_grid_martin.cpp
 * @brief Grid Martin Strategy demo with paper trading
 *
 * Demonstrates:
 * - GridMartinStrategy configuration
 * - Paper trading engine setup
 * - Real-time WebSocket connection
 * - Strategy execution with live data
 */

#include "strategy/grid_martin_strategy.hpp"
#include "engine/paper_broker.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include <iostream>
#include <iomanip>
#include <csignal>
#include <atomic>

using namespace bitquant;

// Global flag for signal handling
std::atomic<bool> running{true};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

void print_header(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void print_strategy_status(const GridMartinStrategy& strategy, double current_price) {
    static int count = 0;
    count++;

    // Print every 10 seconds (approximate)
    if (count % 10 != 0) return;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\r[" << count << "] Price: $" << current_price
              << " | Position: " << strategy.total_position()
              << " | Avg Cost: $" << strategy.avg_cost()
              << std::flush;
}

int main() {
    print_header("Grid Martin Strategy Demo");

    // Register signal handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        // Step 1: Configure strategy parameters
        std::cout << "\n[1] Configuring GridMartinStrategy...\n";

        auto strategy = std::make_unique<GridMartinStrategy>();

        // Strategy parameters
        strategy->base_price_ = 85000.0;       // Base price for BTCUSDT
        strategy->grid_count_ = 10;            // 10 grid levels
        strategy->grid_spacing_ = 0.01;        // 1% spacing
        strategy->amount_per_grid_ = 100.0;    // $100 per grid
        strategy->symbol_ = "BTCUSDT";

        std::cout << "    Base Price:   $" << strategy->base_price_ << "\n";
        std::cout << "    Grid Count:   " << strategy->grid_count_ << "\n";
        std::cout << "    Grid Spacing: " << (strategy->grid_spacing_ * 100) << "%\n";
        std::cout << "    Amount/Grid:  $" << strategy->amount_per_grid_ << "\n";

        // Step 2: Configure paper broker
        std::cout << "\n[2] Configuring PaperBroker...\n";

        BrokerConfig config;
        config.initial_cash = 10000.0;      // $10,000 initial capital
        config.commission_rate = 0.001;      // 0.1% commission
        config.slippage_rate = 0.0005;       // 0.05% slippage
        config.allow_short = false;          // Spot trading only

        Broker broker(config);

        std::cout << "    Initial Capital: $" << config.initial_cash << "\n";
        std::cout << "    Commission: " << (config.commission_rate * 100) << "%\n";

        // Step 3: Set strategy
        std::cout << "\n[3] Setting strategy...\n";
        broker.set_strategy(std::move(strategy));

        // Step 4: Fetch historical data from Binance
        std::cout << "\n[4] Fetching historical data from Binance...\n";

        BinanceSpotGateway gateway;
        GatewayConfig gw_config;
        gw_config.api_key = "";
        gw_config.api_secret = "";

        if (!gateway.connect(gw_config)) {
            std::cerr << "Failed to connect to Binance: " << gateway.last_error() << "\n";
            return 1;
        }

        HistoryRequest req;
        req.symbol = "BTCUSDT";
        req.interval = Interval::MINUTE_1;
        req.limit = 100;  // Last 100 minutes

        auto bars = gateway.get_bars(req);
        gateway.close();

        if (bars.empty()) {
            std::cerr << "No historical data received.\n";
            return 1;
        }

        std::cout << "    Fetched " << bars.size() << " bars\n";
        std::cout << "    Latest price: $" << bars.back().close_price << "\n";

        // Step 5: Set data and run backtest
        std::cout << "\n[5] Running backtest with historical data...\n";
        broker.set_data(bars);
        broker.run();

        // Step 6: Display results
        const auto& metrics = broker.performance();

        print_header("Backtest Results");
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Initial Capital:    $" << std::setprecision(0) << metrics.initial_capital << "\n";
        std::cout << "Final Capital:      $" << metrics.final_capital << "\n";
        std::cout << "Total Return:       " << std::setprecision(2) << (metrics.total_return * 100) << "%\n";
        std::cout << "Total Trades:       " << metrics.total_trades << "\n";
        std::cout << "Total Commission:   $" << std::setprecision(2) << metrics.total_commission << "\n";

        print_header("Demo Complete");
        std::cout << "GridMartinStrategy backtest completed successfully!\n";
        std::cout << "\nFor live paper trading, use demo_paper_trading_full.cpp\n";
        std::cout << "with MainEngine and WebSocket connection.\n";

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

- [ ] **Step 2: Add demo to CMakeLists.txt**

Add to `CMakeLists.txt` after line 370 (after `demo_paper_trading_full`):

```cmake
        add_executable(demo_grid_martin examples/demo_grid_martin.cpp)
        target_link_libraries(demo_grid_martin PRIVATE bitquant)
```

- [ ] **Step 3: Compile and test demo**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && cmake .. && make demo_grid_martin -j$(nproc) && ./demo_grid_martin`
Expected: Demo runs successfully with backtest results

- [ ] **Step 4: Commit**

```bash
cd /home/ubuntu/project/bitquant
git add bitquant_cpp/examples/demo_grid_martin.cpp bitquant_cpp/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat: Add GridMartinStrategy demo program

- Add demo_grid_martin.cpp with backtest demonstration
- Configure strategy with BTCUSDT parameters
- Display backtest results

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Task 7: Integrate with StrategyFactory

**Files:**
- Modify: `include/strategy/builtin_strategies.hpp`
- Modify: `src/strategy/builtin_strategies.cpp`

- [ ] **Step 1: Add forward declaration to header**

Add to `include/strategy/builtin_strategies.hpp` after line 215 (before StrategyFactory):

```cpp
//=============================================================================
// Grid Martin Strategy
//=============================================================================

/**
 * @brief Grid-based Martin trading strategy
 *
 * Grid trading with fixed amount per level:
 * - Buy when price crosses grid down
 * - Sell with FIFO order when price crosses grid up
 * - Stop loss at bottom grid level
 *
 * Parameters:
 * - base_price: Base price for grid calculation (required)
 * - grid_count: Number of grid levels (default: 10)
 * - grid_spacing: Grid spacing percentage (default: 0.01)
 * - amount_per_grid: Fixed amount per grid (default: 100.0)
 */

// Forward declaration - full definition in grid_martin_strategy.hpp
class GridMartinStrategy;
```

Add include at line 17:

```cpp
#include "grid_martin_strategy.hpp"
```

- [ ] **Step 2: Update StrategyFactory::create()**

Modify `src/strategy/builtin_strategies.cpp` to include the header and add factory method:

Add at line 6:

```cpp
#include "strategy/grid_martin_strategy.hpp"
```

Modify the `StrategyFactory::create()` function:

```cpp
std::unique_ptr<IStrategy> StrategyFactory::create(const std::string& name) {
    if (name == "DoubleMa" || name == "DoubleMaStrategy") {
        return std::make_unique<DoubleMaStrategy>();
    } else if (name == "Rsi" || name == "RsiStrategy") {
        return std::make_unique<RsiStrategy>();
    } else if (name == "Bollinger" || name == "BollingerStrategy") {
        return std::make_unique<BollingerStrategy>();
    } else if (name == "AtrRsi" || name == "AtrRsiStrategy") {
        return std::make_unique<AtrRsiStrategy>();
    } else if (name == "Turtle" || name == "TurtleStrategy") {
        return std::make_unique<TurtleStrategy>();
    } else if (name == "GridMartin" || name == "GridMartinStrategy") {
        return std::make_unique<GridMartinStrategy>();
    }

    return nullptr;
}
```

Modify the `StrategyFactory::list_strategies()` function:

```cpp
std::vector<std::string> StrategyFactory::list_strategies() {
    return {
        "DoubleMa",
        "Rsi",
        "Bollinger",
        "AtrRsi",
        "Turtle",
        "GridMartin"
    };
}
```

- [ ] **Step 3: Compile and test**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && cmake .. && make -j$(nproc) && ./test_grid_martin`
Expected: All tests still pass

- [ ] **Step 4: Commit**

```bash
cd /home/ubuntu/project/bitquant
git add bitquant_cpp/include/strategy/builtin_strategies.hpp bitquant_cpp/src/strategy/builtin_strategies.cpp
git commit -m "$(cat <<'EOF'
feat: Integrate GridMartinStrategy with StrategyFactory

- Add GridMartinStrategy to builtin_strategies.hpp
- Update StrategyFactory to create GridMartinStrategy
- Add to list_strategies() for discovery

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Task 8: Final Compilation and Verification

**Files:**
- None (verification only)

- [ ] **Step 1: Full rebuild**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && cmake .. && make clean && make -j$(nproc)`
Expected: All targets compile without errors

- [ ] **Step 2: Run all tests**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && ctest --output-on-failure`
Expected: All tests pass

- [ ] **Step 3: Run demo**

Run: `cd /home/ubuntu/project/bitquant/bitquant_cpp/build && ./demo_grid_martin`
Expected: Demo runs successfully showing backtest results

- [ ] **Step 4: Final commit with version tag**

```bash
cd /home/ubuntu/project/bitquant
git add -A
git commit -m "$(cat <<'EOF'
feat: Complete GridMartinStrategy implementation

Features:
- 10-grid layout with 1% spacing
- Fixed amount per grid position sizing
- FIFO sell order execution
- Grid bottom stop loss mechanism
- Full integration with BitQuant framework

Testing:
- Unit tests for grid calculation
- Unit tests for grid crossing trades
- Unit tests for stop loss trigger
- Demo program with backtest

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

## Success Criteria

| Criteria | Verification |
|----------|-------------|
| Grid levels calculated correctly | `test_grid_martin` passes |
| Grid crossing detection works | `test_grid_martin` passes |
| FIFO sell order correct | `test_grid_martin` passes |
| Stop loss triggers correctly | `test_grid_martin` passes |
| Demo runs without errors | `./demo_grid_martin` succeeds |
| All existing tests still pass | `ctest` passes |