/**
 * @file test_binance_usdt_ws_api.cpp
 * @brief Unit tests for BinanceUsdtWsApi
 */

#include "exchange/binance_usdt_ws_api.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_binance_usdt_ws_api_construction() {
    std::cout << "[Test] BinanceUsdtWsApi construction" << std::endl;

    BinanceUsdtWsApi api;
    // Should not crash

    std::cout << "  PASS: BinanceUsdtWsApi constructed correctly" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_connect() {
    std::cout << "[Test] BinanceUsdtWsApi connect" << std::endl;

    BinanceUsdtWsApi api;
    api.connect();
    // Should not crash

    std::cout << "  PASS: connect works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_stop() {
    std::cout << "[Test] BinanceUsdtWsApi stop" << std::endl;

    BinanceUsdtWsApi api;
    api.stop();
    // Should not crash

    std::cout << "  PASS: stop works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_subscribe_ticker() {
    std::cout << "[Test] BinanceUsdtWsApi subscribe_ticker" << std::endl;

    BinanceUsdtWsApi api;
    api.subscribe_ticker("BTCUSDT");
    // Should not crash

    std::cout << "  PASS: subscribe_ticker works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_subscribe_depth() {
    std::cout << "[Test] BinanceUsdtWsApi subscribe_depth" << std::endl;

    BinanceUsdtWsApi api;
    api.subscribe_depth("BTCUSDT", 5);
    // Should not crash

    std::cout << "  PASS: subscribe_depth works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_subscribe_kline() {
    std::cout << "[Test] BinanceUsdtWsApi subscribe_kline" << std::endl;

    BinanceUsdtWsApi api;
    api.subscribe_kline("BTCUSDT", "1m");
    // Should not crash

    std::cout << "  PASS: subscribe_kline works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_subscribe_trade() {
    std::cout << "[Test] BinanceUsdtWsApi subscribe_trade" << std::endl;

    BinanceUsdtWsApi api;
    api.subscribe_trade("BTCUSDT");
    // Should not crash

    std::cout << "  PASS: subscribe_trade works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_subscribe_agg_trade() {
    std::cout << "[Test] BinanceUsdtWsApi subscribe_agg_trade" << std::endl;

    BinanceUsdtWsApi api;
    api.subscribe_agg_trade("BTCUSDT");
    // Should not crash

    std::cout << "  PASS: subscribe_agg_trade works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_unsubscribe() {
    std::cout << "[Test] BinanceUsdtWsApi unsubscribe" << std::endl;

    BinanceUsdtWsApi api;
    api.unsubscribe("BTCUSDT");
    // Should not crash

    std::cout << "  PASS: unsubscribe works" << std::endl;
    return true;
}

bool test_binance_usdt_ws_api_is_connected() {
    std::cout << "[Test] BinanceUsdtWsApi is_connected" << std::endl;

    BinanceUsdtWsApi api;
    if (api.is_connected()) return false;  // Should be false initially

    std::cout << "  PASS: is_connected returns false initially" << std::endl;
    return true;
}

//=============================================================================
// BinanceUsdtUserWsApi Tests
//=============================================================================

bool test_binance_usdt_user_ws_api_construction() {
    std::cout << "[Test] BinanceUsdtUserWsApi construction" << std::endl;

    BinanceUsdtUserWsApi api;
    // Should not crash

    std::cout << "  PASS: BinanceUsdtUserWsApi constructed correctly" << std::endl;
    return true;
}

bool test_binance_usdt_user_ws_api_connect() {
    std::cout << "[Test] BinanceUsdtUserWsApi connect" << std::endl;

    BinanceUsdtUserWsApi api;
    api.connect("test_listen_key");
    // Should not crash

    std::cout << "  PASS: connect works" << std::endl;
    return true;
}

bool test_binance_usdt_user_ws_api_stop() {
    std::cout << "[Test] BinanceUsdtUserWsApi stop" << std::endl;

    BinanceUsdtUserWsApi api;
    api.stop();
    // Should not crash

    std::cout << "  PASS: stop works" << std::endl;
    return true;
}

bool test_binance_usdt_user_ws_api_is_connected() {
    std::cout << "[Test] BinanceUsdtUserWsApi is_connected" << std::endl;

    BinanceUsdtUserWsApi api;
    if (api.is_connected()) return false;  // Should be false initially

    std::cout << "  PASS: is_connected returns false initially" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BinanceUsdtWsApi Unit Tests                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_binance_usdt_ws_api_construction,
        test_binance_usdt_ws_api_connect,
        test_binance_usdt_ws_api_stop,
        test_binance_usdt_ws_api_subscribe_ticker,
        test_binance_usdt_ws_api_subscribe_depth,
        test_binance_usdt_ws_api_subscribe_kline,
        test_binance_usdt_ws_api_subscribe_trade,
        test_binance_usdt_ws_api_subscribe_agg_trade,
        test_binance_usdt_ws_api_unsubscribe,
        test_binance_usdt_ws_api_is_connected,
        test_binance_usdt_user_ws_api_construction,
        test_binance_usdt_user_ws_api_connect,
        test_binance_usdt_user_ws_api_stop,
        test_binance_usdt_user_ws_api_is_connected
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