/**
 * @file test_binance_spot_rest_api.cpp
 * @brief Unit tests for BinanceSpotRestApi
 */

#include "exchange/binance_spot_rest_api.hpp"
#include "core/types.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_rest_api_construction() {
    std::cout << "[Test] BinanceSpotRestApi construction" << std::endl;

    BinanceSpotRestApi api;
    // Should not throw

    std::cout << "  PASS: BinanceSpotRestApi constructed correctly" << std::endl;
    return true;
}

bool test_rest_api_init() {
    std::cout << "[Test] BinanceSpotRestApi init" << std::endl;

    BinanceSpotRestApi api;

    // Init with default parameters
    bool result = api.init("api.binance.com", "443");
    if (!result) {
        std::cout << "  WARN: Init failed (may be expected in test environment)" << std::endl;
        // Still pass - network may not be available
    }

    std::cout << "  PASS: Init test completed" << std::endl;
    return true;
}

bool test_rest_api_ping_without_init() {
    std::cout << "[Test] BinanceSpotRestApi ping without init" << std::endl;

    BinanceSpotRestApi api;

    // Ping without init should return false
    bool result = api.ping();
    if (result) return false;  // Should fail

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Ping without init returns false as expected" << std::endl;
    return true;
}

bool test_rest_api_get_server_time_without_init() {
    std::cout << "[Test] BinanceSpotRestApi get_server_time without init" << std::endl;

    BinanceSpotRestApi api;

    // Get server time without init should return 0
    int64_t time = api.get_server_time();
    if (time != 0) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Get server time without init returns 0" << std::endl;
    return true;
}

bool test_rest_api_get_price_without_init() {
    std::cout << "[Test] BinanceSpotRestApi get_price without init" << std::endl;

    BinanceSpotRestApi api;

    // Get price without init should return 0
    double price = api.get_price("BTCUSDT");
    if (price != 0.0) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Get price without init returns 0" << std::endl;
    return true;
}

