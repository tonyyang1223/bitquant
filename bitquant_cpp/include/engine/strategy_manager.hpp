/**
 * @file strategy_manager.hpp
 * @brief Multi-strategy management system
 *
 * Based on howtrader's CtaEngine design:
 * - Strategy registration and lifecycle management
 * - Event dispatch to multiple strategies
 * - Order tracking per strategy
 * - Symbol to strategy mapping for efficient event routing
 */

#pragma once

#include "core/types.hpp"
#include "strategy.hpp"
#include "event.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

namespace bitquant {

//=============================================================================
// Forward Declarations
//=============================================================================

class Broker;
class RiskManager;

//=============================================================================
// Strategy Status
//=============================================================================

/**
 * @brief Strategy running status
 */
enum class StrategyStatus : uint8_t {
    CREATED,    // Strategy created but not initialized
    INITING,    // Initializing (loading data)
    READY,      // Ready to run
    RUNNING,    // Strategy is running
    STOPPED,    // Strategy stopped
    ERROR       // Error state
};

//=============================================================================
// Strategy Wrapper
//=============================================================================

/**
 * @brief Wrapper for managing individual strategy state
 *
 * Based on howtrader's strategy management pattern.
 * Each strategy has its own:
 * - Active orders
 * - Stop orders
 * - Position tracking
 */
struct StrategyWrapper {
    std::string strategy_name;
    std::string vt_symbol;  // Subscribed symbol

    std::unique_ptr<IStrategy> strategy;
    StrategyStatus status = StrategyStatus::CREATED;

    // Order tracking (Based on howtrader's strategy_orderid_map)
    std::unordered_set<std::string> active_orderids;
    std::unordered_map<std::string, StopOrder> stop_orders;

    // Strategy configuration
    std::unordered_map<std::string, double> setting;

    // Statistics
    int total_trades = 0;
    int winning_trades = 0;
    double total_pnl = 0.0;

    bool inited = false;
    bool trading = false;
};

//=============================================================================
// Strategy Manager
//=============================================================================

/**
 * @brief Multi-strategy coordination manager
 *
 * Based on howtrader's CtaEngine:
 * - Manages multiple strategy instances
 * - Routes market data to subscribed strategies
 * - Tracks orders and routes updates to correct strategy
 * - Handles strategy lifecycle (init/start/stop)
 *
 * Usage:
 * @code
 * StrategyManager manager(&broker, &event_engine);
 *
 * // Add strategies
 * manager.add_strategy("my_strategy", std::make_unique<MyStrategy>());
 * manager.init_strategy("my_strategy");
 * manager.start_strategy("my_strategy");
 *
 * // Process events
 * manager.on_tick(tick);
 * manager.on_bar(bar);
 * @endcode
 */
class StrategyManager {
public:
    /**
     * @brief Construct strategy manager
     * @param broker Broker for order execution
     * @param event_engine Event engine for async processing
     */
    StrategyManager(Broker* broker = nullptr, EventEngine* event_engine = nullptr);
    ~StrategyManager();

    //=========================================================================
    // Strategy Lifecycle Management
    //=========================================================================

    /**
     * @brief Add a new strategy
     * @param name Unique strategy name
     * @param strategy Strategy instance
     * @param vt_symbol Subscribed symbol (empty for all)
     * @param setting Strategy parameters
     * @return true on success
     */
    bool add_strategy(
        const std::string& name,
        std::unique_ptr<IStrategy> strategy,
        const std::string& vt_symbol = "",
        const std::unordered_map<std::string, double>& setting = {}
    );

    /**
     * @brief Initialize strategy (load historical data)
     * @param name Strategy name
     * @return true on success
     */
    bool init_strategy(const std::string& name);

    /**
     * @brief Initialize all strategies
     */
    void init_all_strategies();

    /**
     * @brief Start strategy trading
     * @param name Strategy name
     * @return true on success
     */
    bool start_strategy(const std::string& name);

