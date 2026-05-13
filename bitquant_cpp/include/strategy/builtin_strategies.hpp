/**
 * @file builtin_strategies.hpp
 * @brief Built-in trading strategies
 *
 * Includes common strategies based on howtrader design:
 * - DoubleMaStrategy: Double moving average crossover
 * - RsiStrategy: RSI overbought/oversold
 * - BollingerStrategy: Bollinger band breakout
 * - AtrRsiStrategy: ATR + RSI combined
 */

#pragma once

#include "engine/strategy.hpp"
#include "data/array_manager.hpp"
#include "core/types.hpp"
#include <memory>
#include <string>

namespace bitquant {

//=============================================================================
// Double Moving Average Strategy
//=============================================================================

/**
 * @brief Double MA crossover strategy
 *
 * Based on howtrader's DoubleMaStrategy:
 * - Fast MA crosses above Slow MA: Buy signal
 * - Fast MA crosses below Slow MA: Sell signal
 *
 * Parameters:
 * - fast_window: Fast MA period (default: 10)
 * - slow_window: Slow MA period (default: 20)
 */
class DoubleMaStrategy : public IStrategy {
public:
    DoubleMaStrategy();

    std::string name() const { return "DoubleMaStrategy"; }

    void on_init() override;
    void on_bar(const BarData& bar) override;

    // Parameters
    void set_fast_window(int window) { fast_window_ = window; }
    void set_slow_window(int window) { slow_window_ = window; }

private:
    int fast_window_ = 10;
    int slow_window_ = 20;

    double fast_ma0_ = 0.0;
    double fast_ma1_ = 0.0;
    double slow_ma0_ = 0.0;
    double slow_ma1_ = 0.0;

    std::unique_ptr<ArrayManager> am_;
};

//=============================================================================
// RSI Strategy
//=============================================================================

/**
 * @brief RSI overbought/oversold strategy
 *
 * Based on howtrader's AtrRsiStrategy:
 * - RSI < oversold_level: Buy signal
 * - RSI > overbought_level: Sell signal
 *
 * Parameters:
 * - rsi_period: RSI calculation period (default: 14)
 * - oversold_level: Oversold threshold (default: 30)
 * - overbought_level: Overbought threshold (default: 70)
 */
class RsiStrategy : public IStrategy {
public:
    RsiStrategy();

    std::string name() const { return "RsiStrategy"; }

    void on_init() override;
    void on_bar(const BarData& bar) override;

    void set_rsi_period(int period) { rsi_period_ = period; }
    void set_oversold(double level) { oversold_level_ = level; }
    void set_overbought(double level) { overbought_level_ = level; }

private:
    int rsi_period_ = 14;
    double oversold_level_ = 30.0;
    double overbought_level_ = 70.0;

    double current_rsi_ = 50.0;
    double previous_rsi_ = 50.0;

    std::unique_ptr<ArrayManager> am_;
};

//=============================================================================
// Bollinger Band Strategy
//=============================================================================

/**
 * @brief Bollinger Band breakout strategy
 *
 * Based on howtrader's BollChannelStrategy:
 * - Price breaks above upper band: Buy signal
 * - Price breaks below lower band: Sell signal
 *
 * Parameters:
 * - boll_period: Bollinger period (default: 20)
 * - boll_dev: Standard deviation multiplier (default: 2.0)
 */
class BollingerStrategy : public IStrategy {
public:
    BollingerStrategy();

    std::string name() const { return "BollingerStrategy"; }

    void on_init() override;
    void on_bar(const BarData& bar) override;

    void set_period(int period) { period_ = period; }
    void set_dev(double dev) { dev_ = dev; }

private:
    int period_ = 20;
    double dev_ = 2.0;

    double upper_band_ = 0.0;
    double middle_band_ = 0.0;
    double lower_band_ = 0.0;

    std::unique_ptr<ArrayManager> am_;
};

//=============================================================================
// ATR + RSI Combined Strategy
//=============================================================================

/**
 * @brief ATR + RSI combined strategy
 *
 * Based on howtrader's AtrRsiStrategy:
 * - Uses ATR for volatility and RSI for trend strength
 * - RSI oversold + low ATR: Buy signal
 * - RSI overbought + high ATR: Sell signal
 *
 * Parameters:
 * - atr_period: ATR period (default: 22)
 * - rsi_period: RSI period (default: 14)
 * - oversold_level: RSI oversold (default: 30)
 * - overbought_level: RSI overbought (default: 70)
 */
class AtrRsiStrategy : public IStrategy {
public:
    AtrRsiStrategy();

    std::string name() const { return "AtrRsiStrategy"; }

    void on_init() override;
    void on_bar(const BarData& bar) override;

private:
    int atr_period_ = 22;
    int rsi_period_ = 14;
    double oversold_level_ = 30.0;
    double overbought_level_ = 70.0;

    double current_atr_ = 0.0;
    double current_rsi_ = 50.0;

    std::unique_ptr<ArrayManager> am_;
};

//=============================================================================
// Turtle Signal Strategy
//=============================================================================

/**
 * @brief Turtle trading system strategy
 *
 * Based on howtrader's TurtleSignalStrategy:
 * - Uses Donchian channel for entry
 * - Uses ATR for position sizing and stop loss
 *
 * Parameters:
 * - entry_window: Entry channel period (default: 20)
 * - exit_window: Exit channel period (default: 10)
 * - atr_period: ATR period (default: 20)
 */
class TurtleStrategy : public IStrategy {
public:
    TurtleStrategy();

    std::string name() const { return "TurtleStrategy"; }

    void on_init() override;
    void on_bar(const BarData& bar) override;

private:
    int entry_window_ = 20;
    int exit_window_ = 10;
    int atr_period_ = 20;

    double entry_high_ = 0.0;  // Donchian high
    double entry_low_ = 0.0;   // Donchian low
    double exit_high_ = 0.0;
    double exit_low_ = 0.0;
    double current_atr_ = 0.0;

    std::unique_ptr<ArrayManager> am_;
};

//=============================================================================
// Strategy Factory
//=============================================================================

/**
 * @brief Factory for creating built-in strategies
 */
class StrategyFactory {
public:
    static std::unique_ptr<IStrategy> create(const std::string& name);
    static std::vector<std::string> list_strategies();
};

} // namespace bitquant
