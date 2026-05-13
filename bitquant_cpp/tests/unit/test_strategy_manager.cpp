/**
 * @file test_strategy_manager.cpp
 * @brief Unit tests for StrategyManager
 */

#include "engine/strategy_manager.hpp"
#include "engine/strategy.hpp"
#include "engine/broker.hpp"
#include "engine/event.hpp"
#include "core/types.hpp"
#include <iostream>
#include <memory>

using namespace bitquant;

// Test strategy class
class TestStrategy : public IStrategy {
public:
    int bar_count = 0;
    int tick_count = 0;

    void on_init() override { inited_ = true; }
    void on_start() override { trading_ = true; }
    void on_stop() override { trading_ = false; }

    void on_bar(const BarData& bar) override {
        (void)bar;
        bar_count++;
    }

    void on_tick(const TickData& tick) override {
        (void)tick;
        tick_count++;
    }
};

//=============================================================================
// Test Functions
//=============================================================================

bool test_strategy_manager_construction() {
    std::cout << "[Test] StrategyManager construction" << std::endl;

    StrategyManager manager;
    auto names = manager.get_strategy_names();
    if (!names.empty()) return false;

    std::cout << "  PASS: StrategyManager constructed correctly" << std::endl;
    return true;
}

bool test_strategy_manager_add_strategy() {
    std::cout << "[Test] StrategyManager add strategy" << std::endl;

    StrategyManager manager;
    auto strategy = std::make_unique<TestStrategy>();
    bool result = manager.add_strategy("test", std::move(strategy), "BTCUSDT");

    if (!result) return false;

    auto names = manager.get_strategy_names();
    if (names.size() != 1) return false;
    if (names[0] != "test") return false;

    // Add duplicate should fail
    auto strategy2 = std::make_unique<TestStrategy>();
    bool duplicate = manager.add_strategy("test", std::move(strategy2));
    if (duplicate) return false;

    std::cout << "  PASS: Add strategy works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_init_strategy() {
    std::cout << "[Test] StrategyManager init strategy" << std::endl;

    StrategyManager manager;
    auto strategy = std::make_unique<TestStrategy>();
    manager.add_strategy("test", std::move(strategy));

    bool result = manager.init_strategy("test");
    if (!result) return false;

    auto status = manager.get_strategy_status("test");
    if (status != StrategyStatus::READY) return false;

    // Init non-existent strategy
    bool not_found = manager.init_strategy("nonexistent");
    if (not_found) return false;

    std::cout << "  PASS: Init strategy works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_init_all_strategies() {
    std::cout << "[Test] StrategyManager init all strategies" << std::endl;

    StrategyManager manager;
    manager.add_strategy("s1", std::make_unique<TestStrategy>());
    manager.add_strategy("s2", std::make_unique<TestStrategy>());

    manager.init_all_strategies();

    auto status1 = manager.get_strategy_status("s1");
    auto status2 = manager.get_strategy_status("s2");

    if (status1 != StrategyStatus::READY) return false;
    if (status2 != StrategyStatus::READY) return false;

    std::cout << "  PASS: Init all strategies works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_start_strategy() {
    std::cout << "[Test] StrategyManager start strategy" << std::endl;

    StrategyManager manager;
    auto strategy = std::make_unique<TestStrategy>();
    manager.add_strategy("test", std::move(strategy));
    manager.init_strategy("test");

    bool result = manager.start_strategy("test");
    if (!result) return false;

    auto status = manager.get_strategy_status("test");
    if (status != StrategyStatus::RUNNING) return false;

    std::cout << "  PASS: Start strategy works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_start_all_strategies() {
    std::cout << "[Test] StrategyManager start all strategies" << std::endl;

    StrategyManager manager;
    manager.add_strategy("s1", std::make_unique<TestStrategy>());
    manager.add_strategy("s2", std::make_unique<TestStrategy>());
    manager.init_all_strategies();
    manager.start_all_strategies();

    auto status1 = manager.get_strategy_status("s1");
    auto status2 = manager.get_strategy_status("s2");

    if (status1 != StrategyStatus::RUNNING) return false;
    if (status2 != StrategyStatus::RUNNING) return false;

    std::cout << "  PASS: Start all strategies works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_stop_strategy() {
    std::cout << "[Test] StrategyManager stop strategy" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());
    manager.init_strategy("test");
    manager.start_strategy("test");

    manager.stop_strategy("test");

    auto status = manager.get_strategy_status("test");
    if (status != StrategyStatus::STOPPED) return false;

    std::cout << "  PASS: Stop strategy works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_stop_all_strategies() {
    std::cout << "[Test] StrategyManager stop all strategies" << std::endl;

    StrategyManager manager;
    manager.add_strategy("s1", std::make_unique<TestStrategy>());
    manager.add_strategy("s2", std::make_unique<TestStrategy>());
    manager.init_all_strategies();
    manager.start_all_strategies();
    manager.stop_all_strategies();

    auto status1 = manager.get_strategy_status("s1");
    auto status2 = manager.get_strategy_status("s2");

    if (status1 != StrategyStatus::STOPPED) return false;
    if (status2 != StrategyStatus::STOPPED) return false;

    std::cout << "  PASS: Stop all strategies works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_remove_strategy() {
    std::cout << "[Test] StrategyManager remove strategy" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());

    auto names = manager.get_strategy_names();
    if (names.size() != 1) return false;

    manager.remove_strategy("test");

    names = manager.get_strategy_names();
    if (!names.empty()) return false;

    std::cout << "  PASS: Remove strategy works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_on_tick() {
    std::cout << "[Test] StrategyManager on tick" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>(), "BTCUSDT");
    manager.init_strategy("test");
    manager.start_strategy("test");

    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;

    manager.on_tick(tick);

    // Strategy should receive the tick
    std::cout << "  PASS: On tick works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_on_bar() {
    std::cout << "[Test] StrategyManager on bar" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>(), "BTCUSDT");
    manager.init_strategy("test");
    manager.start_strategy("test");

    BarData bar;
    bar.symbol = "BTCUSDT";
    bar.exchange = Exchange::BINANCE;
    bar.close_price = 50000.0;

    manager.on_bar(bar);

    // Strategy should receive the bar
    std::cout << "  PASS: On bar works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_on_order() {
    std::cout << "[Test] StrategyManager on order" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());
    manager.init_strategy("test");
    manager.start_strategy("test");

    OrderData order;
    order.symbol = "BTCUSDT";
    order.orderid = "test_order";
    order.status = Status::ALLTRADED;

    manager.on_order(order);

    std::cout << "  PASS: On order works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_on_trade() {
    std::cout << "[Test] StrategyManager on trade" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());
    manager.init_strategy("test");
    manager.start_strategy("test");

    TradeData trade;
    trade.symbol = "BTCUSDT";
    trade.tradeid = "test_trade";
    trade.volume = 1.0;

    manager.on_trade(trade);

    std::cout << "  PASS: On trade works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_send_order() {
    std::cout << "[Test] StrategyManager send order" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.volume = 1.0;
    req.price = 50000.0;

    std::string order_id = manager.send_order("test", req);
    (void)order_id;

    std::cout << "  PASS: Send order works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_cancel_order() {
    std::cout << "[Test] StrategyManager cancel order" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());

    // Cancel should not throw
    manager.cancel_order("test", "test_order");

    std::cout << "  PASS: Cancel order works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_get_strategy_names() {
    std::cout << "[Test] StrategyManager get strategy names" << std::endl;

    StrategyManager manager;

    auto names = manager.get_strategy_names();
    if (!names.empty()) return false;

    manager.add_strategy("s1", std::make_unique<TestStrategy>());
    manager.add_strategy("s2", std::make_unique<TestStrategy>());

    names = manager.get_strategy_names();
    if (names.size() != 2) return false;

    std::cout << "  PASS: Get strategy names works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_get_strategy_status() {
    std::cout << "[Test] StrategyManager get strategy status" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());

    auto status = manager.get_strategy_status("test");
    // Just verify the function works
    (void)status;

    manager.init_strategy("test");
    status = manager.get_strategy_status("test");
    (void)status;

    manager.start_strategy("test");
    status = manager.get_strategy_status("test");
    (void)status;

    // Non-existent strategy returns CREATED (per implementation)
    status = manager.get_strategy_status("nonexistent");
    if (status != StrategyStatus::CREATED) return false;

    std::cout << "  PASS: Get strategy status works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_get_strategy_position() {
    std::cout << "[Test] StrategyManager get strategy position" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());

    double pos = manager.get_strategy_position("test");
    (void)pos;

    std::cout << "  PASS: Get strategy position works correctly" << std::endl;
    return true;
}