    /**
     * @brief Start all strategies
     */
    void start_all_strategies();

    /**
     * @brief Stop strategy
     * @param name Strategy name
     */
    void stop_strategy(const std::string& name);

    /**
     * @brief Stop all strategies
     */
    void stop_all_strategies();

    /**
     * @brief Remove strategy
     * @param name Strategy name
     */
    void remove_strategy(const std::string& name);

    //=========================================================================
    // Market Data Processing
    //=========================================================================

    /**
     * @brief Process tick data - dispatch to subscribed strategies
     * @param tick Tick data
     */
    void on_tick(const TickData& tick);

    /**
     * @brief Process bar data - dispatch to subscribed strategies
     * @param bar Bar data
     */
    void on_bar(const BarData& bar);

    //=========================================================================
    // Order/Trade Processing
    //=========================================================================

    /**
     * @brief Process order update
     * @param order Order data
     */
    void on_order(const OrderData& order);

    /**
     * @brief Process trade execution
     * @param trade Trade data
     */
    void on_trade(const TradeData& trade);

    //=========================================================================
    // Order Management (Called by strategies through broker)
    //=========================================================================

    /**
     * @brief Send order from strategy
     * @param strategy_name Strategy name
     * @param req Order request
     * @return Order ID
     */
    std::string send_order(
        const std::string& strategy_name,
        const OrderRequest& req
    );

    /**
     * @brief Cancel order
     * @param strategy_name Strategy name
     * @param vt_orderid Order ID
     */
    void cancel_order(
        const std::string& strategy_name,
        const std::string& vt_orderid
    );

    //=========================================================================
    // Query Methods
    //=========================================================================

    /**
     * @brief Get strategy names
     */
    std::vector<std::string> get_strategy_names() const;

    /**
     * @brief Get strategy status
     */
    StrategyStatus get_strategy_status(const std::string& name) const;

    /**
     * @brief Get strategy position
     */
    double get_strategy_position(const std::string& name) const;

    /**
     * @brief Get strategy statistics
     */
    void get_strategy_stats(
        const std::string& name,
        int& trades,
        int& wins,
        double& pnl
    ) const;

    //=========================================================================
    // Configuration
    //=========================================================================

    void set_broker(Broker* broker) { broker_ = broker; }
    void set_event_engine(EventEngine* engine) { event_engine_ = engine; }
    void set_risk_manager(RiskManager* rm) { risk_manager_ = rm; }

    //=========================================================================
    // Callbacks
    //=========================================================================

    using LogCallback = std::function<void(const std::string&)>;
    void set_log_callback(LogCallback callback) { log_callback_ = std::move(callback); }

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    void write_log(const std::string& msg);

    StrategyWrapper* get_wrapper(const std::string& name);
    const StrategyWrapper* get_wrapper(const std::string& name) const;

    //=========================================================================
    // Member Variables
    //=========================================================================

    Broker* broker_ = nullptr;
    EventEngine* event_engine_ = nullptr;
    RiskManager* risk_manager_ = nullptr;

    // Strategy management (Based on howtrader's CtaEngine)
    std::unordered_map<std::string, StrategyWrapper> strategies_;

    // Symbol to strategies mapping (Based on howtrader's symbol_strategy_map)
    std::unordered_map<std::string, std::vector<std::string>> symbol_strategy_map_;

    // Order to strategy mapping (Based on howtrader's orderid_strategy_map)
    std::unordered_map<std::string, std::string> orderid_strategy_map_;

    // Stop order counter
    int stop_order_count_ = 0;

    // Thread safety
    mutable std::mutex mutex_;

    // Callbacks
    LogCallback log_callback_;
};

//=============================================================================
// Engine Type (Based on howtrader)
//=============================================================================

/**
 * @brief Trading engine type
 */
enum class TradingEngineType {
    BACKTESTING,  // Backtesting mode
    PAPER,        // Paper trading
    LIVE          // Live trading
};

} // namespace bitquant
