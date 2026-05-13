/**
 * @file test_binance_gateway.cpp
 * @brief Unit tests for BinanceGateway
 */

#include "exchange/binance_gateway.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_binance_gateway_construction() {
    std::cout << "[Test] BinanceGateway construction" << std::endl;

    BinanceGateway gateway;
    if (gateway.name() != "Binance") return false;
    if (gateway.exchange() != Exchange::BINANCE) return false;
    if (gateway.gateway_name() != "BINANCE") return false;

    std::cout << "  PASS: BinanceGateway constructed correctly" << std::endl;
    return true;
}

bool test_binance_gateway_config_construction() {
    std::cout << "[Test] BinanceGateway config construction" << std::endl;

    BinanceGatewayConfig config;
    config.host = "api.binance.com";
    config.testnet = false;

    BinanceGateway gateway(config);
    // Should not crash

    std::cout << "  PASS: Config construction works" << std::endl;
    return true;
}

bool test_binance_gateway_capabilities() {
    std::cout << "[Test] BinanceGateway capabilities" << std::endl;

    BinanceGateway gateway;
    auto caps = gateway.capabilities();

    if (!caps.market_data) return false;
    if (!caps.trading) return false;
    if (!caps.history_data) return false;
    if (!caps.websocket) return false;

    std::cout << "  PASS: Capabilities correct" << std::endl;
    return true;
}

bool test_binance_gateway_supported_exchanges() {
    std::cout << "[Test] BinanceGateway supported_exchanges" << std::endl;

    BinanceGateway gateway;
    auto exchanges = gateway.supported_exchanges();

    if (exchanges.size() != 1) return false;
    if (exchanges[0] != Exchange::BINANCE) return false;

    std::cout << "  PASS: Supported exchanges correct" << std::endl;
    return true;
}

bool test_binance_gateway_is_connected_initial() {
    std::cout << "[Test] BinanceGateway is_connected initial" << std::endl;

    BinanceGateway gateway;
    if (gateway.is_connected()) return false;

    std::cout << "  PASS: is_connected returns false initially" << std::endl;
    return true;
}

bool test_binance_gateway_close_without_connect() {
    std::cout << "[Test] BinanceGateway close without connect" << std::endl;

    BinanceGateway gateway;
    gateway.close();
    // Should not crash

    std::cout << "  PASS: Close without connect works" << std::endl;
    return true;
}

bool test_binance_gateway_get_tick_without_connect() {
    std::cout << "[Test] BinanceGateway get_tick without connect" << std::endl;

    BinanceGateway gateway;
    auto tick = gateway.get_tick("BTCUSDT");
    if (tick.has_value()) return false;

    std::cout << "  PASS: get_tick returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_gateway_get_bars_without_connect() {
    std::cout << "[Test] BinanceGateway get_bars without connect" << std::endl;

    BinanceGateway gateway;
    HistoryRequest req;
    req.symbol = "BTCUSDT";
    req.interval = Interval::MINUTE_1;
    req.limit = 100;

    auto bars = gateway.get_bars(req);
    if (!bars.empty()) return false;

    std::cout << "  PASS: get_bars returns empty without connect" << std::endl;
    return true;
}

bool test_binance_gateway_get_price_without_connect() {
    std::cout << "[Test] BinanceGateway get_price without connect" << std::endl;

    BinanceGateway gateway;
    double price = gateway.get_price("BTCUSDT");
    if (price != 0.0) return false;

    std::cout << "  PASS: get_price returns 0 without connect" << std::endl;
    return true;
}

bool test_binance_gateway_get_contract_without_connect() {
    std::cout << "[Test] BinanceGateway get_contract without connect" << std::endl;

    BinanceGateway gateway;
    auto contract = gateway.get_contract("BTCUSDT");
    if (contract.has_value()) return false;

    std::cout << "  PASS: get_contract returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_gateway_send_order_without_connect() {
    std::cout << "[Test] BinanceGateway send_order without connect" << std::endl;

    BinanceGateway gateway;
    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.volume = 0.01;

    std::string order_id = gateway.send_order(req);
    if (!order_id.empty()) return false;

    std::cout << "  PASS: send_order returns empty without connect" << std::endl;
    return true;
}

