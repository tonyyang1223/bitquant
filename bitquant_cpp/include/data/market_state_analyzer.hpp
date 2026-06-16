/**
 * @file market_state_analyzer.hpp
 * @brief Market state detection using volatility channel
 *
 * Detects consolidation vs trending states based on price channel width.
 * Grid trading is suitable for consolidation, not for trending.
 */

#pragma once

#include "data/ring_buffer.hpp"
#include <mutex>

namespace bitquant {

/**
 * @brief Market state enumeration
 */
enum class MarketState {
    WAITING,        // Initializing, not enough data
    CONSOLIDATION,  // Range-bound, suitable for grid trading
    TRENDING        // Trending, should pause grid trading
};

/**
 * @brief Configuration for MarketStateAnalyzer
 */
struct MarketStateConfig {
    int period = 50;                 // Rolling calculation period
    double width_threshold_pct = 5.0;    // Channel width threshold (%)
    int confirmation_bars = 2;      // Bars needed to confirm state change
};

/**
 * @brief Market State Analyzer
 *
 * Uses Bollinger Band width (volatility channel) to determine market state:
 * - Calculate SMA and StdDev over N periods
 * - Channel Width = (Upper - Lower) / Middle * 100
 * - Width <= threshold: CONSOLIDATION
 * - Width > threshold: TRENDING
 *
 * Thread-safe for use in WebSocket callback threads.
 */
class MarketStateAnalyzer {
public:
    explicit MarketStateAnalyzer(const MarketStateConfig& config = {});
    ~MarketStateAnalyzer() = default;

    // Non-copyable (contains state)
    MarketStateAnalyzer(const MarketStateAnalyzer&) = delete;
    MarketStateAnalyzer& operator=(const MarketStateAnalyzer&) = delete;

    /**
     * @brief Process new bar data
     * @param high High price
     * @param low Low price
     * @param close Close price
     * @return Current confirmed market state
     */
    MarketState update(double high, double low, double close);

    /**
     * @brief Get current market state
     */
    MarketState state() const;

    /**
     * @brief Get upper band value
     */
    double upper_band() const;

    /**
     * @brief Get lower band value
     */
    double lower_band() const;

    /**
     * @brief Get middle band (SMA) value
     */
    double middle_band() const;

    /**
     * @brief Get channel width as percentage
     */
    double channel_width_pct() const;

    /**
     * @brief Check if analyzer is initialized (has enough data)
     */
    bool is_initialized() const;

    /**
     * @brief Reset analyzer state
     */
    void reset();

    /**
     * @brief Get current configuration
     */
    const MarketStateConfig& config() const { return config_; }

    /**
     * @brief Get number of bars processed
     */
    size_t bar_count() const;

private:
    /**
     * @brief Calculate bands and channel width
     */
    void calculate_bands();

    /**
     * @brief Determine state from channel width
     */
    MarketState determine_state(double width) const;

    MarketStateConfig config_;
    RingBuffer<double> close_buffer_;

    double current_middle_ = 0.0;
    double current_upper_ = 0.0;
    double current_lower_ = 0.0;
    double current_width_ = 0.0;

    MarketState current_state_ = MarketState::WAITING;
    MarketState pending_state_ = MarketState::WAITING;
    int confirmation_count_ = 0;

    mutable std::mutex mutex_;  // Thread safety
};

} // namespace bitquant
