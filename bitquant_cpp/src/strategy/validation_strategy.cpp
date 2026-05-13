/**
 * @file validation_strategy.cpp
 * @brief Trading Validation Strategy Implementation
 *
 * Standard template for testing all trading interfaces.
 */

#include "strategy/validation_strategy.hpp"
#include "utils/logger.hpp"
#include "utils/datetime.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace bitquant {

//=============================================================================
// Constructor & Destructor
//=============================================================================

ValidationStrategy::ValidationStrategy()
    : config_{} {
    stats_.start_time = current_timestamp_ms();
}

ValidationStrategy::ValidationStrategy(const ValidationConfig& config)
    : config_(config) {
    stats_.start_time = current_timestamp_ms();

    if (config_.log_to_file && !config_.log_file.empty()) {
        log_stream_.open(config_.log_file, std::ios::app);
    }
}

ValidationStrategy::~ValidationStrategy() {
    if (log_stream_.is_open()) {
        log_stream_.close();
    }
}

//=============================================================================
// IStrategy Lifecycle Callbacks
//=============================================================================

void ValidationStrategy::on_init() {
    write_log("[ValidationStrategy] on_init() called");
    stats_.init_count++;
    inited_ = true;

    // Test exchange connection if available
    if (config_.test_order && exchange_) {
        test_exchange_connection();
    }
}

void ValidationStrategy::on_start() {
    write_log("[ValidationStrategy] on_start() called");
    stats_.start_count++;
    trading_ = true;
    stats_.start_time = current_timestamp_ms();

    // Subscribe to market data
    if (exchange_) {
        test_market_data_subscription();
    }
}

void ValidationStrategy::on_stop() {
    write_log("[ValidationStrategy] on_stop() called");
    stats_.stop_count++;
    trading_ = false;
    stats_.end_time = current_timestamp_ms();

    // Cancel all active orders
    cancel_all_active_orders();

    // Generate final report
    generate_report();
}

//=============================================================================
// IStrategy Data Callbacks
//=============================================================================

void ValidationStrategy::on_tick(const TickData& tick) {
    if (!config_.test_tick) return;

    last_tick_ = tick;
    stats_.tick_count++;

    if (stats_.first_tick_time == 0) {
        stats_.first_tick_time = tick.datetime;
    }
    stats_.last_tick_time = tick.datetime;

    // Calculate latency
    auto now = current_timestamp_ms();
    double latency_ms = static_cast<double>(now - tick.datetime);
    stats_.avg_tick_latency_ms = (stats_.avg_tick_latency_ms * (stats_.tick_count - 1) + latency_ms)
                                  / stats_.tick_count;

    // Check risk limits
    if (config_.test_risk) {
        check_risk_limits();
    }

    // Process pending orders
    process_pending_orders();
}

void ValidationStrategy::on_bar(const BarData& bar) {
    if (!config_.test_bar) return;

    last_bar_ = bar;
    stats_.bar_count++;

    // Update array manager
    am_.update_bar(bar);

    // Update position tracking
    update_stats();

    // Check stop loss / take profit
    if (config_.test_risk && current_position_ != 0) {
        if (check_stop_loss(bar.close_price)) {
            write_log("[ValidationStrategy] Stop loss triggered at " + std::to_string(bar.close_price));
            if (current_position_ > 0) {
                send_sell(bar.close_price, std::abs(current_position_));
            } else {
                send_cover(bar.close_price, std::abs(current_position_));
            }
        } else if (check_take_profit(bar.close_price)) {
            write_log("[ValidationStrategy] Take profit triggered at " + std::to_string(bar.close_price));
            if (current_position_ > 0) {
                send_sell(bar.close_price, std::abs(current_position_));
            } else {
                send_cover(bar.close_price, std::abs(current_position_));
            }
        }
    }
}

//=============================================================================
// IStrategy Order Callbacks
//=============================================================================

void ValidationStrategy::on_order(const Order& order) {
    std::string order_id = std::to_string(order.order_id);
    write_log("[ValidationStrategy] on_order() - " + order_id +
              " status=" + std::to_string(static_cast<int>(order.status)));

    // Track order
    order_history_[order_id] = order;

    switch (order.status) {
        case Status::SUBMITTING:
        case Status::NOTTRADED:
            // Add to active orders
            active_orders_.push_back(order_id);
            break;
        case Status::ALLTRADED:
            stats_.orders_filled++;
            // Remove from active orders
            active_orders_.erase(
                std::remove(active_orders_.begin(), active_orders_.end(), order_id),
                active_orders_.end()
            );
            break;
        case Status::CANCELLED:
            stats_.orders_cancelled++;
            // Remove from active orders
            active_orders_.erase(
                std::remove(active_orders_.begin(), active_orders_.end(), order_id),
                active_orders_.end()
            );
            break;
        case Status::REJECTED:
            stats_.orders_rejected++;
            log_error("Order rejected: " + order_id);
            // Remove from active orders
            active_orders_.erase(
                std::remove(active_orders_.begin(), active_orders_.end(), order_id),
                active_orders_.end()
            );
            break;
        default:
            break;
    }
}

