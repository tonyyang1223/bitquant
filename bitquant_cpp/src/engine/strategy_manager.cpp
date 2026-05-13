/**
 * @file strategy_manager.cpp
 * @brief Multi-strategy management implementation
 *
 * Based on howtrader's CtaEngine design patterns
 */

#include "engine/strategy_manager.hpp"
#include "engine/broker.hpp"
#include "engine/paper_broker.hpp"
#include "engine/risk_manager.hpp"
#include <algorithm>
#include <iostream>

namespace bitquant {

//=============================================================================
// Constructor/Destructor
//=============================================================================

StrategyManager::StrategyManager(Broker* broker, EventEngine* event_engine)
    : broker_(broker), event_engine_(event_engine) {}

StrategyManager::~StrategyManager() {
    stop_all_strategies();
}

//=============================================================================
// Strategy Lifecycle Management
//=============================================================================

bool StrategyManager::add_strategy(
    const std::string& name,
    std::unique_ptr<IStrategy> strategy,
    const std::string& vt_symbol,
    const std::unordered_map<std::string, double>& setting
) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if name already exists
    if (strategies_.count(name)) {
        write_log("Strategy " + name + " already exists");
        return false;
    }

    // Create wrapper
    StrategyWrapper wrapper;
    wrapper.strategy_name = name;
    wrapper.vt_symbol = vt_symbol;
    wrapper.strategy = std::move(strategy);
    wrapper.status = StrategyStatus::CREATED;
    wrapper.setting = setting;
    wrapper.inited = false;
    wrapper.trading = false;

    // Apply setting to strategy
    for (const auto& [key, value] : setting) {
        wrapper.strategy->set_param(key, value);
    }

    // Store wrapper
    strategies_[name] = std::move(wrapper);

    // Update symbol mapping
    if (!vt_symbol.empty()) {
        symbol_strategy_map_[vt_symbol].push_back(name);
    }

    write_log("Added strategy: " + name);
    return true;
}

bool StrategyManager::init_strategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    StrategyWrapper* wrapper = get_wrapper(name);
    if (!wrapper) {
        write_log("Strategy " + name + " not found");
        return false;
    }

    if (wrapper->status != StrategyStatus::CREATED &&
        wrapper->status != StrategyStatus::STOPPED) {
        write_log("Strategy " + name + " cannot be initialized (status: " +
                  std::to_string(static_cast<int>(wrapper->status)) + ")");
        return false;
    }

    wrapper->status = StrategyStatus::INITING;

    try {
        wrapper->strategy->on_init();
        wrapper->inited = true;
        wrapper->status = StrategyStatus::READY;
        write_log("Initialized strategy: " + name);
        return true;
    } catch (const std::exception& e) {
        wrapper->status = StrategyStatus::ERROR;
        write_log("Failed to initialize strategy " + name + ": " + e.what());
        return false;
    }
}

void StrategyManager::init_all_strategies() {
    std::vector<std::string> names;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [name, wrapper] : strategies_) {
            names.push_back(name);
        }
    }

    for (const auto& name : names) {
        init_strategy(name);
    }
}

bool StrategyManager::start_strategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    StrategyWrapper* wrapper = get_wrapper(name);
    if (!wrapper) {
        write_log("Strategy " + name + " not found");
        return false;
    }

    if (!wrapper->inited) {
        write_log("Strategy " + name + " not initialized");
        return false;
    }

    if (wrapper->status == StrategyStatus::RUNNING) {
        write_log("Strategy " + name + " already running");
        return true;
    }

    try {
        wrapper->strategy->on_start();
        wrapper->trading = true;
        wrapper->status = StrategyStatus::RUNNING;
        write_log("Started strategy: " + name);
        return true;
    } catch (const std::exception& e) {
        wrapper->status = StrategyStatus::ERROR;
        write_log("Failed to start strategy " + name + ": " + e.what());
        return false;
    }
}

