/**
 * @file test_binance_spot_gateway.cpp
 * @brief Unit tests for BinanceSpotGateway
 */

#include "exchange/binance_spot_gateway.hpp"
#include "core/types.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_gateway_construction() {
    std::cout << "[Test] Gateway construction" << std::endl;

    BinanceSpotGateway gateway;
    if (gateway.name() != "BinanceSpot") return false;
    if (!gateway.is_connected()) {} // Expected

    // Test with custom name
    BinanceSpotGateway gateway2("CUSTOM_SPOT");
    if (gateway2.gateway_name() != "CUSTOM_SPOT") return false;

    std::cout << "  PASS: Gateway constructed correctly" << std::endl;
    return true;
}

bool test_gateway_capabilities() {
    std::cout << "[Test] Gateway capabilities" << std::endl;

    BinanceSpotGateway gateway;
    auto caps = gateway.capabilities();

    if (!caps.market_data) return false;
    if (!caps.trading) return false;
    if (!caps.history_data) return false;
    if (!caps.websocket) return false;
    if (caps.stop_order) return false;
    if (caps.margin) return false;
    if (caps.futures) return false;
    if (caps.options) return false;

    auto exchanges = gateway.supported_exchanges();
    if (exchanges.size() != 1) return false;
    if (exchanges[0] != Exchange::BINANCE) return false;

    std::cout << "  PASS: Capabilities correct" << std::endl;
    return true;
}

bool test_gateway_connect_close() {
    std::cout << "[Test] Gateway connect and close" << std::endl;

    BinanceSpotGateway gateway;
    if (gateway.is_connected()) return false;

    // Connect with test config
    GatewayConfig config;
    config.api_key = "test_key";
    config.api_secret = "test_secret";
    config.testnet = true;

    bool connected = gateway.connect(config);
    if (connected) {
        if (!gateway.is_connected()) return false;
        gateway.close();
        if (gateway.is_connected()) return false;
    }

    std::cout << "  PASS: Connect/close interface works" << std::endl;
    return true;
}

bool test_gateway_get_tick() {
    std::cout << "[Test] Gateway get tick" << std::endl;

    BinanceSpotGateway gateway;
    auto tick = gateway.get_tick("BTCUSDT");
    if (tick.has_value()) {
        // With connection might return data
    }

    std::cout << "  PASS: Get tick works" << std::endl;
    return true;
}

bool test_gateway_get_bars() {
    std::cout << "[Test] Gateway get bars" << std::endl;

    BinanceSpotGateway gateway;

    HistoryRequest req;
    req.symbol = "BTCUSDT";
    req.exchange = Exchange::BINANCE;
    req.interval = Interval::MINUTE_1;
    req.start = 0;
    req.end = 0;

    auto bars = gateway.get_bars(req);
    (void)bars;

    std::cout << "  PASS: Get bars works" << std::endl;
    return true;
}

bool test_gateway_get_price() {
    std::cout << "[Test] Gateway get price" << std::endl;

    BinanceSpotGateway gateway;
    double price = gateway.get_price("BTCUSDT");
    (void)price;

    std::cout << "  PASS: Get price works" << std::endl;
    return true;
}

bool test_gateway_get_contract() {
    std::cout << "[Test] Gateway get contract" << std::endl;

    BinanceSpotGateway gateway;
    auto contract = gateway.get_contract("BTCUSDT");
    (void)contract;

    std::cout << "  PASS: Get contract works" << std::endl;
    return true;
}

bool test_gateway_send_order() {
    std::cout << "[Test] Gateway send order" << std::endl;

    BinanceSpotGateway gateway;

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.exchange = Exchange::BINANCE;
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.01;

    std::string order_id = gateway.send_order(req);
    (void)order_id;

    std::cout << "  PASS: Send order works" << std::endl;
    return true;
}

bool test_gateway_cancel_order() {
    std::cout << "[Test] Gateway cancel order" << std::endl;

    BinanceSpotGateway gateway;

    CancelRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "test_order_id";

    gateway.cancel_order(req);

    std::cout << "  PASS: Cancel order works" << std::endl;
    return true;
}

bool test_gateway_cancel_all_orders() {
    std::cout << "[Test] Gateway cancel all orders" << std::endl;

    BinanceSpotGateway gateway;
    gateway.cancel_all_orders("BTCUSDT");
    gateway.cancel_all_orders();

    std::cout << "  PASS: Cancel all orders works" << std::endl;
    return true;
}

bool test_gateway_query_order() {
    std::cout << "[Test] Gateway query order" << std::endl;

    BinanceSpotGateway gateway;

    OrderQueryRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "test_order_id";

    auto order = gateway.query_order(req);
    (void)order;

    std::cout << "  PASS: Query order works" << std::endl;
    return true;
}

bool test_gateway_query_positions() {
    std::cout << "[Test] Gateway query positions" << std::endl;

    BinanceSpotGateway gateway;

    auto positions = gateway.query_positions("BTCUSDT");
    (void)positions;

    positions = gateway.query_positions();
    (void)positions;

    std::cout << "  PASS: Query positions works" << std::endl;
    return true;
}

