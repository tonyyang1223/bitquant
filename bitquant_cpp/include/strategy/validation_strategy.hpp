/**
 * @file validation_strategy.hpp
 * @brief Trading Validation Strategy - Standard Template for Testing All Trading Interfaces
 *
 * This strategy validates all trading interfaces and serves as a standard template
 * for strategy development. It tests:
 *
 * 1. Strategy Lifecycle: init, start, stop
 * 2. Data Feed: tick, bar
 * 3. Order Management: buy, sell, short, cover, cancel
 * 4. Position Management: query, update
 * 5. Account Management: balance, margin
 * 6. Risk Control: stop loss, take profit
 * 7. Statistics: PnL, win rate, drawdown
 *
 * Can be compiled as shared library (.so) for dynamic loading in live trading.
 *
 * @author BitQuant Team
 * @version 1.0.0
 */

#pragma once

#include "engine/strategy.hpp"
#include "engine/strategy_context.hpp"
#include "exchange/i_exchange.hpp"
#include "data/array_manager.hpp"
#include "statistics/performance.hpp"
#include <memory>
#include <chrono>
#include <fstream>
#include <sstream>

namespace bitquant {

//=============================================================================
// Validation Strategy Configuration
//=============================================================================

/**
 * @brief Configuration for validation strategy
 */
struct ValidationConfig {
    // Symbol and exchange
    std::string symbol = "BTCUSDT";
    Exchange exchange = Exchange::BINANCE;
    std::string gateway_name = "BINANCE";

    // Trading parameters
    double init_capital = 100000.0;      // Initial capital
    double trade_size = 0.01;            // Trade size (BTC)
    double max_position = 0.1;           // Maximum position size
    int max_orders_per_day = 10;         // Maximum orders per day

    // Risk parameters
    double stop_loss_pct = 0.02;         // Stop loss percentage (2%)
    double take_profit_pct = 0.04;       // Take profit percentage (4%)
    double max_drawdown_pct = 0.10;      // Maximum drawdown (10%)

    // Timing parameters
    int bar_interval_seconds = 60;       // Bar interval
    int order_timeout_seconds = 30;      // Order timeout

    // Validation flags
    bool test_tick = true;               // Test tick data
    bool test_bar = true;                // Test bar data
    bool test_order = true;              // Test order management
    bool test_position = true;           // Test position query
    bool test_account = true;            // Test account query
    bool test_risk = true;               // Test risk control

    // Logging
    bool log_to_file = true;
    std::string log_file = "validation_strategy.log";
};

//=============================================================================
// Validation Statistics
//=============================================================================

/**
 * @brief Statistics collected during validation
 */
struct ValidationStats {
    // Lifecycle
    int init_count = 0;
    int start_count = 0;
    int stop_count = 0;

    // Data feed
    int tick_count = 0;
    int bar_count = 0;
    int64_t first_tick_time = 0;
    int64_t last_tick_time = 0;

    // Orders
    int orders_sent = 0;
    int orders_filled = 0;
    int orders_cancelled = 0;
    int orders_rejected = 0;

    // Trades
    int trades_count = 0;
    int winning_trades = 0;
    int losing_trades = 0;

    // Position
    double max_position_held = 0.0;
    double avg_position_held = 0.0;

    // PnL
    double realized_pnl = 0.0;
    double unrealized_pnl = 0.0;
    double max_pnl = 0.0;
    double min_pnl = 0.0;
    double max_drawdown = 0.0;

    // Latency
    double avg_tick_latency_ms = 0.0;
    double avg_order_latency_ms = 0.0;

    // Errors
    int error_count = 0;
    std::vector<std::string> errors;

    // Timing
    int64_t start_time = 0;
    int64_t end_time = 0;
};

//=============================================================================
// Validation Strategy
//=============================================================================

/**
 * @brief Trading Validation Strategy
 *
 * Comprehensive strategy that tests all trading interfaces.
 * Serves as standard template for strategy development.
 *
 * Features:
 * - Tests all IExchange interface methods
 * - Tests all IStrategy callbacks
 * - Tests order management (buy/sell/short/cover/cancel)
 * - Tests position and account queries
 * - Tests risk management (stop loss, take profit)
 * - Collects comprehensive statistics
 * - Can be compiled as shared library for dynamic loading
 */
class ValidationStrategy : public IStrategy {
public:
    //=========================================================================
    // Constructor & Destructor
    //=========================================================================

    ValidationStrategy();
    explicit ValidationStrategy(const ValidationConfig& config);
    ~ValidationStrategy() override;

    //=========================================================================
    // Strategy Metadata
    //=========================================================================

    std::string strategy_name() const { return "ValidationStrategy"; }
    std::string strategy_version() const { return "1.0.0"; }

    //=========================================================================
    // IStrategy Lifecycle Callbacks
    //=========================================================================

    void on_init() override;
    void on_start() override;
    void on_stop() override;

    //=========================================================================
    // IStrategy Data Callbacks
    //=========================================================================

    void on_tick(const TickData& tick) override;
    void on_bar(const BarData& bar) override;

    //=========================================================================
    // IStrategy Order Callbacks
    //=========================================================================

    void on_order(const Order& order) override;
    void on_trade(const Trade& trade) override;
    void on_stop_order(const StopOrder& stop_order) override;

    //=========================================================================
    // Validation Test Methods
    //=========================================================================

    /**
     * @brief Test all tick data functionality
     */
    bool test_tick_data();

    /**
     * @brief Test all bar data functionality
     */
    bool test_bar_data();