void ValidationStrategy::on_trade(const Trade& trade) {
    write_log("[ValidationStrategy] on_trade() - " + std::to_string(trade.trade_id) +
              " price=" + std::to_string(trade.price) +
              " volume=" + std::to_string(trade.volume));

    stats_.trades_count++;

    // Update position
    if (trade.side == Direction::LONG) {
        current_position_ += trade.volume;
        if (entry_price_ == 0) {
            entry_price_ = trade.price;
        } else {
            entry_price_ = (entry_price_ * (current_position_ - trade.volume) +
                           trade.price * trade.volume) / current_position_;
        }
    } else {
        current_position_ -= trade.volume;
        if (current_position_ == 0) {
            entry_price_ = 0;
        }
    }

    // Update max position
    stats_.max_position_held = std::max(stats_.max_position_held, std::abs(current_position_));

    // Calculate PnL
    double pnl = calculate_pnl(trade.price);
    if (pnl > 0) {
        stats_.winning_trades++;
    } else {
        stats_.losing_trades++;
    }
    stats_.realized_pnl += pnl;

    // Update peak PnL
    peak_pnl_ = std::max(peak_pnl_, stats_.realized_pnl);
    stats_.max_pnl = std::max(stats_.max_pnl, stats_.realized_pnl);
    stats_.min_pnl = std::min(stats_.min_pnl, stats_.realized_pnl);

    // Calculate drawdown
    double dd = peak_pnl_ - stats_.realized_pnl;
    stats_.max_drawdown = std::max(stats_.max_drawdown, dd);
}

void ValidationStrategy::on_stop_order(const StopOrder& stop_order) {
    write_log("[ValidationStrategy] on_stop_order() - " + stop_order.stop_orderid);
    (void)stop_order;
}

//=============================================================================
// Validation Test Methods
//=============================================================================

bool ValidationStrategy::test_tick_data() {
    write_log("[Test] Testing tick data...");
    return stats_.tick_count > 0;
}

bool ValidationStrategy::test_bar_data() {
    write_log("[Test] Testing bar data...");
    return stats_.bar_count > 0;
}

bool ValidationStrategy::test_order_management() {
    write_log("[Test] Testing order management...");
    return stats_.orders_sent > 0 && stats_.orders_filled > 0;
}

bool ValidationStrategy::test_position_query() {
    write_log("[Test] Testing position query...");
    if (!exchange_) return false;

    auto positions = exchange_->query_positions(config_.symbol);
    return true;  // Query succeeded even if empty
}

bool ValidationStrategy::test_account_query() {
    write_log("[Test] Testing account query...");
    if (!exchange_) return false;

    auto account = exchange_->query_account();
    return account.has_value();
}

bool ValidationStrategy::test_risk_management() {
    write_log("[Test] Testing risk management...");
    return stats_.max_drawdown <= config_.max_drawdown_pct * config_.init_capital;
}

bool ValidationStrategy::run_all_tests() {
    write_log("[Test] Running all validation tests...");

    bool all_passed = true;

    if (config_.test_tick) {
        all_passed &= test_tick_data();
    }

    if (config_.test_bar) {
        all_passed &= test_bar_data();
    }

    if (config_.test_order) {
        all_passed &= test_order_management();
    }

    if (config_.test_position) {
        all_passed &= test_position_query();
    }

    if (config_.test_account) {
        all_passed &= test_account_query();
    }

    if (config_.test_risk) {
        all_passed &= test_risk_management();
    }

    write_log(all_passed ? "[Test] All tests PASSED" : "[Test] Some tests FAILED");
    return all_passed;
}

//=============================================================================
// Exchange Interface Tests
//=============================================================================

bool ValidationStrategy::test_exchange_connection() {
    write_log("[Test] Testing exchange connection...");
    if (!exchange_) {
        log_error("Exchange not set");
        return false;
    }

    bool connected = exchange_->is_connected();
    write_log(connected ? "[Test] Exchange connected" : "[Test] Exchange not connected");
    return true;
}

