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

    // New grid calculation: base_price * (1 + grid_spacing * (i - half_grid))
    // half_grid = 5, so:
    // Grid 0 (lowest): 100000 * (1 + 0.01 * (0 - 5)) = 100000 * 0.95 = 95000
    ASSERT_NEAR(levels[0], 95000.0, 0.01, "Grid 0 should be 95000");

    // Grid 5 (center): 100000 * (1 + 0.01 * (5 - 5)) = 100000 * 1.00 = 100000
    ASSERT_NEAR(levels[5], 100000.0, 0.01, "Grid 5 should be 100000 (center)");

    // Grid 9 (highest): 100000 * (1 + 0.01 * (9 - 5)) = 100000 * 1.04 = 104000
    ASSERT_NEAR(levels[9], 104000.0, 0.01, "Grid 9 should be 104000");

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

int test_grid_crossing_buy() {
    std::cout << "Testing: GridCrossingBuy..." << std::endl;

    GridMartinStrategy strategy;
    strategy.base_price_ = 100000.0;
    strategy.grid_count_ = 10;
    strategy.grid_spacing_ = 0.01;
    strategy.amount_per_grid_ = 100.0;

    strategy.on_init();
    strategy.on_start();

    // New grid range: 95000 - 104000 (symmetric around base price)
    // Grid 9 = 104000 (highest), Grid 5 = 100000 (center)

    // Simulate being at grid 9 (top grid) initially
    strategy.set_last_grid_index(9);

    // Price drops from ~104000 to ~96000 (crossing multiple grids down)
    BarData bar;
    bar.close_price = 96000.0;
    strategy.on_bar(bar);

    // Should have bought some position
    TEST_ASSERT(strategy.total_position() > 0.0, "Should have position after price drop");

    // Average cost should be around 96000 range
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

    // New grid range: 95000 - 104000
    // Start at grid 9 (104000), price drops to grid 3 (~98000), triggering buys
    strategy.set_last_grid_index(9);
    BarData bar1;
    bar1.close_price = 98000.0;
    strategy.on_bar(bar1);

    double position_after_buy = strategy.total_position();

    // Set last_grid to 3, then price rises to grid 5 (~100000), triggering FIFO sell
    strategy.set_last_grid_index(3);
    BarData bar2;
    bar2.close_price = 100000.0;
    strategy.on_bar(bar2);

    // Position should decrease
    TEST_ASSERT(strategy.total_position() < position_after_buy, "Position should decrease after sell");

    std::cout << "  PASSED: FIFOSellOrder" << std::endl;
    return 0;
}

int test_stop_loss_trigger() {
    std::cout << "Testing: StopLossTrigger..." << std::endl;

    GridMartinStrategy strategy;
    strategy.base_price_ = 100000.0;
    strategy.grid_count_ = 10;
    strategy.grid_spacing_ = 0.01;
    strategy.amount_per_grid_ = 100.0;

    strategy.on_init();
    strategy.on_start();

    // Simulate buying position at grid 5 (100000)
    strategy.set_last_grid_index(9);
    BarData bar1;
    bar1.close_price = 100000.0;
    strategy.on_bar(bar1);

    // Verify position exists
    TEST_ASSERT(strategy.total_position() > 0.0, "Should have position before stop loss");

    // Price drops below stop loss (grid 0 = 95000)
    BarData bar2;
    bar2.close_price = 94000.0;  // Below grid 0
    strategy.on_bar(bar2);

    // Position should be cleared
    TEST_ASSERT(strategy.total_position() == 0.0, "Position should be 0 after stop loss");

    // Trading should be stopped
    TEST_ASSERT(!strategy.is_trading(), "Trading should be stopped after stop loss");

    std::cout << "  PASSED: StopLossTrigger" << std::endl;
    return 0;
}

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
