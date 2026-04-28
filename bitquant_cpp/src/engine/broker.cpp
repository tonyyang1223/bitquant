/**
 * @file broker.cpp
 * @brief Complete implementation of Broker with order matching
 *
 * Based on howtrader's BacktestingEngine design:
 * - cross_limit_order(): Limit order matching logic
 * - cross_stop_order(): Stop order matching logic
 * - Daily PnL calculation
 */

#include "engine/broker.hpp"
#include "statistics/performance.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>

namespace bitquant {

Broker::Broker(const BrokerConfig& config)
    : config_(config), cash_(config.initial_cash), initial_cash_(config.initial_cash) {}

void Broker::set_strategy(std::unique_ptr<IStrategy> strategy) {
    strategy_ = std::move(strategy);
    strategy_->broker_ = this;
}

void Broker::set_data(const std::vector<BarData>& data) {
    data_ = data;
}

double Broker::equity() const {
    return cash_ + asset_value_ + position_.unrealized_pnl;
}

double Broker::available_cash() const {
    double margin = margin_used();
    return cash_ - margin;
}

double Broker::margin_used() const {
    if (position_.volume <= 0) return 0.0;
    return position_.avg_price * position_.volume / config_.leverage;
}

void Broker::run() {
    // Reset state
    cash_ = initial_cash_;
    initial_cash_ = initial_cash_;
    asset_value_ = 0.0;
    position_ = Position{};
    active_orders_.clear();
    active_stop_orders_.clear();
    trades_.clear();
    order_history_.clear();
    stop_orders_.clear();
    equity_curve_.clear();
    daily_results_.clear();
    next_order_id_ = 1;
    next_trade_id_ = 1;
    limit_order_count_ = 0;
    stop_order_count_ = 0;

    if (!strategy_) {
        std::cerr << "Error: No strategy set" << std::endl;
        return;
    }

    // Initialize strategy
    strategy_->on_init();
    strategy_->on_start();

    // Run backtesting
    for (const auto& bar : data_) {
        current_bar_ = &bar;

        // Cross limit orders (match limit orders against current bar)
        cross_limit_order(bar);

        // Cross stop orders (check if stop orders are triggered)
        cross_stop_order(bar);

        // Update position PnL with current bar
        update_margin_pnl(bar);

        // Update daily result
        update_daily_close(bar.close_price, bar.datetime);

        // Record equity
        equity_curve_.emplace_back(bar.datetime, equity());

        // Call strategy
        strategy_->on_bar(bar);
    }

    strategy_->on_stop();

    // Calculate final statistics
    calculate_performance();
}

//=============================================================================
// Order Placement Methods (Based on howtrader CtaTemplate)
//=============================================================================

order_id_t Broker::buy(double price, double volume) {
    return send_limit_order(Direction::LONG, Offset::OPEN, price, volume);
}

order_id_t Broker::sell(double price, double volume) {
    return send_limit_order(Direction::SHORT, Offset::CLOSE, price, volume);
}

order_id_t Broker::short_order(double price, double volume) {
    if (!config_.allow_short) {
        std::cerr << "Error: Short selling not allowed" << std::endl;
        return 0;
    }
    return send_limit_order(Direction::SHORT, Offset::OPEN, price, volume);
}

order_id_t Broker::cover(double price, double volume) {
    return send_limit_order(Direction::LONG, Offset::CLOSE, price, volume);
}

order_id_t Broker::market_buy(double volume) {
    if (!current_bar_) return 0;

    double fill_price = apply_slippage(current_bar_->close_price, Direction::LONG);

    // Execute market order immediately
    Trade trade;
    trade.trade_id = next_trade_id_++;
    trade.order_id = 0;  // Market order has no ID
    trade.side = Direction::LONG;
    trade.price = fill_price;
    trade.volume = volume;
    trade.commission = fill_price * volume * config_.commission_rate;
    trade.timestamp = current_bar_->datetime;

    execute_trade(trade);
    return 0;
}

order_id_t Broker::market_sell(double volume) {
    if (!current_bar_) return 0;

    double fill_price = apply_slippage(current_bar_->close_price, Direction::SHORT);

    Trade trade;
    trade.trade_id = next_trade_id_++;
    trade.order_id = 0;
    trade.side = Direction::SHORT;
    trade.price = fill_price;
    trade.volume = volume;
    trade.commission = fill_price * volume * config_.commission_rate;
    trade.timestamp = current_bar_->datetime;

    execute_trade(trade);
    return 0;
}

//=============================================================================
// Limit Order Methods (Based on howtrader BacktestingEngine)
//=============================================================================

order_id_t Broker::send_limit_order(Direction direction, Offset offset, double price, double volume) {
    limit_order_count_++;

    Order order;
    order.order_id = limit_order_count_;
    order.side = direction;
    order.type = OrderType::LIMIT;
    order.status = Status::SUBMITTING;
    order.price = price;
    order.volume = volume;
    order.create_time = current_bar_ ? current_bar_->datetime : 0;

    active_orders_[order.order_id] = order;
    order_history_.push_back(order);

    return order.order_id;
}

order_id_t Broker::send_stop_order(Direction direction, Offset offset, double price, double volume) {
    stop_order_count_++;

    StopOrder stop_order;
    stop_order.stop_orderid = "STOP." + std::to_string(stop_order_count_);
    stop_order.vt_symbol = "";
    stop_order.direction = direction;
    stop_order.offset = offset;
    stop_order.price = price;
    stop_order.volume = volume;
    stop_order.datetime = current_bar_ ? current_bar_->datetime : 0;
    stop_order.status = StopOrderStatus::WAITING;

    active_stop_orders_[stop_order.stop_orderid] = stop_order;
    stop_orders_[stop_order.stop_orderid] = stop_order;

    return stop_order_count_;
}

//=============================================================================
// Order Cancellation
//=============================================================================

void Broker::cancel_order(order_id_t order_id) {
    // Check limit orders
    auto it = active_orders_.find(order_id);
    if (it != active_orders_.end()) {
        it->second.status = Status::CANCELLED;
        active_orders_.erase(it);
        return;
    }

    // Check stop orders
    std::string stop_id = "STOP." + std::to_string(order_id);
    auto stop_it = active_stop_orders_.find(stop_id);
    if (stop_it != active_stop_orders_.end()) {
        stop_it->second.status = StopOrderStatus::CANCELLED;
        active_stop_orders_.erase(stop_it);
    }
}

void Broker::cancel_all_orders() {
    for (auto& [id, order] : active_orders_) {
        order.status = Status::CANCELLED;
    }
    active_orders_.clear();

    for (auto& [id, stop_order] : active_stop_orders_) {
        stop_order.status = StopOrderStatus::CANCELLED;
    }
    active_stop_orders_.clear();
}

//=============================================================================
// Order Matching (Based on howtrader's cross_limit_order)
//=============================================================================

void Broker::cross_limit_order(const BarData& bar) {
    // For bar mode: use low/high prices for matching
    double long_cross_price = bar.low_price;
    double short_cross_price = bar.high_price;
    double long_best_price = bar.open_price;
    double short_best_price = bar.open_price;

    for (auto& [order_id, order] : active_orders_) {
        // Push order update with status "not traded" (pending)
        if (order.status == Status::SUBMITTING) {
            order.status = Status::NOTTRADED;
            if (strategy_) {
                strategy_->on_order(order);
            }
        }

        // Check whether limit orders can be filled
        bool long_cross = (order.side == Direction::LONG &&
                          order.price >= long_cross_price && long_cross_price > 0);
        bool short_cross = (order.side == Direction::SHORT &&
                           order.price <= short_cross_price && short_cross_price > 0);

        if (!long_cross && !short_cross) {
            continue;
        }

        // Push order update with status "all traded" (filled)
        order.filled_volume = order.volume;
        order.status = Status::ALLTRADED;

        // Create trade
        double trade_price;
        if (long_cross) {
            trade_price = std::min(order.price, long_best_price);
        } else {
            trade_price = std::max(order.price, short_best_price);
        }

        Trade trade;
        trade.trade_id = next_trade_id_++;
        trade.order_id = order.order_id;
        trade.side = order.side;
        trade.price = trade_price;
        trade.volume = order.volume;
        trade.commission = trade_price * order.volume * config_.commission_rate;
        trade.timestamp = bar.datetime;

        execute_trade(trade);

        if (strategy_) {
            strategy_->on_order(order);
            strategy_->on_trade(trade);
        }
    }

    // Remove filled orders
    for (auto it = active_orders_.begin(); it != active_orders_.end(); ) {
        if (it->second.status == Status::ALLTRADED) {
            it = active_orders_.erase(it);
        } else {
            ++it;
        }
    }
}

void Broker::cross_stop_order(const BarData& bar) {
    // For bar mode
    double long_cross_price = bar.high_price;
    double short_cross_price = bar.low_price;
    double long_best_price = bar.open_price;
    double short_best_price = bar.open_price;

    for (auto& [stop_id, stop_order] : active_stop_orders_) {
        // Check whether stop order can be triggered
        bool long_cross = (stop_order.direction == Direction::LONG &&
                          stop_order.price <= long_cross_price);
        bool short_cross = (stop_order.direction == Direction::SHORT &&
                           stop_order.price >= short_cross_price);

        if (!long_cross && !short_cross) {
            continue;
        }

        // Create limit order from stop order
        limit_order_count_++;
        Order order;
        order.order_id = limit_order_count_;
        order.side = stop_order.direction;
        order.type = OrderType::LIMIT;
        order.status = Status::ALLTRADED;
        order.price = stop_order.price;
        order.volume = stop_order.volume;
        order.create_time = bar.datetime;
        order.filled_volume = stop_order.volume;

        // Create trade
        double trade_price;
        if (long_cross) {
            trade_price = std::max(stop_order.price, long_best_price);
        } else {
            trade_price = std::min(stop_order.price, short_best_price);
        }

        Trade trade;
        trade.trade_id = next_trade_id_++;
        trade.order_id = order.order_id;
        trade.side = order.side;
        trade.price = trade_price;
        trade.volume = order.volume;
        trade.commission = trade_price * order.volume * config_.commission_rate;
        trade.timestamp = bar.datetime;

        execute_trade(trade);

        // Update stop order status
        stop_order.status = StopOrderStatus::TRIGGERED;

        if (strategy_) {
            strategy_->on_order(order);
            strategy_->on_trade(trade);
        }
    }

    // Remove triggered stop orders
    for (auto it = active_stop_orders_.begin(); it != active_stop_orders_.end(); ) {
        if (it->second.status == StopOrderStatus::TRIGGERED) {
            it = active_stop_orders_.erase(it);
        } else {
            ++it;
        }
    }
}

//=============================================================================
// Trade Execution and Position Update
//=============================================================================

void Broker::execute_trade(const Trade& trade) {
    trades_.push_back(trade);
    update_position(trade);

    // Update cash
    double trade_value = trade.price * trade.volume;
    if (trade.side == Direction::LONG) {
        cash_ -= (trade_value + trade.commission);
        asset_value_ += trade_value;
    } else {
        cash_ += (trade_value - trade.commission);
        asset_value_ -= trade_value;
    }
}

void Broker::update_position(const Trade& trade) {
    double trade_value = trade.price * trade.volume;

    if (trade.side == Direction::LONG) {
        if (position_.side == Direction::SHORT && position_.volume > 0) {
            // Closing short position
            double close_volume = std::min(position_.volume, trade.volume);
            double pnl = (position_.avg_price - trade.price) * close_volume;
            position_.realized_pnl += pnl - trade.commission;

            position_.volume -= close_volume;
            if (position_.volume <= 0) {
                position_.side = Direction::NET;
                position_.volume = 0;
                position_.avg_price = 0;
            }

            // If trade volume > position volume, open new long
            if (trade.volume > close_volume) {
                double new_volume = trade.volume - close_volume;
                position_.side = Direction::LONG;
                position_.volume = new_volume;
                position_.avg_price = trade.price;
            }
        } else {
            // Opening or adding to long position
            double total_cost = position_.avg_price * position_.volume + trade_value;
            position_.volume += trade.volume;
            if (position_.volume > 0) {
                position_.avg_price = total_cost / position_.volume;
                position_.side = Direction::LONG;
            }
        }
    } else {  // SHORT
        if (position_.side == Direction::LONG && position_.volume > 0) {
            // Closing long position
            double close_volume = std::min(position_.volume, trade.volume);
            double pnl = (trade.price - position_.avg_price) * close_volume;
            position_.realized_pnl += pnl - trade.commission;

            position_.volume -= close_volume;
            if (position_.volume <= 0) {
                position_.side = Direction::NET;
                position_.volume = 0;
                position_.avg_price = 0;
            }

            // If trade volume > position volume, open new short
            if (trade.volume > close_volume) {
                double new_volume = trade.volume - close_volume;
                position_.side = Direction::SHORT;
                position_.volume = new_volume;
                position_.avg_price = trade.price;
            }
        } else {
            // Opening or adding to short position
            double total_cost = position_.avg_price * position_.volume + trade_value;
            position_.volume += trade.volume;
            if (position_.volume > 0) {
                position_.avg_price = total_cost / position_.volume;
                position_.side = Direction::SHORT;
            }
        }
    }
}

//=============================================================================
// Daily Result and PnL Calculation (Based on howtrader's DailyResult)
//=============================================================================

void Broker::update_daily_close(double close_price, int64_t datetime) {
    // Extract date from datetime (assuming datetime is in milliseconds)
    int64_t date = datetime / 86400000;  // Convert ms to days since epoch

    auto it = daily_results_.find(date);
    if (it == daily_results_.end()) {
        daily_results_[date] = DailyResult{date, close_price};
    } else {
        it->second.close_price = close_price;
    }
}

void Broker::update_margin_pnl(const BarData& bar) {
    position_.update_unrealized_pnl(bar.close_price);
}

//=============================================================================
// Helper Methods
//=============================================================================

double Broker::apply_slippage(double price, Direction side) const {
    if (side == Direction::LONG) {
        return price * (1 + config_.slippage_rate);
    } else {
        return price * (1 - config_.slippage_rate);
    }
}

void Broker::calculate_performance() {
    performance_ = PerformanceCalculator::calculate(trades_, equity_curve_, initial_cash_);
}

//=============================================================================
// Strategy Optimization
//=============================================================================

template<typename StrategyClass, typename... Args>
void Broker::optimize_strategy(
    const std::unordered_map<std::string, std::vector<double>>& params,
    Args&&... args) {

    is_optimizing_strategy_ = true;

    // TODO: Implement parameter optimization with multiprocessing
    // For now, just run with default parameters

    is_optimizing_strategy_ = false;
}

} // namespace bitquant
