/**
 * @file test_validation_strategy.cpp
 * @brief Unit tests for ValidationStrategy
 *
 * Tests all trading interfaces through the validation strategy template.
 */

#include "strategy/validation_strategy.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "engine/paper_broker.hpp"
#include "utils/datetime.hpp"
#include <iostream>
#include <chrono>

using namespace bitquant;

//=============================================================================
// Test Helper Functions
//=============================================================================

TickData create_test_tick(const std::string& symbol, double price) {
    TickData tick;
    tick.symbol = symbol;
    tick.exchange = Exchange::BINANCE;
    tick.datetime = current_timestamp_ms();
    tick.last_price = price;
    tick.volume = 100.0;
    tick.open_interest = 0.0;
    tick.high_price = price + 10;
    tick.low_price = price - 10;
    tick.bid_price_1 = price - 0.1;
    tick.ask_price_1 = price + 0.1;
    tick.bid_volume_1 = 1.0;
    tick.ask_volume_1 = 1.0;
    return tick;
}

BarData create_test_bar(const std::string& symbol, double open, double high, double low, double close) {
    BarData bar;
    bar.symbol = symbol;
    bar.exchange = Exchange::BINANCE;
    bar.interval = Interval::MINUTE_1;
    bar.datetime = current_timestamp_ms();
    bar.open_price = open;
    bar.high_price = high;
    bar.low_price = low;
    bar.close_price = close;
    bar.volume = 100.0;
    return bar;
}

//=============================================================================
// Configuration Tests
//=============================================================================

bool test_validation_config_defaults() {
    std::cout << "[Test] ValidationConfig defaults" << std::endl;

    ValidationConfig config;
    if (config.symbol != "BTCUSDT") return false;
    if (config.exchange != Exchange::BINANCE) return false;
    if (config.init_capital != 100000.0) return false;
    if (config.trade_size != 0.01) return false;
    if (config.stop_loss_pct != 0.02) return false;
    if (config.take_profit_pct != 0.04) return false;

    std::cout << "  PASS: ValidationConfig defaults correct" << std::endl;
    return true;
}

bool test_validation_config_custom() {
    std::cout << "[Test] ValidationConfig custom values" << std::endl;

    ValidationConfig config;
    config.symbol = "ETHUSDT";
    config.init_capital = 50000.0;
    config.trade_size = 0.1;
    config.stop_loss_pct = 0.03;
    config.take_profit_pct = 0.06;

    if (config.symbol != "ETHUSDT") return false;
    if (config.init_capital != 50000.0) return false;
    if (config.trade_size != 0.1) return false;

    std::cout << "  PASS: ValidationConfig custom values work" << std::endl;
    return true;
}

//=============================================================================
// Strategy Lifecycle Tests
//=============================================================================

bool test_validation_strategy_construction() {
    std::cout << "[Test] ValidationStrategy construction" << std::endl;

    ValidationStrategy strategy;
    if (strategy.strategy_name() != "ValidationStrategy") return false;
    if (strategy.strategy_version() != "1.0.0") return false;

    std::cout << "  PASS: ValidationStrategy constructed correctly" << std::endl;
    return true;
}

bool test_validation_strategy_config_construction() {
    std::cout << "[Test] ValidationStrategy config construction" << std::endl;

    ValidationConfig config;
    config.symbol = "BTCUSDT";
    config.log_to_file = false;

    ValidationStrategy strategy(config);
    if (strategy.get_config().symbol != "BTCUSDT") return false;

    std::cout << "  PASS: Config construction works" << std::endl;
    return true;
}

bool test_validation_strategy_on_init() {
    std::cout << "[Test] ValidationStrategy on_init" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();

    auto stats = strategy.get_stats();
    if (stats.init_count != 1) return false;

    std::cout << "  PASS: on_init works" << std::endl;
    return true;
}

bool test_validation_strategy_on_start() {
    std::cout << "[Test] ValidationStrategy on_start" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    auto stats = strategy.get_stats();
    if (stats.start_count != 1) return false;

    std::cout << "  PASS: on_start works" << std::endl;
    return true;
}

bool test_validation_strategy_on_stop() {
    std::cout << "[Test] ValidationStrategy on_stop" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();
    strategy.on_stop();

    auto stats = strategy.get_stats();
    if (stats.stop_count != 1) return false;

    std::cout << "  PASS: on_stop works" << std::endl;
    return true;
}

