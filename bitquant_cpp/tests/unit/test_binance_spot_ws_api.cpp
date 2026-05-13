/**
 * @file test_binance_spot_ws_api.cpp
 * @brief Unit tests for BinanceSpotWsApi
 */

#include "exchange/binance_spot_ws_api.hpp"
#include "exchange/binance_spot_gateway.hpp"
#include "core/types.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_ws_api_construction() {
    std::cout << "[Test] BinanceSpotWsApi construction" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    // Should not throw

    std::cout << "  PASS: BinanceSpotWsApi constructed correctly" << std::endl;
    return true;
}

bool test_ws_api_construction_with_gateway() {
    std::cout << "[Test] BinanceSpotWsApi construction with gateway" << std::endl;

    BinanceSpotGateway gateway;
    BinanceSpotWsApi ws(&gateway);
    // Should not throw

    std::cout << "  PASS: BinanceSpotWsApi constructed with gateway" << std::endl;
    return true;
}

bool test_ws_api_is_connected_initial() {
    std::cout << "[Test] BinanceSpotWsApi is_connected initial" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    if (ws.is_connected()) return false;

    std::cout << "  PASS: is_connected returns false initially" << std::endl;
    return true;
}

bool test_ws_api_close_without_connect() {
    std::cout << "[Test] BinanceSpotWsApi close without connect" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    ws.close();
    // Should not throw

    std::cout << "  PASS: Close without connect works" << std::endl;
    return true;
}

bool test_ws_api_subscribe_without_connect() {
    std::cout << "[Test] BinanceSpotWsApi subscribe without connect" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    // These should not crash
    ws.subscribe_ticker("BTCUSDT");
    ws.subscribe_depth("BTCUSDT");
    ws.subscribe_kline("BTCUSDT", "1m");

    std::cout << "  PASS: Subscribe without connect works (no-op)" << std::endl;
    return true;
}

bool test_ws_api_unsubscribe_without_connect() {
    std::cout << "[Test] BinanceSpotWsApi unsubscribe without connect" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    ws.unsubscribe_ticker("BTCUSDT");
    // Should not crash

    std::cout << "  PASS: Unsubscribe without connect works" << std::endl;
    return true;
}

bool test_ws_api_get_tick_empty() {
    std::cout << "[Test] BinanceSpotWsApi get_tick empty" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    auto tick = ws.get_tick("BTCUSDT");
    if (tick.has_value()) return false;

    std::cout << "  PASS: get_tick returns nullopt for unknown symbol" << std::endl;
    return true;
}

bool test_ws_api_on_tick_callback() {
    std::cout << "[Test] BinanceSpotWsApi on_tick callback" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    int callback_count = 0;

    ws.on_tick([&callback_count](const TickData&) {
        callback_count++;
    });

    // Callback is set, just verify no crash
    (void)callback_count;

    std::cout << "  PASS: on_tick callback set successfully" << std::endl;
    return true;
}

bool test_ws_api_on_bar_callback() {
    std::cout << "[Test] BinanceSpotWsApi on_bar callback" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    int callback_count = 0;

    ws.on_bar([&callback_count](const BarData&) {
        callback_count++;
    });

    (void)callback_count;

    std::cout << "  PASS: on_bar callback set successfully" << std::endl;
    return true;
}

bool test_ws_api_on_order_callback() {
    std::cout << "[Test] BinanceSpotWsApi on_order callback" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    int callback_count = 0;

    ws.on_order([&callback_count](const flatjson::fjson&) {
        callback_count++;
    });

    (void)callback_count;

    std::cout << "  PASS: on_order callback set successfully" << std::endl;
    return true;
}

bool test_ws_api_close_after_close() {
    std::cout << "[Test] BinanceSpotWsApi close after close" << std::endl;

    BinanceSpotWsApi ws(nullptr);
    ws.close();
    ws.close();  // Should not crash

    std::cout << "  PASS: Double close works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║       BinanceSpotWsApi Unit Tests                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_ws_api_construction,
        test_ws_api_construction_with_gateway,
        test_ws_api_is_connected_initial,
        test_ws_api_close_without_connect,
        test_ws_api_subscribe_without_connect,
        test_ws_api_unsubscribe_without_connect,
        test_ws_api_get_tick_empty,
        test_ws_api_on_tick_callback,
        test_ws_api_on_bar_callback,
        test_ws_api_on_order_callback,
        test_ws_api_close_after_close
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
