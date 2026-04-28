/**
 * @file strategy_context.cpp
 * @brief Strategy context implementation
 */

#include "engine/strategy_context.hpp"
#include "engine/broker.hpp"
#include "engine/strategy_manager.hpp"
#include <algorithm>

namespace bitquant {

//=============================================================================
// Constructor
//=============================================================================

StrategyContext::StrategyContext(
    const std::string& name,
    Broker* broker,
    StrategyManager* manager
)
    : name_(name), broker_(broker), manager_(manager) {}

//=============================================================================
// Position Management
//=============================================================================

void StrategyContext::update_position(const TradeData& trade) {
    double trade_value = trade.price * trade.volume;

    if (trade.direction == Direction::LONG) {
        // Long position
        if (position_direction_ == Direction::SHORT && position_ > 0) {
            // Closing short position
            double close_volume = std::min(position_, trade.volume);
            double pnl = (avg_price_ - trade.price) * close_volume;
            realized_pnl_ += pnl;

            position_ -= close_volume;
            if (position_ <= 0) {
                position_ = 0;
                position_direction_ = Direction::NET;
                avg_price_ = 0;
                total_cost_ = 0;
            }

            // Opening new long if trade volume > close volume
            if (trade.volume > close_volume) {
                double new_volume = trade.volume - close_volume;
                position_ = new_volume;
                position_direction_ = Direction::LONG;
                avg_price_ = trade.price;
                total_cost_ = trade.price * new_volume;
            }
        } else {
            // Adding to long position
            double new_cost = total_cost_ + trade_value;
            position_ += trade.volume;
            total_cost_ = new_cost;
            avg_price_ = position_ > 0 ? total_cost_ / position_ : 0;
            position_direction_ = Direction::LONG;
        }
    } else {
        // Short position
        if (position_direction_ == Direction::LONG && position_ > 0) {
            // Closing long position
            double close_volume = std::min(position_, trade.volume);
            double pnl = (trade.price - avg_price_) * close_volume;
            realized_pnl_ += pnl;

            position_ -= close_volume;
            if (position_ <= 0) {
                position_ = 0;
                position_direction_ = Direction::NET;
                avg_price_ = 0;
                total_cost_ = 0;
            }

            // Opening new short if trade volume > close volume
            if (trade.volume > close_volume) {
                double new_volume = trade.volume - close_volume;
                position_ = new_volume;
                position_direction_ = Direction::SHORT;
                avg_price_ = trade.price;
                total_cost_ = trade.price * new_volume;
            }
        } else {
            // Adding to short position
            double new_cost = total_cost_ + trade_value;
            position_ += trade.volume;
            total_cost_ = new_cost;
            avg_price_ = position_ > 0 ? total_cost_ / position_ : 0;
            position_direction_ = Direction::SHORT;
        }
    }

    // Track trade
    add_trade(trade);
}

//=============================================================================
// PnL Tracking
//=============================================================================

double StrategyContext::unrealized_pnl(double current_price) const {
    if (position_ <= 0) return 0.0;

    if (position_direction_ == Direction::LONG) {
        return (current_price - avg_price_) * position_;
    } else {
        return (avg_price_ - current_price) * position_;
    }
}

double StrategyContext::total_pnl(double current_price) const {
    return realized_pnl_ + unrealized_pnl(current_price);
}

//=============================================================================
// Order Tracking
//=============================================================================

void StrategyContext::add_order(const std::string& vt_orderid) {
    active_orders_.insert(vt_orderid);
}

void StrategyContext::remove_order(const std::string& vt_orderid) {
    active_orders_.erase(vt_orderid);
}

//=============================================================================
// Trade History
//=============================================================================

void StrategyContext::add_trade(const TradeData& trade) {
    trades_.push_back(trade);

    // Update winning/losing count based on round-trip
    // This is simplified - proper implementation would track round trips
}

//=============================================================================
// Statistics
//=============================================================================

double StrategyContext::win_rate() const {
    int total = winning_trades_ + losing_trades_;
    return total > 0 ? static_cast<double>(winning_trades_) / total : 0.0;
}

//=============================================================================
// State Persistence
//=============================================================================

std::unordered_map<std::string, double> StrategyContext::save_state() const {
    std::unordered_map<std::string, double> state;

    state["position"] = position_;
    state["avg_price"] = avg_price_;
    state["realized_pnl"] = realized_pnl_;
    state["winning_trades"] = static_cast<double>(winning_trades_);
    state["losing_trades"] = static_cast<double>(losing_trades_);

    // Save custom variables
    for (const auto& [key, value] : variables_) {
        state[key] = value;
    }

    return state;
}

void StrategyContext::load_state(const std::unordered_map<std::string, double>& state) {
    auto get_value = [&state](const std::string& key, double default_val) -> double {
        auto it = state.find(key);
        return it != state.end() ? it->second : default_val;
    };

    position_ = get_value("position", 0.0);
    avg_price_ = get_value("avg_price", 0.0);
    realized_pnl_ = get_value("realized_pnl", 0.0);
    winning_trades_ = static_cast<int>(get_value("winning_trades", 0.0));
    losing_trades_ = static_cast<int>(get_value("losing_trades", 0.0));

    // Determine position direction from position value
    if (position_ > 0) {
        position_direction_ = Direction::LONG;
    } else if (position_ < 0) {
        position_direction_ = Direction::SHORT;
        position_ = -position_;  // Store as positive
    } else {
        position_direction_ = Direction::NET;
    }
}

//=============================================================================
// Factory
//=============================================================================

std::unique_ptr<StrategyContext> StrategyContextFactory::create(
    const std::string& name,
    Broker* broker,
    StrategyManager* manager
) {
    return std::make_unique<StrategyContext>(name, broker, manager);
}

} // namespace bitquant
