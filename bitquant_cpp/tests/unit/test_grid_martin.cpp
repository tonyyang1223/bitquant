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

    // Simulate being at grid 9 (top grid) initially
    strategy.set_last_grid_index(9);

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

    // Start at grid 9 (top), price drops to grid 5, triggering buys
    strategy.set_last_grid_index(9);
    BarData bar1;
    bar1.close_price = 95000.0;
    strategy.on_bar(bar1);

    double position_after_buy = strategy.total_position();

    // Set last_grid to 5, then price rises to grid 7, triggering FIFO sell
    strategy.set_last_grid_index(5);
    BarData bar2;
    bar2.close_price = 97000.0;
    strategy.on_bar(bar2);

    // Position should decrease
    TEST_ASSERT(strategy.total_position() < position_after_buy, "Position should decrease after sell");

    std::cout << "  PASSED: FIFOSellOrder" << std::endl;
    return 0;
}

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
