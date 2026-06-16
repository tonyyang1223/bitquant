/**
 * @file grid_martin_strategy.cpp
 * @brief Grid Martin trading strategy implementation
 */

#include "strategy/grid_martin_strategy.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace bitquant {

GridMartinStrategy::GridMartinStrategy() {
    author = "BitQuant";
    parameters = {"base_price", "grid_count", "grid_spacing", "amount_per_grid"};
    variables = {"total_position", "avg_cost", "last_grid_index"};
}

void GridMartinStrategy::on_init() {
    std::cout << "[GridMartin::on_init] DEBUG: base_price_=" << base_price_
              << " grid_count_=" << grid_count_
              << " grid_spacing_=" << grid_spacing_
              << " amount_per_grid_=" << amount_per_grid_
              << " symbol_=" << symbol_ << std::endl;

    calculate_grid_levels();

    // Initialize grid positions and costs
    grid_positions_.resize(grid_count_, 0.0);
    grid_costs_.resize(grid_count_, 0.0);

    // Initialize price smoother for outlier protection
    PriceSmootherConfig smoother_config;
    smoother_config.smoothing_period = smoothing_period_;
    smoother_config.outlier_threshold_pct = outlier_threshold_pct_;
    smoother_config.ema_alpha = 1.0 / smoothing_period_;  // Standard EMA alpha
    smoother_config.require_confirmation = true;
    price_smoother_ = std::make_unique<PriceSmoother>(smoother_config);

    // Initialize market state analyzer
    MarketStateConfig state_config;
    state_config.period = state_period_;
    state_config.width_threshold_pct = state_width_threshold_;
    state_config.confirmation_bars = state_confirmation_bars_;
    state_analyzer_ = std::make_unique<MarketStateAnalyzer>(state_config);

    write_log("GridMartinStrategy initialized with " + std::to_string(grid_count_) + " grids");
    write_log("PriceSmoother initialized: period=" + std::to_string(smoothing_period_) +
             " threshold=" + std::to_string(static_cast<int>(outlier_threshold_pct_)) + "%");
    write_log("MarketStateAnalyzer initialized: period=" + std::to_string(state_period_) +
             " threshold=" + std::to_string(state_width_threshold_) + "%" +
             " confirmation=" + std::to_string(state_confirmation_bars_) + " bars");

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
    double smoothed_price = price;

    // Apply price smoothing for outlier protection
    if (price_smoother_ && price_smoother_->is_initialized()) {
        auto result = price_smoother_->process(price, bar.datetime);
        smoothed_price = result.smoothed_price;

        if (result.is_anomaly) {
            write_log("[PriceSmoother] ANOMALY detected! Raw: $" +
                     std::to_string(static_cast<int64_t>(price)) +
                     " | Smoothed: $" + std::to_string(static_cast<int64_t>(smoothed_price)));
        }
    } else if (price_smoother_) {
        price_smoother_->process(price, bar.datetime);
    }

    // Update market state analyzer
    MarketState new_state = current_market_state_;
    if (state_analyzer_) {
        new_state = state_analyzer_->update(bar.high_price, bar.low_price, bar.close_price);
    }

    // Handle state transition
    if (new_state != current_market_state_) {
        handle_state_transition(new_state, smoothed_price);
        last_grid_index_ = get_grid_index(smoothed_price);
        return;  // Wait for next bar after transition
    }

    // Only trade in consolidation state (or WAITING during warm-up)
    if (current_market_state_ == MarketState::TRENDING) {
        return;  // Wait in trending state
    }

    // Normal grid trading logic (consolidation state)
    int current_grid = get_grid_index(smoothed_price);

    if (last_grid_index_ < 0) {
        write_log("First bar in consolidation: $" + std::to_string(static_cast<int64_t>(smoothed_price)) +
                 " | Grid: " + std::to_string(current_grid));
    } else if (current_grid != last_grid_index_) {
        write_log("Bar: $" + std::to_string(static_cast<int64_t>(smoothed_price)) +
                 " | Grid CROSS: " + std::to_string(last_grid_index_) + " -> " + std::to_string(current_grid));
    }

    // Check stop loss first
    check_stop_loss(smoothed_price);

    if (!trading_) {
        return;  // Stopped after stop loss
    }

    // Execute grid trades if grid changed
    if (last_grid_index_ >= 0 && current_grid != last_grid_index_) {
        execute_grid_trade(last_grid_index_, current_grid, smoothed_price);
    }

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

    // Calculate grid levels symmetrically around base price
    // Half the grids above base_price, half below
    // Grid levels from bottom to top:
    // grid_levels_[i] = base_price * (1 - grid_spacing * (grid_count/2 - i))
    // This puts base_price in the middle of the grid range

    int half_grid = grid_count_ / 2;

    for (int i = 0; i < grid_count_; ++i) {
        // Grid i is at offset (i - half_grid) from center
        // Negative offset = below base_price, positive = above
        double level = base_price_ * (1.0 + grid_spacing_ * (i - half_grid));
        grid_levels_.push_back(level);
    }

    write_log("Grid range: $" + std::to_string(static_cast<int64_t>(grid_levels_.front())) +
             " - $" + std::to_string(static_cast<int64_t>(grid_levels_.back())));
    write_log("Base price $" + std::to_string(static_cast<int64_t>(base_price_)) +
             " is at grid index " + std::to_string(half_grid));
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
        write_log("Grid UP: " + std::to_string(from_grid) + " -> " + std::to_string(to_grid) +
                 " | Selling " + std::to_string(grid_diff) + " position(s)");

        for (int i = 0; i < grid_diff; ++i) {
            if (buy_queue_.empty()) {
                write_log("  No more positions to sell (queue empty)");
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

                write_log("  SELL Grid " + std::to_string(grid_to_sell) +
                         " | Price: $" + std::to_string(static_cast<int64_t>(price)) +
                         " | Volume: " + std::to_string(sell_volume) +
                         " | Cost: $" + std::to_string(static_cast<int64_t>(cost)) +
                         " | Profit: $" + std::to_string(static_cast<int64_t>(profit)));

                // Reset grid position
                grid_positions_[grid_to_sell] = 0.0;
                grid_costs_[grid_to_sell] = 0.0;
                total_position_ -= sell_volume;
            }
        }

        // Log remaining position
        write_log("  Remaining position: " + std::to_string(total_position_) +
                 " | Avg cost: $" + std::to_string(static_cast<int64_t>(avg_cost_)));

    } else if (grid_diff < 0) {
        // Price went DOWN - buy positions
        int grids_crossed = -grid_diff;

        write_log("Grid DOWN: " + std::to_string(from_grid) + " -> " + std::to_string(to_grid) +
                 " | Buying " + std::to_string(grids_crossed) + " position(s)");

        for (int i = 0; i < grids_crossed; ++i) {
            int grid_to_buy = from_grid - i - 1;

            if (grid_to_buy < 0 || grid_to_buy >= grid_count_) {
                write_log("  Skip grid " + std::to_string(grid_to_buy) + " (out of range)");
                break;
            }

            // Skip if already have position at this grid
            if (grid_positions_[grid_to_buy] > 0.0) {
                write_log("  Skip grid " + std::to_string(grid_to_buy) + " (already has position)");
                continue;
            }

            // Calculate buy volume based on fixed amount
            double buy_volume = amount_per_grid_ / price;

            std::cout << std::fixed << std::setprecision(8);
            std::cout << "[GridMartin] DEBUG before buy: price=" << price
                      << " amount_per_grid_=" << amount_per_grid_
                      << " buy_volume=" << buy_volume << std::endl;

            buy(price, buy_volume);

            // Record position
            grid_positions_[grid_to_buy] = buy_volume;
            grid_costs_[grid_to_buy] = price;
            buy_queue_.push(grid_to_buy);
            total_position_ += buy_volume;

            // Update average cost
            double total_cost = avg_cost_ * (total_position_ - buy_volume) + price * buy_volume;
            avg_cost_ = total_cost / total_position_;

            write_log("  BUY Grid " + std::to_string(grid_to_buy) +
                     " | Price: $" + std::to_string(static_cast<int64_t>(price)) +
                     " | Volume: " + std::to_string(buy_volume) +
                     " | Amount: $" + std::to_string(static_cast<int64_t>(amount_per_grid_)));
        }

        // Log total position after buying
        write_log("  Total position: " + std::to_string(total_position_) +
                 " | Avg cost: $" + std::to_string(static_cast<int64_t>(avg_cost_)));
    }
}

