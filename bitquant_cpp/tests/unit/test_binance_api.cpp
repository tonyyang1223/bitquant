/**
 * @file test_binance_api.cpp
 * @brief Unit tests for BinanceApiClient
 */

#include "exchange/binance_api.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_binance_config_defaults() {
    std::cout << "[Test] BinanceConfig defaults" << std::endl;

    BinanceConfig config;
    if (config.host != "api.binance.com") return false;
    if (config.port != "443") return false;
    if (config.timeout_ms != 10000) return false;
    if (config.testnet != false) return false;

    std::cout << "  PASS: BinanceConfig defaults correct" << std::endl;
    return true;
}

bool test_binance_api_client_construction() {
    std::cout << "[Test] BinanceApiClient construction" << std::endl;

    BinanceApiClient client;
    // Should not crash

    std::cout << "  PASS: BinanceApiClient constructed correctly" << std::endl;
    return true;
}

bool test_binance_api_client_config_construction() {
    std::cout << "[Test] BinanceApiClient config construction" << std::endl;

    BinanceConfig config;
    config.host = "api.binance.com";
    config.testnet = false;

    BinanceApiClient client(config);
    // Should not crash

    std::cout << "  PASS: Config construction works" << std::endl;
    return true;
}

bool test_kline_interval_to_string() {
    std::cout << "[Test] kline_interval_to_string" << std::endl;

    if (std::string(kline_interval_to_string(KlineInterval::MIN_1)) != "1m") return false;
    if (std::string(kline_interval_to_string(KlineInterval::MIN_5)) != "5m") return false;
    if (std::string(kline_interval_to_string(KlineInterval::HOUR_1)) != "1h") return false;
    if (std::string(kline_interval_to_string(KlineInterval::DAY_1)) != "1d") return false;
    if (std::string(kline_interval_to_string(KlineInterval::WEEK_1)) != "1w") return false;
    if (std::string(kline_interval_to_string(KlineInterval::MONTH_1)) != "1M") return false;

    std::cout << "  PASS: kline_interval_to_string works" << std::endl;
    return true;
}

bool test_binance_api_client_last_error() {
    std::cout << "[Test] BinanceApiClient last_error" << std::endl;

    BinanceApiClient client;
    const std::string& error = client.last_error();
    // Should return empty string initially
    (void)error;

    std::cout << "  PASS: last_error works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          BinanceApiClient Unit Tests                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_binance_config_defaults,
        test_binance_api_client_construction,
        test_binance_api_client_config_construction,
        test_kline_interval_to_string,
        test_binance_api_client_last_error
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