bool test_binance_gateway_cancel_order_without_connect() {
    std::cout << "[Test] BinanceGateway cancel_order without connect" << std::endl;

    BinanceGateway gateway;
    CancelRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "12345";

    bool result = gateway.cancel_order(req);
    if (result) return false;

    std::cout << "  PASS: cancel_order returns false without connect" << std::endl;
    return true;
}

bool test_binance_gateway_query_positions_without_connect() {
    std::cout << "[Test] BinanceGateway query_positions without connect" << std::endl;

    BinanceGateway gateway;
    auto positions = gateway.query_positions();
    if (!positions.empty()) return false;

    std::cout << "  PASS: query_positions returns empty without connect" << std::endl;
    return true;
}

bool test_binance_gateway_query_account_without_connect() {
    std::cout << "[Test] BinanceGateway query_account without connect" << std::endl;

    BinanceGateway gateway;
    auto account = gateway.query_account();
    if (account.has_value()) return false;

    std::cout << "  PASS: query_account returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_gateway_query_open_orders_without_connect() {
    std::cout << "[Test] BinanceGateway query_open_orders without connect" << std::endl;

    BinanceGateway gateway;
    auto orders = gateway.query_open_orders();
    if (!orders.empty()) return false;

    std::cout << "  PASS: query_open_orders returns empty without connect" << std::endl;
    return true;
}

bool test_binance_gateway_get_server_time_without_connect() {
    std::cout << "[Test] BinanceGateway get_server_time without connect" << std::endl;

    BinanceGateway gateway;
    uint64_t time = gateway.get_server_time();
    if (time != 0) return false;

    std::cout << "  PASS: get_server_time returns 0 without connect" << std::endl;
    return true;
}

bool test_binance_gateway_ping_without_connect() {
    std::cout << "[Test] BinanceGateway ping without connect" << std::endl;

    BinanceGateway gateway;
    bool result = gateway.ping();
    if (result) return false;

    std::cout << "  PASS: ping returns false without connect" << std::endl;
    return true;
}

bool test_binance_gateway_subscribe_tick_without_connect() {
    std::cout << "[Test] BinanceGateway subscribe_tick without connect" << std::endl;

    BinanceGateway gateway;
    SubscribeRequest req;
    req.symbol = "BTCUSDT";

    gateway.subscribe_tick(req);
    // Should not crash

    std::cout << "  PASS: subscribe_tick works without connect" << std::endl;
    return true;
}

bool test_binance_gateway_subscribe_bar_without_connect() {
    std::cout << "[Test] BinanceGateway subscribe_bar without connect" << std::endl;

    BinanceGateway gateway;
    gateway.subscribe_bar("BTCUSDT", Interval::MINUTE_1);
    // Should not crash

    std::cout << "  PASS: subscribe_bar works without connect" << std::endl;
    return true;
}

bool test_binance_gateway_unsubscribe_tick_without_connect() {
    std::cout << "[Test] BinanceGateway unsubscribe_tick without connect" << std::endl;

    BinanceGateway gateway;
    gateway.unsubscribe_tick("BTCUSDT");
    // Should not crash

    std::cout << "  PASS: unsubscribe_tick works without connect" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BinanceGateway Unit Tests                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_binance_gateway_construction,
        test_binance_gateway_config_construction,
        test_binance_gateway_capabilities,
        test_binance_gateway_supported_exchanges,
        test_binance_gateway_is_connected_initial,
        test_binance_gateway_close_without_connect,
        test_binance_gateway_get_tick_without_connect,
        test_binance_gateway_get_bars_without_connect,
        test_binance_gateway_get_price_without_connect,
        test_binance_gateway_get_contract_without_connect,
        test_binance_gateway_send_order_without_connect,
        test_binance_gateway_cancel_order_without_connect,
        test_binance_gateway_query_positions_without_connect,
        test_binance_gateway_query_account_without_connect,
        test_binance_gateway_query_open_orders_without_connect,
        test_binance_gateway_get_server_time_without_connect,
        test_binance_gateway_ping_without_connect,
        test_binance_gateway_subscribe_tick_without_connect,
        test_binance_gateway_subscribe_bar_without_connect,
        test_binance_gateway_unsubscribe_tick_without_connect
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