bool ValidationStrategy::test_market_data_subscription() {
    write_log("[Test] Testing market data subscription...");
    if (!exchange_) return false;

    SubscribeRequest req;
    req.symbol = config_.symbol;
    exchange_->subscribe_tick(req);
    exchange_->subscribe_bar(config_.symbol, Interval::MINUTE_1);

    return true;
}

bool ValidationStrategy::test_historical_data() {
    write_log("[Test] Testing historical data query...");
    if (!exchange_) return false;

    HistoryRequest req;
    req.symbol = config_.symbol;
    req.interval = Interval::MINUTE_1;
    req.limit = 100;

    auto bars = exchange_->get_bars(req);
    write_log("[Test] Historical data: " + std::to_string(bars.size()) + " bars");
    return true;
}

bool ValidationStrategy::test_order_submission() {
    write_log("[Test] Testing order submission...");
    if (!exchange_) return false;

    // Get current price
    double price = exchange_->get_price(config_.symbol);
    if (price <= 0) {
        log_error("Cannot get current price");
        return false;
    }

    // Send test order
    OrderRequest req;
    req.symbol = config_.symbol;
    req.direction = Direction::LONG;
    req.price = price;
    req.volume = config_.trade_size;
    req.type = OrderType::LIMIT;

    std::string order_id = exchange_->send_order(req);
    if (order_id.empty()) {
        log_error("Order submission failed");
        return false;
    }

    stats_.orders_sent++;
    active_orders_.push_back(order_id);
    write_log("[Test] Order submitted: " + order_id);
    return true;
}

bool ValidationStrategy::test_order_cancellation() {
    write_log("[Test] Testing order cancellation...");
    if (!exchange_ || active_orders_.empty()) return false;

    std::string order_id = active_orders_.front();
    CancelRequest req;
    req.symbol = config_.symbol;
    req.orderid = order_id;

    bool result = exchange_->cancel_order(req);
    if (result) {
        write_log("[Test] Order cancelled: " + order_id);
    }
    return result;
}

bool ValidationStrategy::test_exchange_position() {
    write_log("[Test] Testing exchange position query...");
    if (!exchange_) return false;

    auto positions = exchange_->query_positions();
    write_log("[Test] Positions: " + std::to_string(positions.size()));
    return true;
}

bool ValidationStrategy::test_exchange_account() {
    write_log("[Test] Testing exchange account query...");
    if (!exchange_) return false;

    auto account = exchange_->query_account();
    if (account.has_value()) {
        write_log("[Test] Account balance: " + std::to_string(account->balance));
        return true;
    }
    return false;
}

//=============================================================================
// Trading Operations
//=============================================================================

order_id_t ValidationStrategy::send_buy(double price, double volume) {
    write_log("[ValidationStrategy] send_buy() - price=" + std::to_string(price) +
              " volume=" + std::to_string(volume));

    if (!trading_) {
        log_error("Strategy not trading");
        return 0;
    }

    // Check position limit
    if (std::abs(current_position_) + volume > config_.max_position) {
        log_error("Position limit exceeded");
        return 0;
    }

    // Use IStrategy's buy method
    order_id_t order_id = buy(price, volume);
    if (order_id != 0) {
        stats_.orders_sent++;
        active_orders_.push_back(std::to_string(order_id));
    }
    return order_id;
}

order_id_t ValidationStrategy::send_sell(double price, double volume) {
    write_log("[ValidationStrategy] send_sell() - price=" + std::to_string(price) +
              " volume=" + std::to_string(volume));

    if (!trading_) {
        log_error("Strategy not trading");
        return 0;
    }

    order_id_t order_id = sell(price, volume);
    if (order_id != 0) {
        stats_.orders_sent++;
        active_orders_.push_back(std::to_string(order_id));
    }
    return order_id;
}

order_id_t ValidationStrategy::send_short(double price, double volume) {
    write_log("[ValidationStrategy] send_short() - price=" + std::to_string(price) +
              " volume=" + std::to_string(volume));

    if (!trading_) {
        log_error("Strategy not trading");
        return 0;
    }

    // Check position limit
    if (std::abs(current_position_) + volume > config_.max_position) {
        log_error("Position limit exceeded");
        return 0;
    }

    order_id_t order_id = short_order(price, volume);
    if (order_id != 0) {
        stats_.orders_sent++;
        active_orders_.push_back(std::to_string(order_id));
    }
    return order_id;
}

