/**
 * @file main_engine.hpp
 * @brief Main trading engine - core coordinator for all trading components
 *
 * Based on howtrader's MainEngine design:
 * - Manages multiple gateways
 * - Coordinates OMS, RiskManager, and other engines
 * - Event-driven architecture
 * - Unified interface for all trading operations
 */

#pragma once

#include "core/types.hpp"
#include "engine/event.hpp"
#include "engine/risk_manager.hpp"
#include "exchange/i_exchange.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace bitquant {

//=============================================================================
// Forward Declarations
//=============================================================================

class OmsEngine;
class LogEngine;

//=============================================================================
// MainEngine Configuration
//=============================================================================

/**
 * @brief Main engine configuration
 */
struct MainEngineConfig {
    // Event engine settings
    int timer_interval_seconds = 1;

    // Log settings
    bool log_to_console = true;
    bool log_to_file = false;
    std::string log_path = "logs/";

    // Risk settings
    bool enable_risk_manager = true;
    int order_flow_limit = 50;
    double order_size_limit = 100.0;
    int active_order_limit = 50;
};

//=============================================================================
// MainEngine
//=============================================================================

/**
 * @brief Core trading engine - coordinator for all components
 *
 * Based on howtrader's MainEngine:
 * - Gateway management (add/get/connect)
 * - Engine management (OMS, Log, Risk)
 * - Unified trading interface
 * - Event distribution
 *
 * Usage:
 * @code
 * MainEngine engine;
 * engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>());
 * engine.connect("binance", config);
 * engine.subscribe("BTCUSDT", "binance");
 * engine.send_order(req, "binance");
 * @endcode
 */
class MainEngine {
public:
    explicit MainEngine(const MainEngineConfig& config = {});
    ~MainEngine();

    //=========================================================================
    // Engine Control
    //=========================================================================

    /**
     * @brief Start the main engine
     */
    void start();

    /**
     * @brief Stop the main engine
     */
    void stop();

    /**
     * @brief Check if engine is running
     */
    bool is_running() const;

    //=========================================================================
    // Gateway Management
    //=========================================================================

    /**
     * @brief Add gateway
     * @param name Gateway name
     * @param gateway Gateway instance
     * @return Reference to gateway
     */
    IExchange* add_gateway(const std::string& name, std::unique_ptr<IExchange> gateway);

    /**
     * @brief Get gateway by name
     */
    IExchange* get_gateway(const std::string& name) const;

    /**
     * @brief Get all gateway names
     */
    std::vector<std::string> get_gateway_names() const;

    /**
     * @brief Get all supported exchanges
     */
    std::vector<Exchange> get_all_exchanges() const;

    //=========================================================================
    // Connection Management
    //=========================================================================

    /**
     * @brief Connect to gateway
     * @param gateway_name Gateway name
     * @param config Connection config (JSON string)
     */
    bool connect(const std::string& gateway_name, const std::string& config = "");

    /**
     * @brief Disconnect from gateway
     */
    void disconnect(const std::string& gateway_name);

    //=========================================================================
    // Market Data Subscription
    //=========================================================================

    /**
     * @brief Subscribe to tick data
     */
    void subscribe(const std::string& symbol, const std::string& gateway_name);

    /**
     * @brief Subscribe to bar data
     */
    void subscribe_bar(const std::string& symbol, Interval interval, const std::string& gateway_name);

    /**
     * @brief Unsubscribe from tick data
     */
    void unsubscribe(const std::string& symbol, const std::string& gateway_name);

    //=========================================================================
    // Order Management
    //=========================================================================

    /**
     * @brief Send order
     * @param req Order request
     * @param gateway_name Gateway name
     * @return Order ID, empty on failure
     */
    std::string send_order(const OrderRequest& req, const std::string& gateway_name);

    /**
     * @brief Cancel order
     */
    bool cancel_order(const CancelRequest& req, const std::string& gateway_name);

    /**
     * @brief Cancel all orders
     */
    void cancel_all_orders(const std::string& symbol, const std::string& gateway_name);

    //=========================================================================
    // Query Methods (delegated to OMS)
    //=========================================================================

    /**
     * @brief Get tick data
     */
    std::optional<TickData> get_tick(const std::string& vt_symbol) const;

    /**
     * @brief Get order by ID
     */
    std::optional<OrderData> get_order(const std::string& vt_orderid) const;

    /**
     * @brief Get trade by ID
     */
    std::optional<TradeData> get_trade(const std::string& vt_tradeid) const;

    /**
     * @brief Get position
     */
    std::optional<PositionData> get_position(const std::string& vt_positionid) const;

    /**
     * @brief Get account
     */
    std::optional<AccountData> get_account(const std::string& vt_accountid) const;

    /**
     * @brief Get contract
     */
    std::optional<ContractData> get_contract(const std::string& vt_symbol) const;

    /**
     * @brief Get all ticks
     */
    std::vector<TickData> get_all_ticks() const;

    /**
     * @brief Get all orders
     */
    std::vector<OrderData> get_all_orders() const;

