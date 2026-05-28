/**
 * @file price_smoother.hpp
 * @brief Price smoothing and outlier detection module
 *
 * Filters outlier prices from dirty data (e.g., testnet anomalies)
 * using EMA smoothing and confirmation mechanism.
 */

#pragma once

#include "data/ring_buffer.hpp"
#include <mutex>
#include <cstdint>
#include <memory>

namespace bitquant {

/**
 * @brief Configuration for PriceSmoother
 */
struct PriceSmootherConfig {
    size_t smoothing_period = 10;       // Number of ticks for EMA buffer
    double outlier_threshold_pct = 5.0; // 5% threshold for anomaly detection
    double ema_alpha = 0.1;              // EMA smoothing factor
    bool require_confirmation = true;    // Wait for next tick to confirm outlier
};

/**
 * @brief Price smoothing result
 */
struct SmoothedPrice {
    double smoothed_price = 0.0;  // EMA-smoothed price
    double raw_price = 0.0;       // Original raw price
    bool is_anomaly = false;      // True if current tick is flagged as anomaly
    int anomaly_count = 0;        // Consecutive anomaly ticks
};

/**
 * @brief Price Smoother for filtering outlier prices
 *
 * Uses RingBuffer for O(1) operations and incremental EMA for smoothing.
 * Thread-safe for use in WebSocket callback threads.
 *
 * Key features:
 * 1. EMA smoothing for noise reduction
 * 2. Outlier detection based on percentage threshold
 * 3. Confirmation requirement to avoid false positives
 *
 * Usage:
 * @code
 * PriceSmoother smoother;
 * for (int i = 0; i < 10; ++i) {
 *     smoother.process(76000.0);
 * }
 * auto result = smoother.process(67714.0);  // 11% outlier
 * // result.is_anomaly == true
 * // result.smoothed_price still ~76000
 * @endcode
 */
class PriceSmoother {
public:
    explicit PriceSmoother(const PriceSmootherConfig& config = {});
    ~PriceSmoother() = default;

    // Non-copyable (contains mutex)
    PriceSmoother(const PriceSmoother&) = delete;
    PriceSmoother& operator=(const PriceSmoother&) = delete;

    /**
     * @brief Process a new price tick
     * @param price Raw price from exchange
     * @param timestamp Tick timestamp (milliseconds, optional)
     * @return SmoothedPrice with filtered/smoothed price
     */
    SmoothedPrice process(double price, int64_t timestamp = 0);

    /**
     * @brief Get current smoothed price (thread-safe)
     */
    double get_smoothed_price() const;

    /**
     * @brief Get last raw price from buffer
     */
    double get_last_raw_price() const;

    /**
     * @brief Check if currently in anomaly state
     */
    bool is_anomaly() const;

    /**
     * @brief Check if buffer is initialized (has enough data)
     */
    bool is_initialized() const;

    /**
     * @brief Reset smoother state
     */
    void reset();

    /**
     * @brief Get current configuration
     */
    const PriceSmootherConfig& config() const { return config_; }

    /**
     * @brief Get number of ticks processed
     */
    size_t tick_count() const;

private:
    /**
     * @brief Check if price is an outlier
     * @param price Price to check
     * @param reference Reference price (previous smoothed)
     */
    bool is_outlier(double price, double reference) const;

    PriceSmootherConfig config_;
    RingBuffer<double> price_buffer_;

    double current_ema_ = 0.0;
    double last_smoothed_price_ = 0.0;
    bool anomaly_state_ = false;
    int anomaly_count_ = 0;
    double pending_outlier_price_ = 0.0;

    mutable std::mutex mutex_;  // Thread safety for WebSocket callbacks
};

} // namespace bitquant