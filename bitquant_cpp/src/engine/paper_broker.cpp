/**
 * @file paper_broker.cpp
 * @brief Paper trading broker implementation
 */

#include "engine/paper_broker.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace bitquant {

//=============================================================================
// Constructor
//=============================================================================

PaperBroker::PaperBroker(const PaperBrokerConfig& config)
    : config_(config)
    , cash_(config.initial_capital)
{
}

//=============================================================================
// Configuration
//=============================================================================

void PaperBroker::set_capital(double capital) {
    cash_ = capital;
}

void PaperBroker::set_commission(double rate) {
    config_.commission_rate = rate;
}

void PaperBroker::set_slippage(double rate) {
    config_.slippage_rate = rate;
}

//=============================================================================
// Order Interface
//=============================================================================

std::string PaperBroker::send_order(const OrderRequest& req) {
    std::lock_guard<std::mutex> lock(orders_mutex_);

    // Generate order ID
    std::string order_id = generate_order_id();

    // Create order data
    OrderData order;
    order.symbol = req.symbol;
    order.exchange = req.exchange;
    order.orderid = order_id;
    order.type = req.type;
    order.direction = req.direction;
    order.price = req.price;
    order.volume = req.volume;
    order.traded = 0.0;
    order.traded_price = 0.0;
    order.status = Status::NOTTRADED;
    order.datetime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    order.gateway_name = "PAPER";

    // Store order
    open_orders_[order_id] = order;
    all_orders_[order_id] = order;

    // Notify callback
    if (order_callback_) {
        order_callback_(order);
    }

    std::cout << "[PaperBroker] Order placed: " << order_id
              << " " << (order.direction == Direction::LONG ? "BUY" : "SELL")
              << " " << order.volume << " @ " << order.price
              << " (" << order.symbol << ")" << std::endl;

    return order_id;
}

bool PaperBroker::cancel_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(orders_mutex_);

    auto it = open_orders_.find(order_id);
    if (it == open_orders_.end()) {
        return false;
    }

    // Update status
    it->second.status = Status::CANCELLED;
    all_orders_[order_id].status = Status::CANCELLED;

    // Remove from open orders
    open_orders_.erase(it);

    // Notify callback
    if (order_callback_) {
        order_callback_(all_orders_[order_id]);
    }

    std::cout << "[PaperBroker] Order cancelled: " << order_id << std::endl;
    return true;
}

void PaperBroker::cancel_all_orders() {
    std::lock_guard<std::mutex> lock(orders_mutex_);

    for (auto& [id, order] : open_orders_) {
        order.status = Status::CANCELLED;
        all_orders_[id].status = Status::CANCELLED;

        if (order_callback_) {
            order_callback_(order);
        }
    }

    open_orders_.clear();
    std::cout << "[PaperBroker] All orders cancelled" << std::endl;
}

//=============================================================================
// Market Data Input
//=============================================================================

void PaperBroker::on_tick(const TickData& tick) {
    // Update last price
    {
        std::lock_guard<std::mutex> lock(prices_mutex_);
        last_prices_[tick.symbol] = tick.last_price;
    }

    // Try to fill orders using bid/ask
    double bid = tick.bid_price_1 > 0 ? tick.bid_price_1 : tick.last_price;
    double ask = tick.ask_price_1 > 0 ? tick.ask_price_1 : tick.last_price;

    try_fill_orders(bid, ask, tick.symbol, tick.datetime);
}

void PaperBroker::on_bar(const BarData& bar) {
    // Update last price
    {
        std::lock_guard<std::mutex> lock(prices_mutex_);
        last_prices_[bar.symbol] = bar.close_price;
    }

    // Use close price for both bid/ask (approximation)
    try_fill_orders(bar.close_price, bar.close_price, bar.symbol, bar.datetime);
}

//=============================================================================
// State Queries
//=============================================================================

double PaperBroker::get_balance() const {
    return cash_;
}

double PaperBroker::get_position(const std::string& symbol) const {
    auto it = positions_.find(symbol);
    return it != positions_.end() ? it->second : 0.0;
}

std::unordered_map<std::string, double> PaperBroker::get_positions() const {
    return positions_;
}

std::vector<OrderData> PaperBroker::get_open_orders() const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<OrderData> orders;
    for (const auto& [id, order] : open_orders_) {
        orders.push_back(order);
    }
    return orders;
}

std::vector<TradeData> PaperBroker::get_trades() const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    return trades_;
}

double PaperBroker::get_equity() const {
    double equity = cash_;

    std::lock_guard<std::mutex> lock(prices_mutex_);
    for (const auto& [symbol, volume] : positions_) {
        auto price_it = last_prices_.find(symbol);
        if (price_it != last_prices_.end()) {
            equity += volume * price_it->second;
        }
    }

    return equity;
}

