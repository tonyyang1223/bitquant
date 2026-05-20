/**
 * @file strategy.hpp
 * @brief Abstract strategy interface for backtesting
 *
 * Based on howtrader's CtaTemplate design:
 * - on_init(): Strategy initialization
 * - on_start(): Strategy start
 * - on_stop(): Strategy stop
 * - on_tick(): Tick data callback
 * - on_bar(): Bar data callback
 * - on_order(): Order update callback
 * - on_trade(): Trade update callback
 * - on_stop_order(): Stop order callback
 */

#pragma once

#include "core/types.hpp"
#include "data/array_manager.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace bitquant {

// Forward declaration
class Broker;
class PaperBroker;

/**
 * @brief Engine type enumeration
 */
enum class EngineType {
    BACKTESTING,  // Backtesting mode
    PAPER,        // Paper trading
    LIVE          // Live trading
};

/**
 * @brief Abstract strategy interface (Based on howtrader's CtaTemplate)
 *
 * Inherit from this class to implement custom trading strategies.
 *
 * Usage:
 * 1. Set strategy parameters in the params_ map
 * 2. Initialize ArrayManager in constructor
 * 3. Implement on_init() for initialization
 * 4. Implement on_bar() for signal generation
 * 5. Use buy()/sell()/short()/cover() for orders
 */
class IStrategy {
public:
    virtual ~IStrategy() = default;

    //=========================================================================
    // Strategy metadata
    //=========================================================================
    std::string author;
    std::vector<std::string> parameters;
    std::vector<std::string> variables;

    //=========================================================================
    // Lifecycle callbacks (Based on howtrader CtaTemplate)
    //=========================================================================

    /**
     * @brief Called when strategy is initialized
     * Use this to load historical data and set up indicators
     */
    virtual void on_init() {}

    /**
     * @brief Called when strategy starts
     * Strategy is ready to trade after this callback
     */
    virtual void on_start() {}

    /**
     * @brief Called when strategy stops
     * Clean up resources here
     */
    virtual void on_stop() {}

    /**
     * @brief Called on new tick data update
     * @param tick Tick data
     */
    virtual void on_tick(const TickData& tick) { (void)tick; }

    /**
     * @brief Called on new bar data update (main signal generation)
     * @param bar Bar data
     */
    virtual void on_bar(const BarData& bar) = 0;

    //=========================================================================
    // Order callbacks
    //=========================================================================

    /**
     * @brief Called on new order update
     * @param order Order data
     */
    virtual void on_order(const Order& order) { (void)order; }

    /**
     * @brief Called on new trade update
     * @param trade Trade data
     */
    virtual void on_trade(const Trade& trade) { (void)trade; }

    /**
     * @brief Called on stop order update
     * @param stop_order Stop order data
     */
    virtual void on_stop_order(const StopOrder& stop_order) { (void)stop_order; }

    //=========================================================================
    // Parameter management
    //=========================================================================

    void set_param(const std::string& name, double value) {
        params_[name] = value;
    }

    double get_param(const std::string& name, double default_val = 0.0) const {
        auto it = params_.find(name);
        return (it != params_.end()) ? it->second : default_val;
    }

    //=========================================================================
    // Broker configuration
    //=========================================================================

    /**
     * @brief Set PaperBroker for paper trading
     */
    void set_paper_broker(PaperBroker* broker);

    //=========================================================================
    // Position queries
    //=========================================================================

    /**
     * @brief Get current position volume
     * Positive for long, negative for short
     */
    double position() const;

    /**
     * @brief Get current position direction
     */
    Direction position_side() const;

    /**
     * @brief Check if position is flat
     */
    bool is_flat() const;

    /**
     * @brief Check if position is long
     */
    bool is_long() const;

    /**
     * @brief Check if position is short
     */
    bool is_short() const;

    //=========================================================================
    // Engine type query
    //=========================================================================

    /**
     * @brief Get engine type (backtesting, paper, live)
     */
    EngineType get_engine_type() const;