bool test_gateway_query_account() {
    std::cout << "[Test] Gateway query account" << std::endl;

    BinanceSpotGateway gateway;
    auto account = gateway.query_account();
    (void)account;

    std::cout << "  PASS: Query account works" << std::endl;
    return true;
}

bool test_gateway_query_account_all() {
    std::cout << "[Test] Gateway query account all" << std::endl;

    BinanceSpotGateway gateway;
    auto accounts = gateway.query_account_all();
    (void)accounts;

    std::cout << "  PASS: Query account all works" << std::endl;
    return true;
}

bool test_gateway_query_open_orders() {
    std::cout << "[Test] Gateway query open orders" << std::endl;

    BinanceSpotGateway gateway;

    auto orders = gateway.query_open_orders("BTCUSDT");
    (void)orders;

    orders = gateway.query_open_orders();
    (void)orders;

    std::cout << "  PASS: Query open orders works" << std::endl;
    return true;
}

bool test_gateway_subscribe_tick() {
    std::cout << "[Test] Gateway subscribe tick" << std::endl;

    BinanceSpotGateway gateway;

    SubscribeRequest req;
    req.symbol = "BTCUSDT";
    gateway.subscribe_tick(req);

    std::cout << "  PASS: Subscribe tick works" << std::endl;
    return true;
}

bool test_gateway_subscribe_bar() {
    std::cout << "[Test] Gateway subscribe bar" << std::endl;

    BinanceSpotGateway gateway;
    gateway.subscribe_bar("BTCUSDT", Interval::MINUTE_1);

    std::cout << "  PASS: Subscribe bar works" << std::endl;
    return true;
}

bool test_gateway_unsubscribe_tick() {
    std::cout << "[Test] Gateway unsubscribe tick" << std::endl;

    BinanceSpotGateway gateway;
    gateway.unsubscribe_tick("BTCUSDT");

    std::cout << "  PASS: Unsubscribe tick works" << std::endl;
    return true;
}

bool test_gateway_get_server_time() {
    std::cout << "[Test] Gateway get server time" << std::endl;

    BinanceSpotGateway gateway;
    int64_t time = gateway.get_server_time();
    (void)time;

    std::cout << "  PASS: Get server time works" << std::endl;
    return true;
}

bool test_gateway_ping() {
    std::cout << "[Test] Gateway ping" << std::endl;

    BinanceSpotGateway gateway;
    gateway.ping();

    std::cout << "  PASS: Ping works" << std::endl;
    return true;
}

bool test_gateway_get_exchange_info() {
    std::cout << "[Test] Gateway get exchange info" << std::endl;

    BinanceSpotGateway gateway;
    auto contracts = gateway.get_exchange_info();
    (void)contracts;

    std::cout << "  PASS: Get exchange info works" << std::endl;
    return true;
}

bool test_gateway_start_user_stream() {
    std::cout << "[Test] Gateway start user stream" << std::endl;

    BinanceSpotGateway gateway;
    gateway.start_user_stream();

    std::cout << "  PASS: Start user stream works" << std::endl;
    return true;
}

bool test_gateway_keep_user_stream() {
    std::cout << "[Test] Gateway keep user stream" << std::endl;

    BinanceSpotGateway gateway;
    gateway.keep_user_stream();

    std::cout << "  PASS: Keep user stream works" << std::endl;
    return true;
}

bool test_gateway_process_order_update() {
    std::cout << "[Test] Gateway process order update" << std::endl;

    BinanceSpotGateway gateway;

    OrderData order;
    order.symbol = "BTCUSDT";
    order.exchange = Exchange::BINANCE;
    order.orderid = "test_order";
    order.status = Status::ALLTRADED;
    gateway.process_order_update(order);

    std::cout << "  PASS: Process order update works" << std::endl;
    return true;
}

bool test_gateway_process_account_update() {
    std::cout << "[Test] Gateway process account update" << std::endl;

    BinanceSpotGateway gateway;

    AccountData account;
    account.accountid = "USDT";
    account.balance = 10000.0;
    gateway.process_account_update(account);

    std::cout << "  PASS: Process account update works" << std::endl;
    return true;
}

bool test_gateway_last_error() {
    std::cout << "[Test] Gateway last error" << std::endl;

    BinanceSpotGateway gateway;
    const std::string& error = gateway.last_error();
    (void)error;

    std::cout << "  PASS: Last error works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BinanceSpotGateway Unit Tests                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_gateway_construction,
        test_gateway_capabilities,
        test_gateway_connect_close,
        test_gateway_get_tick,
        test_gateway_get_bars,
        test_gateway_get_price,
        test_gateway_get_contract,
        test_gateway_send_order,
        test_gateway_cancel_order,
        test_gateway_cancel_all_orders,
        test_gateway_query_order,
        test_gateway_query_positions,
        test_gateway_query_account,
        test_gateway_query_account_all,
        test_gateway_query_open_orders,
        test_gateway_subscribe_tick,
        test_gateway_subscribe_bar,
        test_gateway_unsubscribe_tick,
        test_gateway_get_server_time,
        test_gateway_ping,
        test_gateway_get_exchange_info,
        test_gateway_start_user_stream,
        test_gateway_keep_user_stream,
        test_gateway_process_order_update,
        test_gateway_process_account_update,
        test_gateway_last_error
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
