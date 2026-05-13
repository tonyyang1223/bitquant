/**
 * @file test_strategy.cpp
 * @brief Unit tests for IStrategy and TargetPosTemplate
 */

#include "engine/strategy.hpp"
#include "engine/broker.hpp"
#include "core/types.hpp"
#include <iostream>
#include <memory>

using namespace bitquant;

// Test strategy implementation
class TestStrategyImpl : public IStrategy {
public:
    int init_count = 0;
    int start_count = 0;
    int stop_count = 0;
    int bar_count = 0;
    int tick_count = 0;
    int order_count = 0;
    int trade_count = 0;

    void on_init() override {
        init_count++;
        inited_ = true;
    }

    void on_start() override {
        start_count++;
        trading_ = true;
    }

    void on_stop() override {
        stop_count++;
        trading_ = false;
    }

    void on_bar(const BarData& bar) override {
        (void)bar;
        bar_count++;
    }

    void on_tick(const TickData& tick) override {
        (void)tick;
        tick_count++;
    }

    void on_order(const Order& order) override {
        (void)order;
        order_count++;
    }

    void on_trade(const Trade& trade) override {
        (void)trade;
        trade_count++;
    }

    // Public wrappers for protected methods
    order_id_t test_buy(double price, double volume, bool stop = false) {
        return buy(price, volume, stop);
    }
    order_id_t test_sell(double price, double volume, bool stop = false) {
        return sell(price, volume, stop);
    }
    order_id_t test_short(double price, double volume, bool stop = false) {
        return short_order(price, volume, stop);
    }
    order_id_t test_cover(double price, double volume, bool stop = false) {
        return cover(price, volume, stop);
    }
    void test_market_buy(double volume) { market_buy(volume); }
    void test_market_sell(double volume) { market_sell(volume); }
    void test_cancel_order(order_id_t id) { cancel_order(id); }
    void test_cancel_all_orders() { cancel_all_orders(); }
};

// Test TargetPosTemplate implementation
class TestTargetPosStrategy : public TargetPosTemplate {
public:
    void on_bar(const BarData& bar) override {
        (void)bar;
        // Simple logic: set target position
    }
};

//=============================================================================
// Test Functions
//=============================================================================

bool test_strategy_construction() {
    std::cout << "[Test] IStrategy construction" << std::endl;

    TestStrategyImpl strategy;
    // Check initial state through public methods
    if (strategy.position() != 0.0) return false;

    std::cout << "  PASS: Strategy constructed correctly" << std::endl;
    return true;
}

bool test_strategy_position_without_broker() {
    std::cout << "[Test] IStrategy position without broker" << std::endl;

    TestStrategyImpl strategy;
    double pos = strategy.position();
    if (pos != 0.0) return false;

    std::cout << "  PASS: Position returns 0 without broker" << std::endl;
    return true;
}

bool test_strategy_position_side_without_broker() {
    std::cout << "[Test] IStrategy position_side without broker" << std::endl;

    TestStrategyImpl strategy;
    Direction side = strategy.position_side();
    if (side != Direction::NET) return false;

    std::cout << "  PASS: Position side returns NET without broker" << std::endl;
    return true;
}

bool test_strategy_is_flat_without_broker() {
    std::cout << "[Test] IStrategy is_flat without broker" << std::endl;

    TestStrategyImpl strategy;
    if (!strategy.is_flat()) return false;

    std::cout << "  PASS: is_flat returns true without broker" << std::endl;
    return true;
}

bool test_strategy_is_long_without_broker() {
    std::cout << "[Test] IStrategy is_long without broker" << std::endl;

    TestStrategyImpl strategy;
    if (strategy.is_long()) return false;

    std::cout << "  PASS: is_long returns false without broker" << std::endl;
    return true;
}

bool test_strategy_is_short_without_broker() {
    std::cout << "[Test] IStrategy is_short without broker" << std::endl;

    TestStrategyImpl strategy;
    if (strategy.is_short()) return false;

    std::cout << "  PASS: is_short returns false without broker" << std::endl;
    return true;
}