    //=========================================================================
    // Logging
    //=========================================================================

    /**
     * @brief Write log message
     */
    void write_log(const std::string& msg);

protected:
    //=========================================================================
    // Order methods (Based on howtrader CtaTemplate)
    //=========================================================================

    /**
     * @brief Send buy order to open long position
     * @param price Limit price
     * @param volume Order volume
     * @param stop Whether this is a stop order
     * @return Order ID
     */
    order_id_t buy(double price, double volume, bool stop = false);

    /**
     * @brief Send sell order to close long position
     * @param price Limit price
     * @param volume Order volume
     * @param stop Whether this is a stop order
     * @return Order ID
     */
    order_id_t sell(double price, double volume, bool stop = false);

    /**
     * @brief Send short order to open short position
     * @param price Limit price
     * @param volume Order volume
     * @param stop Whether this is a stop order
     * @return Order ID
     */
    order_id_t short_order(double price, double volume, bool stop = false);

    /**
     * @brief Send cover order to close short position
     * @param price Limit price
     * @param volume Order volume
     * @param stop Whether this is a stop order
     * @return Order ID
     */
    order_id_t cover(double price, double volume, bool stop = false);

    /**
     * @brief Send market buy order
     */
    void market_buy(double volume);

    /**
     * @brief Send market sell order
     */
    void market_sell(double volume);

    //=========================================================================
    // Order cancellation
    //=========================================================================

    /**
     * @brief Cancel specific order
     */
    void cancel_order(order_id_t order_id);

    /**
     * @brief Cancel all active orders
     */
    void cancel_all_orders();

    //=========================================================================
    // Strategy state
    //=========================================================================

    /**
     * @brief Strategy initialization flag
     */
    bool inited_ = false;

    /**
     * @brief Strategy trading flag
     */
    bool trading_ = false;

    /**
     * @brief Strategy parameters
     */
    std::unordered_map<std::string, double> params_;

    /**
     * @brief Broker access
     */
    Broker* broker_ = nullptr;

    /**
     * @brief PaperBroker access (for paper trading mode)
     */
    PaperBroker* paper_broker_ = nullptr;

    /**
     * @brief Array manager for technical indicators
     */
    ArrayManager am_;

    friend class Broker;
    friend class PaperBroker;
};

//=============================================================================
// Signal-based strategy helper (Based on howtrader's CtaSignal)
//=============================================================================

/**
 * @brief Signal class for signal-based strategy design
 */
class CtaSignal {
public:
    CtaSignal() = default;
    virtual ~CtaSignal() = default;

    virtual void on_tick(const TickData& tick) { (void)tick; }
    virtual void on_bar(const BarData& bar) { (void)bar; }

    void set_signal_pos(int pos) { signal_pos_ = pos; }
    int get_signal_pos() const { return signal_pos_; }

protected:
    int signal_pos_ = 0;
};

//=============================================================================
// Target position strategy template (Based on howtrader's TargetPosTemplate)
//=============================================================================

/**
 * @brief Strategy template for target position trading
 *
 * Instead of manually managing orders, simply call set_target_pos()
 * and the strategy will automatically manage orders to reach the target.
 */
class TargetPosTemplate : public IStrategy {
public:
    TargetPosTemplate() = default;

    void on_tick(const TickData& tick) override;
    void on_bar(const BarData& bar) override;
    void on_order(const Order& order) override;

    /**
     * @brief Set target position
     * Positive for long, negative for short
     */
    void set_target_pos(double target_pos);

    /**
     * @brief Check if all orders are finished
     */
    bool check_order_finished() const;

protected:
    void trade();
    void cancel_old_orders();
    void send_new_orders();

    double target_pos_ = 0.0;
    std::vector<order_id_t> active_orderids_;
    std::vector<order_id_t> cancel_orderids_;
    const TickData* last_tick_ = nullptr;
    const BarData* last_bar_ = nullptr;
    double tick_add_ = 1.0;
};

} // namespace bitquant
