/**
 * @file trading_engine.hpp
 * @brief Unified trading engine for backtesting and live trading
 *
 * Based on howtrader's MainEngine design:
 * - Coordinates all components: Broker, StrategyManager, RiskManager, Exchange
 * - Supports multiple modes: Backtesting, Paper, Live
 * - Event-driven architecture
 * - Unified interface for all trading operations
 */

#pragma once

#include "core/types.hpp"
#include "engine/broker.hpp"
#include "engine/strategy_manager.hpp"
#include "engine/risk_manager.hpp"
#include "engine/event.hpp"
#include "exchange/i_exchange.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace bitquant {

//=============================================================================
// Engine Mode
//=============================================================================

/**
 * @brief Trading engine mode
 */
enum class EngineMode : uint8_t {
    BACKTESTING,  // Backtesting with historical data
    PAPER,        // Paper trading (simulation)
    LIVE          // Live trading with real exchange
};

//=============================================================================
// Engine Configuration
//=============================================================================

/**
 * @brief Trading engine configuration
 */
struct TradingEngineConfig {
    EngineMode mode = EngineMode::BACKTESTING;

    // Account settings
    double initial_capital = 1'000'000.0;
    double commission_rate = 0.002;     // 0.2%
    double slippage_rate = 0.0005;      // 0.05%
    double leverage = 1.0;
    bool allow_short = true;

    // Risk settings
    bool enable_risk_manager = true;
    int order_flow_limit = 50;
    double order_size_limit = 100.0;
    double trade_limit = 1000.0;
    int active_order_limit = 50;
    int order_cancel_limit = 500;

    // Exchange settings
    std::string exchange_name = "binance";
    std::string api_key;
    std::string api_secret;
    bool use_testnet = false;

    // Event settings
    int timer_interval_seconds = 1;
};

//=============================================================================
// Trading Engine
//=============================================================================

/**
 * @brief Unified trading engine
 *
 * Central coordinator for all trading components:
 * - Broker: Order execution and matching
 * - StrategyManager: Multi-strategy coordination
 * - RiskManager: Pre-trade risk checks
 * - Exchange: Market data and order routing
 *
 * Usage (Backtesting):
 * @code
 * TradingEngine engine;
 * engine.configure(config);
 * engine.load_data("BTCUSDT", bars);
 * engine.add_strategy("my_strategy", std::make_unique<MyStrategy>());
 * engine.run();
 * auto metrics = engine.get_performance();
 * @endcode
 *
 * Usage (Live):
 * @code
 * TradingEngine engine;
 * engine.configure(config);
 * engine.connect();
 * engine.subscribe("BTCUSDT");
 * engine.start_all_strategies();
 * // ... trading ...
 * engine.stop_all_strategies();
 * engine.disconnect();
 * @endcode
 */
class TradingEngine {
public:
    TradingEngine();
    explicit TradingEngine(const TradingEngineConfig& config);
    ~TradingEngine();

    //=========================================================================
    // Configuration
    //=========================================================================

    /**
     * @brief Configure the engine
     */
    void configure(const TradingEngineConfig& config);

    /**
     * @brief Get current configuration
     */
    const TradingEngineConfig& config() const { return config_; }

    /**
     * @brief Get engine mode
     */
    EngineMode mode() const { return config_.mode; }

    //=========================================================================
    // Connection Management (Live/Paper mode)
    //=========================================================================

    /**
     * @brief Connect to exchange
     * @return true on success
     */
    bool connect();

    /**
     * @brief Disconnect from exchange
     */
    void disconnect();

    /**
     * @brief Check if connected
     */
    bool is_connected() const;

    //=========================================================================
    // Data Management (Backtesting mode)
    //=========================================================================

    /**
     * @brief Load historical data for backtesting
     * @param symbol Trading symbol
     * @param bars Historical bar data
     */
    void load_data(const std::string& symbol, const std::vector<BarData>& bars);

    /**
     * @brief Load data from CSV file
     * @param symbol Trading symbol
     * @param filename CSV file path
     * @return Number of bars loaded
     */
    size_t load_data_from_csv(const std::string& symbol, const std::string& filename);

    //=========================================================================
    // Strategy Management
    //=========================================================================

    /**
     * @brief Add strategy
     */
    bool add_strategy(
        const std::string& name,
        std::unique_ptr<IStrategy> strategy,
        const std::string& symbol = "",
        const std::unordered_map<std::string, double>& params = {}
    );

    /**
     * @brief Initialize all strategies
     */
    void init_all_strategies();

    /**
     * @brief Start all strategies
     */
    void start_all_strategies();

    /**
     * @brief Stop all strategies
     */
    void stop_all_strategies();

    /**
     * @brief Get strategy names
     */
    std::vector<std::string> get_strategy_names() const;

    //=========================================================================
    // Market Data Subscription (Live/Paper mode)
    //=========================================================================

    /**
     * @brief Subscribe to market data
     */
    void subscribe(const std::string& symbol);

    /**
     * @brief Subscribe to bar data
     */
    void subscribe_bar(const std::string& symbol, Interval interval);