void GridMartinStrategy::check_stop_loss(double price) {
    // Check if price is below the bottom grid
    if (grid_levels_.empty()) {
        return;
    }

    double stop_price = grid_levels_[0];

    // Log when price approaches stop loss
    if (price < stop_price * 1.02 && price >= stop_price && total_position_ > 0.0) {
        write_log("[WARNING] Price approaching stop loss: $" + std::to_string(static_cast<int64_t>(price)) +
                 " | Stop price: $" + std::to_string(static_cast<int64_t>(stop_price)) +
                 " | Position: " + std::to_string(total_position_));
    }

    if (price < stop_price && total_position_ > 0.0) {
        write_log("[STOP LOSS] Triggered at $" + std::to_string(static_cast<int64_t>(stop_price)) +
                 " | Current: $" + std::to_string(static_cast<int64_t>(price)));

        // Close all positions
        sell(price, total_position_);

        // Calculate loss
        double loss = (price - avg_cost_) * total_position_;
        write_log("[STOP LOSS] Position closed | Position: " + std::to_string(total_position_) +
                 " | Avg cost: $" + std::to_string(static_cast<int64_t>(avg_cost_)) +
                 " | Loss: $" + std::to_string(static_cast<int64_t>(loss)));

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
        write_log("[STOP LOSS] Trading stopped. Strategy will not execute further trades.");
    }
}

