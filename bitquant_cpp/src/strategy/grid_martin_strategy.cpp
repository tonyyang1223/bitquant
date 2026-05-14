/**
 * @file grid_martin_strategy.cpp
 * @brief Grid Martin trading strategy implementation
 */

#include "strategy/grid_martin_strategy.hpp"
#include "utils/logger.hpp"
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