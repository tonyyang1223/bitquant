/**
 * @file market_state_analyzer.cpp
 * @brief Market state detection implementation
 */

#include "data/market_state_analyzer.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace bitquant {

MarketStateAnalyzer::MarketStateAnalyzer(const MarketStateConfig& config)
    : config_(config)
    , close_buffer_(static_cast<size_t>(config.period))
{
}

MarketState MarketStateAnalyzer::update(double high, double low, double close) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Skip invalid prices
    if (close <= 0) {
        return current_state_;
    }

    // Add to buffer
    close_buffer_.push(close);

    // Check if we have enough data
    if (!close_buffer_.full()) {
        current_state_ = MarketState::WAITING;
        return current_state_;
    }

    // Calculate bands
    calculate_bands();

    // Determine new state
    MarketState new_state = determine_state(current_width_);

    // Handle state transition with confirmation
    if (new_state != current_state_) {
        if (new_state == pending_state_) {
            confirmation_count_++;
            if (confirmation_count_ >= config_.confirmation_bars) {
                current_state_ = new_state;
                confirmation_count_ = 0;
                pending_state_ = MarketState::WAITING;
            }
        } else {
            // Start new confirmation cycle
            pending_state_ = new_state;
            confirmation_count_ = 1;
        }
    } else {
        // Reset confirmation if state returned to current
        pending_state_ = MarketState::WAITING;
        confirmation_count_ = 0;
    }

    return current_state_;
}

void MarketStateAnalyzer::calculate_bands() {
    size_t n = close_buffer_.size();
    if (n == 0) return;

    // Calculate SMA
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum += close_buffer_[i];
    }
    current_middle_ = sum / n;

    // Calculate StdDev
    double sq_sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double diff = close_buffer_[i] - current_middle_;
        sq_sum += diff * diff;
    }
    double stddev = std::sqrt(sq_sum / n);

    // Calculate bands (2 standard deviations)
    current_upper_ = current_middle_ + 2.0 * stddev;
    current_lower_ = current_middle_ - 2.0 * stddev;

    // Calculate channel width as percentage
    if (current_middle_ > 0) {
        current_width_ = (current_upper_ - current_lower_) / current_middle_ * 100.0;
    } else {
        current_width_ = 0.0;
    }
}

MarketState MarketStateAnalyzer::determine_state(double width) const {
    if (width <= config_.width_threshold_pct) {
        return MarketState::CONSOLIDATION;
    } else {
        return MarketState::TRENDING;
    }
}

MarketState MarketStateAnalyzer::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_state_;
}

double MarketStateAnalyzer::upper_band() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_upper_;
}

double MarketStateAnalyzer::lower_band() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_lower_;
}

double MarketStateAnalyzer::middle_band() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_middle_;
}

double MarketStateAnalyzer::channel_width_pct() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_width_;
}

bool MarketStateAnalyzer::is_initialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return close_buffer_.full();
}

void MarketStateAnalyzer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    close_buffer_.clear();
    current_middle_ = 0.0;
    current_upper_ = 0.0;
    current_lower_ = 0.0;
    current_width_ = 0.0;
    current_state_ = MarketState::WAITING;
    pending_state_ = MarketState::WAITING;
    confirmation_count_ = 0;
}

size_t MarketStateAnalyzer::bar_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return close_buffer_.size();
}

} // namespace bitquant