void GridMartinStrategy::handle_state_transition(MarketState new_state, double price) {
    if (new_state == MarketState::TRENDING) {
        transition_to_trending(price);
    } else if (new_state == MarketState::CONSOLIDATION) {
        transition_to_consolidation(price);
    }
}

void GridMartinStrategy::transition_to_trending(double price) {
    write_log("[MarketState] TRANSITION: CONSOLIDATION -> TRENDING");
    if (state_analyzer_) {
        write_log("[MarketState] Channel width: " + std::to_string(state_analyzer_->channel_width_pct()) + "%");
    }

    // Close all positions immediately
    if (total_position_ > 0.0) {
        sell(price, total_position_);

        double profit = (price - avg_cost_) * total_position_;
        write_log("[MarketState] Closed position | Price: $" + std::to_string(static_cast<int64_t>(price)) +
                 " | Position: " + std::to_string(total_position_) +
                 " | Avg Cost: $" + std::to_string(static_cast<int64_t>(avg_cost_)) +
                 " | P&L: $" + std::to_string(static_cast<int64_t>(profit)));

        // Clear all state
        total_position_ = 0.0;
        avg_cost_ = 0.0;
        std::fill(grid_positions_.begin(), grid_positions_.end(), 0.0);
        std::fill(grid_costs_.begin(), grid_costs_.end(), 0.0);
        while (!buy_queue_.empty()) buy_queue_.pop();
    }

    current_market_state_ = MarketState::TRENDING;
    trading_ = false;
    write_log("[MarketState] Trading paused, waiting for consolidation");
}

void GridMartinStrategy::transition_to_consolidation(double price) {
    write_log("[MarketState] TRANSITION: TRENDING -> CONSOLIDATION");
    if (state_analyzer_) {
        write_log("[MarketState] Channel width: " + std::to_string(state_analyzer_->channel_width_pct()) + "%");
    }

    // Reset grid with new base price
    base_price_ = price;
    calculate_grid_levels();

    // Reset grid state
    std::fill(grid_positions_.begin(), grid_positions_.end(), 0.0);
    std::fill(grid_costs_.begin(), grid_costs_.end(), 0.0);
    total_position_ = 0.0;
    avg_cost_ = 0.0;

    // Reset PriceSmoother
    if (price_smoother_) {
        price_smoother_->reset();
    }

    // Clear queue
    while (!buy_queue_.empty()) buy_queue_.pop();

    current_market_state_ = MarketState::CONSOLIDATION;
    trading_ = true;
    last_grid_index_ = -1;  // Force re-initialization

    write_log("[MarketState] Grid restarted | Base: $" + std::to_string(static_cast<int64_t>(base_price_)) +
             " | Range: $" + std::to_string(static_cast<int64_t>(grid_levels_.front())) +
             " - $" + std::to_string(static_cast<int64_t>(grid_levels_.back())));
    write_log("[MarketState] Trading resumed");
}

} // namespace bitquant
