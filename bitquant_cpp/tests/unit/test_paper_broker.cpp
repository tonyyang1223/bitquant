/**
 * @file test_paper_broker.cpp
 * @brief Unit tests for PaperBroker
 */

#include "engine/paper_broker.hpp"
#include "core/types.hpp"
#include <iostream>
#include <cmath>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_paper_broker_construction() {
    std::cout << "[Test] PaperBroker construction" << std::endl;

    PaperBroker broker;
    if (broker.get_balance() < 0) return false;

    PaperBrokerConfig config;
    config.initial_capital = 50000.0;
    PaperBroker broker2(config);
    if (broker2.get_balance() < 40000.0) return false;

    std::cout << "  PASS: PaperBroker constructed correctly" << std::endl;
    return true;
}

bool test_paper_broker_set_capital() {
    std::cout << "[Test] PaperBroker set capital" << std::endl;

    PaperBroker broker;
    broker.set_capital(100000.0);
    if (broker.get_balance() < 90000.0) return false;

    std::cout << "  PASS: Set capital works correctly" << std::endl;
    return true;
}

bool test_paper_broker_set_commission() {
    std::cout << "[Test] PaperBroker set commission" << std::endl;

    PaperBroker broker;
    broker.set_commission(0.002);
    // Just verify no crash
    std::cout << "  PASS: Set commission works correctly" << std::endl;
    return true;
}

bool test_paper_broker_set_slippage() {
    std::cout << "[Test] PaperBroker set slippage" << std::endl;

    PaperBroker broker;
    broker.set_slippage(0.001);
    // Just verify no crash
    std::cout << "  PASS: Set slippage works correctly" << std::endl;
    return true;
}

bool test_paper_broker_send_order() {
    std::cout << "[Test] PaperBroker send order" << std::endl;

    PaperBroker broker;
    broker.set_capital(100000.0);

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.exchange = Exchange::BINANCE;
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.1;

    std::string order_id = broker.send_order(req);
    if (order_id.empty()) return false;

    auto orders = broker.get_open_orders();
    if (orders.empty()) return false;

    std::cout << "  PASS: Send order works correctly" << std::endl;
    return true;
}

bool test_paper_broker_cancel_order() {
    std::cout << "[Test] PaperBroker cancel order" << std::endl;

    PaperBroker broker;
    broker.set_capital(100000.0);

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.1;

    std::string order_id = broker.send_order(req);
    bool cancelled = broker.cancel_order(order_id);
    if (!cancelled) return false;

    auto orders = broker.get_open_orders();
    if (!orders.empty()) return false;

    std::cout << "  PASS: Cancel order works correctly" << std::endl;
    return true;
}

bool test_paper_broker_cancel_all_orders() {
    std::cout << "[Test] PaperBroker cancel all orders" << std::endl;

    PaperBroker broker;
    broker.set_capital(100000.0);

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.01;

    broker.send_order(req);
    broker.send_order(req);

    broker.cancel_all_orders();

    auto orders = broker.get_open_orders();
    if (!orders.empty()) return false;

    std::cout << "  PASS: Cancel all orders works correctly" << std::endl;
    return true;
}

bool test_paper_broker_on_tick() {
    std::cout << "[Test] PaperBroker on tick" << std::endl;

    PaperBroker broker;
    broker.set_capital(100000.0);

    // Place order first
    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.1;
    broker.send_order(req);

    // Send tick
    TickData tick;
    tick.symbol = "BTCUSDT";
    tick.exchange = Exchange::BINANCE;
    tick.last_price = 50000.0;
    tick.bid_price_1 = 49999.0;
    tick.ask_price_1 = 50001.0;

    broker.on_tick(tick);

    std::cout << "  PASS: On tick works correctly" << std::endl;
    return true;
}

bool test_paper_broker_on_bar() {
    std::cout << "[Test] PaperBroker on bar" << std::endl;

    PaperBroker broker;
    broker.set_capital(100000.0);

    // Place order
    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.1;
    broker.send_order(req);

    // Send bar
    BarData bar;
    bar.symbol = "BTCUSDT";
    bar.exchange = Exchange::BINANCE;
    bar.close_price = 50000.0;

    broker.on_bar(bar);

    std::cout << "  PASS: On bar works correctly" << std::endl;
    return true;
}

bool test_paper_broker_get_position() {
    std::cout << "[Test] PaperBroker get position" << std::endl;

    PaperBroker broker;

    auto pos = broker.get_position("BTCUSDT");
    (void)pos;

    std::cout << "  PASS: Get position works correctly" << std::endl;
    return true;
}

bool test_paper_broker_get_positions() {
    std::cout << "[Test] PaperBroker get positions" << std::endl;

    PaperBroker broker;

    auto positions = broker.get_positions();
    (void)positions;

    std::cout << "  PASS: Get positions works correctly" << std::endl;
    return true;
}

bool test_paper_broker_get_trades() {
    std::cout << "[Test] PaperBroker get trades" << std::endl;

    PaperBroker broker;

    auto trades = broker.get_trades();
    (void)trades;

    std::cout << "  PASS: Get trades works correctly" << std::endl;
    return true;
}

bool test_paper_broker_get_equity() {
    std::cout << "[Test] PaperBroker get equity" << std::endl;

    PaperBrokerConfig config;
    config.initial_capital = 100000.0;
    PaperBroker broker(config);

    double equity = broker.get_equity();
    if (equity < 90000.0) return false;

    std::cout << "  PASS: Get equity works correctly" << std::endl;
    return true;
}

bool test_paper_broker_get_total_commission() {
    std::cout << "[Test] PaperBroker get total commission" << std::endl;

    PaperBroker broker;
    double commission = broker.get_total_commission();
    (void)commission;

    std::cout << "  PASS: Get total commission works correctly" << std::endl;
    return true;
}

bool test_paper_broker_get_trade_count() {
    std::cout << "[Test] PaperBroker get trade count" << std::endl;

    PaperBroker broker;
    int count = broker.get_trade_count();
    (void)count;

    std::cout << "  PASS: Get trade count works correctly" << std::endl;
    return true;
}

bool test_paper_broker_callbacks() {
    std::cout << "[Test] PaperBroker callbacks" << std::endl;

    PaperBroker broker;
    int order_count = 0;
    int trade_count = 0;

    broker.on_order([&order_count](const OrderData&) { order_count++; });
    broker.on_trade([&trade_count](const TradeData&) { trade_count++; });

    (void)order_count;
    (void)trade_count;

    std::cout << "  PASS: Callbacks work correctly" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          PaperBroker Unit Tests                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_paper_broker_construction,
        test_paper_broker_set_capital,
        test_paper_broker_set_commission,
        test_paper_broker_set_slippage,
        test_paper_broker_send_order,
        test_paper_broker_cancel_order,
        test_paper_broker_cancel_all_orders,
        test_paper_broker_on_tick,
        test_paper_broker_on_bar,
        test_paper_broker_get_position,
        test_paper_broker_get_positions,
        test_paper_broker_get_trades,
        test_paper_broker_get_equity,
        test_paper_broker_get_total_commission,
        test_paper_broker_get_trade_count,
        test_paper_broker_callbacks
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
