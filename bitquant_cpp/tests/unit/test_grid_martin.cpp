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