void StrategyManager::start_all_strategies() {
    std::vector<std::string> names;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [name, wrapper] : strategies_) {
            if (wrapper.inited && wrapper.status != StrategyStatus::RUNNING) {
                names.push_back(name);
            }
        }
    }

    for (const auto& name : names) {
        start_strategy(name);
    }
}

void StrategyManager::stop_strategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    StrategyWrapper* wrapper = get_wrapper(name);
    if (!wrapper) {
        return;
    }

    if (wrapper->status == StrategyStatus::RUNNING) {
        try {
            wrapper->strategy->on_stop();
            wrapper->trading = false;
            wrapper->status = StrategyStatus::STOPPED;
            write_log("Stopped strategy: " + name);
        } catch (const std::exception& e) {
            write_log("Error stopping strategy " + name + ": " + e.what());
        }
    }
}

void StrategyManager::stop_all_strategies() {
    std::vector<std::string> names;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [name, wrapper] : strategies_) {
            if (wrapper.status == StrategyStatus::RUNNING) {
                names.push_back(name);
            }
        }
    }

    for (const auto& name : names) {
        stop_strategy(name);
    }
}

void StrategyManager::remove_strategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = strategies_.find(name);
    if (it == strategies_.end()) {
        return;
    }

    // Stop if running
    if (it->second.status == StrategyStatus::RUNNING) {
        it->second.strategy->on_stop();
    }

    // Remove from symbol mapping
    const std::string& vt_symbol = it->second.vt_symbol;
    if (!vt_symbol.empty()) {
        auto& strategy_list = symbol_strategy_map_[vt_symbol];
        strategy_list.erase(
            std::remove(strategy_list.begin(), strategy_list.end(), name),
            strategy_list.end()
        );
        if (strategy_list.empty()) {
            symbol_strategy_map_.erase(vt_symbol);
        }
    }

    // Remove from order mapping
    std::vector<std::string> orders_to_remove;
    for (const auto& [orderid, strategy_name] : orderid_strategy_map_) {
        if (strategy_name == name) {
            orders_to_remove.push_back(orderid);
        }
    }
    for (const auto& orderid : orders_to_remove) {
        orderid_strategy_map_.erase(orderid);
    }

    strategies_.erase(it);
    write_log("Removed strategy: " + name);
}

//=============================================================================
// Market Data Processing
//=============================================================================

void StrategyManager::on_tick(const TickData& tick) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find strategies subscribed to this symbol
    std::string vt_symbol = tick.vt_symbol();
    auto it = symbol_strategy_map_.find(vt_symbol);
    if (it == symbol_strategy_map_.end()) {
        return;
    }

    // Dispatch to each strategy
    for (const auto& strategy_name : it->second) {
        StrategyWrapper* wrapper = get_wrapper(strategy_name);
        if (wrapper && wrapper->trading && wrapper->inited) {
            try {
                wrapper->strategy->on_tick(tick);
            } catch (const std::exception& e) {
                write_log("Error in " + strategy_name + ".on_tick: " + e.what());
            }
        }
    }
}

void StrategyManager::on_bar(const BarData& bar) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find strategies subscribed to this symbol
    std::string vt_symbol = bar.vt_symbol();
    auto it = symbol_strategy_map_.find(vt_symbol);
    if (it == symbol_strategy_map_.end()) {
        return;
    }

    // Dispatch to each strategy
    for (const auto& strategy_name : it->second) {
        StrategyWrapper* wrapper = get_wrapper(strategy_name);
        if (wrapper && wrapper->trading && wrapper->inited) {
            try {
                wrapper->strategy->on_bar(bar);
            } catch (const std::exception& e) {
                write_log("Error in " + strategy_name + ".on_bar: " + e.what());
            }
        }
    }
}

//=============================================================================
// Order/Trade Processing
//=============================================================================

