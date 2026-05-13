/**
 * @file offset_converter.hpp
 * @brief Offset converter for futures trading
 *
 * Based on howtrader's OffsetConverter design:
 * - Converts strategy buy/sell/short/cover to Direction+Offset
 * - Handles SHFE/INE today/yesterday position separation
 * - Supports lock mode and net mode
 *
 * Reference: howtrader/trader/converter.py
 */

#pragma once

#include "core/types.hpp"
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <functional>

namespace bitquant {

// Forward declaration
class MainEngine;

//=============================================================================
// PositionHolding - Tracks position and active orders for a symbol
//=============================================================================

/**
 * @brief Position holding for a single symbol
 *
 * Tracks:
 * - Long/short position (total, today, yesterday)
 * - Frozen volume for pending orders
 * - Active orders
 */
class PositionHolding {
public:
    explicit PositionHolding(const ContractData& contract);

    //=========================================================================
    // Position Updates
    //=========================================================================

    /**
     * @brief Update position from PositionData
     */
    void update_position(const PositionData& position);

    /**
     * @brief Update position from trade
     */
    void update_trade(const TradeData& trade);

    /**
     * @brief Update active order
     */
    void update_order(const OrderData& order);

    /**
     * @brief Update from order request (before order is placed)
     */
    void update_order_request(const OrderRequest& req, const std::string& vt_orderid);

    //=========================================================================
    // Order Conversion (Based on howtrader's logic)
    //=========================================================================

    /**
     * @brief Convert order request for SHFE/INE exchange
     *
     * SHFE/INE require explicit today/yesterday separation:
     * - If volume <= td_available: use CLOSETODAY
     * - If volume > td_available: split into CLOSETODAY + CLOSEYESTERDAY
     *
     * @param req Original order request
     * @return List of converted order requests
     */
    std::vector<OrderRequest> convert_order_request_shfe(const OrderRequest& req);

    /**
     * @brief Convert order request for lock mode
     *
     * Lock mode:
     * - If there is td_volume, use OPEN (lock position)
     * - If no td_volume, close yd position first, then open new position
     *
     * @param req Original order request
     * @return List of converted order requests
     */
    std::vector<OrderRequest> convert_order_request_lock(const OrderRequest& req);

    /**
     * @brief Convert order request for net mode
     *
     * Net mode:
     * - Close existing position first (today/yesterday split for SHFE/INE)
     * - Then open new position for remaining volume
     *
     * @param req Original order request
     * @return List of converted order requests
     */
    std::vector<OrderRequest> convert_order_request_net(const OrderRequest& req);

    //=========================================================================
    // Available Volume Calculation
    //=========================================================================

    /**
     * @brief Calculate frozen volume from active orders
     */
    void calculate_frozen();

    /**
     * @brief Sum frozen volume and ensure it doesn't exceed total
     */
    void sum_pos_frozen();

    //=========================================================================
    // Position Accessors
    //=========================================================================

    // Long position
    double long_pos() const { return long_pos_; }
    double long_yd() const { return long_yd_; }
    double long_td() const { return long_td_; }
    double long_pos_frozen() const { return long_pos_frozen_; }
    double long_yd_frozen() const { return long_yd_frozen_; }
    double long_td_frozen() const { return long_td_frozen_; }

    // Short position
    double short_pos() const { return short_pos_; }
    double short_yd() const { return short_yd_; }
    double short_td() const { return short_td_; }
    double short_pos_frozen() const { return short_pos_frozen_; }
    double short_yd_frozen() const { return short_yd_frozen_; }
    double short_td_frozen() const { return short_td_frozen_; }

    // VT symbol
    const std::string& vt_symbol() const { return vt_symbol_; }

    // Exchange
    Exchange exchange() const { return exchange_; }

    // Active order count
    size_t active_order_count() const { return active_orders_.size(); }

private:
    std::string vt_symbol_;
    Exchange exchange_;

    // Active orders
    std::map<std::string, OrderData> active_orders_;

    // Long position
    double long_pos_ = 0.0;
    double long_yd_ = 0.0;  // Yesterday position
    double long_td_ = 0.0;  // Today position

    // Short position
    double short_pos_ = 0.0;
    double short_yd_ = 0.0;
    double short_td_ = 0.0;

    // Frozen volume (for pending close orders)
    double long_pos_frozen_ = 0.0;
    double long_yd_frozen_ = 0.0;
    double long_td_frozen_ = 0.0;

    double short_pos_frozen_ = 0.0;
    double short_yd_frozen_ = 0.0;
    double short_td_frozen_ = 0.0;
};

//=============================================================================
// OffsetConverter - Converts order offset for futures trading
//=============================================================================

/**
 * @brief Offset converter for futures trading
 *
 * Handles:
 * 1. SHFE/INE today/yesterday position separation
 * 2. Lock mode (lock position instead of closing)
 * 3. Net mode (net position management)
 *
 * Usage:
 * 1. Update position/trade/order data
 * 2. Call convert_order_request() before sending order
 * 3. Send converted order requests to gateway
 */
class OffsetConverter {
public:
    using LogCallback = std::function<void(const std::string&)>;

    /**
     * @brief Constructor
     * @param main_engine Main engine for contract lookup
     */
    explicit OffsetConverter(MainEngine* main_engine = nullptr);

    //=========================================================================
    // Data Updates
    //=========================================================================

    /**
     * @brief Update position data
     */
    void update_position(const PositionData& position);

    /**
     * @brief Update trade data
     */
    void update_trade(const TradeData& trade);

    /**
     * @brief Update order data
     */
    void update_order(const OrderData& order);

    /**
     * @brief Update from order request
     */
    void update_order_request(const OrderRequest& req, const std::string& vt_orderid);

    //=========================================================================
    // Order Conversion
    //=========================================================================

    /**
     * @brief Convert order request based on exchange and mode
     *
     * @param req Original order request
     * @param lock Use lock mode (lock position instead of closing)
     * @param net Use net mode (net position management)
     * @return List of converted order requests
     */
    std::vector<OrderRequest> convert_order_request(
        const OrderRequest& req,
        bool lock = false,
        bool net = false
    );

    //=========================================================================
    // Position Holding Access
    //=========================================================================

    /**
     * @brief Get position holding for symbol
     */
    PositionHolding& get_position_holding(const std::string& vt_symbol);

    /**
     * @brief Check if conversion is required for symbol
     *
     * Conversion is required for:
     * - Futures contracts with long-short position mode
     * - Not net position mode
     */
    bool is_convert_required(const std::string& vt_symbol) const;

    //=========================================================================
    // Contract Registration (for standalone use without MainEngine)
    //=========================================================================

    /**
     * @brief Register contract for conversion
     */
    void register_contract(const ContractData& contract);

    //=========================================================================
    // Logging
    //=========================================================================

    void set_log_callback(LogCallback callback) { log_callback_ = std::move(callback); }

private:
    void write_log(const std::string& msg);

    MainEngine* main_engine_;
    std::unordered_map<std::string, PositionHolding> holdings_;
    std::unordered_map<std::string, ContractData> contracts_;
    LogCallback log_callback_;
};

} // namespace bitquant