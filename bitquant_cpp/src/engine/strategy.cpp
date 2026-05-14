/**
 * @file strategy.cpp
 * @brief Strategy implementation
 *
 * Based on howtrader's CtaTemplate design patterns
 */

#include "engine/strategy.hpp"
#include "engine/broker.hpp"
#include <iostream>

namespace bitquant {

//=============================================================================
// IStrategy Implementation
//=============================================================================

double IStrategy::position() const {
    return broker_ ? broker_->position() : 0.0;
}

Direction IStrategy::position_side() const {
    return broker_ ? broker_->position_side() : Direction::NET;
}

bool IStrategy::is_flat() const {
    return broker_ ? broker_->position() == 0.0 : true;
}

bool IStrategy::is_long() const {
    return broker_ ? broker_->position() > 0.0 : false;
}

bool IStrategy::is_short() const {
    return broker_ ? broker_->position() < 0.0 : false;
}

EngineType IStrategy::get_engine_type() const {
    return broker_ ? EngineType::BACKTESTING : EngineType::BACKTESTING;
}

void IStrategy::write_log(const std::string& msg) {
    // Output to stdout for debugging
    // In production, this could be routed through a logging system
    std::cout << "[GridMartin] " << msg << std::endl;
}

order_id_t IStrategy::buy(double price, double volume, bool stop) {
    if (broker_ && trading_) {
        if (stop) {
            // Send stop order for buy
            return broker_->send_stop_order(Direction::LONG, Offset::OPEN, price, volume);
        }
        return broker_->buy(price, volume);
    }
    return 0;
}

order_id_t IStrategy::sell(double price, double volume, bool stop) {
    if (broker_ && trading_) {
        if (stop) {
            return broker_->send_stop_order(Direction::SHORT, Offset::CLOSE, price, volume);
        }
        return broker_->sell(price, volume);
    }
    return 0;
}

order_id_t IStrategy::short_order(double price, double volume, bool stop) {
    if (broker_ && trading_) {
        if (stop) {
            return broker_->send_stop_order(Direction::SHORT, Offset::OPEN, price, volume);
        }
        return broker_->short_order(price, volume);
    }
    return 0;
}

order_id_t IStrategy::cover(double price, double volume, bool stop) {
    if (broker_ && trading_) {
        if (stop) {
            return broker_->send_stop_order(Direction::LONG, Offset::CLOSE, price, volume);
        }
        return broker_->cover(price, volume);
    }
    return 0;
}

void IStrategy::market_buy(double volume) {
    if (broker_ && trading_) {
        broker_->market_buy(volume);
    }
}

void IStrategy::market_sell(double volume) {
    if (broker_ && trading_) {
        broker_->market_sell(volume);
    }
}

void IStrategy::cancel_order(order_id_t order_id) {
    if (broker_) {
        broker_->cancel_order(order_id);
    }
}

void IStrategy::cancel_all_orders() {
    if (broker_) {
        broker_->cancel_all_orders();
    }
}

//=============================================================================
// TargetPosTemplate Implementation
//=============================================================================

void TargetPosTemplate::on_tick(const TickData& tick) {
    last_tick_ = &tick;
    if (trading_) {
        trade();
    }
}

void TargetPosTemplate::on_bar(const BarData& bar) {
    last_bar_ = &bar;
}

void TargetPosTemplate::on_order(const Order& order) {
    // Remove filled/cancelled orders from active list
    if (!order.is_active()) {
        auto it = std::find(active_orderids_.begin(), active_orderids_.end(), order.order_id);
        if (it != active_orderids_.end()) {
            active_orderids_.erase(it);
        }
    }
}

void TargetPosTemplate::set_target_pos(double target_pos) {
    target_pos_ = target_pos;
    trade();
}

bool TargetPosTemplate::check_order_finished() const {
    return active_orderids_.empty();
}

void TargetPosTemplate::trade() {
    if (!check_order_finished()) {
        cancel_old_orders();
    } else {
        send_new_orders();
    }
}

void TargetPosTemplate::cancel_old_orders() {
    for (order_id_t order_id : active_orderids_) {
        auto it = std::find(cancel_orderids_.begin(), cancel_orderids_.end(), order_id);
        if (it == cancel_orderids_.end()) {
            cancel_order(order_id);
            cancel_orderids_.push_back(order_id);
        }
    }
}

void TargetPosTemplate::send_new_orders() {
    double pos_change = target_pos_ - position();
    if (pos_change == 0) return;

    double order_price = 0;
    if (last_tick_) {
        if (pos_change > 0) {
            order_price = last_tick_->ask_price_1 + tick_add_;
        } else {
            order_price = last_tick_->bid_price_1 - tick_add_;
        }
    } else if (last_bar_) {
        if (pos_change > 0) {
            order_price = last_bar_->close_price + tick_add_;
        } else {
            order_price = last_bar_->close_price - tick_add_;
        }
    }

    EngineType engine_type = get_engine_type();
    if (engine_type == EngineType::BACKTESTING) {
        order_id_t order_id;
        if (pos_change > 0) {
            order_id = buy(order_price, std::abs(pos_change));
        } else {
            order_id = short_order(order_price, std::abs(pos_change));
        }
        active_orderids_.push_back(order_id);
    } else {
        // Live trading logic with proper offset handling
        if (!active_orderids_.empty()) return;

        order_id_t order_id;
        if (pos_change > 0) {
            if (position() < 0) {
                // Cover short first
                double cover_vol = std::min(std::abs(pos_change), std::abs(position()));
                order_id = cover(order_price, cover_vol);
            } else {
                order_id = buy(order_price, std::abs(pos_change));
            }
        } else {
            if (position() > 0) {
                // Sell long first
                double sell_vol = std::min(std::abs(pos_change), position());
                order_id = sell(order_price, sell_vol);
            } else {
                order_id = short_order(order_price, std::abs(pos_change));
            }
        }
        active_orderids_.push_back(order_id);
    }
}

} // namespace bitquant