void StrategyManager::on_order(const OrderData& order) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find strategy for this order
    auto it = orderid_strategy_map_.find(order.vt_orderid());
    if (it == orderid_strategy_map_.end()) {
        return;
    }

    const std::string& strategy_name = it->second;
    StrategyWrapper* wrapper = get_wrapper(strategy_name);
    if (!wrapper) {
        return;
    }

    // Update active orders
    if (!order.is_active()) {
        wrapper->active_orderids.erase(order.vt_orderid());
    }

    // Dispatch to strategy
    try {
        // Convert to legacy Order for compatibility
        Order legacy_order;
        legacy_order.order_id = std::stoull(order.orderid);
        legacy_order.side = order.direction;
        legacy_order.status = order.status;
        legacy_order.price = order.price;
        legacy_order.volume = order.volume;
        legacy_order.filled_volume = order.traded;

        wrapper->strategy->on_order(legacy_order);
    } catch (const std::exception& e) {
        write_log("Error in " + strategy_name + ".on_order: " + e.what());
    }
}

void StrategyManager::on_trade(const TradeData& trade) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find strategy for this trade
    auto it = orderid_strategy_map_.find(trade.vt_orderid());
    if (it == orderid_strategy_map_.end()) {
        return;
    }

    const std::string& strategy_name = it->second;
    StrategyWrapper* wrapper = get_wrapper(strategy_name);
    if (!wrapper) {
        return;
    }

    // Update strategy statistics
    wrapper->total_trades++;

    // Dispatch to strategy
    try {
        // Convert to legacy Trade for compatibility
        Trade legacy_trade;
        legacy_trade.trade_id = std::stoull(trade.tradeid);
        legacy_trade.order_id = std::stoull(trade.orderid);
        legacy_trade.side = trade.direction;
        legacy_trade.price = trade.price;
        legacy_trade.volume = trade.volume;
        legacy_trade.timestamp = trade.datetime;

        wrapper->strategy->on_trade(legacy_trade);
    } catch (const std::exception& e) {
        write_log("Error in " + strategy_name + ".on_trade: " + e.what());
    }
}

//=============================================================================
// Order Management
//=============================================================================

std::string StrategyManager::send_order(
    const std::string& strategy_name,
    const OrderRequest& req
) {
    std::lock_guard<std::mutex> lock(mutex_);

    StrategyWrapper* wrapper = get_wrapper(strategy_name);
    if (!wrapper || !wrapper->trading) {
        write_log("Strategy " + strategy_name + " not found or not trading");
        return "";
    }

    // Risk check
    if (risk_manager_) {
        if (!risk_manager_->check_risk(req, "")) {
            write_log("Order rejected by risk manager for strategy: " + strategy_name);
            return "";
        }
    }

    // Generate order ID
    static std::atomic<uint64_t> next_order_id{1};
    std::string vt_orderid = strategy_name + "." + std::to_string(next_order_id++);

    // Store mapping
    orderid_strategy_map_[vt_orderid] = strategy_name;
    wrapper->active_orderids.insert(vt_orderid);

    // Send to broker (PaperBroker for paper trading, Broker for backtesting)
    std::string broker_orderid;

    if (paper_broker_) {
        // Paper trading mode
        OrderRequest broker_req = req;
        broker_req.reference = vt_orderid;  // Link strategy order to broker order
        broker_orderid = paper_broker_->send_order(broker_req);

        if (broker_orderid.empty()) {
            write_log("Failed to send order to paper broker for strategy: " + strategy_name);
            orderid_strategy_map_.erase(vt_orderid);
            wrapper->active_orderids.erase(vt_orderid);
            return "";
        }

        write_log("Order sent to paper broker: " + vt_orderid + " -> " + broker_orderid);

    } else if (broker_) {
        // Backtesting mode - use Broker's buy/sell/short/cover methods
        order_id_t id = 0;
        if (req.direction == Direction::LONG) {
            if (req.price > 0) {
                id = broker_->buy(req.price, req.volume);
            } else {
                id = broker_->market_buy(req.volume);
            }
        } else {
            if (req.price > 0) {
                id = broker_->short_order(req.price, req.volume);
            } else {
                id = broker_->market_sell(req.volume);
            }
        }
        broker_orderid = std::to_string(id);

        if (broker_orderid == "0") {
            write_log("Failed to send order to broker for strategy: " + strategy_name);
            orderid_strategy_map_.erase(vt_orderid);
            wrapper->active_orderids.erase(vt_orderid);
            return "";
        }

        write_log("Order sent to broker: " + vt_orderid + " -> " + broker_orderid);
    } else {
        write_log("No broker configured for strategy: " + strategy_name);
        orderid_strategy_map_.erase(vt_orderid);
        wrapper->active_orderids.erase(vt_orderid);
        return "";
    }

    // Store broker order ID mapping
    orderid_strategy_map_[broker_orderid] = strategy_name;

    return vt_orderid;
}

