/**
 * @file risk_manager.cpp
 * @brief Risk management engine implementation
 *
 * Based on howtrader's RiskEngine design patterns
 */

#include "engine/risk_manager.hpp"
#include <algorithm>

namespace bitquant {

//=============================================================================
// ActiveOrderBook Implementation
//=============================================================================

ActiveOrderBook::ActiveOrderBook(const std::string& vt_symbol)
    : vt_symbol_(vt_symbol) {}

void ActiveOrderBook::update_order(const OrderData& order) {
    std::string order_id = order.vt_orderid();

    if (order.is_active()) {
        // Add to appropriate side
        if (order.direction == Direction::LONG) {
            bid_prices_[order_id] = order.price;
            ask_prices_.erase(order_id);
        } else {
            ask_prices_[order_id] = order.price;
            bid_prices_.erase(order_id);
        }
    } else {
        // Remove from both sides
        bid_prices_.erase(order_id);
        ask_prices_.erase(order_id);
    }
}

double ActiveOrderBook::get_best_bid() const {
    if (bid_prices_.empty()) return 0.0;
    auto it = std::max_element(bid_prices_.begin(), bid_prices_.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    return it->second;
}

double ActiveOrderBook::get_best_ask() const {
    if (ask_prices_.empty()) return 0.0;
    auto it = std::min_element(ask_prices_.begin(), ask_prices_.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    return it->second;
}

void ActiveOrderBook::clear() {
    bid_prices_.clear();
    ask_prices_.clear();
}

//=============================================================================
// RiskManager Implementation
//=============================================================================

RiskManager::RiskManager(const RiskConfig& config)
    : config_(config) {}

bool RiskManager::check_risk(const OrderRequest& req, const std::string& gateway_name) {
    if (!config_.active) {
        return true;
    }

    // 1. Check order volume is positive
    if (req.volume <= 0) {
        write_log("委托数量必须大于0 (Order volume must be positive)");
        return false;
    }

    // 2. Check order size limit
    if (req.volume > config_.order_size_limit) {
        write_log("单笔委托数量" + std::to_string(req.volume) +
                  "，超过限制" + std::to_string(config_.order_size_limit));
        return false;
    }

    // 3. Check trade volume limit
    if (trade_count_ >= config_.trade_limit) {
        write_log("今日总成交合约数量" + std::to_string(trade_count_) +
                  "，超过限制" + std::to_string(config_.trade_limit));
        return false;
    }

    // 4. Check order flow limit
    if (order_flow_count_ >= config_.order_flow_limit) {
        write_log("委托流数量" + std::to_string(order_flow_count_) +
                  "，超过限制每" + std::to_string(config_.order_flow_clear) +
                  "秒" + std::to_string(config_.order_flow_limit) + "次");
        return false;
    }

    // 5. Check active order limit
    std::string vt_symbol = req.vt_symbol();
    ActiveOrderBook& order_book = get_order_book(vt_symbol);
    int active_bid_count = static_cast<int>(order_book.bid_count());
    int active_ask_count = static_cast<int>(order_book.ask_count());
    int active_order_count = active_bid_count + active_ask_count;

    if (active_order_count >= config_.active_order_limit) {
        write_log("活跃委托数量" + std::to_string(active_order_count) +
                  "，超过限制" + std::to_string(config_.active_order_limit));
        return false;
    }

    // 6. Check order cancel limit
    int cancel_count = order_cancel_counts_.count(vt_symbol) ?
                       order_cancel_counts_[vt_symbol] : 0;
    if (cancel_count >= config_.order_cancel_limit) {
        write_log("当日" + vt_symbol + "撤单次数" + std::to_string(cancel_count) +
                  "，超过限制" + std::to_string(config_.order_cancel_limit));
        return false;
    }

    // 7. Check self-trade prevention (Based on howtrader's logic)
    if (config_.prevent_self_trade) {
        if (req.direction == Direction::LONG) {
            double best_ask = order_book.get_best_ask();
            if (best_ask > 0 && req.price >= best_ask) {
                write_log("买入价格" + std::to_string(req.price) +
                          "大于等于已挂最低卖价" + std::to_string(best_ask) +
                          "，可能导致自成交 (Self-trade risk)");
                return false;
            }
        } else {
            double best_bid = order_book.get_best_bid();
            if (best_bid > 0 && req.price <= best_bid) {
                write_log("卖出价格" + std::to_string(req.price) +
                          "小于等于已挂最低买价" + std::to_string(best_bid) +
                          "，可能导致自成交 (Self-trade risk)");
                return false;
            }
        }
    }

    // Increment flow count if all checks pass
    order_flow_count_++;
    return true;
}

void RiskManager::on_trade(const TradeData& trade) {
    trade_count_ += trade.volume;
}

void RiskManager::on_order(const OrderData& order) {
    ActiveOrderBook& order_book = get_order_book(order.vt_symbol());
    order_book.update_order(order);

    // Track cancelled orders
    if (order.status == Status::CANCELLED) {
        std::string vt_symbol = order.vt_symbol();
        order_cancel_counts_[vt_symbol]++;
    }
}

void RiskManager::on_timer() {
    order_flow_timer_++;

    if (order_flow_timer_ >= config_.order_flow_clear) {
        order_flow_count_ = 0;
        order_flow_timer_ = 0;
    }
}

void RiskManager::reset_daily_counters() {
    trade_count_ = 0;
    order_flow_count_ = 0;
    order_flow_timer_ = 0;
    order_cancel_counts_.clear();

    for (auto& [symbol, order_book] : active_order_books_) {
        order_book.clear();
    }
}

void RiskManager::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    }
}

ActiveOrderBook& RiskManager::get_order_book(const std::string& vt_symbol) {
    auto it = active_order_books_.find(vt_symbol);
    if (it == active_order_books_.end()) {
        it = active_order_books_.emplace(vt_symbol, ActiveOrderBook(vt_symbol)).first;
    }
    return it->second;
}

} // namespace bitquant
