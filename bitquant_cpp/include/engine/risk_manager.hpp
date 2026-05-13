/**
 * @file risk_manager.hpp
 * @brief Risk management engine for trading
 *
 * Based on howtrader's RiskEngine design:
 * - Order flow limit (orders per time window)
 * - Order size limit (max volume per order)
 * - Trade limit (total trades per day)
 * - Active order limit
 * - Order cancel limit
 * - Self-trade prevention
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <unordered_map>
#include <map>
#include <functional>

namespace bitquant {

//=============================================================================
// Risk Configuration
//=============================================================================

/**
 * @brief Risk manager configuration
 */
struct RiskConfig {
    bool active = true;

    // Order flow limit
    int order_flow_limit = 50;        // Max orders per window
    int order_flow_clear = 1;         // Window size in seconds

    // Order size limit
    double order_size_limit = 100.0;  // Max volume per order

    // Trade limit
    double trade_limit = 1000.0;      // Max total trades per day

    // Active order limit
    int active_order_limit = 50;      // Max active orders

    // Cancel limit
    int order_cancel_limit = 500;     // Max cancels per symbol per day

    // Self-trade prevention
    bool prevent_self_trade = true;
};

//=============================================================================
// Active Order Book (Based on howtrader's ActiveOrderBook)
//=============================================================================

/**
 * @brief Tracks active orders for self-trade prevention
 */
class ActiveOrderBook {
public:
    explicit ActiveOrderBook(const std::string& vt_symbol);

    /**
     * @brief Update order book with order
     */
    void update_order(const OrderData& order);

    /**
     * @brief Get best bid price
     */
    double get_best_bid() const;

    /**
     * @brief Get best ask price
     */
    double get_best_ask() const;

    /**
     * @brief Get bid order count
     */
    size_t bid_count() const { return bid_prices_.size(); }

    /**
     * @brief Get ask order count
     */
    size_t ask_count() const { return ask_prices_.size(); }

    /**
     * @brief Get total order count
     */
    size_t total_count() const { return bid_prices_.size() + ask_prices_.size(); }

    /**
     * @brief Clear all orders
     */
    void clear();

private:
    std::string vt_symbol_;
    std::map<std::string, double> bid_prices_;  // order_id -> price
    std::map<std::string, double> ask_prices_;  // order_id -> price
};

//=============================================================================
// Risk Manager
//=============================================================================

/**
 * @brief Risk management engine
 *
 * Implements risk checking before order submission:
 * 1. Order volume check
 * 2. Order flow check
 * 3. Trade limit check
 * 4. Active order count check
 * 5. Cancel limit check
 * 6. Self-trade prevention
 */
class RiskManager {
public:
    using LogCallback = std::function<void(const std::string&)>;

    explicit RiskManager(const RiskConfig& config = RiskConfig{});

    //=========================================================================
    // Configuration
    //=========================================================================

    void set_config(const RiskConfig& config) { config_ = config; }
    const RiskConfig& config() const { return config_; }

    void set_active(bool active) { config_.active = active; }
    bool is_active() const { return config_.active; }

    //=========================================================================
    // Risk Checking (Based on howtrader's check_risk)
    //=========================================================================

    /**
     * @brief Check if order passes all risk checks
     * @param req Order request
     * @param gateway_name Gateway name
     * @return true if order passes all checks
     */
    bool check_risk(const OrderRequest& req, const std::string& gateway_name);

    //=========================================================================
    // Event Processing
    //=========================================================================

    /**
     * @brief Process trade event (update trade count)
     */
    void on_trade(const TradeData& trade);

    /**
     * @brief Process order event (update active order book)
     */
    void on_order(const OrderData& order);

    /**
     * @brief Process timer event (reset order flow)
     */
    void on_timer();

    //=========================================================================
    // Statistics
    //=========================================================================

    int trade_count() const { return trade_count_; }
    int order_flow_count() const { return order_flow_count_; }
    void reset_daily_counters();

    //=========================================================================
    // Logging
    //=========================================================================

    void set_log_callback(LogCallback callback) { log_callback_ = std::move(callback); }

private:
    //=========================================================================
    // Internal methods
    //=========================================================================

    void write_log(const std::string& msg);
    ActiveOrderBook& get_order_book(const std::string& vt_symbol);

    //=========================================================================
    // Member variables
    //=========================================================================

    RiskConfig config_;
    LogCallback log_callback_;

    // Order flow tracking
    int order_flow_count_ = 0;
    int order_flow_timer_ = 0;

    // Trade tracking
    double trade_count_ = 0.0;

    // Cancel tracking per symbol
    std::unordered_map<std::string, int> order_cancel_counts_;

    // Active order books per symbol
    std::unordered_map<std::string, ActiveOrderBook> active_order_books_;
};

//=============================================================================
// Risk Rule Interface
//=============================================================================

/**
 * @brief Interface for custom risk rules
 */
class IRiskRule {
public:
    virtual ~IRiskRule() = default;

    /**
     * @brief Check if order passes this risk rule
     * @param req Order request
     * @return true if order passes
     */
    virtual bool check(const OrderRequest& req) = 0;

    /**
     * @brief Get rule name
     */
    virtual std::string name() const = 0;
};

//=============================================================================
// Built-in Risk Rules
//=============================================================================

/**
 * @brief Rule: Order volume must be positive
 */
class PositiveVolumeRule : public IRiskRule {
public:
    bool check(const OrderRequest& req) override {
        return req.volume > 0;
    }
    std::string name() const override { return "PositiveVolume"; }
};

/**
 * @brief Rule: Order volume must not exceed limit
 */
class MaxVolumeRule : public IRiskRule {
public:
    explicit MaxVolumeRule(double max_volume) : max_volume_(max_volume) {}

    bool check(const OrderRequest& req) override {
        return req.volume <= max_volume_;
    }
    std::string name() const override { return "MaxVolume"; }

private:
    double max_volume_;
};

/**
 * @brief Rule: Order price must be positive
 */
class PositivePriceRule : public IRiskRule {
public:
    bool check(const OrderRequest& req) override {
        return req.price > 0;
    }
    std::string name() const override { return "PositivePrice"; }
};

} // namespace bitquant