    /**
     * @brief Get all active orders
     */
    std::vector<OrderData> get_all_active_orders(const std::string& symbol = "") const;

    /**
     * @brief Get all trades
     */
    std::vector<TradeData> get_all_trades() const;

    /**
     * @brief Get all positions
     */
    std::vector<PositionData> get_all_positions() const;

    /**
     * @brief Get all accounts
     */
    std::vector<AccountData> get_all_accounts() const;

    /**
     * @brief Get all contracts
     */
    std::vector<ContractData> get_all_contracts() const;

    //=========================================================================
    // History Data Query
    //=========================================================================

    /**
     * @brief Query historical bar data
     */
    std::vector<BarData> query_history(const HistoryRequest& req, const std::string& gateway_name);

    //=========================================================================
    // Logging
    //=========================================================================

    /**
     * @brief Write log message
     */
    void write_log(const std::string& msg, const std::string& source = "");

    //=========================================================================
    // Event Engine Access
    //=========================================================================

    /**
     * @brief Get event engine
     */
    EventEngine* event_engine() { return event_engine_.get(); }

    //=========================================================================
    // OMS Access
    //=========================================================================

    /**
     * @brief Get OMS engine
     */
    OmsEngine* oms_engine() { return oms_engine_.get(); }

    //=========================================================================
    // Risk Manager Access
    //=========================================================================

    /**
     * @brief Get risk manager
     */
    RiskManager* risk_manager() { return risk_manager_.get(); }

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    void init_engines();
    void register_event_handlers();

    //=========================================================================
    // Member Variables
    //=========================================================================

    MainEngineConfig config_;

    // Core components
    std::unique_ptr<EventEngine> event_engine_;
    std::unique_ptr<OmsEngine> oms_engine_;
    std::unique_ptr<LogEngine> log_engine_;
    std::unique_ptr<RiskManager> risk_manager_;

    // Gateways
    std::unordered_map<std::string, std::unique_ptr<IExchange>> gateways_;
    std::vector<Exchange> exchanges_;
    mutable std::mutex gateways_mutex_;

    // State
    bool running_ = false;
};

//=============================================================================
// OmsEngine - Order Management System
//=============================================================================

/**
 * @brief Order Management System engine
 *
 * Based on howtrader's OmsEngine:
 * - Maintains ticks, orders, trades, positions, accounts, contracts
 * - Processes events and updates internal state
 * - Provides query methods
 */
class OmsEngine {
public:
    explicit OmsEngine(MainEngine* main_engine, EventEngine* event_engine);
    ~OmsEngine();

    //=========================================================================
    // Data Access
    //=========================================================================

    std::optional<TickData> get_tick(const std::string& vt_symbol) const;
    std::optional<OrderData> get_order(const std::string& vt_orderid) const;
    std::optional<TradeData> get_trade(const std::string& vt_tradeid) const;
    std::optional<PositionData> get_position(const std::string& vt_positionid) const;
    std::optional<AccountData> get_account(const std::string& vt_accountid) const;
    std::optional<ContractData> get_contract(const std::string& vt_symbol) const;

    std::vector<TickData> get_all_ticks() const;
    std::vector<OrderData> get_all_orders() const;
    std::vector<OrderData> get_all_active_orders(const std::string& symbol = "") const;
    std::vector<TradeData> get_all_trades() const;
    std::vector<PositionData> get_all_positions() const;
    std::vector<AccountData> get_all_accounts() const;
    std::vector<ContractData> get_all_contracts() const;

    //=========================================================================
    // Event Handlers
    //=========================================================================

    void process_tick_event(const Event& event);
    void process_order_event(const Event& event);
    void process_trade_event(const Event& event);
    void process_position_event(const Event& event);
    void process_account_event(const Event& event);
    void process_contract_event(const Event& event);

private:
    void register_event();

    MainEngine* main_engine_;
    EventEngine* event_engine_;

    // Data storage
    std::unordered_map<std::string, TickData> ticks_;
    std::unordered_map<std::string, OrderData> orders_;
    std::unordered_map<std::string, OrderData> active_orders_;
    std::unordered_map<std::string, TradeData> trades_;
    std::unordered_map<std::string, PositionData> positions_;
    std::unordered_map<std::string, AccountData> accounts_;
    std::unordered_map<std::string, ContractData> contracts_;

    mutable std::mutex data_mutex_;
};

//=============================================================================
// LogEngine
//=============================================================================

/**
 * @brief Log processing engine
 *
 * Based on howtrader's LogEngine:
 * - Processes log events
 * - Outputs to console and/or file
 */
class LogEngine {
public:
    LogEngine(MainEngine* main_engine, EventEngine* event_engine,
              bool console = true, bool file = false,
              const std::string& path = "logs/");
    ~LogEngine();

    void process_log_event(const Event& event);

private:
    void register_event();

    MainEngine* main_engine_;
    EventEngine* event_engine_;

    bool console_;
    bool file_;
    std::string log_path_;
};

} // namespace bitquant
