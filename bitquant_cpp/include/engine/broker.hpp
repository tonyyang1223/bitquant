/**
 * @file broker.hpp
 * @brief Order matching engine and backtest orchestrator
 *
 * Complete implementation of order matching, position management,
 * and performance statistics based on howtrader design.
 *
 * Based on: https://github.com/51bitquant/howtrader
 * - cross_limit_order(): Limit order matching
 * - cross_stop_order(): Stop order matching
 * - Daily PnL calculation
 */

#pragma once

#include "core/types.hpp"
#include "strategy.hpp"
#include "statistics/performance.hpp"
#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include <unordered_map>
#include <map>

namespace bitquant {

/**
 * @brief Broker configuration
 */
struct BrokerConfig {
    double initial_cash = 1'000'000.0;
    double commission_rate = 0.002;    // 0.2% 手续费
    double slippage_rate = 0.0005;     // 0.05% 滑点率
    double leverage = 1.0;              // 杠杆比例
    double min_margin_rate = 0.15;      // 最低保证金比例 (bitfinex标准)
    bool allow_short = true;            // 是否允许做空
    bool strict_order_matching = true;  // 严格订单匹配
    double size = 1.0;                  // Contract size
    double pricetick = 0.01;            // Minimum price tick
    int annual_days = 365;              // Trading days per year
};

/**
 * @brief Daily result for PnL calculation (Based on howtrader's DailyResult)
 */
struct DailyResult {
    int64_t date = 0;
    double close_price = 0.0;
    double pre_close = 0.0;

    int trade_count = 0;
    double start_pos = 0.0;
    double end_pos = 0.0;

    double turnover = 0.0;
    double commission = 0.0;
    double slippage = 0.0;

    double trading_pnl = 0.0;
    double holding_pnl = 0.0;
    double total_pnl = 0.0;
    double net_pnl = 0.0;
};

/**
 * @brief Order matching engine for backtesting
 *
 * This class implements complete order matching logic based on howtrader's
 * BacktestingEngine design. It supports:
 * - Limit order matching (cross_limit_order)
 * - Stop order matching (cross_stop_order)
 * - Position management with Direction and Offset
 * - Daily PnL calculation
 * - Performance statistics
 */
class Broker {
public:
    explicit Broker(const BrokerConfig& config = BrokerConfig{});

    // Configuration
    void set_strategy(std::unique_ptr<IStrategy> strategy);
    void set_data(const std::vector<BarData>& data);
    void set_cash(double cash) { config_.initial_cash = cash; }
    void set_leverage(double leverage) { config_.leverage = leverage; }
    void set_commission(double commission) { config_.commission_rate = commission; }

    //=========================================================================
    // Order placement (called by strategy - Based on howtrader CtaTemplate)
    //=========================================================================
    order_id_t buy(double price, double volume);      // Open long position
    order_id_t sell(double price, double volume);     // Close long position
    order_id_t short_order(double price, double volume);  // Open short position
    order_id_t cover(double price, double volume);    // Close short position
    order_id_t market_buy(double volume);
    order_id_t market_sell(double volume);

    void cancel_order(order_id_t order_id);
    void cancel_all_orders();

    //=========================================================================
    // Position queries
    //=========================================================================
    double position() const { return position_.volume; }
    Direction position_side() const { return position_.side; }
    double cash() const { return cash_; }
    double equity() const;
    double available_cash() const;
    double margin_used() const;
    double asset_value() const { return asset_value_; }

    //=========================================================================
    // Run backtest
    //=========================================================================
    void run();

    //=========================================================================
    // Strategy optimization (对应Python的optimize_strategy)
    //=========================================================================
    template<typename StrategyClass, typename... Args>
    void optimize_strategy(
        const std::unordered_map<std::string, std::vector<double>>& params,
        Args&&... args
    );

    //=========================================================================
    // Results
    //=========================================================================
    const std::vector<Trade>& trades() const { return trades_; }
    const PerformanceMetrics& performance() const { return performance_; }
    const std::vector<std::pair<int64_t, double>>& equity_curve() const {
        return equity_curve_;
    }

private:
    //=========================================================================
    // Order matching (Based on howtrader BacktestingEngine)
    //=========================================================================

    /**
     * @brief Cross limit orders with current bar/tick data
     * Implements howtrader's cross_limit_order() logic:
     * - For bar mode: long_cross_price = bar.low, short_cross_price = bar.high
     * - Creates TradeData on match
     * - Updates strategy position
     */
    void cross_limit_order(const BarData& bar);

    /**
     * @brief Cross stop orders with current bar/tick data
     * Implements howtrader's cross_stop_order() logic:
     * - For bar mode: long_cross_price = bar.high, short_cross_price = bar.low
     * - Creates limit order from stop order when triggered
     */
    void cross_stop_order(const BarData& bar);

    /**
     * @brief Send limit order
     */
    order_id_t send_limit_order(Direction direction, Offset offset, double price, double volume);

    /**
     * @brief Send stop order
     */
    order_id_t send_stop_order(Direction direction, Offset offset, double price, double volume);

    //=========================================================================
    // Trade execution and position updates
    //=========================================================================
    void execute_trade(const Trade& trade);
    void update_position(const Trade& trade);
    void update_margin_pnl(const BarData& bar);

    //=========================================================================
    // Daily PnL calculation (Based on howtrader's DailyResult)
    //=========================================================================
    void update_daily_close(double close_price, int64_t datetime);

    //=========================================================================
    // Statistics
    //=========================================================================
    void calculate_performance();

    //=========================================================================
    // Helper methods
    //=========================================================================
    double apply_slippage(double price, Direction side) const;

    //=========================================================================
    // Configuration and state
    //=========================================================================
    BrokerConfig config_;
    std::unique_ptr<IStrategy> strategy_;
    std::vector<BarData> data_;
    const BarData* current_bar_ = nullptr;

    // Account state (对应Python的字段)
    double cash_;
    double initial_cash_;
    double asset_value_ = 0.0;           // 资产估值 (Python: asset_value)
    Position position_;
    bool is_optimizing_strategy_ = false; // 是否优化模式 (Python: is_optimizing_strategy)

    //=========================================================================
    // Limit orders
    //=========================================================================
    std::unordered_map<order_id_t, Order> active_orders_;
    std::vector<Order> order_history_;
    order_id_t next_order_id_ = 1;
    int limit_order_count_ = 0;

    //=========================================================================
    // Stop orders (Based on howtrader)
    //=========================================================================
    std::unordered_map<std::string, StopOrder> active_stop_orders_;
    std::unordered_map<std::string, StopOrder> stop_orders_;
    int stop_order_count_ = 0;

    // Friend class for strategy access
    friend class IStrategy;

    //=========================================================================
    // Trades
    //=========================================================================
    std::vector<Trade> trades_;
    trade_id_t next_trade_id_ = 1;

    //=========================================================================
    // Daily results and equity curve
    //=========================================================================
    std::map<int64_t, DailyResult> daily_results_;
    std::vector<std::pair<int64_t, double>> equity_curve_;

    //=========================================================================
    // Performance results
    //=========================================================================
    PerformanceMetrics performance_;
};

} // namespace bitquant