    //=========================================================================
    // Trading Operations
    //=========================================================================

    /**
     * @brief Send order (with risk check)
     * @param req Order request
     * @return Order ID, empty on failure
     */
    std::string send_order(const OrderRequest& req);

    /**
     * @brief Cancel order
     */
    bool cancel_order(const std::string& vt_orderid);

    /**
     * @brief Cancel all orders
     */
    void cancel_all_orders(const std::string& symbol = "");

    //=========================================================================
    // Query Methods
    //=========================================================================

    /**
     * @brief Get current price
     */
    double get_price(const std::string& symbol);

    /**
     * @brief Get account balance
     */
    double get_balance() const;

    /**
     * @brief Get position
     */
    double get_position(const std::string& symbol = "") const;

    /**
     * @brief Get open orders
     */
    std::vector<OrderData> get_open_orders(const std::string& symbol = "") const;

    //=========================================================================
    // Run Backtest (Backtesting mode)
    //=========================================================================

    /**
     * @brief Run backtesting
     */
    void run_backtest();

    /**
     * @brief Run backtesting with progress callback
     */
    using ProgressCallback = std::function<void(int current, int total)>;
    void run_backtest(ProgressCallback progress);

    //=========================================================================
    // Results
    //=========================================================================

    /**
     * @brief Get performance metrics
     */
    const PerformanceMetrics& get_performance() const;

    /**
     * @brief Get equity curve
     */
    const std::vector<std::pair<int64_t, double>>& get_equity_curve() const;

    /**
     * @brief Get all trades
     */
    const std::vector<Trade>& get_trades() const;

    //=========================================================================
    // Event Handlers
    //=========================================================================

    /**
     * @brief Process tick event
     */
    void on_tick(const TickData& tick);

    /**
     * @brief Process bar event
     */
    void on_bar(const BarData& bar);

    /**
     * @brief Process order event
     */
    void on_order(const OrderData& order);

    /**
     * @brief Process trade event
     */
    void on_trade(const TradeData& trade);

    //=========================================================================
    // Callbacks
    //=========================================================================

    using LogCallback = std::function<void(const std::string&)>;
    void set_log_callback(LogCallback callback);

    using OrderCallback = std::function<void(const OrderData&)>;
    void set_order_callback(OrderCallback callback);

    using TradeCallback = std::function<void(const TradeData&)>;
    void set_trade_callback(TradeCallback callback);

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    void initialize_components();
    void register_event_handlers();
    void write_log(const std::string& msg);

    //=========================================================================
    // Member Variables
    //=========================================================================

    TradingEngineConfig config_;

    // Core components
    std::unique_ptr<EventEngine> event_engine_;
    std::unique_ptr<Broker> broker_;
    std::unique_ptr<StrategyManager> strategy_manager_;
    std::unique_ptr<RiskManager> risk_manager_;
    std::unique_ptr<IExchange> exchange_;

    // State
    bool initialized_ = false;
    bool running_ = false;

    // Data storage for backtesting
    std::unordered_map<std::string, std::vector<BarData>> data_store_;

    // Callbacks
    LogCallback log_callback_;
    OrderCallback order_callback_;
    TradeCallback trade_callback_;
};

//=============================================================================
// Engine Builder (Fluent Interface)
//=============================================================================

/**
 * @brief Builder for TradingEngine configuration
 *
 * Usage:
 * @code
 * auto engine = TradingEngineBuilder()
 *     .set_mode(EngineMode::BACKTESTING)
 *     .set_capital(1000000)
 *     .set_commission(0.001)
 *     .enable_risk_manager(true)
 *     .build();
 * @endcode
 */
class TradingEngineBuilder {
public:
    TradingEngineBuilder& set_mode(EngineMode mode) {
        config_.mode = mode;
        return *this;
    }

    TradingEngineBuilder& set_capital(double capital) {
        config_.initial_capital = capital;
        return *this;
    }

    TradingEngineBuilder& set_commission(double rate) {
        config_.commission_rate = rate;
        return *this;
    }

    TradingEngineBuilder& set_slippage(double rate) {
        config_.slippage_rate = rate;
        return *this;
    }

    TradingEngineBuilder& set_leverage(double leverage) {
        config_.leverage = leverage;
        return *this;
    }

    TradingEngineBuilder& allow_short(bool allow) {
        config_.allow_short = allow;
        return *this;
    }

    TradingEngineBuilder& enable_risk_manager(bool enable) {
        config_.enable_risk_manager = enable;
        return *this;
    }

    TradingEngineBuilder& set_exchange(const std::string& name) {
        config_.exchange_name = name;
        return *this;
    }

    TradingEngineBuilder& set_api_credentials(
        const std::string& key,
        const std::string& secret
    ) {
        config_.api_key = key;
        config_.api_secret = secret;
        return *this;
    }

    std::unique_ptr<TradingEngine> build() {
        auto engine = std::make_unique<TradingEngine>();
        engine->configure(config_);
        return engine;
    }

private:
    TradingEngineConfig config_;
};

} // namespace bitquant
