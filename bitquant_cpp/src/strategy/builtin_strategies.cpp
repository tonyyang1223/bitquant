/**
 * @file builtin_strategies.cpp
 * @brief Built-in trading strategies implementation
 */

#include "strategy/builtin_strategies.hpp"
#include "strategy/grid_martin_strategy.hpp"
#include "utils/logger.hpp"
#include <cmath>

namespace bitquant {

//=============================================================================
// Double MA Strategy
//=============================================================================

DoubleMaStrategy::DoubleMaStrategy() {
    author = "BitQuant";
    parameters = {"fast_window", "slow_window"};
    variables = {"fast_ma0", "fast_ma1", "slow_ma0", "slow_ma1"};

    am_ = std::make_unique<ArrayManager>(100);
}

void DoubleMaStrategy::on_init() {
    write_log("DoubleMaStrategy initializing...");
}

void DoubleMaStrategy::on_bar(const BarData& bar) {
    // Update ArrayManager
    am_->update_bar(bar);

    if (!am_->inited()) {
        return;
    }

    // Calculate MAs
    auto fast_ma = am_->sma_array(fast_window_);
    auto slow_ma = am_->sma_array(slow_window_);

    if (fast_ma.empty() || slow_ma.empty()) {
        return;
    }

    fast_ma1_ = fast_ma0_;
    slow_ma1_ = slow_ma0_;
    fast_ma0_ = fast_ma.back();
    slow_ma0_ = slow_ma.back();

    // Check crossover
    bool cross_over = (fast_ma0_ > slow_ma0_) && (fast_ma1_ <= slow_ma1_);
    bool cross_below = (fast_ma0_ < slow_ma0_) && (fast_ma1_ >= slow_ma1_);

    double pos = position();

    if (cross_over) {
        if (pos == 0) {
            buy(bar.close_price, 1.0);
        } else if (pos < 0) {
            cover(bar.close_price, std::abs(pos));
            buy(bar.close_price, 1.0);
        }
    } else if (cross_below) {
        if (pos == 0) {
            short_order(bar.close_price, 1.0);
        } else if (pos > 0) {
            sell(bar.close_price, pos);
            short_order(bar.close_price, 1.0);
        }
    }
}

//=============================================================================
// RSI Strategy
//=============================================================================

RsiStrategy::RsiStrategy() {
    author = "BitQuant";
    parameters = {"rsi_period", "oversold_level", "overbought_level"};
    variables = {"current_rsi"};

    am_ = std::make_unique<ArrayManager>(100);
}

void RsiStrategy::on_init() {
    write_log("RsiStrategy initializing...");
}

void RsiStrategy::on_bar(const BarData& bar) {
    am_->update_bar(bar);

    if (!am_->inited()) {
        return;
    }

    previous_rsi_ = current_rsi_;
    current_rsi_ = am_->rsi(rsi_period_);

    double pos = position();

    // RSI oversold - buy signal
    if (current_rsi_ < oversold_level_ && previous_rsi_ >= oversold_level_) {
        if (pos <= 0) {
            if (pos < 0) {
                cover(bar.close_price, std::abs(pos));
            }
            buy(bar.close_price, 1.0);
        }
    }
    // RSI overbought - sell signal
    else if (current_rsi_ > overbought_level_ && previous_rsi_ <= overbought_level_) {
        if (pos >= 0) {
            if (pos > 0) {
                sell(bar.close_price, pos);
            }
            short_order(bar.close_price, 1.0);
        }
    }
}

//=============================================================================
// Bollinger Band Strategy
//=============================================================================

BollingerStrategy::BollingerStrategy() {
    author = "BitQuant";
    parameters = {"period", "dev"};
    variables = {"upper_band", "middle_band", "lower_band"};

    am_ = std::make_unique<ArrayManager>(100);
}

void BollingerStrategy::on_init() {
    write_log("BollingerStrategy initializing...");
}

void BollingerStrategy::on_bar(const BarData& bar) {
    am_->update_bar(bar);

    if (!am_->inited()) {
        return;
    }

    auto [upper, middle, lower] = am_->bollinger(period_, dev_);
    upper_band_ = upper;
    middle_band_ = middle;
    lower_band_ = lower;

    double pos = position();
    double price = bar.close_price;

    // Price breaks above upper band - buy signal
    if (price > upper_band_) {
        if (pos <= 0) {
            if (pos < 0) {
                cover(price, std::abs(pos));
            }
            buy(price, 1.0);
        }
    }
    // Price breaks below lower band - sell signal
    else if (price < lower_band_) {
        if (pos >= 0) {
            if (pos > 0) {
                sell(price, pos);
            }
            short_order(price, 1.0);
        }
    }
    // Price returns to middle band - exit
    else if (pos > 0 && price < middle_band_) {
        sell(price, pos);
    } else if (pos < 0 && price > middle_band_) {
        cover(price, std::abs(pos));
    }
}

//=============================================================================
// ATR + RSI Combined Strategy
//=============================================================================

AtrRsiStrategy::AtrRsiStrategy() {
    author = "BitQuant";
    parameters = {"atr_period", "rsi_period", "oversold_level", "overbought_level"};
    variables = {"current_atr", "current_rsi"};

    am_ = std::make_unique<ArrayManager>(100);
}

void AtrRsiStrategy::on_init() {
    write_log("AtrRsiStrategy initializing...");
}

void AtrRsiStrategy::on_bar(const BarData& bar) {
    am_->update_bar(bar);

    if (!am_->inited()) {
        return;
    }

    current_atr_ = am_->atr(atr_period_);
    current_rsi_ = am_->rsi(rsi_period_);

    double pos = position();
    double price = bar.close_price;

    // Calculate volatility threshold
    double atr_threshold = current_atr_ * 0.5;

    // Buy signal: RSI oversold and price moved enough (volatility)
    if (current_rsi_ < oversold_level_) {
        if (pos <= 0) {
            if (pos < 0) {
                cover(price, std::abs(pos));
            }
            // Size based on ATR
            double size = std::min(1.0, atr_threshold / price);
            buy(price, std::max(0.1, size));
        }
    }
    // Sell signal: RSI overbought
    else if (current_rsi_ > overbought_level_) {
        if (pos >= 0) {
            if (pos > 0) {
                sell(price, pos);
            }
            double size = std::min(1.0, atr_threshold / price);
            short_order(price, std::max(0.1, size));
        }
    }
}

//=============================================================================
// Turtle Strategy
//=============================================================================

TurtleStrategy::TurtleStrategy() {
    author = "BitQuant";
    parameters = {"entry_window", "exit_window", "atr_period"};
    variables = {"entry_high", "entry_low", "current_atr"};

    am_ = std::make_unique<ArrayManager>(100);
}

void TurtleStrategy::on_init() {
    write_log("TurtleStrategy initializing...");
}

void TurtleStrategy::on_bar(const BarData& bar) {
    am_->update_bar(bar);

    if (!am_->inited()) {
        return;
    }

    // Calculate Donchian channels using close prices
    // (since we don't have high_array/low_array, use available data)
    double highest = bar.high_price;
    double lowest = bar.low_price;

    // Use recent bars for channel calculation
    // Simplified approach: use current bar's high/low
    // In real implementation, would track historical highs/low

    current_atr_ = am_->atr(atr_period_);

    double pos = position();
    double price = bar.close_price;

    // Entry signals - simplified
    // Price breaks recent high - buy
    if (price > highest * 0.99) {  // Near high
        if (pos <= 0) {
            if (pos < 0) {
                cover(price, std::abs(pos));
            }
            buy(price, 1.0);
        }
    }
    // Price breaks recent low - sell
    else if (price < lowest * 1.01) {  // Near low
        if (pos >= 0) {
            if (pos > 0) {
                sell(price, pos);
            }
            short_order(price, 1.0);
        }
    }
    // Exit signals - simplified ATR-based stop
    else if (pos > 0 && price < bar.open_price - current_atr_) {
        sell(price, pos);
    } else if (pos < 0 && price > bar.open_price + current_atr_) {
        cover(price, std::abs(pos));
    }
}

//=============================================================================
// Strategy Factory
//=============================================================================

std::unique_ptr<IStrategy> StrategyFactory::create(const std::string& name) {
    if (name == "DoubleMa" || name == "DoubleMaStrategy") {
        return std::make_unique<DoubleMaStrategy>();
    } else if (name == "Rsi" || name == "RsiStrategy") {
        return std::make_unique<RsiStrategy>();
    } else if (name == "Bollinger" || name == "BollingerStrategy") {
        return std::make_unique<BollingerStrategy>();
    } else if (name == "AtrRsi" || name == "AtrRsiStrategy") {
        return std::make_unique<AtrRsiStrategy>();
    } else if (name == "Turtle" || name == "TurtleStrategy") {
        return std::make_unique<TurtleStrategy>();
    } else if (name == "GridMartin" || name == "GridMartinStrategy") {
        return std::make_unique<GridMartinStrategy>();
    }

    return nullptr;
}

std::vector<std::string> StrategyFactory::list_strategies() {
    return {
        "DoubleMa",
        "Rsi",
        "Bollinger",
        "AtrRsi",
        "Turtle",
        "GridMartin"
    };
}

} // namespace bitquant
