/**
 * @file price_smoother.cpp
 * @brief Price smoothing and outlier detection implementation
 */

#include "data/price_smoother.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace bitquant {

PriceSmoother::PriceSmoother(const PriceSmootherConfig& config)
    : config_(config)
    , price_buffer_(config.smoothing_period)
{
}

SmoothedPrice PriceSmoother::process(double price, int64_t timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);

    SmoothedPrice result;
    result.raw_price = price;

    // Skip invalid prices
    if (price <= 0) {
        result.smoothed_price = last_smoothed_price_;
        return result;
    }

    // Cold start: use raw price for first ticks until buffer is full
    if (price_buffer_.size() < config_.smoothing_period) {
        price_buffer_.push(price);

        // Initialize EMA with first price, then update incrementally
        if (price_buffer_.size() == 1) {
            current_ema_ = price;
        } else {
            current_ema_ = config_.ema_alpha * price + (1.0 - config_.ema_alpha) * current_ema_;
        }

        last_smoothed_price_ = current_ema_;
        result.smoothed_price = price;  // During warm-up, return raw price
        return result;
    }

    // Buffer is full - apply outlier detection
    bool outlier = is_outlier(price, last_smoothed_price_);

    if (outlier) {
        anomaly_count_++;
        result.anomaly_count = anomaly_count_;

        if (config_.require_confirmation) {
            // First outlier: mark as pending, don't update EMA
            if (!anomaly_state_) {
                anomaly_state_ = true;
                pending_outlier_price_ = price;
                result.is_anomaly = true;
                result.smoothed_price = last_smoothed_price_;  // Keep previous smoothed

                std::cout << "[PriceSmoother] OUTLIER detected (pending): Raw $" << price
                          << " vs Smoothed $" << last_smoothed_price_
                          << " (change: " << std::abs(price - last_smoothed_price_) / last_smoothed_price_ * 100 << "%)"
                          << std::endl;
                return result;
            }

            // Second consecutive outlier: confirmed anomaly
            // Allow price to gradually catch up (blend to avoid staying stuck forever)
            result.is_anomaly = true;
            result.smoothed_price = last_smoothed_price_;

            // Gradual blend: 70% old smoothed + 30% new price
            current_ema_ = 0.7 * last_smoothed_price_ + 0.3 * price;
            price_buffer_.push(price);
            last_smoothed_price_ = current_ema_;

            std::cout << "[PriceSmoother] ANOMALY confirmed: keeping smoothed at $" << result.smoothed_price
                      << " (raw was $" << price << ")"
                      << std::endl;
            return result;
        }

        // No confirmation required - reject outlier directly
        result.is_anomaly = true;
        result.smoothed_price = last_smoothed_price_;
        return result;
    }

    // Normal price (not an outlier)
    anomaly_state_ = false;
    anomaly_count_ = 0;

    // Update EMA incrementally (O(1) operation)
    current_ema_ = config_.ema_alpha * price + (1.0 - config_.ema_alpha) * current_ema_;
    price_buffer_.push(price);
    last_smoothed_price_ = current_ema_;

    result.smoothed_price = current_ema_;

    // Log recovery from anomaly
    if (pending_outlier_price_ > 0) {
        std::cout << "[PriceSmoother] ANOMALY cleared: price returned to normal $" << price
                  << " (smoothed: $" << result.smoothed_price << ")"
                  << std::endl;
        pending_outlier_price_ = 0.0;
    }

    return result;
}

bool PriceSmoother::is_outlier(double price, double reference) const {
    if (reference <= 0) return false;

    double pct_change = std::abs(price - reference) / reference * 100.0;
    return pct_change > config_.outlier_threshold_pct;
}

double PriceSmoother::get_smoothed_price() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_smoothed_price_;
}

double PriceSmoother::get_last_raw_price() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return price_buffer_.size() > 0 ? price_buffer_.latest() : 0.0;
}

bool PriceSmoother::is_anomaly() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return anomaly_state_;
}

bool PriceSmoother::is_initialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return price_buffer_.size() >= config_.smoothing_period;
}

void PriceSmoother::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    price_buffer_.clear();
    current_ema_ = 0.0;
    last_smoothed_price_ = 0.0;
    anomaly_state_ = false;
    anomaly_count_ = 0;
    pending_outlier_price_ = 0.0;
}

size_t PriceSmoother::tick_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return price_buffer_.size();
}

} // namespace bitquant