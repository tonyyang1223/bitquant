/**
 * @file array_manager.hpp
 * @brief Array manager with TA-Lib integration for technical indicators
 *
 * Provides a unified interface for calculating technical indicators
 * using the TA-Lib C library.
 */

#pragma once

#include "data/ring_buffer.hpp"
#include "core/types.hpp"
#include <ta-lib/ta_libc.h>
#include <tuple>
#include <memory>

namespace bitquant {

/**
 * @brief Array manager for technical indicator calculations
 *
 * Wraps OHLCVRingBuffer and provides TA-Lib indicator methods.
 */
class ArrayManager {
public:
    explicit ArrayManager(size_t size = 500);
    ~ArrayManager() = default;

    // Non-copyable
    ArrayManager(const ArrayManager&) = delete;
    ArrayManager& operator=(const ArrayManager&) = delete;

    // Movable
    ArrayManager(ArrayManager&&) = default;
    ArrayManager& operator=(ArrayManager&&) = default;

    /**
     * @brief Update with new bar data
     */
    void update_bar(const BarData& bar);
    void update_bar(int64_t datetime, double open, double high,
                    double low, double close, double volume);

    // Status
    bool inited() const { return buffer_.inited(); }
    size_t count() const { return buffer_.count(); }
    size_t size() const { return buffer_.capacity(); }

    // Direct array access for TA-Lib
    const double* open() const { return buffer_.open_data(); }
    const double* high() const { return buffer_.high_data(); }
    const double* low() const { return buffer_.low_data(); }
    const double* close() const { return buffer_.close_data(); }
    const double* volume() const { return buffer_.volume_data(); }

    // ==================== Trend Indicators ====================

    /**
     * @brief Simple Moving Average
     * @param period Period for SMA
     * @return Latest SMA value
     */
    double sma(int period) const;
    std::vector<double> sma_array(int period) const;

    /**
     * @brief Exponential Moving Average
     */
    double ema(int period) const;
    std::vector<double> ema_array(int period) const;

    /**
     * @brief MACD (Moving Average Convergence Divergence)
     * @return tuple<macd_line, signal_line, histogram>
     */
    std::tuple<double, double, double> macd(int fast_period = 12,
                                             int slow_period = 26,
                                             int signal_period = 9) const;

    /**
     * @brief Average Directional Index
     */
    double adx(int period = 14) const;

    // ==================== Momentum Indicators ====================

    /**
     * @brief Relative Strength Index
     */
    double rsi(int period = 14) const;
    std::vector<double> rsi_array(int period) const;

    /**
     * @brief Commodity Channel Index
     */
    double cci(int period = 20) const;

    /**
     * @brief Stochastic Oscillator
     * @return tuple<fast_k, fast_d>
     */
    std::tuple<double, double> stoch(int k_period = 5,
                                      int d_period = 3) const;

    // ==================== Volatility Indicators ====================

    /**
     * @brief Average True Range
     */
    double atr(int period = 14) const;
    std::vector<double> atr_array(int period) const;

    /**
     * @brief Standard Deviation
     */
    double stddev(int period = 20) const;

    /**
     * @brief Normalized Average True Range
     */
    double natr(int period = 14) const;

    // ==================== Channel Indicators ====================

    /**
     * @brief Bollinger Bands
     */
    struct BollingerBands {
        double upper;
        double middle;
        double lower;
    };
    BollingerBands bollinger(int period = 20, double dev = 2.0) const;

    /**
     * @brief Keltner Channel
     */
    struct KeltnerChannel {
        double upper;
        double middle;
        double lower;
    };
    KeltnerChannel keltner(int period = 20, double dev = 1.0) const;

    /**
     * @brief Donchian Channel
     */
    struct DonchianChannel {
        double upper;
        double lower;
    };
    DonchianChannel donchian(int period = 20) const;

    // ==================== Custom Indicators ====================

    /**
     * @brief Choppiness Market Index (CMI)
     *
     * CMI = 100 * abs(close - close[period]) / (HH - LL)
     * Values < 20: ranging market
     * Values >= 20: trending market
     */
    double cmi(int period = 30, int ma_period = 10) const;

    /**
     * @brief Stochastic RSI
     * @return tuple<fast_k, fast_d>
     */
    std::pair<double, double> stoch_rsi(int rsi_period = 14,
                                         int stoch_period = 14,
                                         int k_smooth = 3,
                                         int d_smooth = 3) const;

    // ==================== Utility Methods ====================

    /**
     * @brief Get latest close price
     */
    double latest_close() const { return buffer_.latest_close(); }

    /**
     * @brief Get close at offset from latest
     */
    double close_at(size_t offset) const { return buffer_.close_at(offset); }

private:
    OHLCVRingBuffer buffer_;

    // Working arrays for TA-Lib outputs
    mutable std::vector<double> output_;
    mutable std::vector<double> output2_;
    mutable std::vector<double> output3_;

    // Check TA-Lib return code
    bool check_ret(TA_RetCode ret) const;
};

} // namespace bitquant