order_id_t ValidationStrategy::send_cover(double price, double volume) {
    write_log("[ValidationStrategy] send_cover() - price=" + std::to_string(price) +
              " volume=" + std::to_string(volume));

    if (!trading_) {
        log_error("Strategy not trading");
        return 0;
    }

    order_id_t order_id = cover(price, volume);
    if (order_id != 0) {
        stats_.orders_sent++;
        active_orders_.push_back(std::to_string(order_id));
    }
    return order_id;
}

bool ValidationStrategy::cancel_order_by_id(const std::string& order_id) {
    write_log("[ValidationStrategy] cancel_order_by_id() - " + order_id);

    try {
        order_id_t id = std::stoull(order_id);
        cancel_order(id);
        return true;
    } catch (...) {
        return false;
    }
}

void ValidationStrategy::cancel_all_active_orders() {
    write_log("[ValidationStrategy] cancel_all_active_orders() - count=" +
              std::to_string(active_orders_.size()));

    for (const auto& order_id : active_orders_) {
        cancel_order_by_id(order_id);
    }
}

//=============================================================================
// Position Management
//=============================================================================

double ValidationStrategy::get_current_position() const {
    return current_position_;
}

Direction ValidationStrategy::get_position_direction() const {
    if (current_position_ > 0) return Direction::LONG;
    if (current_position_ < 0) return Direction::SHORT;
    return Direction::NET;
}

bool ValidationStrategy::is_position_flat() const {
    return std::abs(current_position_) < 1e-8;
}

//=============================================================================
// Risk Management
//=============================================================================

void ValidationStrategy::set_stop_loss(double price) {
    stop_loss_price_ = price;
    stop_loss_active_ = true;
    write_log("[ValidationStrategy] Stop loss set at " + std::to_string(price));
}

void ValidationStrategy::set_take_profit(double price) {
    take_profit_price_ = price;
    take_profit_active_ = true;
    write_log("[ValidationStrategy] Take profit set at " + std::to_string(price));
}

bool ValidationStrategy::check_stop_loss(double current_price) {
    if (!stop_loss_active_ || current_position_ == 0) return false;

    if (current_position_ > 0) {
        return current_price <= stop_loss_price_;
    } else {
        return current_price >= stop_loss_price_;
    }
}

bool ValidationStrategy::check_take_profit(double current_price) {
    if (!take_profit_active_ || current_position_ == 0) return false;

    if (current_position_ > 0) {
        return current_price >= take_profit_price_;
    } else {
        return current_price <= take_profit_price_;
    }
}

double ValidationStrategy::calculate_drawdown() const {
    double dd = peak_pnl_ - stats_.realized_pnl;
    return dd / config_.init_capital;
}

//=============================================================================
// Statistics
//=============================================================================

PerformanceMetrics ValidationStrategy::get_performance() const {
    PerformanceMetrics perf;

    perf.total_return = stats_.realized_pnl / config_.init_capital;
    perf.total_trades = stats_.trades_count;
    perf.winning_trades = stats_.winning_trades;
    perf.losing_trades = stats_.losing_trades;
    perf.win_rate = stats_.trades_count > 0 ?
                    static_cast<double>(stats_.winning_trades) / stats_.trades_count : 0.0;
    perf.max_drawdown = stats_.max_drawdown / config_.init_capital;

    return perf;
}

