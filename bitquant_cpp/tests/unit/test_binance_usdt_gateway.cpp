/**
 * @file test_binance_usdt_gateway.cpp
 * @brief Unit tests for BinanceUsdtGateway
 */

#include "exchange/binance_usdt_gateway.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_binance_usdt_gateway_construction() {
    std::cout << "[Test] BinanceUsdtGateway construction" << std::endl;

    BinanceUsdtGateway gateway;
    if (gateway.name() != "BinanceUsdt") return false;
    if (gateway.exchange() != Exchange::BINANCE) return false;
    if (gateway.gateway_name() != "BINANCE_USDT") return false;

    std::cout << "  PASS: BinanceUsdtGateway constructed correctly" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_custom_name() {
    std::cout << "[Test] BinanceUsdtGateway custom name" << std::endl;

    BinanceUsdtGateway gateway("CUSTOM_USDT");
    if (gateway.gateway_name() != "CUSTOM_USDT") return false;

    std::cout << "  PASS: Custom gateway name works" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_capabilities() {
    std::cout << "[Test] BinanceUsdtGateway capabilities" << std::endl;

    BinanceUsdtGateway gateway;
    auto caps = gateway.capabilities();

    if (!caps.market_data) return false;
    if (!caps.trading) return false;
    if (!caps.stop_order) return false;
    if (!caps.history_data) return false;
    if (!caps.websocket) return false;
    if (!caps.margin) return false;
    if (!caps.futures) return false;
    if (caps.options) return false;  // Should be false

    std::cout << "  PASS: Capabilities correct" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_supported_exchanges() {
    std::cout << "[Test] BinanceUsdtGateway supported_exchanges" << std::endl;

    BinanceUsdtGateway gateway;
    auto exchanges = gateway.supported_exchanges();

    if (exchanges.size() != 1) return false;
    if (exchanges[0] != Exchange::BINANCE) return false;

    std::cout << "  PASS: Supported exchanges correct" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_is_connected_initial() {
    std::cout << "[Test] BinanceUsdtGateway is_connected initial" << std::endl;

    BinanceUsdtGateway gateway;
    if (gateway.is_connected()) return false;

    std::cout << "  PASS: is_connected returns false initially" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_close_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway close without connect" << std::endl;

    BinanceUsdtGateway gateway;
    gateway.close();
    // Should not crash

    std::cout << "  PASS: Close without connect works" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_get_tick_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway get_tick without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto tick = gateway.get_tick("BTCUSDT");
    if (tick.has_value()) return false;

    std::cout << "  PASS: get_tick returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_get_bars_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway get_bars without connect" << std::endl;

    BinanceUsdtGateway gateway;
    HistoryRequest req;
    req.symbol = "BTCUSDT";
    req.interval = Interval::MINUTE_1;
    req.limit = 100;

    auto bars = gateway.get_bars(req);
    if (!bars.empty()) return false;

    std::cout << "  PASS: get_bars returns empty without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_get_price_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway get_price without connect" << std::endl;

    BinanceUsdtGateway gateway;
    double price = gateway.get_price("BTCUSDT");
    if (price != 0.0) return false;

    std::cout << "  PASS: get_price returns 0 without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_get_contract_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway get_contract without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto contract = gateway.get_contract("BTCUSDT");
    if (contract.has_value()) return false;

    std::cout << "  PASS: get_contract returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_send_order_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway send_order without connect" << std::endl;

    BinanceUsdtGateway gateway;
    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.volume = 0.01;

    std::string order_id = gateway.send_order(req);
    if (!order_id.empty()) return false;

    std::cout << "  PASS: send_order returns empty without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_cancel_order_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway cancel_order without connect" << std::endl;

    BinanceUsdtGateway gateway;
    CancelRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "12345";

    bool result = gateway.cancel_order(req);
    if (result) return false;

    std::cout << "  PASS: cancel_order returns false without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_cancel_all_orders_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway cancel_all_orders without connect" << std::endl;

    BinanceUsdtGateway gateway;
    gateway.cancel_all_orders("BTCUSDT");
    gateway.cancel_all_orders();  // All symbols
    // Should not crash

    std::cout << "  PASS: cancel_all_orders works without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_query_order_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway query_order without connect" << std::endl;

    BinanceUsdtGateway gateway;
    OrderQueryRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "12345";

    auto order = gateway.query_order(req);
    if (order.has_value()) return false;

    std::cout << "  PASS: query_order returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_query_positions_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway query_positions without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto positions = gateway.query_positions();
    if (!positions.empty()) return false;

    auto positions_symbol = gateway.query_positions("BTCUSDT");
    if (!positions_symbol.empty()) return false;

    std::cout << "  PASS: query_positions returns empty without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_query_account_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway query_account without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto account = gateway.query_account();
    if (account.has_value()) return false;

    std::cout << "  PASS: query_account returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_query_open_orders_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway query_open_orders without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto orders = gateway.query_open_orders();
    if (!orders.empty()) return false;

    auto orders_symbol = gateway.query_open_orders("BTCUSDT");
    if (!orders_symbol.empty()) return false;

    std::cout << "  PASS: query_open_orders returns empty without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_set_leverage_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway set_leverage without connect" << std::endl;

    BinanceUsdtGateway gateway;
    bool result = gateway.set_leverage("BTCUSDT", 10);
    if (result) return false;

    std::cout << "  PASS: set_leverage returns false without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_query_funding_rate_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway query_funding_rate without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto rates = gateway.query_funding_rate();
    if (!rates.empty()) return false;

    std::cout << "  PASS: query_funding_rate returns empty without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_get_position_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway get_position without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto position = gateway.get_position("BTCUSDT");
    if (position.has_value()) return false;

    std::cout << "  PASS: get_position returns nullopt without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_get_server_time_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway get_server_time without connect" << std::endl;

    BinanceUsdtGateway gateway;
    int64_t time = gateway.get_server_time();
    // Returns current system time, not 0
    if (time <= 0) return false;

    std::cout << "  PASS: get_server_time returns current time" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_ping_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway ping without connect" << std::endl;

    BinanceUsdtGateway gateway;
    bool result = gateway.ping();
    if (result) return false;

    std::cout << "  PASS: ping returns false without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_get_exchange_info_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway get_exchange_info without connect" << std::endl;

    BinanceUsdtGateway gateway;
    auto info = gateway.get_exchange_info();
    // Returns default contracts for testing
    if (info.empty()) return false;

    std::cout << "  PASS: get_exchange_info returns default contracts" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_subscribe_tick_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway subscribe_tick without connect" << std::endl;

    BinanceUsdtGateway gateway;
    SubscribeRequest req;
    req.symbol = "BTCUSDT";

    gateway.subscribe_tick(req);
    // Should not crash

    std::cout << "  PASS: subscribe_tick works without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_subscribe_bar_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway subscribe_bar without connect" << std::endl;

    BinanceUsdtGateway gateway;
    gateway.subscribe_bar("BTCUSDT", Interval::MINUTE_1);
    // Should not crash

    std::cout << "  PASS: subscribe_bar works without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_unsubscribe_tick_without_connect() {
    std::cout << "[Test] BinanceUsdtGateway unsubscribe_tick without connect" << std::endl;

    BinanceUsdtGateway gateway;
    gateway.unsubscribe_tick("BTCUSDT");
    // Should not crash

    std::cout << "  PASS: unsubscribe_tick works without connect" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_process_order_update() {
    std::cout << "[Test] BinanceUsdtGateway process_order_update" << std::endl;

    BinanceUsdtGateway gateway;
    OrderData order;
    order.symbol = "BTCUSDT";
    order.orderid = "12345";

    gateway.process_order_update(order);
    // Should not crash

    std::cout << "  PASS: process_order_update works" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_process_account_update() {
    std::cout << "[Test] BinanceUsdtGateway process_account_update" << std::endl;

    BinanceUsdtGateway gateway;
    AccountData account;

    gateway.process_account_update(account);
    // Should not crash

    std::cout << "  PASS: process_account_update works" << std::endl;
    return true;
}

bool test_binance_usdt_gateway_process_position_update() {
    std::cout << "[Test] BinanceUsdtGateway process_position_update" << std::endl;

    BinanceUsdtGateway gateway;
    PositionData position;
    position.symbol = "BTCUSDT";

    gateway.process_position_update(position);
    // Should not crash

    std::cout << "  PASS: process_position_update works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BinanceUsdtGateway Unit Tests                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_binance_usdt_gateway_construction,
        test_binance_usdt_gateway_custom_name,
        test_binance_usdt_gateway_capabilities,
        test_binance_usdt_gateway_supported_exchanges,
        test_binance_usdt_gateway_is_connected_initial,
        test_binance_usdt_gateway_close_without_connect,
        test_binance_usdt_gateway_get_tick_without_connect,
        test_binance_usdt_gateway_get_bars_without_connect,
        test_binance_usdt_gateway_get_price_without_connect,
        test_binance_usdt_gateway_get_contract_without_connect,
        test_binance_usdt_gateway_send_order_without_connect,
        test_binance_usdt_gateway_cancel_order_without_connect,
        test_binance_usdt_gateway_cancel_all_orders_without_connect,
        test_binance_usdt_gateway_query_order_without_connect,
        test_binance_usdt_gateway_query_positions_without_connect,
        test_binance_usdt_gateway_query_account_without_connect,
        test_binance_usdt_gateway_query_open_orders_without_connect,
        test_binance_usdt_gateway_set_leverage_without_connect,
        test_binance_usdt_gateway_query_funding_rate_without_connect,
        test_binance_usdt_gateway_get_position_without_connect,
        test_binance_usdt_gateway_get_server_time_without_connect,
        test_binance_usdt_gateway_ping_without_connect,
        test_binance_usdt_gateway_get_exchange_info_without_connect,
        test_binance_usdt_gateway_subscribe_tick_without_connect,
        test_binance_usdt_gateway_subscribe_bar_without_connect,
        test_binance_usdt_gateway_unsubscribe_tick_without_connect,
        test_binance_usdt_gateway_process_order_update,
        test_binance_usdt_gateway_process_account_update,
        test_binance_usdt_gateway_process_position_update
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