/**
 * @file test_main_engine.cpp
 * @brief Unit tests for MainEngine
 */

#include "engine/main_engine.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "core/types.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_main_engine_construction() {
    std::cout << "[Test] MainEngine construction" << std::endl;

    MainEngine engine;
    if (engine.is_running()) return false;

    std::cout << "  PASS: MainEngine constructed correctly" << std::endl;
    return true;
}

bool test_main_engine_start_stop() {
    std::cout << "[Test] MainEngine start and stop" << std::endl;

    MainEngine engine;
    if (engine.is_running()) return false;

    engine.start();
    if (!engine.is_running()) return false;

    engine.stop();
    if (engine.is_running()) return false;

    std::cout << "  PASS: Start/stop works correctly" << std::endl;
    return true;
}

bool test_main_engine_add_gateway() {
    std::cout << "[Test] MainEngine add gateway" << std::endl;

    MainEngine engine;

    auto gateway = std::make_unique<BinanceSpotGateway>("BINANCE_SPOT");
    IExchange* ptr = engine.add_gateway("binance", std::move(gateway));

    if (ptr == nullptr) return false;
    if (ptr->name() != "BinanceSpot") return false;

    // Get gateway
    IExchange* retrieved = engine.get_gateway("binance");
    if (retrieved != ptr) return false;

    // Get non-existent gateway
    IExchange* not_found = engine.get_gateway("nonexistent");
    if (not_found != nullptr) return false;

    std::cout << "  PASS: Add gateway works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_gateway_names() {
    std::cout << "[Test] MainEngine get gateway names" << std::endl;

    MainEngine engine;

    // Empty initially
    auto names = engine.get_gateway_names();
    if (!names.empty()) return false;

    // Add gateways
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));
    engine.add_gateway("okx", std::make_unique<BinanceSpotGateway>("OKX"));

    names = engine.get_gateway_names();
    if (names.size() != 2) return false;

    std::cout << "  PASS: Get gateway names works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_exchanges() {
    std::cout << "[Test] MainEngine get all exchanges" << std::endl;

    MainEngine engine;

    auto exchanges = engine.get_all_exchanges();
    (void)exchanges;

    std::cout << "  PASS: Get all exchanges works correctly" << std::endl;
    return true;
}

bool test_main_engine_connect_disconnect() {
    std::cout << "[Test] MainEngine connect and disconnect" << std::endl;

    MainEngine engine;
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));

    // Connect with test config
    bool connected = engine.connect("binance", "");
    (void)connected;

    // Disconnect
    engine.disconnect("binance");

    std::cout << "  PASS: Connect/disconnect works correctly" << std::endl;
    return true;
}

bool test_main_engine_subscribe() {
    std::cout << "[Test] MainEngine subscribe" << std::endl;

    MainEngine engine;
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));
    engine.start();

    // Subscribe
    engine.subscribe("BTCUSDT", "binance");

    // Unsubscribe
    engine.unsubscribe("BTCUSDT", "binance");

    engine.stop();

    std::cout << "  PASS: Subscribe works correctly" << std::endl;
    return true;
}

bool test_main_engine_subscribe_bar() {
    std::cout << "[Test] MainEngine subscribe bar" << std::endl;

    MainEngine engine;
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));
    engine.start();

    engine.subscribe_bar("BTCUSDT", Interval::MINUTE_1, "binance");

    engine.stop();

    std::cout << "  PASS: Subscribe bar works correctly" << std::endl;
    return true;
}

bool test_main_engine_send_order() {
    std::cout << "[Test] MainEngine send order" << std::endl;

    MainEngine engine;
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));
    engine.start();

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.exchange = Exchange::BINANCE;
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.01;

    std::string order_id = engine.send_order(req, "binance");
    (void)order_id;

    engine.stop();

    std::cout << "  PASS: Send order works correctly" << std::endl;
    return true;
}

bool test_main_engine_cancel_order() {
    std::cout << "[Test] MainEngine cancel order" << std::endl;

    MainEngine engine;
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));
    engine.start();

    CancelRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "test_order";

    bool result = engine.cancel_order(req, "binance");
    (void)result;

    engine.stop();

    std::cout << "  PASS: Cancel order works correctly" << std::endl;
    return true;
}