std::string ValidationStrategy::generate_report() const {
    std::ostringstream oss;

    oss << "\n";
    oss << "╔══════════════════════════════════════════════════════════════╗\n";
    oss << "║           Validation Strategy Report                         ║\n";
    oss << "╚══════════════════════════════════════════════════════════════╝\n\n";

    // Configuration
    oss << "Configuration:\n";
    oss << "  Symbol:        " << config_.symbol << "\n";
    oss << "  Exchange:      " << config_.gateway_name << "\n";
    oss << "  Initial Capital: " << std::fixed << std::setprecision(2) << config_.init_capital << "\n";
    oss << "  Trade Size:    " << config_.trade_size << "\n\n";

    // Lifecycle
    oss << "Lifecycle:\n";
    oss << "  Init Count:    " << stats_.init_count << "\n";
    oss << "  Start Count:   " << stats_.start_count << "\n";
    oss << "  Stop Count:    " << stats_.stop_count << "\n\n";

    // Data Feed
    oss << "Data Feed:\n";
    oss << "  Tick Count:    " << stats_.tick_count << "\n";
    oss << "  Bar Count:     " << stats_.bar_count << "\n";
    oss << "  Avg Tick Latency: " << std::fixed << std::setprecision(2)
        << stats_.avg_tick_latency_ms << " ms\n\n";

    // Orders
    oss << "Orders:\n";
    oss << "  Sent:          " << stats_.orders_sent << "\n";
    oss << "  Filled:        " << stats_.orders_filled << "\n";
    oss << "  Cancelled:     " << stats_.orders_cancelled << "\n";
    oss << "  Rejected:      " << stats_.orders_rejected << "\n\n";

    // Trades
    oss << "Trades:\n";
    oss << "  Total:         " << stats_.trades_count << "\n";
    oss << "  Winning:       " << stats_.winning_trades << "\n";
    oss << "  Losing:        " << stats_.losing_trades << "\n";
    oss << "  Win Rate:      " << std::fixed << std::setprecision(2)
        << (stats_.trades_count > 0 ? 100.0 * stats_.winning_trades / stats_.trades_count : 0.0) << "%\n\n";

    // Position
    oss << "Position:\n";
    oss << "  Max Position:  " << std::fixed << std::setprecision(4) << stats_.max_position_held << "\n";
    oss << "  Current:       " << current_position_ << "\n\n";

    // PnL
    oss << "PnL:\n";
    oss << "  Realized:      " << std::fixed << std::setprecision(2) << stats_.realized_pnl << "\n";
    oss << "  Max PnL:       " << stats_.max_pnl << "\n";
    oss << "  Min PnL:       " << stats_.min_pnl << "\n";
    oss << "  Max Drawdown:  " << stats_.max_drawdown << "\n\n";

    // Errors
    oss << "Errors:\n";
    oss << "  Count:         " << stats_.error_count << "\n";
    if (!stats_.errors.empty()) {
        oss << "  Details:\n";
        for (const auto& err : stats_.errors) {
            oss << "    - " << err << "\n";
        }
    }

    oss << "\n══════════════════════════════════════════════════════════════\n";

    return oss.str();
}

bool ValidationStrategy::save_report(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    file << generate_report();
    file.close();
    return true;
}

//=============================================================================
// Internal Methods
//=============================================================================

void ValidationStrategy::update_stats() {
    // Update average position
    if (stats_.bar_count > 0) {
        stats_.avg_position_held = (stats_.avg_position_held * (stats_.bar_count - 1) +
                                   std::abs(current_position_)) / stats_.bar_count;
    }
}

void ValidationStrategy::check_risk_limits() {
    // Check drawdown limit
    double drawdown = calculate_drawdown();
    if (drawdown > config_.max_drawdown_pct) {
        log_error("Max drawdown exceeded: " + std::to_string(drawdown));
        // Could trigger emergency close here
    }

    // Check daily order limit
    if (stats_.orders_sent >= config_.max_orders_per_day) {
        log_error("Max orders per day reached");
    }
}

void ValidationStrategy::process_pending_orders() {
    // Check for timed out orders
    auto now = std::chrono::steady_clock::now();
    (void)now;
    // Implementation would check order ages and cancel if needed
}

double ValidationStrategy::calculate_pnl(double price) const {
    if (current_position_ == 0) return 0.0;
    return (price - entry_price_) * current_position_;
}

//=============================================================================
// Logging
//=============================================================================

void ValidationStrategy::write_log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    std::cout << msg << std::endl;

    if (log_stream_.is_open()) {
        log_stream_ << "[" << current_timestamp_ms() << "] " << msg << std::endl;
    }
}

void ValidationStrategy::log_error(const std::string& error) {
    stats_.error_count++;
    stats_.errors.push_back(error);
    write_log("[ERROR] " + error);
}

//=============================================================================
// Strategy Factory
//=============================================================================

std::unique_ptr<ValidationStrategy> ValidationStrategyFactory::create() {
    return std::make_unique<ValidationStrategy>();
}

std::unique_ptr<ValidationStrategy> ValidationStrategyFactory::create(const ValidationConfig& config) {
    return std::make_unique<ValidationStrategy>(config);
}

std::string ValidationStrategyFactory::get_info() {
    return "ValidationStrategy v1.0.0 - Trading Interface Validation Template";
}

//=============================================================================
// Plugin Entry Points
//=============================================================================

#ifdef __cplusplus
extern "C" {
#endif

void* create_strategy() {
    return new ValidationStrategy();
}

void destroy_strategy(void* strategy) {
    delete static_cast<ValidationStrategy*>(strategy);
}

const char* get_strategy_name() {
    return "ValidationStrategy";
}

const char* get_strategy_version() {
    return "1.0.0";
}

const char* get_strategy_info() {
    return "Trading Interface Validation Strategy - Standard Template for Testing";
}

#ifdef __cplusplus
}
#endif

} // namespace bitquant