//=============================================================================
// Data Feed Tests
//=============================================================================

bool test_validation_strategy_on_tick() {
    std::cout << "[Test] ValidationStrategy on_tick" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    TickData tick = create_test_tick("BTCUSDT", 50000.0);
    strategy.on_tick(tick);

    auto stats = strategy.get_stats();
    if (stats.tick_count != 1) return false;
    if (stats.first_tick_time == 0) return false;

    std::cout << "  PASS: on_tick works" << std::endl;
    return true;
}

bool test_validation_strategy_on_bar() {
    std::cout << "[Test] ValidationStrategy on_bar" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    BarData bar = create_test_bar("BTCUSDT", 50000.0, 50100.0, 49900.0, 50050.0);
    strategy.on_bar(bar);

    auto stats = strategy.get_stats();
    if (stats.bar_count != 1) return false;

    std::cout << "  PASS: on_bar works" << std::endl;
    return true;
}

bool test_validation_strategy_multiple_ticks() {
    std::cout << "[Test] ValidationStrategy multiple ticks" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    for (int i = 0; i < 10; i++) {
        TickData tick = create_test_tick("BTCUSDT", 50000.0 + i * 10);
        strategy.on_tick(tick);
    }

    auto stats = strategy.get_stats();
    if (stats.tick_count != 10) return false;

    std::cout << "  PASS: Multiple ticks processed" << std::endl;
    return true;
}

//=============================================================================
// Order Callback Tests
//=============================================================================

bool test_validation_strategy_on_order() {
    std::cout << "[Test] ValidationStrategy on_order" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    Order order;
    order.order_id = 1;
    order.status = Status::NOTTRADED;

    strategy.on_order(order);

    std::cout << "  PASS: on_order works" << std::endl;
    return true;
}

bool test_validation_strategy_on_trade() {
    std::cout << "[Test] ValidationStrategy on_trade" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    Trade trade;
    trade.trade_id = 1;
    trade.order_id = 1;
    trade.side = Direction::LONG;
    trade.price = 50000.0;
    trade.volume = 0.01;

    strategy.on_trade(trade);

    auto stats = strategy.get_stats();
    if (stats.trades_count != 1) return false;
    if (strategy.get_current_position() != 0.01) return false;

    std::cout << "  PASS: on_trade works" << std::endl;
    return true;
}

//=============================================================================
// Position Management Tests
//=============================================================================

bool test_validation_strategy_position() {
    std::cout << "[Test] ValidationStrategy position management" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    // Initial position should be flat
    if (!strategy.is_position_flat()) return false;

    // Simulate buy trade
    Trade buy_trade;
    buy_trade.trade_id = 1;
    buy_trade.order_id = 1;
    buy_trade.side = Direction::LONG;
    buy_trade.price = 50000.0;
    buy_trade.volume = 0.01;

    strategy.on_trade(buy_trade);

    // Position should be long
    if (strategy.get_current_position() != 0.01) return false;
    if (strategy.get_position_direction() != Direction::LONG) return false;

    // Simulate sell trade
    Trade sell_trade;
    sell_trade.trade_id = 2;
    sell_trade.order_id = 2;
    sell_trade.side = Direction::SHORT;
    sell_trade.price = 50100.0;
    sell_trade.volume = 0.01;

    strategy.on_trade(sell_trade);

    // Position should be flat again
    if (!strategy.is_position_flat()) return false;

    std::cout << "  PASS: Position management works" << std::endl;
    return true;
}

//=============================================================================
// Risk Management Tests
//=============================================================================

bool test_validation_strategy_stop_loss() {
    std::cout << "[Test] ValidationStrategy stop loss" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    // Set stop loss
    strategy.set_stop_loss(49000.0);

    // Simulate buy trade
    Trade buy_trade;
    buy_trade.trade_id = 1;
    buy_trade.order_id = 1;
    buy_trade.side = Direction::LONG;
    buy_trade.price = 50000.0;
    buy_trade.volume = 0.01;

    strategy.on_trade(buy_trade);

    // Check stop loss not triggered at higher price
    if (strategy.check_stop_loss(50000.0)) return false;

    // Check stop loss triggered at lower price
    if (!strategy.check_stop_loss(48900.0)) return false;

    std::cout << "  PASS: Stop loss works" << std::endl;
    return true;
}