//=============================================================================
// Callback Registration
//=============================================================================

void PaperBroker::on_order(PaperOrderCallback callback) {
    order_callback_ = std::move(callback);
}

void PaperBroker::on_trade(PaperTradeCallback callback) {
    trade_callback_ = std::move(callback);
}

//=============================================================================
// Statistics
//=============================================================================

double PaperBroker::get_total_commission() const {
    return total_commission_;
}

int PaperBroker::get_trade_count() const {
    return trade_count_;
}

//=============================================================================
// Internal Methods
//=============================================================================

std::string PaperBroker::generate_order_id() {
    order_counter_++;
    std::ostringstream ss;
    ss << "PAPER_" << order_counter_;
    return ss.str();
}

void PaperBroker::try_fill_orders(double bid, double ask, const std::string& symbol, int64_t timestamp) {
    std::lock_guard<std::mutex> lock(orders_mutex_);

    std::vector<std::string> filled_orders;

    for (auto& [id, order] : open_orders_) {
        if (order.symbol != symbol) {
            continue;
        }

        bool should_fill = false;
        double fill_price = 0.0;

        if (order.direction == Direction::LONG) {
            // Buy order - use ask price
            double effective_ask = ask * (1.0 + config_.slippage_rate);

            if (order.type == OrderType::TAKER) {
                // Market order - fill immediately
                should_fill = true;
                fill_price = effective_ask;
            } else if (order.type == OrderType::LIMIT || order.type == OrderType::MAKER) {
                // Limit order - fill if price <= order price
                if (order.price >= effective_ask) {
                    should_fill = true;
                    fill_price = std::min(order.price, effective_ask);
                }
            }
        } else {
            // Sell order - use bid price
            double effective_bid = bid * (1.0 - config_.slippage_rate);

            if (order.type == OrderType::TAKER) {
                should_fill = true;
                fill_price = effective_bid;
            } else if (order.type == OrderType::LIMIT || order.type == OrderType::MAKER) {
                if (order.price <= effective_bid) {
                    should_fill = true;
                    fill_price = std::max(order.price, effective_bid);
                }
            }
        }

        if (should_fill) {
            execute_fill(order, fill_price, order.volume - order.traded, timestamp);
            filled_orders.push_back(id);
        }
    }

    // Remove filled orders
    for (const auto& id : filled_orders) {
        open_orders_.erase(id);
    }
}

void PaperBroker::execute_fill(OrderData& order, double fill_price, double fill_volume, int64_t timestamp) {
    double trade_value = fill_price * fill_volume;
    double commission = trade_value * config_.commission_rate;

    // Update order
    order.traded += fill_volume;
    order.traded_price = fill_price;
    order.update_time = timestamp;

    if (order.traded >= order.volume) {
        order.status = Status::ALLTRADED;
    } else {
        order.status = Status::PARTTRADED;
    }

    all_orders_[order.orderid] = order;

    // Update position and cash
    if (order.direction == Direction::LONG) {
        // Buy
        cash_ -= trade_value;
        cash_ -= commission;

        double old_pos = positions_[order.symbol];
        double new_volume = old_pos + fill_volume;

        // Update average cost
        if (old_pos > 0) {
            double old_cost = avg_cost_[order.symbol] * old_pos;
            avg_cost_[order.symbol] = (old_cost + trade_value) / new_volume;
        } else {
            avg_cost_[order.symbol] = fill_price;
        }

        positions_[order.symbol] = new_volume;
    } else {
        // Sell
        cash_ += trade_value;
        cash_ -= commission;

        positions_[order.symbol] -= fill_volume;

        if (positions_[order.symbol] <= 0.0001) {
            positions_[order.symbol] = 0.0;
            avg_cost_[order.symbol] = 0.0;
        }
    }

    // Create trade record
    TradeData trade;
    trade.symbol = order.symbol;
    trade.exchange = order.exchange;
    trade.orderid = order.orderid;
    trade.tradeid = generate_order_id();
    trade.direction = order.direction;
    trade.price = fill_price;
    trade.volume = fill_volume;
    trade.datetime = timestamp;
    trade.gateway_name = "PAPER";

    trades_.push_back(trade);

    // Update statistics
    total_commission_ += commission;
    trade_count_++;

    // Notify callbacks
    if (order_callback_) {
        order_callback_(order);
    }
    if (trade_callback_) {
        trade_callback_(trade);
    }

    std::cout << "[PaperBroker] Order filled: " << order.orderid
              << " " << (order.direction == Direction::LONG ? "BUY" : "SELL")
              << " " << fill_volume << " @ " << fill_price
              << " (commission: " << commission << ")" << std::endl;
}

} // namespace bitquant