    /**
     * @brief Test order management
     */
    bool test_order_management();

    /**
     * @brief Test position queries
     */
    bool test_position_query();

    /**
     * @brief Test account queries
     */
    bool test_account_query();

    /**
     * @brief Test risk management
     */
    bool test_risk_management();

    /**
     * @brief Run all validation tests
     */
    bool run_all_tests();

    //=========================================================================
    // Exchange Interface Tests
    //=========================================================================

    /**
     * @brief Test exchange connection
     */
    bool test_exchange_connection();

    /**
     * @brief Test market data subscription
     */
    bool test_market_data_subscription();

    /**
     * @brief Test historical data query
     */
    bool test_historical_data();

    /**
     * @brief Test order submission
     */
    bool test_order_submission();

    /**
     * @brief Test order cancellation
     */
    bool test_order_cancellation();

    /**
     * @brief Test position query
     */
    bool test_exchange_position();

    /**
     * @brief Test account query
     */
    bool test_exchange_account();

    //=========================================================================
    // Trading Operations (Exposed for Testing)
    //=========================================================================

    /**
     * @brief Send buy order
     */
    order_id_t send_buy(double price, double volume);

    /**
     * @brief Send sell order
     */
    order_id_t send_sell(double price, double volume);

    /**
     * @brief Send short order
     */
    order_id_t send_short(double price, double volume);

    /**
     * @brief Send cover order
     */
    order_id_t send_cover(double price, double volume);

    /**
     * @brief Cancel order by ID
     */
    bool cancel_order_by_id(const std::string& order_id);

    /**
     * @brief Cancel all active orders
     */
    void cancel_all_active_orders();

    //=========================================================================
    // Position Management
    //=========================================================================

    /**
     * @brief Get current position
     */
    double get_current_position() const;

    /**
     * @brief Get position direction
     */
    Direction get_position_direction() const;

    /**
     * @brief Check if position is flat
     */
    bool is_position_flat() const;

    //=========================================================================
    // Risk Management
    //=========================================================================

    /**
     * @brief Set stop loss
     */
    void set_stop_loss(double price);

    /**
     * @brief Set take profit
     */
    void set_take_profit(double price);

    /**
     * @brief Check stop loss triggered
     */
    bool check_stop_loss(double current_price);

    /**
     * @brief Check take profit triggered
     */
    bool check_take_profit(double current_price);

    /**
     * @brief Calculate current drawdown
     */
    double calculate_drawdown() const;

    //=========================================================================
    // Statistics
    //=========================================================================

    /**
     * @brief Get validation statistics
     */
    const ValidationStats& get_stats() const { return stats_; }

    /**
     * @brief Get performance metrics
     */
    PerformanceMetrics get_performance() const;

    /**
     * @brief Generate validation report
     */
    std::string generate_report() const;

    /**
     * @brief Save report to file
     */
    bool save_report(const std::string& filename) const;

    //=========================================================================
    // Configuration
    //=========================================================================

    void set_config(const ValidationConfig& config) { config_ = config; }
    const ValidationConfig& get_config() const { return config_; }

    void set_exchange(IExchange* exchange) { exchange_ = exchange; }
    void set_context(StrategyContext* context) { context_ = context; }

    //=========================================================================
    // Logging
    //=========================================================================

    void write_log(const std::string& msg);
    void log_error(const std::string& error);

protected:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    void update_stats();
    void check_risk_limits();
    void process_pending_orders();
    double calculate_pnl(double price) const;

private:
    //=========================================================================
    // Members
    //=========================================================================

    ValidationConfig config_;
    ValidationStats stats_;
    PerformanceMetrics performance_;

    // Exchange and context
    IExchange* exchange_ = nullptr;
    StrategyContext* context_ = nullptr;

    // Current state
    TickData last_tick_;
    BarData last_bar_;
    double current_position_ = 0.0;
    double entry_price_ = 0.0;
    double peak_pnl_ = 0.0;

    // Orders
    std::vector<std::string> active_orders_;
    std::unordered_map<std::string, Order> order_history_;

    // Risk management
    double stop_loss_price_ = 0.0;
    double take_profit_price_ = 0.0;
    bool stop_loss_active_ = false;
    bool take_profit_active_ = false;

    // Logging
    std::ofstream log_stream_;
    std::mutex log_mutex_;

    // Timing
    std::chrono::steady_clock::time_point last_bar_time_;
    std::chrono::steady_clock::time_point last_order_time_;
};

//=============================================================================
// Strategy Factory for Dynamic Loading
//=============================================================================

/**
 * @brief Factory for creating validation strategy instances
 *
 * Used for dynamic loading from shared library.
 */
class ValidationStrategyFactory {
public:
    /**
     * @brief Create validation strategy
     */
    static std::unique_ptr<ValidationStrategy> create();

    /**
     * @brief Create with configuration
     */
    static std::unique_ptr<ValidationStrategy> create(const ValidationConfig& config);

    /**
     * @brief Get strategy info for registration
     */
    static std::string get_info();
};

//=============================================================================
// Plugin Entry Points (for .so loading)
//=============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create strategy instance (plugin entry point)
 */
void* create_strategy();

/**
 * @brief Destroy strategy instance (plugin entry point)
 */
void destroy_strategy(void* strategy);

/**
 * @brief Get strategy name
 */
const char* get_strategy_name();

/**
 * @brief Get strategy version
 */
const char* get_strategy_version();

/**
 * @brief Get strategy info
 */
const char* get_strategy_info();

#ifdef __cplusplus
}
#endif

} // namespace bitquant