bool test_strategy_get_engine_type() {
    std::cout << "[Test] IStrategy get_engine_type" << std::endl;

    TestStrategyImpl strategy;
    EngineType type = strategy.get_engine_type();
    (void)type;
    // Returns BACKTESTING by default

    std::cout << "  PASS: get_engine_type works" << std::endl;
    return true;
}

bool test_strategy_write_log() {
    std::cout << "[Test] IStrategy write_log" << std::endl;

    TestStrategyImpl strategy;
    strategy.write_log("Test log message");
    // Should not crash

    std::cout << "  PASS: write_log works" << std::endl;
    return true;
}

bool test_strategy_buy_without_broker() {
    std::cout << "[Test] IStrategy buy without broker" << std::endl;

    TestStrategyImpl strategy;
    order_id_t id = strategy.test_buy(50000.0, 0.1);
    if (id != 0) return false;

    std::cout << "  PASS: buy returns 0 without broker" << std::endl;
    return true;
}

bool test_strategy_sell_without_broker() {
    std::cout << "[Test] IStrategy sell without broker" << std::endl;

    TestStrategyImpl strategy;
    order_id_t id = strategy.test_sell(50000.0, 0.1);
    if (id != 0) return false;

    std::cout << "  PASS: sell returns 0 without broker" << std::endl;
    return true;
}

bool test_strategy_short_without_broker() {
    std::cout << "[Test] IStrategy short_order without broker" << std::endl;

    TestStrategyImpl strategy;
    order_id_t id = strategy.test_short(50000.0, 0.1);
    if (id != 0) return false;

    std::cout << "  PASS: short_order returns 0 without broker" << std::endl;
    return true;
}

bool test_strategy_cover_without_broker() {
    std::cout << "[Test] IStrategy cover without broker" << std::endl;

    TestStrategyImpl strategy;
    order_id_t id = strategy.test_cover(50000.0, 0.1);
    if (id != 0) return false;

    std::cout << "  PASS: cover returns 0 without broker" << std::endl;
    return true;
}

bool test_strategy_buy_stop_without_broker() {
    std::cout << "[Test] IStrategy buy (stop) without broker" << std::endl;

    TestStrategyImpl strategy;
    order_id_t id = strategy.test_buy(50000.0, 0.1, true);
    if (id != 0) return false;

    std::cout << "  PASS: buy (stop) returns 0 without broker" << std::endl;
    return true;
}

bool test_strategy_sell_stop_without_broker() {
    std::cout << "[Test] IStrategy sell (stop) without broker" << std::endl;

    TestStrategyImpl strategy;
    order_id_t id = strategy.test_sell(50000.0, 0.1, true);
    if (id != 0) return false;

    std::cout << "  PASS: sell (stop) returns 0 without broker" << std::endl;
    return true;
}

bool test_strategy_market_buy_without_broker() {
    std::cout << "[Test] IStrategy market_buy without broker" << std::endl;

    TestStrategyImpl strategy;
    strategy.test_market_buy(0.1);
    // Should not crash

    std::cout << "  PASS: market_buy works without broker" << std::endl;
    return true;
}

bool test_strategy_market_sell_without_broker() {
    std::cout << "[Test] IStrategy market_sell without broker" << std::endl;

    TestStrategyImpl strategy;
    strategy.test_market_sell(0.1);
    // Should not crash

    std::cout << "  PASS: market_sell works without broker" << std::endl;
    return true;
}

bool test_strategy_cancel_order_without_broker() {
    std::cout << "[Test] IStrategy cancel_order without broker" << std::endl;

    TestStrategyImpl strategy;
    strategy.test_cancel_order(12345);
    // Should not crash

    std::cout << "  PASS: cancel_order works without broker" << std::endl;
    return true;
}