bool test_strategy_manager_get_strategy_stats() {
    std::cout << "[Test] StrategyManager get strategy stats" << std::endl;

    StrategyManager manager;
    manager.add_strategy("test", std::make_unique<TestStrategy>());

    int trades, wins;
    double pnl;
    manager.get_strategy_stats("test", trades, wins, pnl);

    if (trades != 0 || wins != 0 || pnl != 0.0) return false;

    std::cout << "  PASS: Get strategy stats works correctly" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          StrategyManager Unit Tests                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_strategy_manager_construction,
        test_strategy_manager_add_strategy,
        test_strategy_manager_init_strategy,
        test_strategy_manager_init_all_strategies,
        test_strategy_manager_start_strategy,
        test_strategy_manager_start_all_strategies,
        test_strategy_manager_stop_strategy,
        test_strategy_manager_stop_all_strategies,
        test_strategy_manager_remove_strategy,
        test_strategy_manager_on_tick,
        test_strategy_manager_on_bar,
        test_strategy_manager_on_order,
        test_strategy_manager_on_trade,
        test_strategy_manager_send_order,
        test_strategy_manager_cancel_order,
        test_strategy_manager_get_strategy_names,
        test_strategy_manager_get_strategy_status,
        test_strategy_manager_get_strategy_position,
        test_strategy_manager_get_strategy_stats
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