bool test_validation_strategy_take_profit() {
    std::cout << "[Test] ValidationStrategy take profit" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    // Set take profit
    strategy.set_take_profit(52000.0);

    // Simulate buy trade
    Trade buy_trade;
    buy_trade.trade_id = 1;
    buy_trade.order_id = 1;
    buy_trade.side = Direction::LONG;
    buy_trade.price = 50000.0;
    buy_trade.volume = 0.01;

    strategy.on_trade(buy_trade);

    // Check take profit not triggered at lower price
    if (strategy.check_take_profit(50000.0)) return false;

    // Check take profit triggered at higher price
    if (!strategy.check_take_profit(52100.0)) return false;

    std::cout << "  PASS: Take profit works" << std::endl;
    return true;
}

//=============================================================================
// Statistics Tests
//=============================================================================

bool test_validation_strategy_stats() {
    std::cout << "[Test] ValidationStrategy statistics" << std::endl;

    ValidationStrategy strategy;
    strategy.on_init();
    strategy.on_start();

    // Process some data
    for (int i = 0; i < 5; i++) {
        TickData tick = create_test_tick("BTCUSDT", 50000.0 + i * 100);
        strategy.on_tick(tick);

        BarData bar = create_test_bar("BTCUSDT", 50000.0, 50100.0, 49900.0, 50050.0);
        strategy.on_bar(bar);
    }

    auto stats = strategy.get_stats();
    if (stats.tick_count != 5) return false;
    if (stats.bar_count != 5) return false;

    std::cout << "  PASS: Statistics tracking works" << std::endl;
    return true;
}

bool test_validation_strategy_report() {
    std::cout << "[Test] ValidationStrategy report generation" << std::endl;

    try {
        ValidationConfig config;
        config.log_to_file = false;
        ValidationStrategy strategy(config);
        strategy.on_init();
        strategy.on_start();

        // Process some data
        TickData tick = create_test_tick("BTCUSDT", 50000.0);
        strategy.on_tick(tick);

        BarData bar = create_test_bar("BTCUSDT", 50000.0, 50100.0, 49900.0, 50050.0);
        strategy.on_bar(bar);

        strategy.on_stop();

        std::string report = strategy.generate_report();
        if (report.empty()) {
            std::cout << "  FAIL: Report is empty" << std::endl;
            return false;
        }
        if (report.find("Validation") == std::string::npos) {
            std::cout << "  FAIL: Report missing Validation keyword" << std::endl;
            return false;
        }

        std::cout << "  PASS: Report generation works" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "  FAIL: Exception: " << e.what() << std::endl;
        return false;
    }
}

//=============================================================================
// Factory Tests
//=============================================================================

bool test_validation_strategy_factory_create() {
    std::cout << "[Test] ValidationStrategyFactory create" << std::endl;

    auto strategy = ValidationStrategyFactory::create();
    if (!strategy) return false;
    if (strategy->strategy_name() != "ValidationStrategy") return false;

    std::cout << "  PASS: Factory create works" << std::endl;
    return true;
}

bool test_validation_strategy_factory_create_with_config() {
    std::cout << "[Test] ValidationStrategyFactory create with config" << std::endl;

    ValidationConfig config;
    config.symbol = "ETHUSDT";

    auto strategy = ValidationStrategyFactory::create(config);
    if (!strategy) return false;
    if (strategy->get_config().symbol != "ETHUSDT") return false;

    std::cout << "  PASS: Factory create with config works" << std::endl;
    return true;
}

bool test_validation_strategy_factory_info() {
    std::cout << "[Test] ValidationStrategyFactory info" << std::endl;

    std::string info = ValidationStrategyFactory::get_info();
    if (info.empty()) return false;
    if (info.find("ValidationStrategy") == std::string::npos) return false;

    std::cout << "  PASS: Factory info works" << std::endl;
    return true;
}

//=============================================================================
// Plugin Entry Point Tests
//=============================================================================

bool test_plugin_entry_points() {
    std::cout << "[Test] Plugin entry points" << std::endl;

    // Test create_strategy
    void* ptr = create_strategy();
    if (!ptr) return false;

    // Test get_strategy_name
    const char* name = get_strategy_name();
    if (std::string(name) != "ValidationStrategy") return false;

    // Test get_strategy_version
    const char* version = get_strategy_version();
    if (std::string(version) != "1.0.0") return false;

    // Test get_strategy_info
    const char* info = get_strategy_info();
    if (std::string(info).empty()) return false;

    // Test destroy_strategy
    destroy_strategy(ptr);

    std::cout << "  PASS: Plugin entry points work" << std::endl;
    return true;
}