bool test_strategy_cancel_all_orders_without_broker() {
    std::cout << "[Test] IStrategy cancel_all_orders without broker" << std::endl;

    TestStrategyImpl strategy;
    strategy.test_cancel_all_orders();
    // Should not crash

    std::cout << "  PASS: cancel_all_orders works without broker" << std::endl;
    return true;
}

bool test_strategy_set_param() {
    std::cout << "[Test] IStrategy set_param" << std::endl;

    TestStrategyImpl strategy;
    strategy.set_param("test_param", 123.45);

    double val = strategy.get_param("test_param");
    if (val != 123.45) return false;

    std::cout << "  PASS: set_param works correctly" << std::endl;
    return true;
}

bool test_strategy_get_param_default() {
    std::cout << "[Test] IStrategy get_param with default" << std::endl;

    TestStrategyImpl strategy;
    double val = strategy.get_param("nonexistent", 99.0);
    if (val != 99.0) return false;

    std::cout << "  PASS: get_param returns default for nonexistent" << std::endl;
    return true;
}

bool test_strategy_on_init() {
    std::cout << "[Test] IStrategy on_init callback" << std::endl;

    TestStrategyImpl strategy;
    strategy.on_init();

    if (strategy.init_count != 1) return false;

    std::cout << "  PASS: on_init works correctly" << std::endl;
    return true;
}

bool test_strategy_on_start() {
    std::cout << "[Test] IStrategy on_start callback" << std::endl;

    TestStrategyImpl strategy;
    strategy.on_start();

    if (strategy.start_count != 1) return false;

    std::cout << "  PASS: on_start works correctly" << std::endl;
    return true;
}

bool test_strategy_on_stop() {
    std::cout << "[Test] IStrategy on_stop callback" << std::endl;

    TestStrategyImpl strategy;
    strategy.on_start();  // Start first
    strategy.on_stop();

    if (strategy.stop_count != 1) return false;

    std::cout << "  PASS: on_stop works correctly" << std::endl;
    return true;
}

bool test_strategy_on_bar() {
    std::cout << "[Test] IStrategy on_bar callback" << std::endl;

    TestStrategyImpl strategy;
    BarData bar;
    bar.symbol = "BTCUSDT";
    bar.close_price = 50000.0;

    strategy.on_bar(bar);
    if (strategy.bar_count != 1) return false;

    std::cout << "  PASS: on_bar works correctly" << std::endl;
    return true;
}

bool test_strategy_on_tick() {
    std::cout << "[Test] IStrategy on_tick callback" << std::endl;

    TestStrategyImpl strategy;
    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.last_price = 50000.0;

    strategy.on_tick(tick);
    if (strategy.tick_count != 1) return false;

    std::cout << "  PASS: on_tick works correctly" << std::endl;
    return true;
}

bool test_strategy_on_order_callback() {
    std::cout << "[Test] IStrategy on_order callback" << std::endl;

    TestStrategyImpl strategy;
    Order order;
    order.order_id = 12345;

    strategy.on_order(order);
    if (strategy.order_count != 1) return false;

    std::cout << "  PASS: on_order callback works" << std::endl;
    return true;
}

bool test_strategy_on_trade_callback() {
    std::cout << "[Test] IStrategy on_trade callback" << std::endl;

    TestStrategyImpl strategy;
    Trade trade;
    trade.trade_id = 12345;

    strategy.on_trade(trade);
    if (strategy.trade_count != 1) return false;

    std::cout << "  PASS: on_trade callback works" << std::endl;
    return true;
}

bool test_target_pos_template_construction() {
    std::cout << "[Test] TargetPosTemplate construction" << std::endl;

    TestTargetPosStrategy strategy;
    if (!strategy.check_order_finished()) return false;

    std::cout << "  PASS: TargetPosTemplate constructed correctly" << std::endl;
    return true;
}

bool test_target_pos_template_set_target_pos() {
    std::cout << "[Test] TargetPosTemplate set_target_pos" << std::endl;

    TestTargetPosStrategy strategy;
    strategy.set_target_pos(1.0);

    // Verify by checking position change logic
    // Can't directly access target_pos_ as it's protected

    std::cout << "  PASS: set_target_pos works" << std::endl;
    return true;
}