void StrategyManager::cancel_order(
    const std::string& strategy_name,
    const std::string& vt_orderid
) {
    std::lock_guard<std::mutex> lock(mutex_);

    StrategyWrapper* wrapper = get_wrapper(strategy_name);
    if (!wrapper) {
        return;
    }

    // Verify order belongs to this strategy
    auto it = orderid_strategy_map_.find(vt_orderid);
    if (it == orderid_strategy_map_.end() || it->second != strategy_name) {
        write_log("Order " + vt_orderid + " not found for strategy " + strategy_name);
        return;
    }

    // Cancel at broker
    if (paper_broker_) {
        paper_broker_->cancel_order(vt_orderid);
        write_log("Order cancelled at paper broker: " + vt_orderid);
    } else if (broker_) {
        try {
            order_id_t id = std::stoull(vt_orderid);
            broker_->cancel_order(id);
            write_log("Order cancelled at broker: " + vt_orderid);
        } catch (...) {
            write_log("Invalid order ID for cancel: " + vt_orderid);
        }
    }

    // Remove from active orders
    wrapper->active_orderids.erase(vt_orderid);
}

//=============================================================================
// Query Methods
//=============================================================================

std::vector<std::string> StrategyManager::get_strategy_names() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names;
    names.reserve(strategies_.size());
    for (const auto& [name, wrapper] : strategies_) {
        names.push_back(name);
    }
    return names;
}

StrategyStatus StrategyManager::get_strategy_status(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const StrategyWrapper* wrapper = get_wrapper(name);
    return wrapper ? wrapper->status : StrategyStatus::CREATED;
}

double StrategyManager::get_strategy_position(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const StrategyWrapper* wrapper = get_wrapper(name);
    if (wrapper && wrapper->strategy) {
        return wrapper->strategy->position();
    }
    return 0.0;
}

void StrategyManager::get_strategy_stats(
    const std::string& name,
    int& trades,
    int& wins,
    double& pnl
) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const StrategyWrapper* wrapper = get_wrapper(name);
    if (wrapper) {
        trades = wrapper->total_trades;
        wins = wrapper->winning_trades;
        pnl = wrapper->total_pnl;
    }
}

//=============================================================================
// Internal Methods
//=============================================================================

void StrategyManager::write_log(const std::string& msg) {
    if (log_callback_) {
        log_callback_(msg);
    } else {
        std::cout << "[StrategyManager] " << msg << std::endl;
    }
}

StrategyWrapper* StrategyManager::get_wrapper(const std::string& name) {
    auto it = strategies_.find(name);
    return it != strategies_.end() ? &it->second : nullptr;
}

const StrategyWrapper* StrategyManager::get_wrapper(const std::string& name) const {
    auto it = strategies_.find(name);
    return it != strategies_.end() ? &it->second : nullptr;
}

} // namespace bitquant
