/**
 * @file paper_broker.hpp
 * @brief Paper trading broker for simulated order execution
 *
 * Provides simulated order execution for paper trading mode:
 * - Orders are NOT sent to real exchange
 * - Uses live market data to simulate fills
 * - Tracks virtual positions and balance
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>

namespace bitquant {

//=============================================================================
// Callback Types
//=============================================================================

/// Paper order callback
using PaperOrderCallback = std::function<void(const OrderData&)>;

/// Paper trade callback
using PaperTradeCallback = std::function<void(const TradeData&)>;

//=============================================================================
// Paper Broker Configuration
//=============================================================================

/**
 * @brief Paper broker configuration
 */
struct PaperBrokerConfig {
    double initial_capital = 100'000.0;
    double commission_rate = 0.001;    // 0.1%
    double slippage_rate = 0.0005;     // 0.05%
    bool allow_short = false;          // Spot trading typically doesn't allow short
};

//=============================================================================
// Paper Broker
//=============================================================================

/**
 * @brief Paper trading broker
 *
 * Simulates order execution without sending to real exchange:
 * - Maintains virtual account balance
 * - Simulates order fills based on market data
 * - Applies commission and slippage
 *
 * Usage:
 * @code
 * PaperBroker broker(config);
 * broker.on_tick(tick);  // Feed live market data
 * broker.send_order(req); // Simulated order
 * // Orders get filled based on tick bid/ask
 * @endcode
 */
class PaperBroker {
public:
    explicit PaperBroker(const PaperBrokerConfig& config = PaperBrokerConfig{});
    ~PaperBroker() = default;

    //=========================================================================
    // Configuration
    //=========================================================================

    void set_capital(double capital);
    void set_commission(double rate);
    void set_slippage(double rate);

    //=========================================================================
    // Order Interface (Simulated)
    //=========================================================================

    /**
     * @brief Send order (simulated - not sent to exchange)
     * @param req Order request
     * @return Order ID (virtual)
     */
    std::string send_order(const OrderRequest& req);

    /**
     * @brief Cancel order
     * @param order_id Order ID
     * @return true if cancelled
     */
    bool cancel_order(const std::string& order_id);

    /**
     * @brief Cancel all orders
     */
    void cancel_all_orders();

    //=========================================================================
    // Market Data Input (for Fill Simulation)
    //=========================================================================

    /**
     * @brief Process tick data - triggers order matching
     */
    void on_tick(const TickData& tick);

    /**
     * @brief Process bar data - triggers order matching
     */
    void on_bar(const BarData& bar);

    //=========================================================================
    // State Queries
    //=========================================================================

    /**
     * @brief Get current cash balance
     */
    double get_balance() const;

    /**
     * @brief Get position for symbol
     */
    double get_position(const std::string& symbol) const;

    /**
     * @brief Get all positions
     */
    std::unordered_map<std::string, double> get_positions() const;

    /**
     * @brief Get open orders
     */
    std::vector<OrderData> get_open_orders() const;

    /**
     * @brief Get all executed trades
     */
    std::vector<TradeData> get_trades() const;

    /**
     * @brief Get current equity (cash + positions value)
     */
    double get_equity() const;

    //=========================================================================
    // Callback Registration
    //=========================================================================

    void on_order(PaperOrderCallback callback);
    void on_trade(PaperTradeCallback callback);

    //=========================================================================
    // Statistics
    //=========================================================================

    /**
     * @brief Get total commission paid
     */
    double get_total_commission() const;

    /**
     * @brief Get total trades count
     */
    int get_trade_count() const;

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    /**
     * @brief Generate order ID
     */
    std::string generate_order_id();

    /**
     * @brief Try to fill orders against current prices
     */
    void try_fill_orders(double bid, double ask, const std::string& symbol, int64_t timestamp);

    /**
     * @brief Execute order fill
     */
    void execute_fill(OrderData& order, double fill_price, double fill_volume, int64_t timestamp);

    //=========================================================================
    // Members
    //=========================================================================

    PaperBrokerConfig config_;

    // Virtual account
    double cash_;
    std::unordered_map<std::string, double> positions_;  // symbol -> volume
    std::unordered_map<std::string, double> avg_cost_;    // symbol -> avg cost

    // Orders
    std::unordered_map<std::string, OrderData> open_orders_;
    std::unordered_map<std::string, OrderData> all_orders_;
    std::vector<TradeData> trades_;
    int order_counter_ = 0;
    mutable std::mutex orders_mutex_;

    // Last prices (for equity calculation)
    std::unordered_map<std::string, double> last_prices_;
    mutable std::mutex prices_mutex_;

    // Callbacks
    PaperOrderCallback order_callback_;
    PaperTradeCallback trade_callback_;

    // Statistics
    double total_commission_ = 0.0;
    int trade_count_ = 0;
};

} // namespace bitquant