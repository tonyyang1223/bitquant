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