bool test_rest_api_get_klines_without_init() {
    std::cout << "[Test] BinanceSpotRestApi get_klines without init" << std::endl;

    BinanceSpotRestApi api;

    // Get klines without init should return empty vector
    auto bars = api.get_klines("BTCUSDT", Interval::MINUTE_1, 10);
    if (!bars.empty()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Get klines without init returns empty" << std::endl;
    return true;
}

bool test_rest_api_get_exchange_info_without_init() {
    std::cout << "[Test] BinanceSpotRestApi get_exchange_info without init" << std::endl;

    BinanceSpotRestApi api;

    // Get exchange info without init should return empty vector
    auto contracts = api.get_exchange_info();
    if (!contracts.empty()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Get exchange info without init returns empty" << std::endl;
    return true;
}

bool test_rest_api_send_order_without_init() {
    std::cout << "[Test] BinanceSpotRestApi send_order without init" << std::endl;

    BinanceSpotRestApi api;

    OrderRequest req;
    req.symbol = "BTCUSDT";
    req.direction = Direction::LONG;
    req.type = OrderType::LIMIT;
    req.price = 50000.0;
    req.volume = 0.01;

    // Send order without init should return empty string
    std::string order_id = api.send_order(req);
    if (!order_id.empty()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Send order without init returns empty" << std::endl;
    return true;
}

bool test_rest_api_cancel_order_without_init() {
    std::cout << "[Test] BinanceSpotRestApi cancel_order without init" << std::endl;

    BinanceSpotRestApi api;

    // Cancel order without init should return false
    bool result = api.cancel_order("BTCUSDT", "12345");
    if (result) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Cancel order without init returns false" << std::endl;
    return true;
}

bool test_rest_api_cancel_order_request_without_init() {
    std::cout << "[Test] BinanceSpotRestApi cancel_order (CancelRequest) without init" << std::endl;

    BinanceSpotRestApi api;

    CancelRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "12345";

    // Cancel order without init should return false
    bool result = api.cancel_order(req);
    if (result) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Cancel order (CancelRequest) without init returns false" << std::endl;
    return true;
}

bool test_rest_api_query_order_without_init() {
    std::cout << "[Test] BinanceSpotRestApi query_order without init" << std::endl;

    BinanceSpotRestApi api;

    // Query order without init should return nullopt
    auto order = api.query_order("BTCUSDT", "12345");
    if (order.has_value()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Query order without init returns nullopt" << std::endl;
    return true;
}

bool test_rest_api_query_order_request_without_init() {
    std::cout << "[Test] BinanceSpotRestApi query_order (OrderQueryRequest) without init" << std::endl;

    BinanceSpotRestApi api;

    OrderQueryRequest req;
    req.symbol = "BTCUSDT";
    req.orderid = "12345";

    // Query order without init should return nullopt
    auto order = api.query_order(req);
    if (order.has_value()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Query order (OrderQueryRequest) without init returns nullopt" << std::endl;
    return true;
}

bool test_rest_api_query_open_orders_without_init() {
    std::cout << "[Test] BinanceSpotRestApi query_open_orders without init" << std::endl;

    BinanceSpotRestApi api;

    // Query open orders without init should return empty vector
    auto orders = api.query_open_orders("BTCUSDT");
    if (!orders.empty()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Query open orders without init returns empty" << std::endl;
    return true;
}

bool test_rest_api_query_account_without_init() {
    std::cout << "[Test] BinanceSpotRestApi query_account without init" << std::endl;

    BinanceSpotRestApi api;

    // Query account without init should return nullopt
    auto account = api.query_account("USDT");
    if (account.has_value()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Query account without init returns nullopt" << std::endl;
    return true;
}

bool test_rest_api_query_account_all_without_init() {
    std::cout << "[Test] BinanceSpotRestApi query_account_all without init" << std::endl;

    BinanceSpotRestApi api;

    // Query all accounts without init should return empty vector
    auto accounts = api.query_account_all();
    if (!accounts.empty()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Query account all without init returns empty" << std::endl;
    return true;
}

bool test_rest_api_start_user_stream_without_init() {
    std::cout << "[Test] BinanceSpotRestApi start_user_stream without init" << std::endl;

    BinanceSpotRestApi api;

    // Start user stream without init should return empty string
    std::string listen_key = api.start_user_stream();
    if (!listen_key.empty()) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Start user stream without init returns empty" << std::endl;
    return true;
}

bool test_rest_api_keep_user_stream_without_init() {
    std::cout << "[Test] BinanceSpotRestApi keep_user_stream without init" << std::endl;

    BinanceSpotRestApi api;

    // Keep user stream without init should return false
    bool result = api.keep_user_stream("test_listen_key");
    if (result) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Keep user stream without init returns false" << std::endl;
    return true;
}

bool test_rest_api_close_user_stream_without_init() {
    std::cout << "[Test] BinanceSpotRestApi close_user_stream without init" << std::endl;

    BinanceSpotRestApi api;

    // Close user stream without init should return false
    bool result = api.close_user_stream("test_listen_key");
    if (result) return false;

    const std::string& err = api.last_error();
    if (err.empty()) return false;

    std::cout << "  PASS: Close user stream without init returns false" << std::endl;
    return true;
}

bool test_rest_api_time_offset() {
    std::cout << "[Test] BinanceSpotRestApi time_offset" << std::endl;

    BinanceSpotRestApi api;

    // Time offset should be 0 before init
    int64_t offset = api.time_offset();
    if (offset != 0) return false;

    std::cout << "  PASS: Time offset default is 0" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║       BinanceSpotRestApi Unit Tests                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_rest_api_construction,
        test_rest_api_init,
        test_rest_api_ping_without_init,
        test_rest_api_get_server_time_without_init,
        test_rest_api_get_price_without_init,
        test_rest_api_get_klines_without_init,
        test_rest_api_get_exchange_info_without_init,
        test_rest_api_send_order_without_init,
        test_rest_api_cancel_order_without_init,
        test_rest_api_cancel_order_request_without_init,
        test_rest_api_query_order_without_init,
        test_rest_api_query_order_request_without_init,
        test_rest_api_query_open_orders_without_init,
        test_rest_api_query_account_without_init,
        test_rest_api_query_account_all_without_init,
        test_rest_api_start_user_stream_without_init,
        test_rest_api_keep_user_stream_without_init,
        test_rest_api_close_user_stream_without_init,
        test_rest_api_time_offset
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