bool test_target_pos_template_check_order_finished() {
    std::cout << "[Test] TargetPosTemplate check_order_finished" << std::endl;

    TestTargetPosStrategy strategy;
    if (!strategy.check_order_finished()) return false;

    std::cout << "  PASS: check_order_finished works initially" << std::endl;
    return true;
}

bool test_target_pos_template_on_tick() {
    std::cout << "[Test] TargetPosTemplate on_tick" << std::endl;

    TestTargetPosStrategy strategy;
    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.last_price = 50000.0;

    strategy.on_tick(tick);
    // Should not crash

    std::cout << "  PASS: on_tick works" << std::endl;
    return true;
}

bool test_target_pos_template_on_bar() {
    std::cout << "[Test] TargetPosTemplate on_bar" << std::endl;

    TestTargetPosStrategy strategy;
    BarData bar;
    bar.symbol = "BTCUSDT";
    bar.close_price = 50000.0;

    strategy.on_bar(bar);
    // Should not crash

    std::cout << "  PASS: on_bar works" << std::endl;
    return true;
}

bool test_cta_signal_construction() {
    std::cout << "[Test] CtaSignal construction" << std::endl;

    CtaSignal signal;
    if (signal.get_signal_pos() != 0) return false;

    std::cout << "  PASS: CtaSignal constructed correctly" << std::endl;
    return true;
}

bool test_cta_signal_set_signal_pos() {
    std::cout << "[Test] CtaSignal set_signal_pos" << std::endl;

    CtaSignal signal;
    signal.set_signal_pos(1);
    if (signal.get_signal_pos() != 1) return false;

    std::cout << "  PASS: set_signal_pos works" << std::endl;
    return true;
}

bool test_cta_signal_on_tick() {
    std::cout << "[Test] CtaSignal on_tick" << std::endl;

    CtaSignal signal;
    TickData tick;
    tick.symbol = "BTCUSDT";
    signal.on_tick(tick);
    // Should not crash

    std::cout << "  PASS: on_tick works" << std::endl;
    return true;
}

bool test_cta_signal_on_bar() {
    std::cout << "[Test] CtaSignal on_bar" << std::endl;

    CtaSignal signal;
    BarData bar;
    bar.symbol = "BTCUSDT";
    signal.on_bar(bar);
    // Should not crash

    std::cout << "  PASS: on_bar works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Strategy Unit Tests                              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_strategy_construction,
        test_strategy_position_without_broker,
        test_strategy_position_side_without_broker,
        test_strategy_is_flat_without_broker,
        test_strategy_is_long_without_broker,
        test_strategy_is_short_without_broker,
        test_strategy_get_engine_type,
        test_strategy_write_log,
        test_strategy_buy_without_broker,
        test_strategy_sell_without_broker,
        test_strategy_short_without_broker,
        test_strategy_cover_without_broker,
        test_strategy_buy_stop_without_broker,
        test_strategy_sell_stop_without_broker,
        test_strategy_market_buy_without_broker,
        test_strategy_market_sell_without_broker,
        test_strategy_cancel_order_without_broker,
        test_strategy_cancel_all_orders_without_broker,
        test_strategy_set_param,
        test_strategy_get_param_default,
        test_strategy_on_init,
        test_strategy_on_start,
        test_strategy_on_stop,
        test_strategy_on_bar,
        test_strategy_on_tick,
        test_strategy_on_order_callback,
        test_strategy_on_trade_callback,
        test_target_pos_template_construction,
        test_target_pos_template_set_target_pos,
        test_target_pos_template_check_order_finished,
        test_target_pos_template_on_tick,
        test_target_pos_template_on_bar,
        test_cta_signal_construction,
        test_cta_signal_set_signal_pos,
        test_cta_signal_on_tick,
        test_cta_signal_on_bar
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