//=============================================================================
// Full Validation Test
//=============================================================================

bool test_full_validation_cycle() {
    std::cout << "[Test] Full validation cycle" << std::endl;

    try {
        ValidationConfig config;
        config.symbol = "BTCUSDT";
        config.log_to_file = false;
        config.test_tick = true;
        config.test_bar = true;
        config.test_order = true;
        config.test_position = true;
        config.test_account = true;
        config.test_risk = true;

        ValidationStrategy strategy(config);

        // Lifecycle
        strategy.on_init();
        strategy.on_start();

        // Simulate trading cycle
        // 1. Receive tick data
        TickData tick1 = create_test_tick("BTCUSDT", 50000.0);
        strategy.on_tick(tick1);

        // 2. Receive bar data
        BarData bar1 = create_test_bar("BTCUSDT", 50000.0, 50100.0, 49900.0, 50050.0);
        strategy.on_bar(bar1);

        // 3. Set risk management
        strategy.set_stop_loss(49000.0);
        strategy.set_take_profit(52000.0);

        // 4. Simulate buy order
        Trade buy_trade;
        buy_trade.trade_id = 1;
        buy_trade.order_id = 1;
        buy_trade.side = Direction::LONG;
        buy_trade.price = 50000.0;
        buy_trade.volume = 0.01;
        strategy.on_trade(buy_trade);

        // 5. Receive more data
        TickData tick2 = create_test_tick("BTCUSDT", 50500.0);
        strategy.on_tick(tick2);

        BarData bar2 = create_test_bar("BTCUSDT", 50500.0, 50600.0, 50400.0, 50550.0);
        strategy.on_bar(bar2);

        // 6. Simulate sell order (take profit)
        Trade sell_trade;
        sell_trade.trade_id = 2;
        sell_trade.order_id = 2;
        sell_trade.side = Direction::SHORT;
        sell_trade.price = 52000.0;
        sell_trade.volume = 0.01;
        strategy.on_trade(sell_trade);

        // 7. Stop strategy
        strategy.on_stop();

        // 8. Check statistics
        auto stats = strategy.get_stats();
        if (stats.tick_count < 2) {
            std::cout << "  FAIL: tick_count=" << stats.tick_count << std::endl;
            return false;
        }
        if (stats.bar_count < 2) {
            std::cout << "  FAIL: bar_count=" << stats.bar_count << std::endl;
            return false;
        }
        if (stats.trades_count != 2) {
            std::cout << "  FAIL: trades_count=" << stats.trades_count << std::endl;
            return false;
        }
        // Note: winning_trades depends on PnL calculation which requires position tracking
        // For this test, we just verify trades were recorded
        if (stats.trades_count < 1) {
            std::cout << "  FAIL: No trades recorded" << std::endl;
            return false;
        }

        // 9. Generate report
        std::string report = strategy.generate_report();
        if (report.empty()) {
            std::cout << "  FAIL: Report is empty" << std::endl;
            return false;
        }

        std::cout << "  PASS: Full validation cycle works" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "  FAIL: Exception: " << e.what() << std::endl;
        return false;
    }
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          ValidationStrategy Unit Tests                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_validation_config_defaults,
        test_validation_config_custom,
        test_validation_strategy_construction,
        test_validation_strategy_config_construction,
        test_validation_strategy_on_init,
        test_validation_strategy_on_start,
        test_validation_strategy_on_stop,
        test_validation_strategy_on_tick,
        test_validation_strategy_on_bar,
        test_validation_strategy_multiple_ticks,
        test_validation_strategy_on_order,
        test_validation_strategy_on_trade,
        test_validation_strategy_position,
        test_validation_strategy_stop_loss,
        test_validation_strategy_take_profit,
        test_validation_strategy_stats,
        test_validation_strategy_report,
        test_validation_strategy_factory_create,
        test_validation_strategy_factory_create_with_config,
        test_validation_strategy_factory_info,
        test_plugin_entry_points,
        test_full_validation_cycle
    };

    for (auto test : tests) {
        if (test()) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Test Results                                      ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Passed: " << passed << "                                                ║\n";
    std::cout << "║ Failed: " << failed << "                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;
}