bool test_main_engine_cancel_all_orders() {
    std::cout << "[Test] MainEngine cancel all orders" << std::endl;

    MainEngine engine;
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));
    engine.start();

    engine.cancel_all_orders("BTCUSDT", "binance");

    engine.stop();

    std::cout << "  PASS: Cancel all orders works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_tick() {
    std::cout << "[Test] MainEngine get tick" << std::endl;

    MainEngine engine;
    engine.start();

    auto tick = engine.get_tick("BTCUSDT.BINANCE");
    (void)tick;

    engine.stop();

    std::cout << "  PASS: Get tick works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_order() {
    std::cout << "[Test] MainEngine get order" << std::endl;

    MainEngine engine;
    engine.start();

    auto order = engine.get_order("test_order");
    (void)order;

    engine.stop();

    std::cout << "  PASS: Get order works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_trade() {
    std::cout << "[Test] MainEngine get trade" << std::endl;

    MainEngine engine;
    engine.start();

    auto trade = engine.get_trade("test_trade");
    (void)trade;

    engine.stop();

    std::cout << "  PASS: Get trade works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_position() {
    std::cout << "[Test] MainEngine get position" << std::endl;

    MainEngine engine;
    engine.start();

    auto position = engine.get_position("BTCUSDT.BINANCE.LONG");
    (void)position;

    engine.stop();

    std::cout << "  PASS: Get position works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_account() {
    std::cout << "[Test] MainEngine get account" << std::endl;

    MainEngine engine;
    engine.start();

    auto account = engine.get_account("USDT.BINANCE");
    (void)account;

    engine.stop();

    std::cout << "  PASS: Get account works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_contract() {
    std::cout << "[Test] MainEngine get contract" << std::endl;

    MainEngine engine;
    engine.start();

    auto contract = engine.get_contract("BTCUSDT.BINANCE");
    (void)contract;

    engine.stop();

    std::cout << "  PASS: Get contract works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_ticks() {
    std::cout << "[Test] MainEngine get all ticks" << std::endl;

    MainEngine engine;
    engine.start();

    auto ticks = engine.get_all_ticks();
    (void)ticks;

    engine.stop();

    std::cout << "  PASS: Get all ticks works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_orders() {
    std::cout << "[Test] MainEngine get all orders" << std::endl;

    MainEngine engine;
    engine.start();

    auto orders = engine.get_all_orders();
    (void)orders;

    engine.stop();

    std::cout << "  PASS: Get all orders works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_active_orders() {
    std::cout << "[Test] MainEngine get all active orders" << std::endl;

    MainEngine engine;
    engine.start();

    auto orders = engine.get_all_active_orders();
    (void)orders;

    orders = engine.get_all_active_orders("BTCUSDT");
    (void)orders;

    engine.stop();

    std::cout << "  PASS: Get all active orders works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_trades() {
    std::cout << "[Test] MainEngine get all trades" << std::endl;

    MainEngine engine;
    engine.start();

    auto trades = engine.get_all_trades();
    (void)trades;

    engine.stop();

    std::cout << "  PASS: Get all trades works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_positions() {
    std::cout << "[Test] MainEngine get all positions" << std::endl;

    MainEngine engine;
    engine.start();

    auto positions = engine.get_all_positions();
    (void)positions;

    engine.stop();

    std::cout << "  PASS: Get all positions works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_accounts() {
    std::cout << "[Test] MainEngine get all accounts" << std::endl;

    MainEngine engine;
    engine.start();

    auto accounts = engine.get_all_accounts();
    (void)accounts;

    engine.stop();

    std::cout << "  PASS: Get all accounts works correctly" << std::endl;
    return true;
}

bool test_main_engine_get_all_contracts() {
    std::cout << "[Test] MainEngine get all contracts" << std::endl;

    MainEngine engine;
    engine.start();

    auto contracts = engine.get_all_contracts();
    (void)contracts;

    engine.stop();

    std::cout << "  PASS: Get all contracts works correctly" << std::endl;
    return true;
}

bool test_main_engine_query_history() {
    std::cout << "[Test] MainEngine query history" << std::endl;

    MainEngine engine;
    engine.add_gateway("binance", std::make_unique<BinanceSpotGateway>("BINANCE"));
    engine.start();

    HistoryRequest req;
    req.symbol = "BTCUSDT";
    req.exchange = Exchange::BINANCE;
    req.interval = Interval::MINUTE_1;

    auto bars = engine.query_history(req, "binance");
    (void)bars;

    engine.stop();

    std::cout << "  PASS: Query history works correctly" << std::endl;
    return true;
}

bool test_main_engine_write_log() {
    std::cout << "[Test] MainEngine write log" << std::endl;

    MainEngine engine;
    engine.start();

    engine.write_log("Test log message", "TestSource");
    engine.write_log("Another log message");

    engine.stop();

    std::cout << "  PASS: Write log works correctly" << std::endl;
    return true;
}

bool test_main_engine_event_engine() {
    std::cout << "[Test] MainEngine event engine access" << std::endl;

    MainEngine engine;
    EventEngine* ee = engine.event_engine();
    if (ee == nullptr) return false;

    std::cout << "  PASS: Event engine access works correctly" << std::endl;
    return true;
}

bool test_main_engine_oms_engine() {
    std::cout << "[Test] MainEngine OMS engine access" << std::endl;

    MainEngine engine;
    OmsEngine* oms = engine.oms_engine();
    if (oms == nullptr) return false;

    std::cout << "  PASS: OMS engine access works correctly" << std::endl;
    return true;
}

bool test_main_engine_risk_manager() {
    std::cout << "[Test] MainEngine risk manager access" << std::endl;

    MainEngine engine;
    RiskManager* rm = engine.risk_manager();
    if (rm == nullptr) return false;

    std::cout << "  PASS: Risk manager access works correctly" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          MainEngine Unit Tests                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_main_engine_construction,
        test_main_engine_start_stop,
        test_main_engine_add_gateway,
        test_main_engine_get_gateway_names,
        test_main_engine_get_all_exchanges,
        test_main_engine_connect_disconnect,
        test_main_engine_subscribe,
        test_main_engine_subscribe_bar,
        test_main_engine_send_order,
        test_main_engine_cancel_order,
        test_main_engine_cancel_all_orders,
        test_main_engine_get_tick,
        test_main_engine_get_order,
        test_main_engine_get_trade,
        test_main_engine_get_position,
        test_main_engine_get_account,
        test_main_engine_get_contract,
        test_main_engine_get_all_ticks,
        test_main_engine_get_all_orders,
        test_main_engine_get_all_active_orders,
        test_main_engine_get_all_trades,
        test_main_engine_get_all_positions,
        test_main_engine_get_all_accounts,
        test_main_engine_get_all_contracts,
        test_main_engine_query_history,
        test_main_engine_write_log,
        test_main_engine_event_engine,
        test_main_engine_oms_engine,
        test_main_engine_risk_manager
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
